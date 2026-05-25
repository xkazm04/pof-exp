#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameplayEffect.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "AbilitySystem/Effects/Generated/GE_Gen_Fireball_FireImpact.h"

/**
 * Combat test gate for the generated Fireball impact effect (Catalog Pipeline
 * Phase B, Skill/Ability row). Asserts the codegen produced the intended
 * GameplayEffect configuration for the canonical seeded ability (damage 35):
 *   - instant duration,
 *   - exactly one modifier,
 *   - additive on Health,
 *   - magnitude -35.
 *
 * Pure config gate (no PIE/world needed) — runs headless via:
 *   UnrealEditor-Cmd PoF.uproject -ExecCmds="Automation RunTests Project.Functional Tests.PoF.GenFireball;Quit" -unattended -nopause -nullrhi -log
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSGenFireballEffectTest,
	"Project.Functional Tests.PoF.GenFireball.EffectConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSGenFireballEffectTest::RunTest(const FString& /*Parameters*/)
{
	const UGE_Gen_Fireball_FireImpact* CDO = GetDefault<UGE_Gen_Fireball_FireImpact>();
	if (!TestNotNull(TEXT("GE_Gen_Fireball_FireImpact CDO resolves"), CDO))
	{
		return false;
	}

	TestEqual(TEXT("Instant duration policy"),
		static_cast<int32>(CDO->DurationPolicy),
		static_cast<int32>(EGameplayEffectDurationType::Instant));

	if (TestEqual(TEXT("Exactly one modifier"), CDO->Modifiers.Num(), 1))
	{
		const FGameplayModifierInfo& Mod = CDO->Modifiers[0];
		TestTrue(TEXT("Modifier targets Health"),
			Mod.Attribute == UARPGAttributeSet::GetHealthAttribute());
		TestEqual(TEXT("Additive modifier op"),
			static_cast<int32>(Mod.ModifierOp),
			static_cast<int32>(EGameplayModOp::Additive));

		float Magnitude = 0.f;
		const bool bStatic = Mod.ModifierMagnitude.GetStaticMagnitudeIfPossible(1.f, Magnitude);
		TestTrue(TEXT("Modifier has a static magnitude"), bStatic);
		TestEqual(TEXT("Magnitude is -35 (canonical Fireball damage)"), Magnitude, -35.f);
	}

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
