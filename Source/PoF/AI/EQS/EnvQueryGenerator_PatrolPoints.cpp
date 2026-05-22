#include "AI/EQS/EnvQueryGenerator_PatrolPoints.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"

UEnvQueryGenerator_PatrolPoints::UEnvQueryGenerator_PatrolPoints()
{
	ProjectionData.TraceMode = EEnvQueryTrace::Navigation;
	ProjectionData.bCanProjectDown = true;
	ProjectionData.bCanDisableTrace = false;
	ProjectionData.ProjectDown = 500.f;
	ProjectionData.ProjectUp = 100.f;
}

void UEnvQueryGenerator_PatrolPoints::GenerateItems(FEnvQueryInstance& QueryInstance) const
{
	TArray<FVector> QuerierLocations;
	QueryInstance.PrepareContext(UEnvQueryContext_Querier::StaticClass(), QuerierLocations);
	if (QuerierLocations.IsEmpty())
	{
		return;
	}

	const FVector Origin = QuerierLocations[0];
	const float MinR = FMath::Max(MinRadius, 0.f);
	const float MaxR = FMath::Max(MaxRadius, MinR + 1.f);

	TArray<FNavLocation> GeneratedPoints;
	GeneratedPoints.Reserve(NumberOfPoints);

	for (int32 i = 0; i < NumberOfPoints; ++i)
	{
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		const float Radius = FMath::FRandRange(MinR, MaxR);

		FNavLocation Point;
		Point.Location = Origin + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
		GeneratedPoints.Add(Point);
	}

	ProjectAndFilterNavPoints(GeneratedPoints, QueryInstance);
	StoreNavPoints(GeneratedPoints, QueryInstance);
}

FText UEnvQueryGenerator_PatrolPoints::GetDescriptionTitle() const
{
	return FText::FromString(TEXT("Patrol Points"));
}

FText UEnvQueryGenerator_PatrolPoints::GetDescriptionDetails() const
{
	return FText::FromString(FString::Printf(TEXT("%d points, radius [%.0f - %.0f]"),
		NumberOfPoints, MinRadius, MaxRadius));
}
