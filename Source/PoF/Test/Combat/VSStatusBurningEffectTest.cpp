#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystem/Effects/Generated/GE_Gen_Fireball_Burning.h"

/**
 * Test gate for the Burning status effect (Catalog Pipeline, Game-Assets /
 * status-effects row; target asset "Burning"). The Burning status effect and the
 * Fireball ability's Burning DoT are the SAME UE artifact — the generated
 * UGE_Gen_Fireball_Burning. This asserts the codegen + native-tag declaration
 * produced the intended status-effect configuration:
 *   - HasDuration policy, 3.0s duration,
 *   - periodic tick every 1.0s,
 *   - exactly one modifier: Health += -5 (Additive),
 *   - grants State.Burning to the target (now that the tag is declared natively
 *     in ARPGGameplayTags — without it the grant is silently skipped at runtime).
 *
 * Pure config gate (no PIE/world needed) — runs headless via:
 *   UnrealEditor-Cmd PoF.uproject -ExecCmds="Automation RunTests Project.Functional Tests.PoF.StatusBurning;Quit" -unattended -nopause -nullrhi -log
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSStatusBurningEffectTest,
	"Project.Functional Tests.PoF.StatusBurning.EffectConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSStatusBurningEffectTest::RunTest(const FString& /*Parameters*/)
{
	const UGE_Gen_Fireball_Burning* CDO = GetDefault<UGE_Gen_Fireball_Burning>();
	if (!TestNotNull(TEXT("GE_Gen_Fireball_Burning CDO resolves"), CDO))
	{
		return false;
	}

	// --- Duration & tick rules ---
	TestEqual(TEXT("HasDuration policy"),
		static_cast<int32>(CDO->DurationPolicy),
		static_cast<int32>(EGameplayEffectDurationType::HasDuration));

	float Duration = 0.f;
	const bool bStaticDuration = CDO->DurationMagnitude.GetStaticMagnitudeIfPossible(1.f, Duration);
	TestTrue(TEXT("Duration has a static magnitude"), bStaticDuration);
	TestEqual(TEXT("Duration is 3.0s"), Duration, 3.0f);

	TestEqual(TEXT("Periodic tick every 1.0s"), CDO->Period.Value, 1.0f);

	// --- Mechanical effect logic: Health -5 per tick ---
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
		TestEqual(TEXT("Magnitude is -5 per tick (Burning DoT)"), Magnitude, -5.f);
	}

	// --- Status identity: grants State.Burning to the target ---
	// This is what makes Burning a *status effect* rather than a bare DoT. The grant
	// only activates because State.Burning is declared natively in ARPGGameplayTags
	// (the generated GE looks the tag up by name at construction).
	const UTargetTagsGameplayEffectComponent* TagComp =
		CDO->FindComponent<UTargetTagsGameplayEffectComponent>();
	if (TestNotNull(TEXT("GE has a Grant-Tags-to-Target component"), TagComp))
	{
		const FGameplayTagContainer& Granted = TagComp->GetConfiguredTargetTagChanges().CombinedTags;
		TestTrue(TEXT("Grants State.Burning to the target"),
			Granted.HasTagExact(ARPGGameplayTags::State_Burning));
	}

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
