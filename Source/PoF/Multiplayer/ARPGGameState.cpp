#include "ARPGGameState.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

AARPGGameState::AARPGGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
}

void AARPGGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AARPGGameState, MatchPhase);
	DOREPLIFETIME(AARPGGameState, MatchElapsedTime);
	DOREPLIFETIME(AARPGGameState, ConnectedPlayerCount);
	DOREPLIFETIME(AARPGGameState, Scoreboard);
}

void AARPGGameState::FillReplicationParams(const FFillReplicationParamsContext& Context, FActorReplicationParams& OutParams)
{
	Super::FillReplicationParams(Context, OutParams);
	// GameState must be replicated to all connections — always relevant, no spatial filtering
	OutParams.FilterType = FActorReplicationParams::AlwaysRelevant;
}

void AARPGGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority() && MatchPhase == EARPGMatchPhase::InProgress)
	{
		MatchElapsedTime += DeltaSeconds;
	}
}

void AARPGGameState::SetMatchPhase(EARPGMatchPhase NewPhase)
{
	if (!HasAuthority()) return;
	if (MatchPhase == NewPhase) return;

	EARPGMatchPhase OldPhase = MatchPhase;
	MatchPhase = NewPhase;
	OnMatchPhaseChanged.Broadcast(OldPhase, NewPhase);
}

void AARPGGameState::OnRep_MatchPhase()
{
	if (PreviousMatchPhase != MatchPhase)
	{
		OnMatchPhaseChanged.Broadcast(PreviousMatchPhase, MatchPhase);
		PreviousMatchPhase = MatchPhase;
	}
}

void AARPGGameState::OnRep_Scoreboard()
{
	OnScoreboardUpdated.Broadcast();
}

FARPGPlayerScore* AARPGGameState::FindScore(int32 PlayerIndex)
{
	for (FARPGPlayerScore& Entry : Scoreboard)
	{
		if (Entry.PlayerIndex == PlayerIndex)
		{
			return &Entry;
		}
	}
	return nullptr;
}

void AARPGGameState::RegisterPlayer(int32 PlayerIndex, const FString& PlayerName)
{
	if (!HasAuthority()) return;
	if (FindScore(PlayerIndex)) return; // Already registered

	FARPGPlayerScore NewEntry;
	NewEntry.PlayerIndex = PlayerIndex;
	NewEntry.PlayerName = PlayerName;
	Scoreboard.Add(NewEntry);
	ConnectedPlayerCount = Scoreboard.Num();
	OnScoreboardUpdated.Broadcast();
}

void AARPGGameState::RemovePlayer(APlayerController* PC)
{
	if (!HasAuthority() || !PC || !PC->PlayerState) return;

	int32 PlayerId = PC->PlayerState->GetPlayerId();
	Scoreboard.RemoveAll([PlayerId](const FARPGPlayerScore& Entry)
	{
		return Entry.PlayerIndex == PlayerId;
	});
	ConnectedPlayerCount = Scoreboard.Num();
	OnScoreboardUpdated.Broadcast();
}

void AARPGGameState::AddKill(int32 PlayerIndex)
{
	if (!HasAuthority()) return;
	if (FARPGPlayerScore* Score = FindScore(PlayerIndex))
	{
		Score->Kills++;
		OnScoreboardUpdated.Broadcast();
	}
}

void AARPGGameState::AddDeath(int32 PlayerIndex)
{
	if (!HasAuthority()) return;
	if (FARPGPlayerScore* Score = FindScore(PlayerIndex))
	{
		Score->Deaths++;
		OnScoreboardUpdated.Broadcast();
	}
}

void AARPGGameState::AddAssist(int32 PlayerIndex)
{
	if (!HasAuthority()) return;
	if (FARPGPlayerScore* Score = FindScore(PlayerIndex))
	{
		Score->Assists++;
		OnScoreboardUpdated.Broadcast();
	}
}

void AARPGGameState::AddDamageDealt(int32 PlayerIndex, float Amount)
{
	if (!HasAuthority()) return;
	if (FARPGPlayerScore* Score = FindScore(PlayerIndex))
	{
		Score->DamageDealt += Amount;
		OnScoreboardUpdated.Broadcast();
	}
}
