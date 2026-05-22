#include "AI/BTDecorator_BossPhase.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/ARPGBossCharacter.h"

UBTDecorator_BossPhase::UBTDecorator_BossPhase()
{
	NodeName = TEXT("Boss Phase Check");
}

bool UBTDecorator_BossPhase::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return false;

	const int32 CurrentPhase = BB->GetValueAsInt(AARPGBossCharacter::KEY_BossPhase);

	if (CurrentPhase < MinPhase) return false;
	if (MaxPhase >= 0 && CurrentPhase > MaxPhase) return false;

	return true;
}

FString UBTDecorator_BossPhase::GetStaticDescription() const
{
	if (MaxPhase < 0)
	{
		return FString::Printf(TEXT("Phase >= %d"), MinPhase);
	}
	if (MinPhase == MaxPhase)
	{
		return FString::Printf(TEXT("Phase == %d"), MinPhase);
	}
	return FString::Printf(TEXT("Phase %d-%d"), MinPhase, MaxPhase);
}
