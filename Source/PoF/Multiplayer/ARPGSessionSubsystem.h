#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Engine/NetDriver.h"
#include "ARPGSessionSubsystem.generated.h"

/** Session search result for UI display. */
USTRUCT(BlueprintType)
struct FARPGSessionSearchResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	FString SessionName;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	FString HostName;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 PingInMs = 0;

	/** Internal index into the search results for joining. */
	int32 SearchResultIndex = -1;
};

/** Session state for tracking the full lifecycle. */
UENUM(BlueprintType)
enum class EARPGSessionState : uint8
{
	None,
	Creating,
	InSession,
	Searching,
	Joining,
	Destroying,
	TravelingToServer,
	Reconnecting
};

// --- Delegates ---

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnARPGCreateSessionComplete, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnARPGFindSessionsComplete, bool, bSuccess, const TArray<FARPGSessionSearchResult>&, Results);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnARPGJoinSessionComplete, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnARPGDestroySessionComplete);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnARPGNetworkError, FString, ErrorMessage, bool, bIsRecoverable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnARPGSessionStateChanged, EARPGSessionState, NewState);

/**
 * Session management subsystem using the Online Subsystem.
 * Handles creating, finding, joining, and destroying game sessions.
 * Includes rejoin support, server travel, and network error handling.
 */
UCLASS()
class POF_API UARPGSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// =====================================================================
	// Session Management
	// =====================================================================

	/** Create a new LAN/online session as host. */
	UFUNCTION(BlueprintCallable, Category = "Session")
	void CreateSession(int32 MaxPlayers = 4, bool bIsLAN = true, const FString& SessionDisplayName = TEXT("ARPG Game"));

	/** Search for available sessions. */
	UFUNCTION(BlueprintCallable, Category = "Session")
	void FindSessions(bool bIsLAN = true, int32 MaxResults = 20);

	/** Join a session from search results by index. */
	UFUNCTION(BlueprintCallable, Category = "Session")
	void JoinSession(int32 SearchResultIndex);

	/** Destroy the current session (disconnect). */
	UFUNCTION(BlueprintCallable, Category = "Session")
	void DestroySession();

	/** Whether we currently have an active session. */
	UFUNCTION(BlueprintPure, Category = "Session")
	bool IsInSession() const { return bInSession; }

	/** Whether a session search is in progress. */
	UFUNCTION(BlueprintPure, Category = "Session")
	bool IsSearching() const { return bIsSearching; }

	/** Get the cached search results from the last FindSessions call. */
	UFUNCTION(BlueprintPure, Category = "Session")
	const TArray<FARPGSessionSearchResult>& GetCachedSearchResults() const { return CachedResults; }

	/** Get the current session state. */
	UFUNCTION(BlueprintPure, Category = "Session")
	EARPGSessionState GetSessionState() const { return SessionState; }

	// =====================================================================
	// Server Travel
	// =====================================================================

	/**
	 * Host: perform a server travel to a map after session creation.
	 * @param MapPath Full path to the map (e.g., "/Game/Maps/Arena").
	 */
	UFUNCTION(BlueprintCallable, Category = "Session|Travel")
	void ServerTravelToMap(const FString& MapPath);

	/** Host: start a listen server on the current map. */
	UFUNCTION(BlueprintCallable, Category = "Session|Travel")
	void StartListenServer();

	// =====================================================================
	// Rejoin / Reconnection
	// =====================================================================

	/**
	 * Attempt to rejoin the last known session after a disconnect.
	 * Uses cached connection info from the previous session.
	 * @return true if a rejoin attempt was started.
	 */
	UFUNCTION(BlueprintCallable, Category = "Session|Rejoin")
	bool TryRejoinLastSession();

	/** Whether we have cached info from a previous session that allows rejoin. */
	UFUNCTION(BlueprintPure, Category = "Session|Rejoin")
	bool CanRejoin() const { return !LastConnectString.IsEmpty(); }

	/** Clear cached rejoin data. */
	UFUNCTION(BlueprintCallable, Category = "Session|Rejoin")
	void ClearRejoinData();

	/** Maximum number of automatic reconnection attempts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Session|Rejoin")
	int32 MaxReconnectAttempts = 3;

	/** Delay between reconnection attempts in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Session|Rejoin")
	float ReconnectDelay = 5.f;

	// =====================================================================
	// Delegates
	// =====================================================================

	UPROPERTY(BlueprintAssignable, Category = "Events|Session")
	FOnARPGCreateSessionComplete OnCreateSessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "Events|Session")
	FOnARPGFindSessionsComplete OnFindSessionsComplete;

	UPROPERTY(BlueprintAssignable, Category = "Events|Session")
	FOnARPGJoinSessionComplete OnJoinSessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "Events|Session")
	FOnARPGDestroySessionComplete OnDestroySessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "Events|Session")
	FOnARPGNetworkError OnNetworkError;

	UPROPERTY(BlueprintAssignable, Category = "Events|Session")
	FOnARPGSessionStateChanged OnSessionStateChanged;

private:
	IOnlineSessionPtr GetSessionInterface() const;
	void SetSessionState(EARPGSessionState NewState);

	// --- Online Subsystem callbacks ---
	void OnCreateSessionCompleteInternal(FName SessionName, bool bSuccess);
	void OnFindSessionsCompleteInternal(bool bSuccess);
	void OnJoinSessionCompleteInternal(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionCompleteInternal(FName SessionName, bool bSuccess);

	// --- Network error handling ---
	void OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void OnTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);
	void AttemptReconnect();

	// --- Delegate handles ---
	FDelegateHandle CreateSessionDelegateHandle;
	FDelegateHandle FindSessionsDelegateHandle;
	FDelegateHandle JoinSessionDelegateHandle;
	FDelegateHandle DestroySessionDelegateHandle;

	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	TArray<FARPGSessionSearchResult> CachedResults;

	bool bInSession = false;
	bool bIsSearching = false;
	FString PendingSessionDisplayName;
	EARPGSessionState SessionState = EARPGSessionState::None;

	// --- Rejoin support ---
	FString LastConnectString;
	FString LastSessionDisplayName;
	int32 ReconnectAttemptsRemaining = 0;
	FTimerHandle ReconnectTimerHandle;
};
