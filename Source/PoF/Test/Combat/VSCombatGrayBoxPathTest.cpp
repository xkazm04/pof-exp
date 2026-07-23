#include "Test/Combat/VSCombatGrayBoxPathTest.h"
#include "Player/ARPGPlayerCharacter.h"
#include "Character/ARPGEnemyCharacter.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

AVSCombatGrayBoxPathTest::AVSCombatGrayBoxPathTest()
{
	PrimaryActorTick.bCanEverTick = true;
	TimeLimit = 20.f;
	// Gray-box slice: incidental anim/montage warnings are not failures.
	LogWarningHandling = EFunctionalTestLogHandling::OutputIgnored;
}

void AVSCombatGrayBoxPathTest::PrepareTest()
{
	Super::PrepareTest();
	Phase = 0;
	PhaseTime = 0.f;
}

void AVSCombatGrayBoxPathTest::StartTest()
{
	Super::StartTest();

	Player = Cast<AARPGPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	for (TActorIterator<AARPGEnemyCharacter> It(GetWorld()); It; ++It)
	{
		Enemy = *It;
		break;
	}

	// Own the fixture (shared-PIE-world hygiene): at some placements the map's
	// enemies are already dead/destroyed by earlier tests — spawn a fresh one
	// instead of failing, and remove it again in CleanUp.
	if (Player.IsValid() && !Enemy.IsValid())
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		FVector Fwd = Player->GetActorForwardVector(); Fwd.Z = 0.f; Fwd = Fwd.GetSafeNormal();
		if (Fwd.IsNearlyZero()) { Fwd = FVector::ForwardVector; }
		if (AARPGEnemyCharacter* E = GetWorld()->SpawnActor<AARPGEnemyCharacter>(
				AARPGEnemyCharacter::StaticClass(),
				Player->GetActorLocation() + Fwd * 400.f, FRotator::ZeroRotator, Params))
		{
			Enemy = E;
			SpawnedFixture = E;
		}
	}

	if (!Player.IsValid() || !Enemy.IsValid())
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("Player or Enemy missing in the slice level"));
	}
}

void AVSCombatGrayBoxPathTest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsRunning() || !Player.IsValid() || !Enemy.IsValid())
	{
		return;
	}

	PhaseTime += DeltaSeconds;

	// Phase 0 — reposition the player into melee range, facing the enemy, and
	// record the enemy's start health. The slice spawns them 400cm apart, which
	// is outside MeleeHitRange (180cm), so we must close the distance.
	if (Phase == 0)
	{
		const FVector EnemyLoc = Enemy->GetActorLocation();
		const FVector ToEnemy = EnemyLoc - Player->GetActorLocation();
		const FVector Dir = ToEnemy.GetSafeNormal2D();
		const FVector StandLoc = EnemyLoc - Dir * 150.f; // 150cm < 180cm MeleeHitRange

		Player->SetActorLocation(
			FVector(StandLoc.X, StandLoc.Y, Player->GetActorLocation().Z),
			false, nullptr, ETeleportType::TeleportPhysics);
		Player->SetActorRotation(Dir.Rotation());

		if (UAbilitySystemComponent* EnemyASC = Enemy->GetAbilitySystemComponent())
		{
			EnemyStartHealth = EnemyASC->GetNumericAttribute(UARPGAttributeSet::GetHealthAttribute());
		}

		if (PhaseTime >= 0.2f)
		{
			Phase = 1;
			PhaseTime = 0.f;
		}
		return;
	}

	// Phase 1 — activate the melee ability. Crucially, we do NOT send Event.MeleeHit.
	if (Phase == 1)
	{
		if (PhaseTime <= DeltaSeconds + KINDA_SMALL_NUMBER)
		{
			UAbilitySystemComponent* PlayerASC = Player->GetAbilitySystemComponent();
			if (!PlayerASC)
			{
				FinishTest(EFunctionalTestResult::Failed, TEXT("Player has no AbilitySystemComponent"));
				return;
			}

			FGameplayTagContainer MeleeTag;
			MeleeTag.AddTag(ARPGGameplayTags::Ability_Melee_LightAttack);
			const bool bActivated = PlayerASC->TryActivateAbilitiesByTag(MeleeTag);
			AssertTrue(bActivated, TEXT("gray-box: melee ability should have activated"));
		}

		// GrayBoxHitDelay is 0.2s; wait 1.0s to cover the self-applied hit + GE propagation.
		if (PhaseTime >= 1.0f)
		{
			Phase = 2;
			PhaseTime = 0.f;
		}
		return;
	}

	// Phase 2 — assert the enemy took damage from the SELF-APPLIED gray-box hit,
	// with no Event.MeleeHit ever sent by this test.
	if (Phase == 2)
	{
		if (PhaseTime >= 0.5f)
		{
			if (UAbilitySystemComponent* EnemyASC = Enemy->GetAbilitySystemComponent())
			{
				const float HealthNow = EnemyASC->GetNumericAttribute(UARPGAttributeSet::GetHealthAttribute());
				AssertTrue(HealthNow < EnemyStartHealth,
					FString::Printf(TEXT("gray-box damage: enemy Health should drop from %.1f via the self-applied hit (no Event.MeleeHit sent), is %.1f"),
						EnemyStartHealth, HealthNow));
			}
			else
			{
				AssertTrue(false, TEXT("gray-box damage: enemy has no ASC to read Health from"));
			}

			FinishTest(EFunctionalTestResult::Default, TEXT("gray-box damage path verified"));
		}
		return;
	}
}

void AVSCombatGrayBoxPathTest::CleanUp()
{
	if (SpawnedFixture.IsValid())
	{
		SpawnedFixture->Destroy();
		SpawnedFixture = nullptr;
	}
	Super::CleanUp();
}
