#include "AI/BTDecorator_CheckArchetype.h"
#include "AIController.h"

UBTDecorator_CheckArchetype::UBTDecorator_CheckArchetype()
{
	NodeName = TEXT("Check Archetype");
}

bool UBTDecorator_CheckArchetype::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return false;
	}

	const AARPGEnemyCharacter* Enemy = Cast<AARPGEnemyCharacter>(Pawn);
	if (!Enemy)
	{
		return false;
	}

	return Enemy->GetArchetype() == RequiredArchetype;
}

FString UBTDecorator_CheckArchetype::GetStaticDescription() const
{
	const UEnum* EnumPtr = StaticEnum<EEnemyArchetype>();
	const FString ArchetypeName = EnumPtr ? EnumPtr->GetDisplayNameTextByValue(static_cast<int64>(RequiredArchetype)).ToString() : TEXT("Unknown");

	return FString::Printf(TEXT("Archetype == %s"), *ArchetypeName);
}
