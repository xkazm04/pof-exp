#pragma once

#include "CoreMinimal.h"
#include "Character/ARPGCharacterBase.h"
#include "ARPGPlayerCharacter.generated.h"

class UAIPerceptionStimuliSourceComponent;
class UARPGAbilityUnlockComponent;
class UArrowComponent;
class UWidgetComponent;
struct FOnAttributeChangeData;

// --- Delegates for HUD/UI binding ---

/** Broadcast when health changes: (CurrentHealth, MaxHealth) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, CurrentHealth, float, MaxHealth);

/** Broadcast when mana changes: (CurrentMana, MaxMana) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnManaChanged, float, CurrentMana, float, MaxMana);

/** Broadcast when the player dies */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerDeath);

/** Broadcast when the player respawns */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerRespawn);

/** Broadcast when the player levels up: (NewLevel) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerLevelUp, int32, NewLevel);

/** Broadcast when an ability slot is activated: (SlotIndex) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAbilityActivated, int32, SlotIndex);

/** Broadcast when unspent attribute points change: (UnspentPoints) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributePointsChanged, int32, UnspentPoints);

/** Broadcast when the ability loadout changes: (SlotIndex) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLoadoutChanged, int32, SlotIndex);

UCLASS()
class POF_API AARPGPlayerCharacter : public AARPGCharacterBase
{
	GENERATED_BODY()

public:
	AARPGPlayerCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// =====================================================================
	// Health
	// =====================================================================

	/** Apply damage to the player. Respects invulnerability. Returns actual damage dealt. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Health")
	float TakeDamageAmount(float DamageAmount);

	/** Heal the player by the given amount. Returns actual amount healed. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Health")
	float Heal(float HealAmount);

	// GAS (UARPGAttributeSet::Health) is the source of truth — these read it via
	// the ASC and fall back to the mirrored float before the ASC exists. See the
	// OnGASHealthChanged binding in BeginPlay that keeps the float in lockstep.
	UFUNCTION(BlueprintPure, Category = "Combat|Health")
	float GetHealth() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Health")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Health")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Health")
	bool IsAlive() const { return bIsAlive; }

	virtual bool IsDead() const override { return !bIsAlive; }

	// =====================================================================
	// Mana
	// =====================================================================

	/** Spend mana. Returns true if there was enough mana. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Mana")
	bool SpendMana(float Amount);

	/** Restore mana by the given amount. Returns actual amount restored. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Mana")
	float RestoreMana(float Amount);

	UFUNCTION(BlueprintPure, Category = "Combat|Mana")
	float GetMana() const { return Mana; }

	UFUNCTION(BlueprintPure, Category = "Combat|Mana")
	float GetMaxMana() const { return MaxMana; }

	UFUNCTION(BlueprintPure, Category = "Combat|Mana")
	float GetManaPercent() const { return MaxMana > 0.f ? Mana / MaxMana : 0.f; }

	// =====================================================================
	// Ability Loadout
	// =====================================================================

	/** Try to activate the ability in the given slot (0-based) via the ASC. */
	UFUNCTION(BlueprintCallable, Category = "Abilities|Loadout")
	bool TryActivateAbilitySlot(int32 SlotIndex);

	/** Assign a learned ability to a hotbar slot (0-based). */
	UFUNCTION(BlueprintCallable, Category = "Abilities|Loadout")
	void AssignAbilityToSlot(int32 SlotIndex, TSubclassOf<UGameplayAbility> AbilityClass);

	/** Clear a hotbar slot. */
	UFUNCTION(BlueprintCallable, Category = "Abilities|Loadout")
	void ClearAbilitySlot(int32 SlotIndex);

	/** Get the ability class assigned to a slot. Returns null if empty. */
	UFUNCTION(BlueprintPure, Category = "Abilities|Loadout")
	TSubclassOf<UGameplayAbility> GetSlotAbility(int32 SlotIndex) const;

	/** Get the number of ability slots available. */
	UFUNCTION(BlueprintPure, Category = "Abilities")
	int32 GetAbilitySlotCount() const { return AbilitySlotCount; }

	/** Get the full loadout map. */
	UFUNCTION(BlueprintPure, Category = "Abilities|Loadout")
	const TMap<int32, TSubclassOf<UGameplayAbility>>& GetLoadout() const { return AbilityLoadout; }

	UPROPERTY(BlueprintAssignable, Category = "Events|Abilities")
	FOnLoadoutChanged OnLoadoutChanged;

	// =====================================================================
	// Interaction
	// =====================================================================

	/** Get the actor currently highlighted for interaction (nearest in range). */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetInteractionTarget() const { return InteractionTarget.Get(); }

	/** Attempt to interact with the current target. Called from controller. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void PerformInteraction();

	// =====================================================================
	// Death / Respawn
	// =====================================================================

	/** Kill the player. Triggers death sequence (legacy — prefer GAS death flow). */
	UFUNCTION(BlueprintCallable, Category = "Combat|Health")
	void Die();

	/**
	 * Called by GA_Death to bridge into the player's death/respawn system.
	 * Sets bIsAlive=false, broadcasts OnPlayerDeath, schedules respawn timer.
	 * Does NOT disable movement/collision — GA_Death handles that.
	 */
	void OnDeathFromAbility();

	/** Respawn the player at the given location (or default spawn if zero). */
	UFUNCTION(BlueprintCallable, Category = "Combat|Health")
	void Respawn(FVector RespawnLocation = FVector::ZeroVector);

	// =====================================================================
	// Level / XP
	// =====================================================================

	UFUNCTION(BlueprintCallable, Category = "Progression")
	void AddExperience(float Amount);

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetPlayerLevel() const { return PlayerLevel; }

	UFUNCTION(BlueprintPure, Category = "Progression")
	float GetExperience() const { return Experience; }

	UFUNCTION(BlueprintPure, Category = "Progression")
	float GetExperienceToNextLevel() const { return ExperienceToNextLevel; }

	UFUNCTION(BlueprintPure, Category = "Progression")
	float GetExperiencePercent() const { return ExperienceToNextLevel > 0.f ? Experience / ExperienceToNextLevel : 0.f; }

	/** Get the ability unlock/upgrade component. */
	UFUNCTION(BlueprintPure, Category = "Abilities")
	UARPGAbilityUnlockComponent* GetAbilityUnlockComponent() const { return AbilityUnlockComp; }

	// =====================================================================
	// Attribute Point Allocation
	// =====================================================================

	/**
	 * Spend one attribute point on Strength.
	 * Applies a permanent GE that adds AttackPower.
	 */
	UFUNCTION(BlueprintCallable, Category = "Progression|Attributes")
	bool AllocateStrength();

	/**
	 * Spend one attribute point on Dexterity.
	 * Applies a permanent GE that adds CriticalChance.
	 */
	UFUNCTION(BlueprintCallable, Category = "Progression|Attributes")
	bool AllocateDexterity();

	/**
	 * Spend one attribute point on Intelligence.
	 * Applies a permanent GE that adds MaxMana.
	 */
	UFUNCTION(BlueprintCallable, Category = "Progression|Attributes")
	bool AllocateIntelligence();

	UFUNCTION(BlueprintPure, Category = "Progression|Attributes")
	int32 GetUnspentAttributePoints() const { return UnspentAttributePoints; }

	UFUNCTION(BlueprintPure, Category = "Progression|Attributes")
	int32 GetAllocatedStrength() const { return AllocatedStrength; }

	UFUNCTION(BlueprintPure, Category = "Progression|Attributes")
	int32 GetAllocatedDexterity() const { return AllocatedDexterity; }

	UFUNCTION(BlueprintPure, Category = "Progression|Attributes")
	int32 GetAllocatedIntelligence() const { return AllocatedIntelligence; }

	UFUNCTION(BlueprintPure, Category = "Progression|Attributes")
	float GetAttackPowerPerStrength() const { return AttackPowerPerStrength; }

	UFUNCTION(BlueprintPure, Category = "Progression|Attributes")
	float GetCritChancePerDexterity() const { return CritChancePerDexterity; }

	UFUNCTION(BlueprintPure, Category = "Progression|Attributes")
	float GetMaxManaPerIntelligence() const { return MaxManaPerIntelligence; }

	UPROPERTY(BlueprintAssignable, Category = "Events|Progression")
	FOnAttributePointsChanged OnAttributePointsChanged;

	// =====================================================================
	// Cursor Aim (isometric cursor-based facing)
	// =====================================================================

	/** Get the world location the cursor is currently pointing at. */
	UFUNCTION(BlueprintPure, Category = "Camera|Cursor")
	FVector GetCursorWorldLocation() const { return CursorWorldLocation; }

	/** Enable/disable cursor-based rotation (e.g., during combat). */
	UFUNCTION(BlueprintCallable, Category = "Camera|Cursor")
	void SetCursorAimEnabled(bool bEnabled) { bCursorAimActive = bEnabled; }

	UFUNCTION(BlueprintPure, Category = "Camera|Cursor")
	bool IsCursorAimEnabled() const { return bCursorAimActive; }

	/** TEST/AI SEAM: force the cursor aim point to a fixed world location (bypasses the
	 *  mouse trace). The harness uses this to make mouse-aim deterministically verifiable;
	 *  AI/scripted aim can use it too. Clear with ClearCursorWorldOverride(). */
	UFUNCTION(BlueprintCallable, Category = "Camera|Cursor")
	void SetCursorWorldOverride(FVector WorldLocation) { CursorWorldOverride = WorldLocation; bUseCursorWorldOverride = true; }

	UFUNCTION(BlueprintCallable, Category = "Camera|Cursor")
	void ClearCursorWorldOverride() { bUseCursorWorldOverride = false; }

	/** TEST SEAM: feed a SCREEN-space cursor position (pixels) so UpdateCursorAim runs its
	 *  REAL deproject path (DeprojectScreenPositionToWorld + ground-plane intersect) with a
	 *  deterministic input. Unlike SetCursorWorldOverride (which skips deproject), this
	 *  exercises the actual mouse-aim code — so the contract suite can catch real-path
	 *  regressions instead of a proxy passing while real play is broken. */
	UFUNCTION(BlueprintCallable, Category = "Camera|Cursor")
	void SetCursorScreenOverride(FVector2D ScreenPos) { CursorScreenOverride = ScreenPos; bUseCursorScreenOverride = true; }

	UFUNCTION(BlueprintCallable, Category = "Camera|Cursor")
	void ClearCursorScreenOverride() { bUseCursorScreenOverride = false; }

	// =====================================================================
	// Delegates — bind from HUD/widgets
	// =====================================================================

	UPROPERTY(BlueprintAssignable, Category = "Events|Health")
	FOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events|Mana")
	FOnManaChanged OnManaChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events|Death")
	FOnPlayerDeath OnPlayerDeath;

	UPROPERTY(BlueprintAssignable, Category = "Events|Death")
	FOnPlayerRespawn OnPlayerRespawn;

	UPROPERTY(BlueprintAssignable, Category = "Events|Progression")
	FOnPlayerLevelUp OnPlayerLevelUp;

	UPROPERTY(BlueprintAssignable, Category = "Events|Abilities")
	FOnAbilityActivated OnAbilityActivated;

protected:
	// --- Debug ---
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Debug")
	TObjectPtr<UArrowComponent> DebugArrow;

	// --- Overhead widget (health bar, name, etc.) ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UWidgetComponent> OverheadWidgetComp;

	// --- Perception stimuli source (makes this character detectable by AI perception) ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAIPerceptionStimuliSourceComponent> PerceptionStimuliSource;

	// --- Ability Unlock/Upgrade ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UARPGAbilityUnlockComponent> AbilityUnlockComp;

	// =====================================================================
	// Health / Mana tuning
	// =====================================================================

	// DEPRECATED as a source of truth: GAS (UARPGAttributeSet) owns Health/MaxHealth.
	// These floats are MIRRORED from GAS by OnGASHealthChanged and read only as a
	// pre-ASC fallback. Do not write them directly expecting the HUD to follow —
	// apply a GameplayEffect to the ASC instead.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Health")
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Health")
	float Health = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Mana")
	float MaxMana = 50.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Mana")
	float Mana = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Mana")
	float ManaRegenPerSec = 2.f;

	// =====================================================================
	// Ability slots / Loadout
	// =====================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities", meta = (ClampMin = "1", ClampMax = "10"))
	int32 AbilitySlotCount = 4;

	// =====================================================================
	// Interaction
	// =====================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float InteractionRange = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float InteractionScanRate = 0.15f;

	// =====================================================================
	// Progression
	// =====================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	int32 PlayerLevel = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	float Experience = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	float ExperienceToNextLevel = 100.f;

	/** Multiplier applied to ExperienceToNextLevel each level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression", meta = (ClampMin = "1.0"))
	float ExperienceCurveExponent = 1.5f;

	/** Bonus health gained per level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	float HealthPerLevel = 10.f;

	/** Bonus mana gained per level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	float ManaPerLevel = 5.f;

	/** Attribute points awarded per level-up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression|Attributes", meta = (ClampMin = "0"))
	int32 AttributePointsPerLevel = 3;

	/** AttackPower gained per Strength point allocated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression|Attributes")
	float AttackPowerPerStrength = 2.f;

	/** CriticalChance gained per Dexterity point allocated (0.005 = 0.5%). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression|Attributes")
	float CritChancePerDexterity = 0.005f;

	/** MaxMana gained per Intelligence point allocated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression|Attributes")
	float MaxManaPerIntelligence = 5.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression|Attributes")
	int32 UnspentAttributePoints = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression|Attributes")
	int32 AllocatedStrength = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression|Attributes")
	int32 AllocatedDexterity = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression|Attributes")
	int32 AllocatedIntelligence = 0;

	// =====================================================================
	// Death / Respawn
	// =====================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Health")
	float RespawnDelay = 3.0f;

	// =====================================================================
	// Cursor Aim
	// =====================================================================

	/** Whether cursor aim rotation is active (character faces cursor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Cursor")
	bool bCursorAimActive = false;

	/** How fast the character rotates toward cursor when aiming. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Cursor")
	float CursorAimRotationSpeed = 15.f;

private:
	bool bIsAlive = true;

	// Cursor aim state
	FVector CursorWorldLocation = FVector::ZeroVector;

	// Cursor aim override (test/AI seam — see SetCursorWorldOverride)
	bool bUseCursorWorldOverride = false;
	FVector CursorWorldOverride = FVector::ZeroVector;

	// Screen-space cursor override (test seam — exercises the REAL deproject path)
	bool bUseCursorScreenOverride = false;
	FVector2D CursorScreenOverride = FVector2D::ZeroVector;

	// Interaction scan
	TWeakObjectPtr<AActor> InteractionTarget;
	float InteractionScanTimer = 0.f;

	// Real-play telemetry accumulator (logs cursor/dodge/speed to PoF.log ~1Hz so movement
	// issues are diagnosable from a real play session, not just the harness).
	float PlayTelemetryTimer = 0.f;

	// Ability loadout: SlotIndex (0..N-1) -> AbilityClass. EditDefaultsOnly so the
	// default hotbar can be authored per-Blueprint (BP_VSPlayer) and set via Python on
	// the CDO; runtime code reassigns via AssignAbilityToSlot. (No BlueprintReadOnly:
	// the member is private — GetLoadout() is the Blueprint accessor — and UHT rejects
	// BlueprintReadOnly on private members.)
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TMap<int32, TSubclassOf<UGameplayAbility>> AbilityLoadout;

	// Respawn
	FTimerHandle RespawnTimerHandle;
	FVector PendingRespawnLocation = FVector::ZeroVector;

	// --- Internal update helpers ---
	void UpdateManaRegen(float DeltaTime);
	void UpdateCursorAim(float DeltaTime);
	void UpdateInteractionScan(float DeltaTime);
	void UpdateDebugDisplay(float DeltaTime);

	/** Called after RespawnDelay to actually perform the respawn. */
	void ExecuteRespawn();

	/** Internal: perform level-up stat boosts and broadcast. */
	void LevelUp();

	/** Calculate required XP for the given level. */
	float CalculateXPForLevel(int32 Level) const;

	// --- GAS-backed health (source of truth = UARPGAttributeSet) ---
	/** Read a GAS health attribute current value; returns Fallback if no ASC/AttributeSet yet. */
	float ReadGASHealth(bool bMax, float Fallback) const;
	/** GAS Health/MaxHealth change handler: mirrors the value into the deprecated float + broadcasts OnHealthChanged. */
	void OnGASHealthChanged(const FOnAttributeChangeData& Data);
};
