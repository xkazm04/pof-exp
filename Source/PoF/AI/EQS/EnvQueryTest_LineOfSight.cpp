#include "AI/EQS/EnvQueryTest_LineOfSight.h"
#include "AI/EQS/EnvQueryContext_TargetActor.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "CollisionQueryParams.h"

UEnvQueryTest_LineOfSight::UEnvQueryTest_LineOfSight()
{
	Cost = EEnvTestCost::High;
	ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();
	ThreatContext = UEnvQueryContext_TargetActor::StaticClass();

	// Higher score = better cover (less exposure)
	SetWorkOnFloatValues(true);
}

void UEnvQueryTest_LineOfSight::RunTest(FEnvQueryInstance& QueryInstance) const
{
	UObject* QueryOwner = QueryInstance.Owner.Get();
	if (!QueryOwner)
	{
		return;
	}

	TArray<FVector> ThreatLocations;
	QueryInstance.PrepareContext(ThreatContext, ThreatLocations);
	if (ThreatLocations.IsEmpty())
	{
		return;
	}

	const FVector ThreatPos = ThreatLocations[0];
	const UWorld* World = GEngine->GetWorldFromContextObject(QueryOwner, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return;
	}

	const int32 TraceCount = FMath::Clamp(NumberOfTraceHeights, 1, 5);
	const float MinH = FMath::Max(MinTraceHeight, 0.f);
	const float MaxH = FMath::Max(MaxTraceHeight, MinH + 10.f);

	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(LOSExposureTrace), true, Cast<AActor>(QueryOwner));

	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		const FVector ItemPos = GetItemLocation(QueryInstance, It.GetIndex());

		int32 OccludedCount = 0;

		for (int32 TraceIdx = 0; TraceIdx < TraceCount; ++TraceIdx)
		{
			// Spread traces vertically from crouch to standing height
			const float HeightAlpha = (TraceCount == 1) ? 0.5f : static_cast<float>(TraceIdx) / (TraceCount - 1);
			const float HeightOffset = FMath::Lerp(MinH, MaxH, HeightAlpha);

			const FVector TraceStart = ItemPos + FVector(0.f, 0.f, HeightOffset);
			// Trace toward the threat at their approximate center mass height
			const FVector TraceEnd = ThreatPos + FVector(0.f, 0.f, 90.f);

			FHitResult Hit;
			const bool bHit = World->LineTraceSingleByChannel(
				Hit, TraceStart, TraceEnd, TraceChannel, TraceParams
			);

			if (bHit && Hit.bBlockingHit)
			{
				// Line of sight is blocked at this height — good for cover
				++OccludedCount;
			}
		}

		// Score: 0.0 = fully exposed, 1.0 = fully hidden
		const float CoverScore = static_cast<float>(OccludedCount) / static_cast<float>(TraceCount);
		It.SetScore(TestPurpose, FilterType, CoverScore, FloatValueMin.DefaultValue, FloatValueMax.DefaultValue);
	}
}

FText UEnvQueryTest_LineOfSight::GetDescriptionTitle() const
{
	return FText::FromString(TEXT("Line of Sight Exposure"));
}

FText UEnvQueryTest_LineOfSight::GetDescriptionDetails() const
{
	return FText::FromString(FString::Printf(
		TEXT("%d traces [%.0f-%.0f height] vs %s (0=exposed, 1=covered)"),
		NumberOfTraceHeights, MinTraceHeight, MaxTraceHeight,
		ThreatContext ? *ThreatContext->GetName() : TEXT("None")));
}
