#include "AI/EQS/EnvQueryTest_ElevationAdvantage.h"
#include "AI/EQS/EnvQueryContext_TargetActor.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"

UEnvQueryTest_ElevationAdvantage::UEnvQueryTest_ElevationAdvantage()
{
	Cost = EEnvTestCost::Low;
	ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();
	ReferenceContext = UEnvQueryContext_TargetActor::StaticClass();

	SetWorkOnFloatValues(true);
}

void UEnvQueryTest_ElevationAdvantage::RunTest(FEnvQueryInstance& QueryInstance) const
{
	UObject* QueryOwner = QueryInstance.Owner.Get();
	if (!QueryOwner)
	{
		return;
	}

	TArray<FVector> RefLocations;
	QueryInstance.PrepareContext(ReferenceContext, RefLocations);
	if (RefLocations.IsEmpty())
	{
		return;
	}

	const float RefZ = RefLocations[0].Z;
	const float MaxBonus = FMath::Max(MaxElevationBonus, 50.f);

	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		const FVector ItemPos = GetItemLocation(QueryInstance, It.GetIndex());
		const float ElevDiff = ItemPos.Z - RefZ;

		float Score;
		if (ElevDiff > 0.f)
		{
			// High ground: scale linearly up to MaxElevationBonus
			Score = FMath::Clamp(ElevDiff / MaxBonus, 0.f, 1.f);
		}
		else if (bPenalizeLowGround)
		{
			// Low ground penalty: mirror the bonus scale into negatives
			Score = FMath::Clamp(ElevDiff / MaxBonus, -1.f, 0.f);
		}
		else
		{
			// No penalty for low ground
			Score = 0.f;
		}

		It.SetScore(TestPurpose, FilterType, Score, FloatValueMin.DefaultValue, FloatValueMax.DefaultValue);
	}
}

FText UEnvQueryTest_ElevationAdvantage::GetDescriptionTitle() const
{
	return FText::FromString(TEXT("Elevation Advantage"));
}

FText UEnvQueryTest_ElevationAdvantage::GetDescriptionDetails() const
{
	return FText::FromString(FString::Printf(
		TEXT("vs %s, max bonus at %.0f UU%s"),
		ReferenceContext ? *ReferenceContext->GetName() : TEXT("None"),
		MaxElevationBonus,
		bPenalizeLowGround ? TEXT(", penalize low") : TEXT("")));
}
