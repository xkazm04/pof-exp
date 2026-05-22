#include "World/ARPGZoneManager.h"
#include "World/ARPGPlayerStart.h"
#include "Quest/ARPGQuestSubsystem.h"
#include "Framework/ARPGGameInstance.h"
#include "Engine/LevelStreaming.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"

void UARPGZoneManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Log, TEXT("[ZoneManager] Initialized"));
}

void UARPGZoneManager::Deinitialize()
{
	// Clear streaming poll timer if a transition was in progress
	if (StreamingPollHandle.IsValid())
	{
		UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
		if (World)
		{
			World->GetTimerManager().ClearTimer(StreamingPollHandle);
		}
	}

	RegisteredZones.Empty();
	CurrentZone = nullptr;
	PendingStreamingZone = nullptr;
	bTransitionInProgress = false;
	Super::Deinitialize();
}

void UARPGZoneManager::RegisterZone(UARPGZoneDefinition* ZoneDef)
{
	if (!ZoneDef || ZoneDef->ZoneID.IsNone()) return;

	RegisteredZones.Add(ZoneDef->ZoneID, ZoneDef);
	UE_LOG(LogTemp, Log, TEXT("[ZoneManager] Registered zone: %s (%s)"),
		*ZoneDef->ZoneID.ToString(), *ZoneDef->ZoneDisplayName.ToString());
}

TArray<UARPGZoneDefinition*> UARPGZoneManager::GetAllZones() const
{
	TArray<UARPGZoneDefinition*> Result;
	for (const auto& Pair : RegisteredZones)
	{
		Result.Add(Pair.Value);
	}
	return Result;
}

UARPGZoneDefinition* UARPGZoneManager::GetZoneByID(FName ZoneID) const
{
	const TObjectPtr<UARPGZoneDefinition>* Found = RegisteredZones.Find(ZoneID);
	return Found ? *Found : nullptr;
}

void UARPGZoneManager::SetCurrentZone(UARPGZoneDefinition* NewZone)
{
	if (NewZone == CurrentZone) return;

	UARPGZoneDefinition* PreviousZone = CurrentZone;
	CurrentZone = NewZone;

	if (NewZone)
	{
		VisitedZoneIDs.Add(NewZone->ZoneID);

		// Sync to game instance
		if (UARPGGameInstance* GI = Cast<UARPGGameInstance>(GetGameInstance()))
		{
			GI->CurrentZone = NewZone->ZoneID.ToString();
		}

		UE_LOG(LogTemp, Log, TEXT("[ZoneManager] Zone changed: %s -> %s"),
			PreviousZone ? *PreviousZone->ZoneID.ToString() : TEXT("None"),
			*NewZone->ZoneID.ToString());
	}

	OnZoneChanged.Broadcast(NewZone, PreviousZone);
}

bool UARPGZoneManager::IsInSafeZone() const
{
	return CurrentZone && CurrentZone->bIsSafeZone;
}

float UARPGZoneManager::GetCurrentZoneDifficulty() const
{
	return CurrentZone ? CurrentZone->ZoneDifficultyMultiplier : 1.0f;
}

TArray<FZoneConnection> UARPGZoneManager::GetCurrentZoneConnections() const
{
	if (!CurrentZone) return {};
	return CurrentZone->Connections;
}

bool UARPGZoneManager::IsConnectionUnlocked(const FZoneConnection& Connection) const
{
	if (!Connection.bRequiresUnlock) return true;
	if (Connection.RequiredQuestID.IsNone()) return true;

	// Check quest system for completion
	UGameInstance* GI = GetGameInstance();
	if (!GI) return false;

	UARPGQuestSubsystem* QuestSys = GI->GetSubsystem<UARPGQuestSubsystem>();
	if (!QuestSys) return false;

	return QuestSys->GetCompletedQuestIDs().Contains(Connection.RequiredQuestID);
}

TArray<UARPGZoneDefinition*> UARPGZoneManager::GetFastTravelDestinations() const
{
	TArray<UARPGZoneDefinition*> Destinations;
	for (const FName& ZoneID : VisitedZoneIDs)
	{
		if (UARPGZoneDefinition* Zone = GetZoneByID(ZoneID))
		{
			if (Zone->bAllowFastTravel && Zone != CurrentZone)
			{
				Destinations.Add(Zone);
			}
		}
	}
	return Destinations;
}

void UARPGZoneManager::RequestZoneTransition(UARPGZoneDefinition* TargetZone)
{
	RequestZoneTransitionToSpawn(TargetZone, NAME_None);
}

void UARPGZoneManager::RequestZoneTransitionToSpawn(UARPGZoneDefinition* TargetZone, FName SpawnPointTag)
{
	if (!TargetZone || bTransitionInProgress) return;
	if (TargetZone == CurrentZone)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ZoneManager] Ignoring transition — already in zone %s"), *TargetZone->ZoneID.ToString());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[ZoneManager] Requesting transition to: %s (spawn: %s)"),
		*TargetZone->ZoneID.ToString(),
		SpawnPointTag.IsNone() ? TEXT("default") : *SpawnPointTag.ToString());

	FadeScreenAndTransition(TargetZone, SpawnPointTag);
}

void UARPGZoneManager::FadeScreenAndTransition(UARPGZoneDefinition* TargetZone, FName SpawnPointTag)
{
	bTransitionInProgress = true;

	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World)
	{
		bTransitionInProgress = false;
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (PC)
	{
		// Disable player input during transition
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);

		// Fade to black
		PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, 0.5f, FLinearColor::Black, false, true);
	}

	// Delay the actual transition to let the fade complete
	FTimerHandle FadeHandle;
	FTimerDelegate FadeDel;
	FadeDel.BindUObject(this, &UARPGZoneManager::OnFadeOutComplete, TargetZone, SpawnPointTag);
	World->GetTimerManager().SetTimer(FadeHandle, FadeDel, 0.5f, false);
}

void UARPGZoneManager::OnFadeOutComplete(UARPGZoneDefinition* TargetZone, FName SpawnPointTag)
{
	UWorld* World = GetGameInstance()->GetWorld();
	if (!World || !TargetZone)
	{
		bTransitionInProgress = false;
		return;
	}

	PendingSpawnTag = SpawnPointTag;

	// If target has a streaming level, handle load/unload
	if (!TargetZone->StreamingLevelName.IsNone())
	{
		// Unload current zone's streaming level if it has one
		if (CurrentZone && !CurrentZone->StreamingLevelName.IsNone() && CurrentZone != TargetZone)
		{
			FLatentActionInfo UnloadInfo;
			UGameplayStatics::UnloadStreamLevel(World, CurrentZone->StreamingLevelName, UnloadInfo, false);
		}

		// Load target zone via GameplayStatics
		FLatentActionInfo LoadInfo;
		UGameplayStatics::LoadStreamLevel(World, TargetZone->StreamingLevelName, true, false, LoadInfo);

		// Poll until the streaming level is actually loaded (up to 10s)
		PendingStreamingZone = TargetZone;
		StreamingPollCount = 0;
		FTimerDelegate PollDel;
		PollDel.BindUObject(this, &UARPGZoneManager::PollStreamingLevelLoaded);
		World->GetTimerManager().SetTimer(StreamingPollHandle, PollDel, 0.1f, true);
	}
	else
	{
		// No streaming — just update zone tracking and teleport
		HandleStreamingLevelLoaded(TargetZone);
	}
}

void UARPGZoneManager::HandleStreamingLevelLoaded(UARPGZoneDefinition* Zone)
{
	SetCurrentZone(Zone);

	// Teleport player to spawn point
	TeleportPlayerToSpawn(PendingSpawnTag);

	// Fade back in and re-enable input
	UWorld* World = GetGameInstance()->GetWorld();
	if (World)
	{
		APlayerController* PC = World->GetFirstPlayerController();
		if (PC)
		{
			PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, 0.5f, FLinearColor::Black, false, false);
			PC->SetIgnoreMoveInput(false);
			PC->SetIgnoreLookInput(false);
		}
	}

	bTransitionInProgress = false;
	PendingSpawnTag = NAME_None;
	OnZoneLoaded.Broadcast(Zone);
}

void UARPGZoneManager::PollStreamingLevelLoaded()
{
	StreamingPollCount++;

	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World || !PendingStreamingZone)
	{
		if (World)
		{
			World->GetTimerManager().ClearTimer(StreamingPollHandle);
		}
		// Don't proceed with a null zone — just re-enable input and bail
		if (!PendingStreamingZone)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ZoneManager] Streaming poll aborted — null pending zone"));
			if (World)
			{
				APlayerController* PC = World->GetFirstPlayerController();
				if (PC)
				{
					PC->SetIgnoreMoveInput(false);
					PC->SetIgnoreLookInput(false);
				}
			}
			bTransitionInProgress = false;
			PendingStreamingZone = nullptr;
			return;
		}
		HandleStreamingLevelLoaded(PendingStreamingZone);
		PendingStreamingZone = nullptr;
		return;
	}

	// Check if the streaming level is loaded
	bool bLoaded = false;
	const TArray<ULevelStreaming*>& Levels = World->GetStreamingLevels();
	for (ULevelStreaming* Level : Levels)
	{
		if (Level && Level->GetWorldAssetPackageFName().ToString().Contains(PendingStreamingZone->StreamingLevelName.ToString()))
		{
			if (Level->IsLevelLoaded() && Level->IsLevelVisible())
			{
				bLoaded = true;
				break;
			}
		}
	}

	// Timeout after 100 polls (~10 seconds) or success
	if (bLoaded || StreamingPollCount > 100)
	{
		if (!bLoaded)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ZoneManager] Streaming timeout for zone %s — proceeding anyway"),
				PendingStreamingZone ? *PendingStreamingZone->ZoneID.ToString() : TEXT("Unknown"));
		}
		World->GetTimerManager().ClearTimer(StreamingPollHandle);
		HandleStreamingLevelLoaded(PendingStreamingZone);
		PendingStreamingZone = nullptr;
	}
}

void UARPGZoneManager::TeleportPlayerToSpawn(FName SpawnPointTag)
{
	UWorld* World = GetGameInstance()->GetWorld();
	if (!World) return;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!PlayerPawn) return;

	// Find matching spawn point
	AARPGPlayerStart* BestSpawn = nullptr;

	for (TActorIterator<AARPGPlayerStart> It(World); It; ++It)
	{
		AARPGPlayerStart* Spawn = *It;
		if (!Spawn) continue;

		// If a specific tag was requested, match against it
		if (!SpawnPointTag.IsNone())
		{
			if (Spawn->ActorHasTag(SpawnPointTag) || Spawn->PlayerStartTag == SpawnPointTag)
			{
				BestSpawn = Spawn;
				break;
			}
		}
		else if (Spawn->OwningZone == CurrentZone && Spawn->bIsZoneDefault)
		{
			BestSpawn = Spawn;
			break;
		}
		else if (Spawn->OwningZone == CurrentZone && !BestSpawn)
		{
			BestSpawn = Spawn;
		}
	}

	if (BestSpawn)
	{
		PlayerPawn->SetActorLocationAndRotation(
			BestSpawn->GetActorLocation(),
			BestSpawn->GetActorRotation());
		UE_LOG(LogTemp, Log, TEXT("[ZoneManager] Teleported player to spawn: %s"), *BestSpawn->GetName());
	}
}
