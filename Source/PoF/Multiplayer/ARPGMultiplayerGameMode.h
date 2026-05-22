#pragma once

#include "CoreMinimal.h"
#include "Framework/ARPGGameMode.h"
#include "Net/Iris/ReplicationSystem/EngineReplicationBridge.h"
#include "ARPGMultiplayerGameMode.generated.h"

class AARPGGameState;

/**
 * Game mode for multiplayer sessions.
 * Extends the base ARPG game mode with player connection handling,
 * match phase management, and scoreboard registration.
 */
UCLASS()
class POF_API AARPGMultiplayerGameMode : public AARPGGameMode
{
	GENERATED_BODY()

public:
	AARPGMultiplayerGameMode();

	virtual void InitGameState() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	/** Start the match (transition from WaitingForPlayers to InProgress). */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Match")
	void StartMatch();

	/** End the match (transition to PostMatch). */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Match")
	void EndMatch();

	/** Minimum players required before match can start. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match")
	int32 MinPlayersToStart = 1;

	/** Maximum players allowed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match")
	int32 MaxPlayersAllowed = 4;

	/** Whether to auto-start when MinPlayersToStart is reached. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match")
	bool bAutoStartWhenReady = true;

	// Iris Replication (UE 5.7) — GameMode is always relevant (server-only actor)
	virtual void FillReplicationParams(const FFillReplicationParamsContext& Context, FActorReplicationParams& OutParams) override;

private:
	void CheckAutoStart();
};
