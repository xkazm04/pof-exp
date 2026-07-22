#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystem/Effects/GE_Dazed.h"
#include "Character/ARPGCharacterBase.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

/**
 * Spellbook catalog gate — Force Push's Dazed landing follow-up (Applies Status →
 * status-effects::status-dazed). Mirrors the CharacterVael config-gate pattern:
 * no PIE/world, CDO + tag-registry assertions only, runs headless:
 *   UnrealEditor-Cmd PoF.uproject -ExecCmds="Automation RunTests Project.Functional Tests.PoF.GenForcePush.DazeConfig;Quit" -unattended -nopause -nullrhi -log
 *
 * Contract asserted (single-sourced from the catalog rows):
 *   - State.Dazed / State.Immune.Daze registered as native tags
 *   - UGE_Dazed: HasDuration, 1.6 s, grants State.Dazed via TargetTags
 *   - AARPGCharacterBase: DazedSpeedMultiplier 0.25, pending-landing flag defaults false
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSGenForcePushDazeTest,
	"Project.Functional Tests.PoF.GenForcePush.DazeConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSGenForcePushDazeTest::RunTest(const FString& /*Parameters*/)
{
	// --- 1. Native tag registry (FNativeGameplayTag → FGameplayTag via GetTag) ---
	TestTrue(TEXT("State.Dazed is a registered native tag"),
		ARPGGameplayTags::State_Dazed.GetTag().IsValid());
	TestEqual(TEXT("State.Dazed literal"),
		ARPGGameplayTags::State_Dazed.GetTag().GetTagName(), FName(TEXT("State.Dazed")));
	TestTrue(TEXT("State.Immune.Daze is a registered native tag"),
		ARPGGameplayTags::State_Immune_Daze.GetTag().IsValid());

	// --- 2. UGE_Dazed contract (CDO) ---
	const UGE_Dazed* Daze = GetDefault<UGE_Dazed>();
	if (!TestNotNull(TEXT("UGE_Dazed CDO resolves"), Daze))
	{
		return false;
	}
	TestEqual(TEXT("Daze is duration-based"),
		static_cast<int32>(Daze->DurationPolicy),
		static_cast<int32>(EGameplayEffectDurationType::HasDuration));

	float DurationSec = 0.f;
	const bool bStaticDuration =
		Daze->DurationMagnitude.GetStaticMagnitudeIfPossible(1.f, DurationSec);
	TestTrue(TEXT("Daze duration is a static magnitude"), bStaticDuration);
	TestEqual(TEXT("Daze duration is the catalog's 1.6 s"), DurationSec, 1.6f, 0.001f);

	const UTargetTagsGameplayEffectComponent* TargetTags =
		Daze->FindComponent<UTargetTagsGameplayEffectComponent>();
	if (TestNotNull(TEXT("Daze has a TargetTags component"), TargetTags))
	{
		TestTrue(TEXT("Daze grants State.Dazed while active"),
			TargetTags->GetConfiguredTargetTagChanges().Added.HasTagExact(
				ARPGGameplayTags::State_Dazed.GetTag()));
	}

	// --- 3. Character-side wiring (CDO) ---
	const AARPGCharacterBase* Char = GetDefault<AARPGCharacterBase>();
	if (!TestNotNull(TEXT("AARPGCharacterBase CDO resolves"), Char))
	{
		return false;
	}
	TestEqual(TEXT("Dazed shamble multiplier is the catalog's 0.25"),
		Char->DazedSpeedMultiplier, 0.25f, 0.001f);
	TestFalse(TEXT("No daze is pending by default"), Char->bPendingDazeOnLanding);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
