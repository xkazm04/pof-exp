#include "AI/EQS/EnvQueryGenerator_CoverPositions.h"
#include "AI/EQS/EnvQueryContext_TargetActor.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "CollisionQueryParams.h"

UEnvQueryGenerator_CoverPositions::UEnvQueryGenerator_CoverPositions()
{
	ThreatContext = UEnvQueryContext_TargetActor::StaticClass();

	ProjectionData.TraceMode = EEnvQueryTrace::Navigation;
	ProjectionData.bCanProjectDown = true;
	ProjectionData.bCanDisableTrace = false;
	ProjectionData.ProjectDown = 500.f;
	ProjectionData.ProjectUp = 100.f;
}

void UEnvQueryGenerator_CoverPositions::GenerateItems(FEnvQueryInstance& QueryInstance) const
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

	const float MinR = FMath::Max(MinRadius, 100.f);
	const float MaxR = FMath::Max(MaxRadius, MinR + 100.f);
	const float CheckDist = FMath::Max(CoverCheckDistance, 50.f);
	const int32 Rings = FMath::Clamp(NumberOfRings, 1, 5);

	TArray<FNavLocation> CoverPoints;
	CoverPoints.Reserve(SampleCount * Rings);

	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(CoverPositionTrace), true, Cast<AActor>(QueryOwner));

	for (int32 RingIdx = 0; RingIdx < Rings; ++RingIdx)
	{
		// Interpolate radius for this ring
		const float Alpha = (Rings == 1) ? 0.5f : static_cast<float>(RingIdx) / (Rings - 1);
		const float RingRadius = FMath::Lerp(MinR, MaxR, Alpha);

		const float AngleStep = 2.f * PI / static_cast<float>(SampleCount);

		for (int32 i = 0; i < SampleCount; ++i)
		{
			const float Angle = AngleStep * i;
			const FVector CandidatePos = ThreatPos + FVector(
				FMath::Cos(Angle) * RingRadius,
				FMath::Sin(Angle) * RingRadius,
				0.f
			);

			// Direction from threat to candidate (this is the "away from threat" direction)
			const FVector AwayDir = (CandidatePos - ThreatPos).GetSafeNormal2D();

			// Trace from candidate outward (away from threat) to find nearby cover geometry
			const FVector TraceStart = CandidatePos;
			const FVector TraceEnd = CandidatePos + AwayDir * CheckDist;

			FHitResult Hit;
			const bool bHit = World->LineTraceSingleByChannel(
				Hit, TraceStart, TraceEnd, TraceChannel, TraceParams
			);

			if (bHit && Hit.bBlockingHit)
			{
				// Place the cover point just in front of the hit surface (on the threat-facing side)
				const FVector CoverPos = Hit.ImpactPoint - AwayDir * 60.f; // 60 UU offset from wall
				FNavLocation Point;
				Point.Location = CoverPos;
				CoverPoints.Add(Point);
			}
			else
			{
				// Also trace from candidate toward the threat — can find pillars/obstacles between
				const FVector TraceEnd2 = CandidatePos - AwayDir * CheckDist;
				FHitResult Hit2;
				const bool bHit2 = World->LineTraceSingleByChannel(
					Hit2, TraceStart, TraceEnd2, TraceChannel, TraceParams
				);

				if (bHit2 && Hit2.bBlockingHit)
				{
					// Place the cover point on the far side of the obstacle from the threat
					const FVector CoverPos = Hit2.ImpactPoint + AwayDir * 60.f;
					FNavLocation Point;
					Point.Location = CoverPos;
					CoverPoints.Add(Point);
				}
			}
		}
	}

	ProjectAndFilterNavPoints(CoverPoints, QueryInstance);
	StoreNavPoints(CoverPoints, QueryInstance);
}

FText UEnvQueryGenerator_CoverPositions::GetDescriptionTitle() const
{
	return FText::FromString(TEXT("Cover Positions"));
}

FText UEnvQueryGenerator_CoverPositions::GetDescriptionDetails() const
{
	return FText::FromString(FString::Printf(
		TEXT("%d samples x %d rings, radius [%.0f-%.0f], check dist %.0f"),
		SampleCount, NumberOfRings, MinRadius, MaxRadius, CoverCheckDistance));
}
