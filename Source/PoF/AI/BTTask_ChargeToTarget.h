#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ChargeToTarget.generated.h"

/**
 * Brute charge attack: boosts movement speed, runs straight at the target,
 * then applies a vulnerability window after reaching it (or after a timeout).
 * Applies State.Charging gameplay tag during charge, State.Vulnerable after.
 */

struct FBTChargeMemory
{
	float OriginalSpeed = 0.f;
	float ElapsedTime = 0.f;
	bool bInVulnerabilityWindow = false;
	float VulnerabilityElapsed = 0.f;
};

UCLASS()
class POF_API UBTTask_ChargeToTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ChargeToTarget();

	/** Blackboard key for the target actor. */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	/** Speed multiplier applied to MaxWalkSpeed during the charge. */
	UPROPERTY(EditAnywhere, Category = "Charge", meta = (ClampMin = "1.0"))
	float ChargeSpeedMultiplier = 3.0f;

	/** Max duration of the charge before it times out. */
	UPROPERTY(EditAnywhere, Category = "Charge", meta = (ClampMin = "0.5"))
	float MaxChargeDuration = 3.0f;

	/** How long the Brute is vulnerable after charging. */
	UPROPERTY(EditAnywhere, Category = "Charge", meta = (ClampMin = "0"))
	float VulnerabilityDuration = 2.0f;

	/** Acceptance radius for reaching the charge target. */
	UPROPERTY(EditAnywhere, Category = "Charge", meta = (ClampMin = "10"))
	float AcceptanceRadius = 100.f;

	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTChargeMemory); }

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

private:
	void StartVulnerabilityWindow(UBehaviorTreeComponent& OwnerComp, FBTChargeMemory* Memory);
	void RestoreSpeed(APawn* Pawn, float OriginalSpeed);
	void CleanupTags(APawn* Pawn);
};
