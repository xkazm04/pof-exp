#pragma once

#include "CoreMinimal.h"
#include "Character/ARPGCharacterBase.h"
#include "GameplayTagContainer.h"
#include "ARPGEnemyCharacter.generated.h"

class UWidgetComponent;
class UGameplayAbility;
class UEnemyHealthBarWidget;
class UARPGLootDropComponent;

/** Broadcast when the enemy dies, before destruction. Payload: this enemy actor. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDeath, AARPGEnemyCharacter*, Enemy);

/** Enemy archetype determines AI behavior branching and combat style. */
UENUM(BlueprintType)
enum class EEnemyArchetype : uint8
{
	MeleeGrunt    UMETA(DisplayName = "Melee Grunt"),
	RangedCaster  UMETA(DisplayName = "Ranged Caster"),
	Brute         UMETA(DisplayName = "Brute")
};

/**
 * Canonical per-archetype combat/loot/XP tuning. Pure, world-free data — the
 * single source of truth consumed by both AARPGEnemyCharacter::ApplyArchetypeDefaults()
 * (applied at possession) and GetBaseXPReward(), and asserted by the bestiary
 * config test gate. Mirrors the app-side ENEMY_ARCHETYPES design data.
 */
struct FEnemyArchetypeDefaults
{
	float AttackRange = 200.f;
	float PreferredCombatDistance = 0.f;
	float RetreatDistance = 0.f;
	float AttackCooldown = 2.0f;
	FGameplayTag PrimaryAbilityTag;
	/** Post-charge vulnerability window (Brute). 0 = no charge. */
	float ChargeVulnerabilityDuration = 0.f;
	/** WalkSpeed multiplier during charge (Brute). 1 = no charge. */
	float ChargeSpeedMultiplier = 1.f;
	int32 LootNumRolls = 1;
	float LootRarityBonusMultiplier = 1.f;
	float BaseXPReward = 10.f;
};

UCLASS()
class POF_API AARPGEnemyCharacter : public AARPGCharacterBase
{
	GENERATED_BODY()

public:
	AARPGEnemyCharacter();

	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

	/** Set the enemy level before possession for difficulty scaling. */
	UFUNCTION(BlueprintCallable, Category = "AI|Difficulty")
	void SetCharacterLevel(int32 NewLevel) { CharacterLevel = FMath::Max(1, NewLevel); }

	/** Current enemy level — drives XP and gold reward scaling. */
	UFUNCTION(BlueprintPure, Category = "AI|Difficulty")
	int32 GetCharacterLevel() const { return CharacterLevel; }

	// =====================================================================
	// Archetype
	// =====================================================================

	UFUNCTION(BlueprintPure, Category = "AI|Archetype")
	EEnemyArchetype GetArchetype() const { return Archetype; }

	/**
	 * Canonical tuning for an archetype — pure and world-free. Single source of
	 * truth for ApplyArchetypeDefaults() (runtime) and GetBaseXPReward(); also the
	 * surface the bestiary config test gate asserts without needing a PIE world.
	 */
	static FEnemyArchetypeDefaults GetArchetypeDefaults(EEnemyArchetype InArchetype);

	UFUNCTION(BlueprintPure, Category = "AI|Archetype")
	float GetAttackRange() const { return AttackRange; }

	UFUNCTION(BlueprintPure, Category = "AI|Archetype")
	float GetPreferredCombatDistance() const { return PreferredCombatDistance; }

	UFUNCTION(BlueprintPure, Category = "AI|Archetype")
	float GetRetreatDistance() const { return RetreatDistance; }

	UFUNCTION(BlueprintPure, Category = "AI|Archetype")
	float GetAttackCooldown() const { return AttackCooldown; }

	UFUNCTION(BlueprintPure, Category = "AI|Archetype")
	const FGameplayTag& GetPrimaryAbilityTag() const { return PrimaryAbilityTag; }

	UFUNCTION(BlueprintPure, Category = "AI|Archetype")
	float GetChargeVulnerabilityDuration() const { return ChargeVulnerabilityDuration; }

	UFUNCTION(BlueprintPure, Category = "AI|Archetype")
	float GetChargeSpeedMultiplier() const { return ChargeSpeedMultiplier; }

	// =====================================================================
	// Health bar widget
	// =====================================================================

	UFUNCTION(BlueprintPure, Category = "UI")
	UWidgetComponent* GetHealthBarWidget() const { return HealthBarWidgetComp; }

	// =====================================================================
	// Death / XP Reward
	// =====================================================================

	/**
	 * Called by GA_Death when this enemy dies.
	 * Awards XP to the killer and sends Event.EnemyKilled to their ASC.
	 * @param KillingActor  The actor that dealt the killing blow (from death event instigator).
	 */
	void OnDeathFromAbility(AActor* KillingActor = nullptr);

	/** Calculate XP reward for killing this enemy, scaled by level difference with the killer. */
	UFUNCTION(BlueprintPure, Category = "Progression")
	float CalculateXPReward(int32 KillerLevel) const;

	/** Base XP value for this enemy's archetype (before level scaling). */
	UFUNCTION(BlueprintPure, Category = "Progression")
	float GetBaseXPReward() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Health")
	bool IsAlive() const { return bIsAlive; }

	virtual bool IsDead() const override { return !bIsAlive; }

	UPROPERTY(BlueprintAssignable, Category = "Events|Death")
	FOnEnemyDeath OnEnemyDeath;

protected:
	// =====================================================================
	// Archetype configuration
	// =====================================================================

	/** Which archetype this enemy uses. Drives AI behavior tree branching. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Archetype")
	EEnemyArchetype Archetype = EEnemyArchetype::MeleeGrunt;

	/** Distance within which this enemy can attack. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Archetype|Combat", meta = (ClampMin = "50"))
	float AttackRange = 200.f;

	/** Ideal combat distance the AI tries to maintain. Casters use this to stay far. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Archetype|Combat", meta = (ClampMin = "0"))
	float PreferredCombatDistance = 0.f;

	/** If the player gets closer than this, the AI retreats. 0 = never retreats. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Archetype|Combat", meta = (ClampMin = "0"))
	float RetreatDistance = 0.f;

	/** Cooldown between attacks in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Archetype|Combat", meta = (ClampMin = "0"))
	float AttackCooldown = 2.0f;

	/** The gameplay tag used to activate this enemy's primary attack ability. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Archetype|Combat")
	FGameplayTag PrimaryAbilityTag;

	/** Abilities to grant when this enemy is possessed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Archetype|Abilities")
	TArray<TSubclassOf<UGameplayAbility>> GrantedAbilities;

	/** Post-charge vulnerability duration for Brute archetype. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Archetype|Brute", meta = (ClampMin = "0"))
	float ChargeVulnerabilityDuration = 2.0f;

	/** Charge speed multiplier (applied to WalkSpeed during charge). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Archetype|Brute", meta = (ClampMin = "1.0"))
	float ChargeSpeedMultiplier = 3.0f;

	/** Loot drop component — rolls loot table and spawns world items on death. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UARPGLootDropComponent> LootDropComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UWidgetComponent> HealthBarWidgetComp;

	/** Widget class for the floating health bar. Assign a UMG Widget BP subclassing UEnemyHealthBarWidget. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UEnemyHealthBarWidget> HealthBarWidgetClass;

	/** Display name shown on the floating health bar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	FText EnemyDisplayName = FText::FromString(TEXT("Enemy"));

private:
	bool bIsAlive = true;

	/** Disable collision, stop movement, and halt AI logic. Called on death. */
	void HandleDeathCleanup();
	void StopBehaviorTree();
	void ApplyArchetypeDefaults();
	void GrantAbilitiesToASC();
	void WriteArchetypeToBlackboard();
	void InitializeHealthBarWidget();
};
