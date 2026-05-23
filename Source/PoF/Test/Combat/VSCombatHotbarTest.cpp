#include "Test/Combat/VSCombatHotbarTest.h"
#include "Player/ARPGPlayerCharacter.h"
#include "AbilitySystem/GA_MeleeAttack.h"
#include "AbilitySystem/GA_Fireball.h"
#include "AbilitySystem/GA_GroundSlam.h"
#include "AbilitySystem/GA_DashStrike.h"
#include "AbilitySystem/GA_WarCry.h"
#include "AbilitySystem/GA_Dodge.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	bool HasGrantedAbility(UAbilitySystemComponent* ASC, TSubclassOf<UGameplayAbility> Cls)
	{
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (Spec.Ability && Spec.Ability->IsA(Cls))
			{
				return true;
			}
		}
		return false;
	}
}

AVSCombatHotbarTest::AVSCombatHotbarTest()
{
	PrimaryActorTick.bCanEverTick = true;
	TimeLimit = 20.f;
	LogWarningHandling = EFunctionalTestLogHandling::OutputIgnored;
}

void AVSCombatHotbarTest::PrepareTest()
{
	Super::PrepareTest();
	Phase = 0;
	PhaseTime = 0.f;
	SlotIndex = 0;
}

void AVSCombatHotbarTest::StartTest()
{
	Super::StartTest();
	Player = Cast<AARPGPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!Player.IsValid())
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("No player character"));
	}
}

void AVSCombatHotbarTest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsRunning() || !Player.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	if (!ASC)
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("Player has no AbilitySystemComponent"));
		return;
	}

	PhaseTime += DeltaSeconds;

	// Phase 0 — clear resource gating (so activation depends only on wiring) and
	// assert the full roster is granted.
	if (Phase == 0)
	{
		if (PhaseTime <= DeltaSeconds + KINDA_SMALL_NUMBER)
		{
			// Remove mana cost as a confound — we are testing wiring, not economy.
			ASC->SetNumericAttributeBase(UARPGAttributeSet::GetManaAttribute(), 9999.f);

			AssertTrue(HasGrantedAbility(ASC, UGA_MeleeAttack::StaticClass()), TEXT("grant: GA_MeleeAttack"));
			AssertTrue(HasGrantedAbility(ASC, UGA_Fireball::StaticClass()),    TEXT("grant: GA_Fireball"));
			AssertTrue(HasGrantedAbility(ASC, UGA_GroundSlam::StaticClass()),  TEXT("grant: GA_GroundSlam"));
			AssertTrue(HasGrantedAbility(ASC, UGA_DashStrike::StaticClass()),  TEXT("grant: GA_DashStrike"));
			AssertTrue(HasGrantedAbility(ASC, UGA_WarCry::StaticClass()),      TEXT("grant: GA_WarCry"));
			AssertTrue(HasGrantedAbility(ASC, UGA_Dodge::StaticClass()),       TEXT("grant: GA_Dodge"));
		}
		if (PhaseTime >= 0.2f)
		{
			Phase = 1;
			PhaseTime = 0.f;
			SlotIndex = 0;
		}
		return;
	}

	// Phase 1 — activate each loadout slot (one per tick), cancelling prior state first.
	if (Phase == 1)
	{
		ASC->CancelAllAbilities();
		const bool bOk = Player->TryActivateAbilitySlot(SlotIndex);
		AssertTrue(bOk, FString::Printf(TEXT("hotbar: slot %d should activate"), SlotIndex));

		SlotIndex++;
		if (SlotIndex >= 4)
		{
			Phase = 2;
			PhaseTime = 0.f;
		}
		return;
	}

	// Phase 2 — dodge via its tag (player is grounded + full stamina at spawn).
	if (Phase == 2)
	{
		ASC->CancelAllAbilities();
		FGameplayTagContainer DodgeTag;
		DodgeTag.AddTag(ARPGGameplayTags::Ability_Dodge);
		const bool bOk = ASC->TryActivateAbilitiesByTag(DodgeTag);
		AssertTrue(bOk, TEXT("hotbar: GA_Dodge should activate via Ability.Dodge tag"));

		FinishTest(EFunctionalTestResult::Default, TEXT("hotbar wiring verified"));
		return;
	}
}
