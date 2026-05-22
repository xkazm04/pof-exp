#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "ProgressionTestCommandlet.generated.h"

/**
 * Commandlet that exercises the full progression loop end-to-end:
 *
 *   1. XP Award → Level Up (with stat boosts, skill points, attribute points)
 *   2. Attribute Point Allocation (STR→AttackPower, DEX→CritChance, INT→MaxMana)
 *   3. Ability Unlock (learn ability via skill points)
 *   4. Ability Upgrade (rank up a learned ability)
 *   5. Ability Loadout (assign to slot, verify slot query)
 *   6. Multi-level XP (large XP gain triggers multiple level-ups)
 *
 * Run via:
 *   UnrealEditor-Cmd.exe PoF.uproject -run=ProgressionTest -nopause -unattended -nosplash
 *
 * Exits with code 0 on all-pass, 1 on any failure.
 */
UCLASS()
class UProgressionTestCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UProgressionTestCommandlet();
	virtual int32 Main(const FString& Params) override;

private:
	int32 TestsPassed = 0;
	int32 TestsFailed = 0;

	/** Log a test result. Returns true if the test passed. */
	bool Check(bool bCondition, const FString& TestName);

	bool TestXPAndLevelUp();
	bool TestAttributeAllocation();
	bool TestAbilityUnlock();
	bool TestAbilityLoadout();
	bool TestMultiLevelXP();
	bool TestFullLoop();
};
