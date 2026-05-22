#include "AI/EQS/EnvQueryTest_PathExists.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "NavigationSystem.h"

UEnvQueryTest_PathExists::UEnvQueryTest_PathExists()
{
	Cost = EEnvTestCost::High;
	ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();
	PathFromContext = UEnvQueryContext_Querier::StaticClass();

	// Boolean test: 1.0 = reachable, 0.0 = not
	SetWorkOnFloatValues(true);
}

void UEnvQueryTest_PathExists::RunTest(FEnvQueryInstance& QueryInstance) const
{
	UObject* QueryOwner = QueryInstance.Owner.Get();
	if (!QueryOwner)
	{
		return;
	}

	TArray<FVector> FromLocations;
	QueryInstance.PrepareContext(PathFromContext, FromLocations);
	if (FromLocations.IsEmpty())
	{
		return;
	}

	const FVector FromLocation = FromLocations[0];

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(QueryOwner->GetWorld());
	if (!NavSys)
	{
		return;
	}

	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		const FVector ItemLocation = GetItemLocation(QueryInstance, It.GetIndex());

		// Use TestPathSync which returns a simple bool for reachability
		const FPathFindingQuery PathQuery(QueryOwner, *NavSys->GetDefaultNavDataInstance(), FromLocation, ItemLocation);
		const bool bReachable = NavSys->TestPathSync(PathQuery);
		const float Score = bReachable ? 1.f : 0.f;

		It.SetScore(TestPurpose, FilterType, Score, FloatValueMin.DefaultValue, FloatValueMax.DefaultValue);
	}
}

FText UEnvQueryTest_PathExists::GetDescriptionTitle() const
{
	return FText::FromString(TEXT("Path Exists"));
}

FText UEnvQueryTest_PathExists::GetDescriptionDetails() const
{
	return FText::FromString(FString::Printf(TEXT("Path from %s (1.0 = reachable, 0.0 = blocked)"),
		PathFromContext ? *PathFromContext->GetName() : TEXT("None")));
}
