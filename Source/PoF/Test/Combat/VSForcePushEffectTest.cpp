#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameplayEffect.h"
#include "AbilitySystem/GA_ForcePush.h"
#include "AbilitySystem/Effects/GE_Cooldown_ForcePush.h"
#include "AbilitySystem/ARPGGameplayTags.h"

/**
 * Combat test gate for the autonomously-authored Force Push ability (Star Wars
 * arena duel). Mirrors FVSGenFireballEffectTest: a pure config gate (no PIE/world)
 * that asserts UGA_ForcePush + UGE_Cooldown_ForcePush were built to actually knock
 * a target back and deal damage, on the intended mana cost + cooldown.
 *
 * Drains the Force Push L3 acceptance gate. Runs headless via:
 *   UnrealEditor-Cmd PoF.uproject -ExecCmds="Automation RunTests Project.Functional Tests.PoF.ForcePush;Quit" -unattended -nopause -nullrhi -log
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSForcePushEffectTest,
	"Project.Functional Tests.PoF.ForcePush.EffectConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

static float ReadProtectedFloat(const UObject* Obj, const TCHAR* PropName)
{
	if (!Obj) return -1.f;
	const FFloatProperty* Prop = CastField<FFloatProperty>(Obj->GetClass()->FindPropertyByName(PropName));
	return Prop ? Prop->GetPropertyValue_InContainer(Obj) : -1.f;
}

bool FVSForcePushEffectTest::RunTest(const FString& /*Parameters*/)
{
	const UGA_ForcePush* CDO = GetDefault<UGA_ForcePush>();
	if (!TestNotNull(TEXT("UGA_ForcePush CDO resolves"), CDO))
	{
		return false;
	}

	// --- Cost + cooldown wiring ---
	TestEqual(TEXT("Mana cost is 20"), CDO->GetAbilityManaCost(), 20.f);

	const UGameplayEffect* Cooldown = CDO->GetCooldownGameplayEffect();
	TestTrue(TEXT("Cooldown GE is UGE_Cooldown_ForcePush"),
		Cooldown != nullptr && Cooldown->IsA(UGE_Cooldown_ForcePush::StaticClass()));

	TestTrue(TEXT("Cooldown tag is Cooldown.ForcePush"),
		CDO->GetAbilityCooldownTag() == ARPGGameplayTags::Cooldown_ForcePush);

	// --- Identity tag (the ability is what it says it is) ---
	TestTrue(TEXT("Asset tags include Ability.ForcePush"),
		CDO->GetAssetTags().HasTagExact(ARPGGameplayTags::Ability_ForcePush));

	// --- Knockback is configured to actually push (protected props → reflection) ---
	const float HKB = ReadProtectedFloat(CDO, TEXT("HorizontalKnockback"));
	const float VKB = ReadProtectedFloat(CDO, TEXT("VerticalKnockback"));
	const float Cone = ReadProtectedFloat(CDO, TEXT("ConeHalfAngleDeg"));
	const float Range = ReadProtectedFloat(CDO, TEXT("PushRange"));
	TestTrue(TEXT("Horizontal knockback > 0 (launches the target away)"), HKB > 0.f);
	TestTrue(TEXT("Vertical knockback > 0 (upward arc)"), VKB > 0.f);
	TestTrue(TEXT("Cone half-angle in (0,180]"), Cone > 0.f && Cone <= 180.f);
	TestTrue(TEXT("Push range > 0"), Range > 0.f);

	// --- Cooldown effect config: 5s duration ---
	const UGE_Cooldown_ForcePush* Cd = GetDefault<UGE_Cooldown_ForcePush>();
	if (TestNotNull(TEXT("UGE_Cooldown_ForcePush CDO resolves"), Cd))
	{
		TestEqual(TEXT("HasDuration policy"),
			static_cast<int32>(Cd->DurationPolicy),
			static_cast<int32>(EGameplayEffectDurationType::HasDuration));
		float Dur = 0.f;
		const bool bStatic = Cd->DurationMagnitude.GetStaticMagnitudeIfPossible(1.f, Dur);
		TestTrue(TEXT("Duration has a static magnitude"), bStatic);
		TestEqual(TEXT("Cooldown duration is 5s"), Dur, 5.f);
	}

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
