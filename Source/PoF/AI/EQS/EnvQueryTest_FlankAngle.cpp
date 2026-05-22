#include "AI/EQS/EnvQueryTest_FlankAngle.h"
#include "AI/EQS/EnvQueryContext_TargetActor.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"

UEnvQueryTest_FlankAngle::UEnvQueryTest_FlankAngle()
{
	Cost = EEnvTestCost::Low;
	ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();
	TargetContext = UEnvQueryContext_TargetActor::StaticClass();

	// Default: score mode, prefer high values (= behind the target)
	SetWorkOnFloatValues(true);
}

void UEnvQueryTest_FlankAngle::RunTest(FEnvQueryInstance& QueryInstance) const
{
	UObject* QueryOwner = QueryInstance.Owner.Get();
	if (!QueryOwner)
	{
		return;
	}

	// Resolve the target context (actor whose forward we measure against)
	TArray<FVector> TargetLocations;
	TArray<FRotator> TargetRotations;
	QueryInstance.PrepareContext(TargetContext, TargetLocations);

	// We also need the target's rotation — get it from the actor
	TArray<AActor*> TargetActors;
	QueryInstance.PrepareContext(TargetContext, TargetActors);

	if (TargetLocations.IsEmpty() || TargetActors.IsEmpty() || !TargetActors[0])
	{
		return;
	}

	const FVector TargetLocation = TargetLocations[0];
	const FVector TargetForward = TargetActors[0]->GetActorForwardVector().GetSafeNormal2D();

	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		const FVector ItemLocation = GetItemLocation(QueryInstance, It.GetIndex());

		// Direction from target to this position (2D)
		const FVector DirToItem = (ItemLocation - TargetLocation).GetSafeNormal2D();

		if (DirToItem.IsNearlyZero())
		{
			It.SetScore(TestPurpose, FilterType, 0.f, FloatValueMin.DefaultValue, FloatValueMax.DefaultValue);
			continue;
		}

		// Dot product gives cos(angle), acos gives angle in radians
		const float DotProduct = FVector::DotProduct(TargetForward, DirToItem);
		const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(DotProduct, -1.f, 1.f)));

		// AngleDegrees: 0 = in front, 180 = behind
		It.SetScore(TestPurpose, FilterType, AngleDegrees, FloatValueMin.DefaultValue, FloatValueMax.DefaultValue);
	}
}

FText UEnvQueryTest_FlankAngle::GetDescriptionTitle() const
{
	return FText::FromString(TEXT("Flank Angle"));
}

FText UEnvQueryTest_FlankAngle::GetDescriptionDetails() const
{
	return FText::FromString(FString::Printf(TEXT("Angle from %s forward (0°=front, 180°=behind)"),
		TargetContext ? *TargetContext->GetName() : TEXT("None")));
}
