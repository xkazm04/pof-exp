#include "PofWebSocketServer.h"
#include "PillarsOfFortuneBridge.h"
#include "IWebSocketServer.h"
#include "IWebSocketNetworkingModule.h"
#include "INetworkingWebSocket.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Misc/EngineVersion.h"
#include "Misc/App.h"

#if WITH_EDITOR
#include "Editor.h"
#include "LevelEditor.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "Engine/Selection.h"
#include "LevelEditorViewport.h"
#include "SLevelViewport.h"
#endif

// ─── JSON Helpers ──────────────────────────────────────────────────────────────

static FString JsonObjectToString(TSharedRef<FJsonObject> Obj)
{
	FString Output;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	FJsonSerializer::Serialize(Obj, Writer);
	return Output;
}

static TSharedPtr<FJsonObject> ParseJsonString(const FString& Str)
{
	TSharedPtr<FJsonObject> Obj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Str);
	FJsonSerializer::Deserialize(Reader, Obj);
	return Obj;
}

static TSharedRef<FJsonObject> MakeVectorJson(const FVector& V)
{
	TSharedRef<FJsonObject> Obj = MakeShareable(new FJsonObject());
	Obj->SetNumberField(TEXT("x"), V.X);
	Obj->SetNumberField(TEXT("y"), V.Y);
	Obj->SetNumberField(TEXT("z"), V.Z);
	return Obj;
}

static TSharedRef<FJsonObject> MakeRotatorJson(const FRotator& R)
{
	TSharedRef<FJsonObject> Obj = MakeShareable(new FJsonObject());
	Obj->SetNumberField(TEXT("pitch"), R.Pitch);
	Obj->SetNumberField(TEXT("yaw"), R.Yaw);
	Obj->SetNumberField(TEXT("roll"), R.Roll);
	return Obj;
}

// ─── Start / Stop ──────────────────────────────────────────────────────────────

void UPofWebSocketLiveState::Start(uint32 Port, const FString& InAllowedOrigins)
{
	AllowedOrigins = InAllowedOrigins;

	FWebSocketClientConnectedCallBack OnConnected;
	OnConnected.BindUObject(this, &UPofWebSocketLiveState::OnClientConnected);

	Server = FModuleManager::Get().LoadModuleChecked<IWebSocketNetworkingModule>(TEXT("WebSocketNetworking"))
		.CreateServer();

	if (!Server.IsValid())
	{
		UE_LOG(LogPofBridge, Error, TEXT("[WS] Failed to create WebSocket server on port %d"), Port);
		return;
	}

	if (!Server->Init(Port, OnConnected))
	{
		UE_LOG(LogPofBridge, Error, TEXT("[WS] Failed to init WebSocket server on port %d"), Port);
		Server.Reset();
		return;
	}

	bIsRunning = true;
	UE_LOG(LogPofBridge, Log, TEXT("[WS] Live state server started on port %d"), Port);
}

void UPofWebSocketLiveState::Stop()
{
	bIsRunning = false;

	for (INetworkingWebSocket* Client : ConnectedClients)
	{
		// Server will handle cleanup
		(void)Client;
	}
	ConnectedClients.Empty();
	PropertyWatches.Empty();
	Server.Reset();

	UE_LOG(LogPofBridge, Log, TEXT("[WS] Live state server stopped"));
}

// ─── Client Lifecycle ──────────────────────────────────────────────────────────

void UPofWebSocketLiveState::OnClientConnected(INetworkingWebSocket* Client)
{
	UE_LOG(LogPofBridge, Log, TEXT("[WS] Client connected (total: %d)"), ConnectedClients.Num() + 1);

	// Bind callbacks
	Client->SetReceiveCallBack(FWebSocketPacketReceivedCallBack::CreateUObject(
		this, &UPofWebSocketLiveState::OnMessageReceived, Client));
	Client->SetSocketClosedCallBack(FWebSocketInfoCallBack::CreateUObject(
		this, &UPofWebSocketLiveState::OnClientDisconnected, Client));

	ConnectedClients.Add(Client);

	// Send full snapshot immediately
	FString Snapshot = BuildFullSnapshot();
	SendToClient(Client, Snapshot);
}

void UPofWebSocketLiveState::OnClientDisconnected(INetworkingWebSocket* Client)
{
	ConnectedClients.Remove(Client);
	UE_LOG(LogPofBridge, Log, TEXT("[WS] Client disconnected (remaining: %d)"), ConnectedClients.Num());

	// Remove any property watches owned by this client
	// (In a multi-client scenario, we'd track ownership. For now, watches persist.)
}

void UPofWebSocketLiveState::OnMessageReceived(void* Data, int32 DataSize, INetworkingWebSocket* Client)
{
	FString Message = FString(DataSize, UTF8_TO_TCHAR(static_cast<const char*>(Data)));
	HandleInboundMessage(Message, Client);
}

// ─── Inbound Message Handling ──────────────────────────────────────────────────

void UPofWebSocketLiveState::HandleInboundMessage(const FString& Message, INetworkingWebSocket* Client)
{
	TSharedPtr<FJsonObject> Json = ParseJsonString(Message);
	if (!Json.IsValid()) return;

	FString Type = Json->GetStringField(TEXT("type"));

	if (Type == TEXT("ping"))
	{
		SendToClient(Client, TEXT("{\"type\":\"pong\"}"));
	}
	else if (Type == TEXT("request.snapshot"))
	{
		SendToClient(Client, BuildFullSnapshot());
	}
	else if (Type == TEXT("subscribe.property"))
	{
		TSharedPtr<FJsonObject> Payload = Json->GetObjectField(TEXT("payload"));
		if (!Payload.IsValid()) return;

		FPropertyWatch Watch;
		Watch.WatchId = Payload->GetStringField(TEXT("watchId"));
		Watch.ObjectPath = Payload->GetStringField(TEXT("objectPath"));
		Watch.PropertyName = Payload->GetStringField(TEXT("propertyName"));
		Watch.IntervalSeconds = (float)(Payload->GetNumberField(TEXT("intervalMs"))) / 1000.0f;
		if (Watch.IntervalSeconds <= 0.0f) Watch.IntervalSeconds = 0.5f;
		Watch.TimeSinceLastPoll = Watch.IntervalSeconds; // poll immediately
		Watch.LastValueJson = TEXT("");

		// Remove existing watch with same ID
		PropertyWatches.RemoveAll([&](const FPropertyWatch& W) { return W.WatchId == Watch.WatchId; });
		PropertyWatches.Add(Watch);

		UE_LOG(LogPofBridge, Verbose, TEXT("[WS] Subscribed to property: %s.%s (watch: %s)"),
			*Watch.ObjectPath, *Watch.PropertyName, *Watch.WatchId);
	}
	else if (Type == TEXT("unsubscribe.property"))
	{
		TSharedPtr<FJsonObject> Payload = Json->GetObjectField(TEXT("payload"));
		if (!Payload.IsValid()) return;

		FString WatchId = Payload->GetStringField(TEXT("watchId"));
		PropertyWatches.RemoveAll([&](const FPropertyWatch& W) { return W.WatchId == WatchId; });
	}
	else if (Type == TEXT("set.property"))
	{
		// Write property via UObject reflection — same as Remote Control API
		TSharedPtr<FJsonObject> Payload = Json->GetObjectField(TEXT("payload"));
		if (!Payload.IsValid()) return;

		FString ObjectPath = Payload->GetStringField(TEXT("objectPath"));
		FString PropertyName = Payload->GetStringField(TEXT("propertyName"));

		UE_LOG(LogPofBridge, Verbose, TEXT("[WS] Property write request: %s.%s"), *ObjectPath, *PropertyName);
		// Property writing would go through UE5's Remote Control internals
		// For now, log the request — full implementation requires FRemoteControlModule
	}
}

// ─── State Collection ──────────────────────────────────────────────────────────

FString UPofWebSocketLiveState::BuildFullSnapshot() const
{
	TSharedRef<FJsonObject> Root = MakeShareable(new FJsonObject());
	Root->SetStringField(TEXT("type"), TEXT("state.snapshot"));

	TSharedRef<FJsonObject> Payload = MakeShareable(new FJsonObject());
	Payload->SetNumberField(TEXT("timestamp"), FPlatformTime::Seconds() * 1000.0);

#if WITH_EDITOR
	// Editor state
	FString EditorState = TEXT("Editing");
	if (GEditor)
	{
		if (GEditor->IsPlaySessionInProgress())
		{
			EditorState = (GEditor->PlayWorld && GEditor->PlayWorld->IsPaused()) ? TEXT("Paused") : TEXT("PIE");
		}
	}
	Payload->SetStringField(TEXT("editorState"), EditorState);

	// Viewport camera
	TSharedRef<FJsonObject> ViewportObj = MakeShareable(new FJsonObject());
	FVector CamLoc = FVector::ZeroVector;
	FRotator CamRot = FRotator::ZeroRotator;
	float FOV = 90.0f;
	FString ViewMode = TEXT("Lit");

	if (GEditor)
	{
		for (FLevelEditorViewportClient* ViewportClient : GEditor->GetLevelViewportClients())
		{
			if (ViewportClient && ViewportClient->IsPerspective())
			{
				CamLoc = ViewportClient->GetViewLocation();
				CamRot = ViewportClient->GetViewRotation();
				FOV = ViewportClient->ViewFOV;
				ViewMode = FString::Printf(TEXT("%d"), static_cast<int32>(ViewportClient->GetViewMode()));
				break;
			}
		}
	}

	ViewportObj->SetObjectField(TEXT("cameraLocation"), MakeVectorJson(CamLoc));
	ViewportObj->SetObjectField(TEXT("cameraRotation"), MakeRotatorJson(CamRot));
	ViewportObj->SetNumberField(TEXT("fov"), FOV);
	ViewportObj->SetStringField(TEXT("viewMode"), ViewMode);
	Payload->SetObjectField(TEXT("viewport"), ViewportObj);

	// Selected actors
	TArray<TSharedPtr<FJsonValue>> ActorsArray;
	if (GEditor)
	{
		USelection* Selection = GEditor->GetSelectedActors();
		for (int32 i = 0; i < Selection->Num(); ++i)
		{
			UObject* Obj = Selection->GetSelectedObject(i);
			AActor* Actor = Cast<AActor>(Obj);
			if (!Actor) continue;

			TSharedRef<FJsonObject> ActorObj = MakeShareable(new FJsonObject());
			ActorObj->SetStringField(TEXT("path"), Actor->GetPathName());
			ActorObj->SetStringField(TEXT("className"), Actor->GetClass()->GetName());
			ActorObj->SetStringField(TEXT("label"), Actor->GetActorLabel());
			ActorObj->SetObjectField(TEXT("location"), MakeVectorJson(Actor->GetActorLocation()));
			ActorObj->SetObjectField(TEXT("rotation"), MakeRotatorJson(Actor->GetActorRotation()));
			ActorsArray.Add(MakeShareable(new FJsonValueObject(ActorObj)));
		}
	}
	Payload->SetArrayField(TEXT("selectedActors"), ActorsArray);

	// PIE state
	if (GEditor && GEditor->IsPlaySessionInProgress())
	{
		TSharedRef<FJsonObject> PIE = MakeShareable(new FJsonObject());
		PIE->SetBoolField(TEXT("isRunning"), true);
		PIE->SetBoolField(TEXT("isPaused"), GEditor->PlayWorld && GEditor->PlayWorld->IsPaused());
		PIE->SetStringField(TEXT("sessionId"), TEXT("default"));
		PIE->SetNumberField(TEXT("playerCount"), 1);
		PIE->SetNumberField(TEXT("elapsedSeconds"), 0.0);
		Payload->SetObjectField(TEXT("pieState"), PIE);
	}
	else
	{
		Payload->SetField(TEXT("pieState"), MakeShareable(new FJsonValueNull()));
	}

	// Open level
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	Payload->SetStringField(TEXT("openLevel"), World ? World->GetMapName() : TEXT(""));

	// Dirty packages
	TArray<TSharedPtr<FJsonValue>> DirtyArray;
	Payload->SetArrayField(TEXT("dirtyPackages"), DirtyArray);

#else
	Payload->SetStringField(TEXT("editorState"), TEXT("Unknown"));
#endif

	Root->SetObjectField(TEXT("payload"), Payload);
	return JsonObjectToString(Root);
}

FString UPofWebSocketLiveState::BuildDeltaSnapshot()
{
	TSharedRef<FJsonObject> Root = MakeShareable(new FJsonObject());
	Root->SetStringField(TEXT("type"), TEXT("state.delta"));

	TSharedRef<FJsonObject> Payload = MakeShareable(new FJsonObject());
	Payload->SetNumberField(TEXT("timestamp"), FPlatformTime::Seconds() * 1000.0);

	bool bHasChanges = false;

#if WITH_EDITOR
	if (GEditor)
	{
		// Check editor state change
		FString CurrentEditorState = TEXT("Editing");
		if (GEditor->IsPlaySessionInProgress())
		{
			CurrentEditorState = (GEditor->PlayWorld && GEditor->PlayWorld->IsPaused()) ? TEXT("Paused") : TEXT("PIE");
		}
		if (CurrentEditorState != LastEditorState)
		{
			Payload->SetStringField(TEXT("editorState"), CurrentEditorState);
			LastEditorState = CurrentEditorState;
			bHasChanges = true;
		}

		// Check viewport camera changes
		for (FLevelEditorViewportClient* ViewportClient : GEditor->GetLevelViewportClients())
		{
			if (ViewportClient && ViewportClient->IsPerspective())
			{
				FVector CamLoc = ViewportClient->GetViewLocation();
				FRotator CamRot = ViewportClient->GetViewRotation();
				float FOV = ViewportClient->ViewFOV;

				if (!CamLoc.Equals(LastCameraLocation, 0.1f) ||
					!CamRot.Equals(LastCameraRotation, 0.1f) ||
					!FMath::IsNearlyEqual(FOV, LastFOV, 0.1f))
				{
					TSharedRef<FJsonObject> ViewportObj = MakeShareable(new FJsonObject());
					ViewportObj->SetObjectField(TEXT("cameraLocation"), MakeVectorJson(CamLoc));
					ViewportObj->SetObjectField(TEXT("cameraRotation"), MakeRotatorJson(CamRot));
					ViewportObj->SetNumberField(TEXT("fov"), FOV);
					ViewportObj->SetStringField(TEXT("viewMode"), FString::Printf(TEXT("%d"), static_cast<int32>(ViewportClient->GetViewMode())));
					Payload->SetObjectField(TEXT("viewport"), ViewportObj);

					LastCameraLocation = CamLoc;
					LastCameraRotation = CamRot;
					LastFOV = FOV;
					bHasChanges = true;
				}
				break;
			}
		}

		// Check selection changes
		USelection* Selection = GEditor->GetSelectedActors();
		int32 CurrentCount = Selection ? Selection->Num() : 0;
		if (CurrentCount != LastSelectedActorCount)
		{
			TArray<TSharedPtr<FJsonValue>> ActorsArray;
			if (Selection)
			{
				for (int32 i = 0; i < Selection->Num(); ++i)
				{
					AActor* Actor = Cast<AActor>(Selection->GetSelectedObject(i));
					if (!Actor) continue;

					TSharedRef<FJsonObject> ActorObj = MakeShareable(new FJsonObject());
					ActorObj->SetStringField(TEXT("path"), Actor->GetPathName());
					ActorObj->SetStringField(TEXT("className"), Actor->GetClass()->GetName());
					ActorObj->SetStringField(TEXT("label"), Actor->GetActorLabel());
					ActorObj->SetObjectField(TEXT("location"), MakeVectorJson(Actor->GetActorLocation()));
					ActorsArray.Add(MakeShareable(new FJsonValueObject(ActorObj)));
				}
			}
			Payload->SetArrayField(TEXT("selectedActors"), ActorsArray);
			LastSelectedActorCount = CurrentCount;
			bHasChanges = true;
		}

		// Check level change
		UWorld* World = GEditor->GetEditorWorldContext().World();
		FString CurrentLevel = World ? World->GetMapName() : TEXT("");
		if (CurrentLevel != LastOpenLevel)
		{
			Payload->SetStringField(TEXT("openLevel"), CurrentLevel);
			LastOpenLevel = CurrentLevel;
			bHasChanges = true;
		}
	}
#endif

	if (!bHasChanges)
	{
		return FString(); // No changes — skip broadcast
	}

	Root->SetObjectField(TEXT("payload"), Payload);
	return JsonObjectToString(Root);
}

// ─── Tick ──────────────────────────────────────────────────────────────────────

void UPofWebSocketLiveState::Tick(float DeltaTime)
{
	if (!bIsRunning || !Server.IsValid()) return;

	// Pump WebSocket messages
	Server->Tick();

	if (ConnectedClients.Num() == 0) return;

	// Delta snapshot broadcast
	SnapshotAccumulator += DeltaTime;
	if (SnapshotAccumulator >= SnapshotInterval)
	{
		SnapshotAccumulator = 0.0f;
		FString Delta = BuildDeltaSnapshot();
		if (!Delta.IsEmpty())
		{
			BroadcastToAll(Delta);
		}
	}

	// Property watch polling
	for (FPropertyWatch& Watch : PropertyWatches)
	{
		Watch.TimeSinceLastPoll += DeltaTime;
		if (Watch.TimeSinceLastPoll < Watch.IntervalSeconds) continue;
		Watch.TimeSinceLastPoll = 0.0f;

		// Read property via UObject reflection
		UObject* Obj = StaticFindObject(UObject::StaticClass(), nullptr, *Watch.ObjectPath);
		if (!Obj) continue;

		FProperty* Prop = Obj->GetClass()->FindPropertyByName(FName(*Watch.PropertyName));
		if (!Prop) continue;

		// Serialize property value to JSON string
		FString ValueJson;
		const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Obj);
		Prop->ExportTextItem_Direct(ValueJson, ValuePtr, nullptr, Obj, PPF_None);

		// Only send if value changed
		if (ValueJson == Watch.LastValueJson) continue;

		FString PreviousValue = Watch.LastValueJson;
		Watch.LastValueJson = ValueJson;

		// Build property.update message
		TSharedRef<FJsonObject> Root = MakeShareable(new FJsonObject());
		Root->SetStringField(TEXT("type"), TEXT("property.update"));

		TSharedRef<FJsonObject> PayloadObj = MakeShareable(new FJsonObject());
		PayloadObj->SetStringField(TEXT("watchId"), Watch.WatchId);
		PayloadObj->SetStringField(TEXT("objectPath"), Watch.ObjectPath);
		PayloadObj->SetStringField(TEXT("propertyName"), Watch.PropertyName);
		PayloadObj->SetStringField(TEXT("value"), ValueJson);
		if (!PreviousValue.IsEmpty())
		{
			PayloadObj->SetStringField(TEXT("previousValue"), PreviousValue);
		}
		PayloadObj->SetNumberField(TEXT("timestamp"), FPlatformTime::Seconds() * 1000.0);

		Root->SetObjectField(TEXT("payload"), PayloadObj);
		BroadcastToAll(JsonObjectToString(Root));
	}
}

TStatId UPofWebSocketLiveState::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UPofWebSocketLiveState, STATGROUP_Tickables);
}

// ─── Send Helpers ──────────────────────────────────────────────────────────────

void UPofWebSocketLiveState::BroadcastToAll(const FString& Message)
{
	TArray<uint8> Data;
	FTCHARToUTF8 Converter(*Message);
	Data.Append(reinterpret_cast<const uint8*>(Converter.Get()), Converter.Length());

	for (INetworkingWebSocket* Client : ConnectedClients)
	{
		Client->Send(Data.GetData(), Data.Num(), /*bPrepend=*/false);
	}
}

void UPofWebSocketLiveState::SendToClient(INetworkingWebSocket* Client, const FString& Message)
{
	TArray<uint8> Data;
	FTCHARToUTF8 Converter(*Message);
	Data.Append(reinterpret_cast<const uint8*>(Converter.Get()), Converter.Length());
	Client->Send(Data.GetData(), Data.Num(), /*bPrepend=*/false);
}
