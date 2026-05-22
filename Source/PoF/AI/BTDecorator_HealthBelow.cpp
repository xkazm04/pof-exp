#include "AI/BTDecorator_HealthBelow.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/ARPGAttributeSet.h"

UBTDecorator_HealthBelow::UBTDecorator_HealthBelow()
{
	NodeName = TEXT("Health Below Threshold");
	FlowAbortMode = EBTFlowAbortMode::Both;
}

bool UBTDecorator_HealthBelow::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return false;
	}

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Pawn);
	UAbilitySystemComponent* ASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return false;
	}

	const UARPGAttributeSet* Attributes = ASC->GetSet<UARPGAttributeSet>();
	if (!Attributes)
	{
		return false;
	}

	const float MaxHealth = Attributes->GetMaxHealth();
	if (MaxHealth <= 0.f)
	{
		return false;
	}

	const float HealthRatio = Attributes->GetHealth() / MaxHealth;
	return HealthRatio < HealthThreshold;
}

FString UBTDecorator_HealthBelow::GetStaticDescription() const
{
	return FString::Printf(TEXT("Health < %.0f%%"), HealthThreshold * 100.f);
}
