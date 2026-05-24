#include "Test/Combat/VSCombatDamageFormulaTest.h"
#include "Player/ARPGPlayerCharacter.h"
#include "Character/ARPGEnemyCharacter.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

AVSCombatDamageFormulaTest::AVSCombatDamageFormulaTest()
{
	PrimaryActorTick.bCanEverTick = true;
	TimeLimit = 20.f;
	LogWarningHandling = EFunctionalTestLogHandling::OutputIgnored;
}

void AVSCombatDamageFormulaTest::PrepareTest()
{
	Super::PrepareTest();
	Phase = 0;
	PhaseTime = 0.f;
}

void AVSCombatDamageFormulaTest::StartTest()
{
	Super::StartTest();

	Player = Cast<AARPGPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	for (TActorIterator<AARPGEnemyCharacter> It(GetWorld()); It; ++It)
	{
		Enemy = *It;
		break;
	}

	if (!Player.IsValid() || !Enemy.IsValid())
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("Player or Enemy missing in the slice level"));
	}
}

void AVSCombatDamageFormulaTest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsRunning() || !Player.IsValid() || !Enemy.IsValid())
	{
		return;
	}

	PhaseTime += DeltaSeconds;

	// Phase 0 — seed known attributes and position the player in melee range.
	if (Phase == 0)
	{
		UAbilitySystemComponent* PlayerASC = Player->GetAbilitySystemComponent();
		UAbilitySystemComponent* EnemyASC = Enemy->GetAbilitySystemComponent();
		if (!PlayerASC || !EnemyASC)
		{
			FinishTest(EFunctionalTestResult::Failed, TEXT("Player or Enemy missing an AbilitySystemComponent"));
			return;
		}

		// Attacker: deterministic — no crit, fixed AttackPower.
		PlayerASC->ApplyModToAttribute(UARPGAttributeSet::GetAttackPowerAttribute(), EGameplayModOp::Override, TestAttackPower);
		PlayerASC->ApplyModToAttribute(UARPGAttributeSet::GetCriticalChanceAttribute(), EGameplayModOp::Override, 0.f);

		// Target: fixed Armor + high Health so the full hit registers (no clamp at 0).
		EnemyASC->ApplyModToAttribute(UARPGAttributeSet::GetArmorAttribute(), EGameplayModOp::Override, TestArmor);
		EnemyASC->ApplyModToAttribute(UARPGAttributeSet::GetMaxHealthAttribute(), EGameplayModOp::Override, TestEnemyHealth);
		EnemyASC->ApplyModToAttribute(UARPGAttributeSet::GetHealthAttribute(), EGameplayModOp::Override, TestEnemyHealth);

		const FVector EnemyLoc = Enemy->GetActorLocation();
		const FVector Dir = (EnemyLoc - Player->GetActorLocation()).GetSafeNormal2D();
		const FVector StandLoc = EnemyLoc - Dir * 150.f; // 150cm < 180cm MeleeHitRange
		Player->SetActorLocation(
			FVector(StandLoc.X, StandLoc.Y, Player->GetActorLocation().Z),
			false, nullptr, ETeleportType::TeleportPhysics);
		Player->SetActorRotation(Dir.Rotation());

		if (PhaseTime >= 0.3f) // let the Override mods settle
		{
			Phase = 1;
			PhaseTime = 0.f;
		}
		return;
	}

	// Phase 1 — record health, fire one gray-box melee hit (no Event.MeleeHit sent).
	if (Phase == 1)
	{
		if (PhaseTime <= DeltaSeconds + KINDA_SMALL_NUMBER)
		{
			if (UAbilitySystemComponent* EnemyASC = Enemy->GetAbilitySystemComponent())
			{
				EnemyStartHealth = EnemyASC->GetNumericAttribute(UARPGAttributeSet::GetHealthAttribute());
			}

			FGameplayTagContainer MeleeTag;
			MeleeTag.AddTag(ARPGGameplayTags::Ability_Melee_LightAttack);
			const bool bActivated = Player->GetAbilitySystemComponent()->TryActivateAbilitiesByTag(MeleeTag);
			AssertTrue(bActivated, TEXT("melee ability should have activated for the formula test"));
		}

		if (PhaseTime >= 1.0f) // cover GrayBoxHitDelay (0.2s) + GE propagation
		{
			Phase = 2;
			PhaseTime = 0.f;
		}
		return;
	}

	// Phase 2 — assert the applied damage matches the C++ formula.
	if (Phase == 2)
	{
		if (PhaseTime >= 0.5f)
		{
			if (UAbilitySystemComponent* EnemyASC = Enemy->GetAbilitySystemComponent())
			{
				const float HealthNow = EnemyASC->GetNumericAttribute(UARPGAttributeSet::GetHealthAttribute());
				const float Applied = EnemyStartHealth - HealthNow;

				// Final = (Base + AttackPower*Scaling) * (1 - Armor/(Armor+100)); Scaling=1, no crit, physical.
				const float Raw = TestBaseDamage + TestAttackPower; // Scaling defaults to 1
				const float Expected = Raw * (1.f - TestArmor / (TestArmor + 100.f));

				AssertTrue(FMath::IsNearlyEqual(Applied, Expected, 1.0f),
					FString::Printf(TEXT("damage formula: applied %.2f should equal %.2f = (%.0f+%.0f)*(1-%.0f/%.0f). If BP_GA_MeleeAttack overrides BaseDamage, update TestBaseDamage."),
						Applied, Expected, TestBaseDamage, TestAttackPower, TestArmor, TestArmor + 100.f));
			}
			else
			{
				AssertTrue(false, TEXT("damage formula: enemy has no ASC to read Health from"));
			}

			FinishTest(EFunctionalTestResult::Default, TEXT("damage formula verified"));
		}
		return;
	}
}
