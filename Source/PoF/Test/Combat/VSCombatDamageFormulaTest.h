#pragma once

#include "CoreMinimal.h"
#include "FunctionalTest.h"
#include "VSCombatDamageFormulaTest.generated.h"

class AARPGPlayerCharacter;
class AARPGEnemyCharacter;

/**
 * Damage-formula regression guard for UARPGDamageExecution. Seeds the attacker
 * (AttackPower=10, CriticalChance=0) and target (Armor=4, high Health) with known
 * values, fires one gray-box melee hit (physical, no crit, no elemental), and
 * asserts the applied damage equals the C++ formula:
 *   Final = (Base + AttackPower*Scaling) * (1 - Armor/(Armor+100))
 * with Base = GA_MeleeAttack::BaseDamage default (20) and Scaling defaulting to 1.
 * NOTE: not the doc's simplified "base+10-4" — armor uses diminishing returns.
 */
UCLASS()
class POF_API AVSCombatDamageFormulaTest : public AFunctionalTest
{
	GENERATED_BODY()

public:
	AVSCombatDamageFormulaTest();

	virtual void PrepareTest() override;
	virtual void StartTest() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	TWeakObjectPtr<AARPGPlayerCharacter> Player;
	TWeakObjectPtr<AARPGEnemyCharacter>  Enemy;

	int32 Phase = 0;
	float PhaseTime = 0.f;
	float EnemyStartHealth = 0.f;

	// Known inputs — keep in sync with the assertion in Tick.
	static constexpr float TestBaseDamage = 20.f;   // GA_MeleeAttack::BaseDamage default
	static constexpr float TestAttackPower = 10.f;
	static constexpr float TestArmor = 4.f;
	static constexpr float TestEnemyHealth = 500.f; // Overridden high so the full hit registers
};
