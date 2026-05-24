#include "Test/Environment/VSArenaBoundsTest.h"
#include "Player/ARPGPlayerCharacter.h"

AVSArenaBoundsTest::AVSArenaBoundsTest()
{
	Phases = { TEXT("WalkIntoWall") };
	TimeLimit = 20.f;
}

void AVSArenaBoundsTest::OnTestStarted()
{
	if (AARPGPlayerCharacter* P = GetPlayerCharacter())
	{
		StartLoc = P->GetActorLocation();
	}
}

EARPGPhaseResult AVSArenaBoundsTest::RunPhase(int32 /*PhaseIndex*/, FName /*PhaseName*/, float /*DeltaSeconds*/)
{
	AARPGPlayerCharacter* P = GetPlayerCharacter();
	if (!P)
	{
		AssertTrue(false, TEXT("#1 bounds: no player character"));
		return EARPGPhaseResult::Fail;
	}

	// Drive toward the +X wall (Wall_E at ~+1000 uu).
	P->AddMovementInput(FVector::ForwardVector, 1.f);

	if (GetPhaseTime() >= 4.f)
	{
		const FVector Loc = P->GetActorLocation();
		AssertTrue(Loc.X > StartLoc.X + 50.f,
			FString::Printf(TEXT("#1 bounds: player moved toward the +X wall (X %.1f -> %.1f)"), StartLoc.X, Loc.X));
		AssertTrue(Loc.X < 1000.f,
			FString::Printf(TEXT("#1 bounds: wall blocked the player inside the arena (X=%.1f, wall ~1000)"), Loc.X));
		return EARPGPhaseResult::Advance;
	}
	return EARPGPhaseResult::Running;
}
