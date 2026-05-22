#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpec.h"
#include "BTTask_ExecuteAttackAbility.generated.h"

struct FBTAttackAbilityMemory
{
	FDelegateHandle AbilityEndedHandle;
	FGameplayAbilitySpecHandle ActivatedAbilityHandle;
	bool bAbilityActivated = false;
};

/**
 * Activates a gameplay ability on the pawn's ASC by tag, then waits for it to end.
 * Reads the ability tag from the blackboard (PrimaryAbilityTag key) if UseBlackboardTag
 * is true, otherwise uses the hardcoded AbilityTag property.
 */
UCLASS()
class POF_API UBTTask_ExecuteAttackAbility : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ExecuteAttackAbility();

	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTAttackAbilityMemory); }

protected:
	/**
	 * Gameplay tag identifying the ability to activate.
	 * Must match the AbilityTags on the granted ability.
	 */
	UPROPERTY(EditAnywhere, Category = "Ability")
	FGameplayTag AbilityTag;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

private:
	void OnAbilityEnded(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory);
};
