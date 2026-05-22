#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "Character/ARPGEnemyCharacter.h"
#include "BTDecorator_CheckArchetype.generated.h"

/**
 * Decorator that checks whether the controlled pawn's archetype matches
 * a specified value. Used to branch behavior trees per archetype
 * (e.g., only Brute runs the charge subtree).
 */
UCLASS()
class POF_API UBTDecorator_CheckArchetype : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_CheckArchetype();

	/** The archetype to check for. */
	UPROPERTY(EditAnywhere, Category = "Archetype")
	EEnemyArchetype RequiredArchetype = EEnemyArchetype::MeleeGrunt;

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;
};
