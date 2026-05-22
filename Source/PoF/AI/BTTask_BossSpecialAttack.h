#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpec.h"
#include "BTTask_BossSpecialAttack.generated.h"

/**
 * BT task that activates a boss ability by gameplay tag.
 * Waits for the ability to finish before returning success.
 * Uses per-instance memory to track the active ability handle.
 */
UCLASS()
class POF_API UBTTask_BossSpecialAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_BossSpecialAttack();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual FString GetStaticDescription() const override;

protected:
	/** Ability tag to activate (must match an ability granted to the boss). */
	UPROPERTY(EditAnywhere, Category = "Boss|Ability")
	FGameplayTag AbilityTag;

	/** If true, waits for the ability to finish. If false, fires and returns immediately. */
	UPROPERTY(EditAnywhere, Category = "Boss|Ability")
	bool bWaitForEnd = true;
};

struct FBTBossAttackMemory
{
	FDelegateHandle AbilityEndedHandle;
	FGameplayAbilitySpecHandle ActiveAbilityHandle;
	bool bAbilityEnded = false;
};
