#pragma once

#include "CoreMinimal.h"
#include "Character/ARPGEnemyCharacter.h"
#include "ARPGBossCharacter.generated.h"

class UGameplayAbility;
class UGameplayEffect;

/** Configuration for a single boss phase. */
USTRUCT(BlueprintType)
struct FBossPhaseConfig
{
	GENERATED_BODY()

	/** Health percentage threshold to enter this phase (1.0 = full health). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Phase", meta = (ClampMin = "0", ClampMax = "1"))
	float HealthThreshold = 0.5f;

	/** Additional abilities granted when entering this phase. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Phase")
	TArray<TSubclassOf<UGameplayAbility>> PhaseAbilities;

	/** GE applied to boss on phase entry (e.g., enrage buff, speed buff). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Phase")
	TSubclassOf<UGameplayEffect> PhaseBuffEffect;

	/** Montage to play on phase transition (roar, transform, etc). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Phase")
	TObjectPtr<UAnimMontage> PhaseTransitionMontage;

	/** Attack cooldown override for this phase. 0 = use default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Phase", meta = (ClampMin = "0"))
	float AttackCooldownOverride = 0.f;

	/** Display name for this phase (e.g., "Enraged"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Phase")
	FText PhaseName;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBossPhaseChanged, int32, NewPhase, int32, TotalPhases);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBossDefeated);

/**
 * Boss enemy with multi-phase support.
 * Phases transition based on health thresholds. Each phase can grant new abilities,
 * apply buff effects, and override attack cooldowns.
 * Works with the standard BT — writes BossPhase to blackboard for BT branching.
 */
UCLASS()
class POF_API AARPGBossCharacter : public AARPGEnemyCharacter
{
	GENERATED_BODY()

public:
	AARPGBossCharacter();

	virtual void PossessedBy(AController* NewController) override;

	UFUNCTION(BlueprintPure, Category = "Boss")
	int32 GetCurrentPhase() const { return CurrentPhaseIndex; }

	UFUNCTION(BlueprintPure, Category = "Boss")
	int32 GetTotalPhases() const { return Phases.Num(); }

	UFUNCTION(BlueprintPure, Category = "Boss")
	FText GetBossName() const { return BossName; }

	UFUNCTION(BlueprintPure, Category = "Boss")
	float GetHealthPercentage() const;

	/** Manually force a phase transition (for scripted encounters). */
	UFUNCTION(BlueprintCallable, Category = "Boss")
	void ForcePhaseTransition(int32 PhaseIndex);

	UPROPERTY(BlueprintAssignable, Category = "Events|Boss")
	FOnBossPhaseChanged OnBossPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events|Boss")
	FOnBossDefeated OnBossDefeated;

	/** Blackboard key name for boss phase. */
	static const FName KEY_BossPhase;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	FText BossName = FText::FromString(TEXT("Boss"));

	/** Phase configurations sorted by health threshold (descending). Phase 0 = full health. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Phases")
	TArray<FBossPhaseConfig> Phases;

	/** If true, boss is briefly invulnerable during phase transitions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Phases")
	bool bInvulnerableDuringTransition = true;

	/** Duration of invulnerability during phase transition. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Phases", meta = (EditCondition = "bInvulnerableDuringTransition", ClampMin = "0"))
	float TransitionInvulnDuration = 2.0f;

private:
	int32 CurrentPhaseIndex = -1;
	bool bInPhaseTransition = false;

	FDelegateHandle HealthChangedHandle;

	void CheckPhaseTransition();
	void EnterPhase(int32 PhaseIndex);
	void OnPhaseTransitionEnd();
	void WritePhaseToBB();

	FTimerHandle TransitionTimerHandle;
};
