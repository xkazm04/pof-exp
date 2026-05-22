#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Net/Iris/ReplicationSystem/EngineReplicationBridge.h"
#include "ARPGGameState.generated.h"

class APlayerController;

/** Replicated match phase for game flow. */
UENUM(BlueprintType)
enum class EARPGMatchPhase : uint8
{
	WaitingForPlayers,
	InProgress,
	PostMatch
};

/** Per-player score entry replicated through the GameState. */
USTRUCT(BlueprintType)
struct FARPGPlayerScore
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Score")
	int32 PlayerIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Score")
	FString PlayerName;

	UPROPERTY(BlueprintReadOnly, Category = "Score")
	int32 Kills = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Score")
	int32 Deaths = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Score")
	int32 Assists = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Score")
	float DamageDealt = 0.f;
};

/** Broadcast when match phase changes. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMatchPhaseChanged, EARPGMatchPhase, OldPhase, EARPGMatchPhase, NewPhase);

/** Broadcast when the scoreboard updates. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnScoreboardUpdated);

/**
 * Replicated game state holding match phase, elapsed time, and player scores.
 * Lives on the server and replicates to all clients.
 */
UCLASS()
class POF_API AARPGGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AARPGGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// =====================================================================
	// Match Phase
	// =====================================================================

	UFUNCTION(BlueprintPure, Category = "Match")
	EARPGMatchPhase GetMatchPhase() const { return MatchPhase; }

	/** Server-only: transition to a new match phase. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Match")
	void SetMatchPhase(EARPGMatchPhase NewPhase);

	UFUNCTION(BlueprintPure, Category = "Match")
	float GetMatchElapsedTime() const { return MatchElapsedTime; }

	UFUNCTION(BlueprintPure, Category = "Match")
	int32 GetConnectedPlayerCount() const { return ConnectedPlayerCount; }

	// =====================================================================
	// Scoreboard
	// =====================================================================

	UFUNCTION(BlueprintPure, Category = "Score")
	const TArray<FARPGPlayerScore>& GetScoreboard() const { return Scoreboard; }

	/** Server-only: add a kill to a player's score. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Score")
	void AddKill(int32 PlayerIndex);

	/** Server-only: add a death to a player's score. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Score")
	void AddDeath(int32 PlayerIndex);

	/** Server-only: add an assist to a player's score. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Score")
	void AddAssist(int32 PlayerIndex);

	/** Server-only: add damage dealt to a player's score. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Score")
	void AddDamageDealt(int32 PlayerIndex, float Amount);

	/** Server-only: register a new player on the scoreboard. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Score")
	void RegisterPlayer(int32 PlayerIndex, const FString& PlayerName);

	/** Server-only: remove a player from the scoreboard (on disconnect). */
	void RemovePlayer(APlayerController* PC);

	// =====================================================================
	// Delegates
	// =====================================================================

	UPROPERTY(BlueprintAssignable, Category = "Events|Match")
	FOnMatchPhaseChanged OnMatchPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events|Score")
	FOnScoreboardUpdated OnScoreboardUpdated;

	// =====================================================================
	// Iris Replication (UE 5.7)
	// =====================================================================

	/** Configure Iris replication params — GameState is always relevant. */
	virtual void FillReplicationParams(const FFillReplicationParamsContext& Context, FActorReplicationParams& OutParams) override;

protected:
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(ReplicatedUsing = OnRep_MatchPhase, BlueprintReadOnly, Category = "Match")
	EARPGMatchPhase MatchPhase = EARPGMatchPhase::WaitingForPlayers;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	float MatchElapsedTime = 0.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	int32 ConnectedPlayerCount = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Scoreboard, BlueprintReadOnly, Category = "Score")
	TArray<FARPGPlayerScore> Scoreboard;

private:
	UFUNCTION()
	void OnRep_MatchPhase();

	UFUNCTION()
	void OnRep_Scoreboard();

	/** Find or create a score entry for a player index. Returns nullptr only if not registered. */
	FARPGPlayerScore* FindScore(int32 PlayerIndex);

	EARPGMatchPhase PreviousMatchPhase = EARPGMatchPhase::WaitingForPlayers;
};
