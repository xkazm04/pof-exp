#include "Test/Environment/VSArenaBoundsTest.h"
#include "Player/ARPGPlayerCharacter.h"

AVSArenaBoundsTest::AVSArenaBoundsTest()
{
	Phases = { TEXT("SweepIntoWall") };
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

	// Headless characters walk far too slowly to traverse the arena in a phase,
	// so we test containment directly: sweep the player capsule from a clear
	// lane (collision-test probes sit at y=0 and y=±600) far past the +X wall.
	// The arena wall geometry must block the swept move and stop the capsule
	// before it escapes — proving the wall contains the player.
	const FVector LaneStart(StartLoc.X, 250.f, StartLoc.Z);
	P->SetActorLocation(LaneStart, /*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);

	FHitResult Hit;
	const FVector Target(5000.f, 250.f, StartLoc.Z);
	P->SetActorLocation(Target, /*bSweep=*/true, &Hit);
	const FVector Loc = P->GetActorLocation();

	AssertTrue(Hit.bBlockingHit,
		FString::Printf(TEXT("#1 bounds: the +X wall blocked the swept move (blocked=%s, hit '%s')"),
			Hit.bBlockingHit ? TEXT("true") : TEXT("false"), *GetNameSafe(Hit.GetActor())));
	AssertTrue(Loc.X > LaneStart.X + 500.f,
		FString::Printf(TEXT("#2 bounds: capsule swept well into the arena before stopping (X %.1f -> %.1f)"), LaneStart.X, Loc.X));
	AssertTrue(Loc.X < 1500.f,
		FString::Printf(TEXT("#3 bounds: wall contained the player inside the arena (stopped at X=%.1f, wall ~1000)"), Loc.X));

	// These functional tests share one world + player and run in sequence, so
	// restore the player to its spawn — otherwise a later test (e.g. the walk
	// leg of VSFunctionalTest) starts pinned against the wall and fails.
	P->SetActorLocation(StartLoc, /*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);
	return EARPGPhaseResult::Advance;
}
