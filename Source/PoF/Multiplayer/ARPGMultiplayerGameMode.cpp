#include "ARPGMultiplayerGameMode.h"
#include "ARPGGameState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

AARPGMultiplayerGameMode::AARPGMultiplayerGameMode()
{
	GameStateClass = AARPGGameState::StaticClass();
}

void AARPGMultiplayerGameMode::InitGameState()
{
	Super::InitGameState();
}

void AARPGMultiplayerGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	AARPGGameState* GS = GetGameState<AARPGGameState>();
	if (GS && NewPlayer && NewPlayer->PlayerState)
	{
		FString PlayerName = NewPlayer->PlayerState->GetPlayerName();
		int32 PlayerId = NewPlayer->PlayerState->GetPlayerId();
		GS->RegisterPlayer(PlayerId, PlayerName);

		CheckAutoStart();
	}
}

void AARPGMultiplayerGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	AARPGGameState* GS = GetGameState<AARPGGameState>();
	if (GS)
	{
		// Find and remove the player from the scoreboard
		APlayerController* PC = Cast<APlayerController>(Exiting);
		if (PC)
		{
			GS->RemovePlayer(PC);
		}
		UE_LOG(LogNet, Log, TEXT("ARPGMultiplayerGameMode: Player disconnected. Remaining: %d"), GS->GetConnectedPlayerCount());
	}
}

void AARPGMultiplayerGameMode::StartMatch()
{
	AARPGGameState* GS = GetGameState<AARPGGameState>();
	if (GS)
	{
		GS->SetMatchPhase(EARPGMatchPhase::InProgress);
		UE_LOG(LogNet, Log, TEXT("ARPGMultiplayerGameMode: Match started"));
	}
}

void AARPGMultiplayerGameMode::EndMatch()
{
	AARPGGameState* GS = GetGameState<AARPGGameState>();
	if (GS)
	{
		GS->SetMatchPhase(EARPGMatchPhase::PostMatch);
		UE_LOG(LogNet, Log, TEXT("ARPGMultiplayerGameMode: Match ended"));
	}
}

void AARPGMultiplayerGameMode::FillReplicationParams(const FFillReplicationParamsContext& Context, FActorReplicationParams& OutParams)
{
	Super::FillReplicationParams(Context, OutParams);
	OutParams.FilterType = FActorReplicationParams::AlwaysRelevant;
}

void AARPGMultiplayerGameMode::CheckAutoStart()
{
	if (!bAutoStartWhenReady) return;

	AARPGGameState* GS = GetGameState<AARPGGameState>();
	if (!GS) return;

	if (GS->GetMatchPhase() != EARPGMatchPhase::WaitingForPlayers) return;

	if (GS->GetConnectedPlayerCount() >= MinPlayersToStart)
	{
		StartMatch();
	}
}
