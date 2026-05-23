#pragma once

#include "CoreMinimal.h"
#include "FunctionalTest.h"
#include "Templates/Function.h"
#include "ARPGFunctionalTestBase.generated.h"

class AARPGPlayerCharacter;
class AARPGEnemyCharacter;
class UAbilitySystemComponent;

/** Result a subclass returns from RunPhase() each tick. */
UENUM()
enum class EARPGPhaseResult : uint8
{
	Running, // stay in this phase next tick
	Advance, // phase satisfied — move to the next phase
	Fail     // phase failed — finish the test as Failed
};

/** Tri-state returned by WaitForCondition() for use inside RunPhase(). */
enum class EARPGWait : uint8
{
	Pending,
	Satisfied,
	TimedOut
};

/**
 * Shared base for ARPG functional tests.
 *
 * `AVSFunctionalTest` open-coded a four-phase Tick state machine; every new
 * per-system test would have re-invented it. This base provides:
 *   - a phased Tick driver — fill `Phases` (a TArray<FName>) and override
 *     RunPhase(); the base advances phases, enforces TimeLimit / per-phase
 *     timeout, and finishes the test;
 *   - GAS + character helpers (player/enemy lookup, ASC access, real-pipeline
 *     ApplyDamage, Health read, WaitForCondition);
 *   - the `LogWarningHandling = OutputIgnored` default the Characters
 *     sub-project landed on (gray-box anim warnings are not failures).
 *
 * Subclasses populate `Phases` in their constructor and implement RunPhase().
 */
UCLASS(Abstract)
class POF_API AARPGFunctionalTestBase : public AFunctionalTest
{
	GENERATED_BODY()

public:
	AARPGFunctionalTestBase();

	virtual void StartTest() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	/** Run the current phase for one tick. Default no-op (Advance immediately). */
	virtual EARPGPhaseResult RunPhase(int32 PhaseIndex, FName PhaseName, float DeltaSeconds);

	/** Hook called once after Player/Enemy are resolved, before phase 0 runs. */
	virtual void OnTestStarted() {}

	// --- Phase configuration (populate in the subclass constructor) ---

	/** Ordered phase names. When the last advances, the test passes. */
	TArray<FName> Phases;

	/** Optional per-phase timeout in seconds; <= 0 means rely on TimeLimit. */
	float PhaseTimeout = 0.f;

	// --- Accessors usable from RunPhase ---

	/** Seconds elapsed in the current phase. Resets to 0 on each advance. */
	float GetPhaseTime() const { return PhaseTime; }

	/** True only on the first tick a phase runs — for one-shot actions. */
	bool IsFirstTickOfPhase(float DeltaSeconds) const
	{
		return PhaseTime <= DeltaSeconds + KINDA_SMALL_NUMBER;
	}

	AARPGPlayerCharacter* GetPlayerCharacter() const;
	AARPGEnemyCharacter*  GetFirstEnemy() const;
	UAbilitySystemComponent* GetPlayerASC() const;
	UAbilitySystemComponent* GetEnemyASC() const;

	/**
	 * Apply real damage to a target via the GE_Damage pipeline from the player
	 * ASC — the same path GA_MeleeAttack::OnMeleeHit uses, so the death/loot
	 * chain fires as a genuine consequence (never a direct Health poke).
	 */
	void ApplyDamage(AActor* Target, float Amount);

	/** Read the Health attribute off an actor's ASC (0 if it has none). */
	float GetHealth(AActor* Actor) const;

	/** Tri-state predicate check for inside RunPhase: Satisfied / TimedOut / Pending. */
	EARPGWait WaitForCondition(TFunctionRef<bool()> Predicate, float TimeoutSeconds) const;

	TWeakObjectPtr<AARPGPlayerCharacter> Player;
	TWeakObjectPtr<AARPGEnemyCharacter>  Enemy;

private:
	int32 CurrentPhase = 0;
	float PhaseTime = 0.f;
};
