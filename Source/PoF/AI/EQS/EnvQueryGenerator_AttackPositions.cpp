#include "AI/EQS/EnvQueryGenerator_AttackPositions.h"
#include "AI/EQS/EnvQueryContext_TargetActor.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"

UEnvQueryGenerator_AttackPositions::UEnvQueryGenerator_AttackPositions()
{
	CenterContext = UEnvQueryContext_TargetActor::StaticClass();

	ProjectionData.TraceMode = EEnvQueryTrace::Navigation;
	ProjectionData.bCanProjectDown = true;
	ProjectionData.bCanDisableTrace = false;
	ProjectionData.ProjectDown = 500.f;
	ProjectionData.ProjectUp = 100.f;
}

void UEnvQueryGenerator_AttackPositions::GenerateItems(FEnvQueryInstance& QueryInstance) const
{
	TArray<FVector> CenterLocations;
	QueryInstance.PrepareContext(CenterContext, CenterLocations);
	if (CenterLocations.IsEmpty())
	{
		return;
	}

	const FVector Center = CenterLocations[0];
	const float Dist = FMath::Max(AttackDistance, 50.f);

	TArray<FNavLocation> GeneratedPoints;
	const int32 TotalRings = bGenerateInnerRing ? 2 : 1;
	GeneratedPoints.Reserve(NumberOfPoints * TotalRings);

	for (int32 Ring = 0; Ring < TotalRings; ++Ring)
	{
		const float RingDist = (Ring == 0) ? Dist : Dist * 0.5f;
		const float AngleStep = 2.f * PI / static_cast<float>(NumberOfPoints);

		for (int32 i = 0; i < NumberOfPoints; ++i)
		{
			const float Angle = AngleStep * i;
			FNavLocation Point;
			Point.Location = Center + FVector(FMath::Cos(Angle) * RingDist, FMath::Sin(Angle) * RingDist, 0.f);
			GeneratedPoints.Add(Point);
		}
	}

	ProjectAndFilterNavPoints(GeneratedPoints, QueryInstance);
	StoreNavPoints(GeneratedPoints, QueryInstance);
}

FText UEnvQueryGenerator_AttackPositions::GetDescriptionTitle() const
{
	return FText::FromString(TEXT("Attack Ring Positions"));
}

FText UEnvQueryGenerator_AttackPositions::GetDescriptionDetails() const
{
	return FText::FromString(FString::Printf(TEXT("%d pts at dist %.0f around %s%s"),
		NumberOfPoints, AttackDistance,
		CenterContext ? *CenterContext->GetName() : TEXT("None"),
		bGenerateInnerRing ? TEXT(" (+inner)") : TEXT("")));
}
