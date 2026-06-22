#include "Test/Combat/VSForcePushKnockbackTest.h"
#include "Player/ARPGPlayerCharacter.h"
#include "Character/ARPGEnemyCharacter.h"
#include "Engine/World.h"

AVSForcePushKnockbackTest::AVSForcePushKnockbackTest()
{
	PrimaryActorTick.bCanEverTick = true;
	TimeLimit = 20.f;
	// Gray-box: empty montages / missing meshes emit warnings that are not failures.
	LogWarningHandling = EFunctionalTestLogHandling::OutputIgnored;

	// One observe phase: the arrangement + activation happen once in OnTestStarted,
	// then we wait for the launch to carry the enemy before asserting displacement.
	Phases = { FName(TEXT("Observe")) };
	PhaseTimeout = 5.f;
}

void AVSForcePushKnockbackTest::OnTestStarted()
{
	AARPGPlayerCharacter* P = GetPlayerCharacter();
	if (!P)
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("Player missing in the test map"));
		return;
	}

	// Spawn a fresh Sith target ~250cm directly ahead of the player (inside the 55-deg
	// cone and the 600cm range) — deterministic regardless of what the map ships with.
	FVector Fwd = P->GetActorForwardVector();
	Fwd.Z = 0.f;
	Fwd = Fwd.GetSafeNormal();
	if (Fwd.IsNearlyZero())
	{
		Fwd = FVector::ForwardVector;
	}
	const FVector SpawnLoc = P->GetActorLocation() + Fwd * 250.f;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AARPGEnemyCharacter* E = GetWorld()->SpawnActor<AARPGEnemyCharacter>(
		AARPGEnemyCharacter::StaticClass(), SpawnLoc, FRotator::ZeroRotator, Params);
	if (!E)
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("Failed to spawn the Sith target"));
		return;
	}
	Enemy = E;
	EnemyStartLoc = E->GetActorLocation();

	// Fire Force Push from slot 0 — the exact path key '1' drives at runtime.
	bActivated = P->TryActivateAbilitySlot(0);
	AssertTrue(bActivated, TEXT("Force Push (hotbar slot 0 / key '1') activated"));
}

EARPGPhaseResult AVSForcePushKnockbackTest::RunPhase(int32 /*PhaseIndex*/, FName PhaseName, float /*DeltaSeconds*/)
{
	if (PhaseName == FName(TEXT("Observe")))
	{
		// Let the launch velocity carry the enemy for a beat before measuring.
		if (GetPhaseTime() < 0.8f)
		{
			return EARPGPhaseResult::Running;
		}

		const AARPGEnemyCharacter* E = GetFirstEnemy();
		const float Moved = E ? FVector::Dist2D(E->GetActorLocation(), EnemyStartLoc) : 0.f;
		const bool bOk = bActivated && Moved > 80.f;
		AssertTrue(bOk,
			FString::Printf(
				TEXT("Force Push should launch the enemy away from its start (moved %.1f cm, need > 80; activated=%d)"),
				Moved, bActivated ? 1 : 0));
		return bOk ? EARPGPhaseResult::Advance : EARPGPhaseResult::Fail;
	}
	return EARPGPhaseResult::Advance;
}
