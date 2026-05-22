#include "ARPGSessionSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

void UARPGSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Register for network/travel failure notifications
	GEngine->OnNetworkFailure().AddUObject(this, &UARPGSessionSubsystem::OnNetworkFailure);
	GEngine->OnTravelFailure().AddUObject(this, &UARPGSessionSubsystem::OnTravelFailure);
}

void UARPGSessionSubsystem::Deinitialize()
{
	// Clear reconnect timer
	UGameInstance* GI = GetGameInstance();
	if (GI)
	{
		UWorld* World = GI->GetWorld();
		if (World)
		{
			World->GetTimerManager().ClearTimer(ReconnectTimerHandle);
		}
	}

	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid())
	{
		if (CreateSessionDelegateHandle.IsValid())
		{
			Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);
		}
		if (FindSessionsDelegateHandle.IsValid())
		{
			Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);
		}
		if (JoinSessionDelegateHandle.IsValid())
		{
			Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);
		}
		if (DestroySessionDelegateHandle.IsValid())
		{
			Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
		}
	}

	GEngine->OnNetworkFailure().RemoveAll(this);
	GEngine->OnTravelFailure().RemoveAll(this);

	Super::Deinitialize();
}

IOnlineSessionPtr UARPGSessionSubsystem::GetSessionInterface() const
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (!OnlineSub) return nullptr;
	return OnlineSub->GetSessionInterface();
}

void UARPGSessionSubsystem::SetSessionState(EARPGSessionState NewState)
{
	if (SessionState == NewState) return;
	SessionState = NewState;
	OnSessionStateChanged.Broadcast(NewState);
}

// =====================================================================
// Session Creation
// =====================================================================

void UARPGSessionSubsystem::CreateSession(int32 MaxPlayers, bool bIsLAN, const FString& SessionDisplayName)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		UE_LOG(LogNet, Warning, TEXT("ARPGSessionSubsystem: No online session interface available"));
		OnCreateSessionComplete.Broadcast(false);
		return;
	}

	PendingSessionDisplayName = SessionDisplayName;
	SetSessionState(EARPGSessionState::Creating);

	FOnlineSessionSettings SessionSettings;
	SessionSettings.NumPublicConnections = MaxPlayers;
	SessionSettings.bIsLANMatch = bIsLAN;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bAllowJoinViaPresence = true;
	SessionSettings.bUsesPresence = !bIsLAN;
	SessionSettings.Set(FName("SESSION_DISPLAY_NAME"), SessionDisplayName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	if (CreateSessionDelegateHandle.IsValid())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);
	}

	CreateSessionDelegateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UARPGSessionSubsystem::OnCreateSessionCompleteInternal));

	Sessions->CreateSession(0, NAME_GameSession, SessionSettings);
}

void UARPGSessionSubsystem::OnCreateSessionCompleteInternal(FName SessionName, bool bSuccess)
{
	bInSession = bSuccess;

	if (bSuccess)
	{
		SetSessionState(EARPGSessionState::InSession);
		UE_LOG(LogNet, Log, TEXT("ARPGSessionSubsystem: Session '%s' created successfully"), *SessionName.ToString());
	}
	else
	{
		SetSessionState(EARPGSessionState::None);
		UE_LOG(LogNet, Warning, TEXT("ARPGSessionSubsystem: Failed to create session '%s'"), *SessionName.ToString());
	}

	OnCreateSessionComplete.Broadcast(bSuccess);
}

// =====================================================================
// Session Search
// =====================================================================

void UARPGSessionSubsystem::FindSessions(bool bIsLAN, int32 MaxResults)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		TArray<FARPGSessionSearchResult> EmptyResults;
		OnFindSessionsComplete.Broadcast(false, EmptyResults);
		return;
	}

	bIsSearching = true;
	SetSessionState(EARPGSessionState::Searching);

	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->bIsLanQuery = bIsLAN;
	SessionSearch->MaxSearchResults = MaxResults;

	if (FindSessionsDelegateHandle.IsValid())
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);
	}

	FindSessionsDelegateHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UARPGSessionSubsystem::OnFindSessionsCompleteInternal));

	Sessions->FindSessions(0, SessionSearch.ToSharedRef());
}

void UARPGSessionSubsystem::OnFindSessionsCompleteInternal(bool bSuccess)
{
	bIsSearching = false;
	CachedResults.Empty();

	if (bSuccess && SessionSearch.IsValid())
	{
		for (int32 i = 0; i < SessionSearch->SearchResults.Num(); ++i)
		{
			const FOnlineSessionSearchResult& Result = SessionSearch->SearchResults[i];

			FARPGSessionSearchResult Entry;
			Entry.SearchResultIndex = i;
			Entry.PingInMs = Result.PingInMs;

			const FOnlineSession& Session = Result.Session;
			Entry.MaxPlayers = Session.SessionSettings.NumPublicConnections;
			Entry.CurrentPlayers = Entry.MaxPlayers - Session.NumOpenPublicConnections;

			FString DisplayName;
			if (Session.SessionSettings.Get(FName("SESSION_DISPLAY_NAME"), DisplayName))
			{
				Entry.SessionName = DisplayName;
			}
			else
			{
				Entry.SessionName = TEXT("Unknown Session");
			}

			Entry.HostName = Session.OwningUserName;
			CachedResults.Add(Entry);
		}
	}

	if (!bInSession)
	{
		SetSessionState(EARPGSessionState::None);
	}
	else
	{
		SetSessionState(EARPGSessionState::InSession);
	}

	OnFindSessionsComplete.Broadcast(bSuccess, CachedResults);
}

// =====================================================================
// Join Session
// =====================================================================

void UARPGSessionSubsystem::JoinSession(int32 SearchResultIndex)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid() || !SessionSearch.IsValid())
	{
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	if (!SessionSearch->SearchResults.IsValidIndex(SearchResultIndex))
	{
		UE_LOG(LogNet, Warning, TEXT("ARPGSessionSubsystem: Invalid search result index %d"), SearchResultIndex);
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	SetSessionState(EARPGSessionState::Joining);

	if (JoinSessionDelegateHandle.IsValid())
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);
	}

	JoinSessionDelegateHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UARPGSessionSubsystem::OnJoinSessionCompleteInternal));

	Sessions->JoinSession(0, NAME_GameSession, SessionSearch->SearchResults[SearchResultIndex]);
}

void UARPGSessionSubsystem::OnJoinSessionCompleteInternal(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	bool bSuccess = (Result == EOnJoinSessionCompleteResult::Success);
	bInSession = bSuccess;

	if (bSuccess)
	{
		IOnlineSessionPtr Sessions = GetSessionInterface();
		if (Sessions.IsValid())
		{
			FString ConnectString;
			if (Sessions->GetResolvedConnectString(SessionName, ConnectString))
			{
				// Cache connection info for potential rejoin
				LastConnectString = ConnectString;
				LastSessionDisplayName = PendingSessionDisplayName;

				UE_LOG(LogNet, Log, TEXT("ARPGSessionSubsystem: Joining server at %s"), *ConnectString);
				SetSessionState(EARPGSessionState::TravelingToServer);

				APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController();
				if (PC)
				{
					PC->ClientTravel(ConnectString, TRAVEL_Absolute);
				}
			}
		}

		if (SessionState != EARPGSessionState::TravelingToServer)
		{
			SetSessionState(EARPGSessionState::InSession);
		}
	}
	else
	{
		SetSessionState(EARPGSessionState::None);

		FString ResultStr;
		switch (Result)
		{
		case EOnJoinSessionCompleteResult::SessionIsFull:
			ResultStr = TEXT("Session is full");
			break;
		case EOnJoinSessionCompleteResult::SessionDoesNotExist:
			ResultStr = TEXT("Session does not exist");
			break;
		case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress:
			ResultStr = TEXT("Could not retrieve address");
			break;
		case EOnJoinSessionCompleteResult::AlreadyInSession:
			ResultStr = TEXT("Already in session");
			break;
		default:
			ResultStr = TEXT("Unknown error");
			break;
		}
		UE_LOG(LogNet, Warning, TEXT("ARPGSessionSubsystem: Failed to join session: %s"), *ResultStr);
	}

	OnJoinSessionComplete.Broadcast(bSuccess);
}

// =====================================================================
// Destroy Session
// =====================================================================

void UARPGSessionSubsystem::DestroySession()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		OnDestroySessionComplete.Broadcast();
		return;
	}

	SetSessionState(EARPGSessionState::Destroying);

	if (DestroySessionDelegateHandle.IsValid())
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
	}

	DestroySessionDelegateHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &UARPGSessionSubsystem::OnDestroySessionCompleteInternal));

	Sessions->DestroySession(NAME_GameSession);
}

void UARPGSessionSubsystem::OnDestroySessionCompleteInternal(FName SessionName, bool bSuccess)
{
	bInSession = false;
	SetSessionState(EARPGSessionState::None);
	OnDestroySessionComplete.Broadcast();

	UE_LOG(LogNet, Log, TEXT("ARPGSessionSubsystem: Session '%s' destroyed (success=%d)"), *SessionName.ToString(), bSuccess);
}

// =====================================================================
// Server Travel
// =====================================================================

void UARPGSessionSubsystem::ServerTravelToMap(const FString& MapPath)
{
	if (!bInSession)
	{
		UE_LOG(LogNet, Warning, TEXT("ARPGSessionSubsystem: Cannot travel - not in session"));
		return;
	}

	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World) return;

	// Append ?listen to make it a listen server
	FString TravelURL = MapPath + TEXT("?listen");
	World->ServerTravel(TravelURL);

	UE_LOG(LogNet, Log, TEXT("ARPGSessionSubsystem: Server traveling to %s"), *TravelURL);
}

void UARPGSessionSubsystem::StartListenServer()
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World) return;

	// Get current map name and travel with ?listen
	FString CurrentMap = World->GetMapName();
	CurrentMap.RemoveFromStart(World->StreamingLevelsPrefix);
	ServerTravelToMap(CurrentMap);
}

// =====================================================================
// Rejoin / Reconnection
// =====================================================================

bool UARPGSessionSubsystem::TryRejoinLastSession()
{
	if (LastConnectString.IsEmpty())
	{
		UE_LOG(LogNet, Warning, TEXT("ARPGSessionSubsystem: No cached session to rejoin"));
		return false;
	}

	SetSessionState(EARPGSessionState::Reconnecting);
	ReconnectAttemptsRemaining = MaxReconnectAttempts;

	AttemptReconnect();
	return true;
}

void UARPGSessionSubsystem::AttemptReconnect()
{
	if (ReconnectAttemptsRemaining <= 0)
	{
		UE_LOG(LogNet, Warning, TEXT("ARPGSessionSubsystem: Max reconnection attempts reached"));
		SetSessionState(EARPGSessionState::None);
		OnNetworkError.Broadcast(TEXT("Failed to reconnect after maximum attempts"), false);
		return;
	}

	ReconnectAttemptsRemaining--;

	APlayerController* PC = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr;
	if (!PC)
	{
		SetSessionState(EARPGSessionState::None);
		return;
	}

	UE_LOG(LogNet, Log, TEXT("ARPGSessionSubsystem: Reconnection attempt %d/%d to %s"),
		MaxReconnectAttempts - ReconnectAttemptsRemaining, MaxReconnectAttempts, *LastConnectString);

	PC->ClientTravel(LastConnectString, TRAVEL_Absolute);
}

void UARPGSessionSubsystem::ClearRejoinData()
{
	LastConnectString.Empty();
	LastSessionDisplayName.Empty();
	ReconnectAttemptsRemaining = 0;

	UGameInstance* GI = GetGameInstance();
	if (GI)
	{
		UWorld* World = GI->GetWorld();
		if (World)
		{
			World->GetTimerManager().ClearTimer(ReconnectTimerHandle);
		}
	}
}

// =====================================================================
// Network Error Handling
// =====================================================================

void UARPGSessionSubsystem::OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	FString ErrorMessage;
	bool bRecoverable = false;

	switch (FailureType)
	{
	case ENetworkFailure::ConnectionLost:
		ErrorMessage = FString::Printf(TEXT("Connection lost: %s"), *ErrorString);
		bRecoverable = true;
		break;
	case ENetworkFailure::ConnectionTimeout:
		ErrorMessage = FString::Printf(TEXT("Connection timed out: %s"), *ErrorString);
		bRecoverable = true;
		break;
	case ENetworkFailure::NetDriverAlreadyExists:
		ErrorMessage = FString::Printf(TEXT("Net driver conflict: %s"), *ErrorString);
		bRecoverable = false;
		break;
	case ENetworkFailure::NetDriverCreateFailure:
		ErrorMessage = FString::Printf(TEXT("Failed to create net driver: %s"), *ErrorString);
		bRecoverable = false;
		break;
	default:
		ErrorMessage = FString::Printf(TEXT("Network error: %s"), *ErrorString);
		bRecoverable = false;
		break;
	}

	UE_LOG(LogNet, Warning, TEXT("ARPGSessionSubsystem: %s (recoverable=%d)"), *ErrorMessage, bRecoverable);

	bInSession = false;
	OnNetworkError.Broadcast(ErrorMessage, bRecoverable);

	// Auto-reconnect for recoverable errors if we have cached session data
	if (bRecoverable && !LastConnectString.IsEmpty() && ReconnectAttemptsRemaining > 0)
	{
		SetSessionState(EARPGSessionState::Reconnecting);

		// Schedule reconnect with delay
		if (World)
		{
			World->GetTimerManager().SetTimer(ReconnectTimerHandle, this,
				&UARPGSessionSubsystem::AttemptReconnect, ReconnectDelay, false);
		}
	}
	else if (!bRecoverable)
	{
		SetSessionState(EARPGSessionState::None);
		ClearRejoinData();
	}
}

void UARPGSessionSubsystem::OnTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	FString ErrorMessage = FString::Printf(TEXT("Travel failure: %s"), *ErrorString);
	UE_LOG(LogNet, Warning, TEXT("ARPGSessionSubsystem: %s"), *ErrorMessage);

	bool bRecoverable = (SessionState == EARPGSessionState::Reconnecting && ReconnectAttemptsRemaining > 0);
	OnNetworkError.Broadcast(ErrorMessage, bRecoverable);

	if (bRecoverable && World)
	{
		// Retry reconnect after delay
		World->GetTimerManager().SetTimer(ReconnectTimerHandle, this,
			&UARPGSessionSubsystem::AttemptReconnect, ReconnectDelay, false);
	}
	else
	{
		SetSessionState(EARPGSessionState::None);
	}
}
