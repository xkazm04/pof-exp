#include "Test/Environment/ProcGenWalkTest.h"
#include "LevelDesign/ARPGLevelGenerator.h"
#include "LevelDesign/ARPGRoomTemplate.h"
#include "Player/ARPGPlayerCharacter.h"
#include "EngineUtils.h"

AProcGenWalkTest::AProcGenWalkTest()
{
	Phases = { TEXT("WalkToConnectedRoom") };
	TimeLimit = 20.f;
}

void AProcGenWalkTest::OnTestStarted()
{
	AARPGLevelGenerator* Gen = nullptr;
	for (TActorIterator<AARPGLevelGenerator> It(GetWorld()); It; ++It) { Gen = *It; break; }
	if (!Gen)
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("No ARPGLevelGenerator in the level"));
		return;
	}

	const TArray<FPlacedRoom>& Rooms = Gen->GetPlacedRooms();
	if (Rooms.Num() < 2)
	{
		FinishTest(EFunctionalTestResult::Failed,
			FString::Printf(TEXT("Need >= 2 placed rooms, found %d"), Rooms.Num()));
		return;
	}

	const FPlacedRoom& Start = Rooms[0];
	if (Start.ConnectedRoomIndices.Num() == 0)
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("Start room has no connections"));
		return;
	}

	const int32 TargetIdx = Start.ConnectedRoomIndices[0];
	if (!Rooms.IsValidIndex(TargetIdx) || !Rooms[TargetIdx].Template)
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("Connected room invalid"));
		return;
	}

	const FPlacedRoom& Target = Rooms[TargetIdx];
	TargetCenter = Target.WorldPosition;
	TargetHalfXY = FVector2D(Target.Template->RoomSize.X * 0.5f, Target.Template->RoomSize.Y * 0.5f);
	bResolved = true;
}

EARPGPhaseResult AProcGenWalkTest::RunPhase(int32 /*PhaseIndex*/, FName /*PhaseName*/, float /*DeltaSeconds*/)
{
	if (!bResolved)
	{
		// OnTestStarted already called FinishTest(Failed) — just stop driving.
		return EARPGPhaseResult::Running;
	}

	AARPGPlayerCharacter* P = GetPlayerCharacter();
	if (!P)
	{
		AssertTrue(false, TEXT("#1 traversal: no player character"));
		return EARPGPhaseResult::Fail;
	}

	const FVector Loc = P->GetActorLocation();
	const FVector Dir = (TargetCenter - Loc).GetSafeNormal2D();
	P->AddMovementInput(Dir, 1.f);

	const bool bArrived =
		FMath::Abs(Loc.X - TargetCenter.X) < TargetHalfXY.X &&
		FMath::Abs(Loc.Y - TargetCenter.Y) < TargetHalfXY.Y;

	if (bArrived)
	{
		AssertTrue(true, TEXT("#1 traversal: player reached the connected room"));
		return EARPGPhaseResult::Advance;
	}

	if (GetPhaseTime() >= 10.f)
	{
		const float Dist = FVector::Dist2D(Loc, TargetCenter);
		AssertTrue(false,
			FString::Printf(TEXT("#1 traversal: player did not reach the connected room (dist %.0f)"), Dist));
		return EARPGPhaseResult::Fail;
	}

	return EARPGPhaseResult::Running;
}
