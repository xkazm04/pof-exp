#include "Test/VSEnemyAttackTest.h"
#include "Player/ARPGPlayerCharacter.h"
#include "Character/ARPGEnemyCharacter.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

AVSEnemyAttackTest::AVSEnemyAttackTest()
{
	PrimaryActorTick.bCanEverTick = true;
	TimeLimit = 20.f;

	// Gray-box slice: empty montages produce incidental engine *warnings* that are
	// NOT failures. The pass/fail criterion is the AssertTrue below. Ignore
	// warning-level log output; genuine Error-level output still fails the run.
	LogWarningHandling = EFunctionalTestLogHandling::OutputIgnored;
}

void AVSEnemyAttackTest::PrepareTest()
{
	Super::PrepareTest();
	PhaseTime = 0.f;
}

void AVSEnemyAttackTest::StartTest()
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
		FinishTest(EFunctionalTestResult::Failed, TEXT("Player or Enemy missing in the test map"));
		return;
	}

	UAbilitySystemComponent* PlayerASC = Player->GetAbilitySystemComponent();
	if (!PlayerASC)
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("Player has no AbilitySystemComponent"));
		return;
	}
	PlayerStartHealth = PlayerASC->GetNumericAttribute(UARPGAttributeSet::GetHealthAttribute());

	// Teleport the player squarely into the enemy's front arc, within AttackRange,
	// so the enemy's attack lands deterministically without depending on chase
	// movement. PerformFrontArcDamage uses the enemy's forward vector; the
	// controller re-faces the enemy toward the player each tick, keeping the
	// player in-arc.
	const FVector EnemyLoc = Enemy->GetActorLocation();
	const FVector EnemyFwd = Enemy->GetActorForwardVector();
	const float PlaceDist = Enemy->GetAttackRange() * 0.5f;
	Player->SetActorLocation(EnemyLoc + EnemyFwd * PlaceDist, false, nullptr, ETeleportType::TeleportPhysics);
}

void AVSEnemyAttackTest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsRunning() || !Player.IsValid())
	{
		return;
	}

	PhaseTime += DeltaSeconds;

	// Give the controller time to activate the attack (immediate first attack +
	// 0.3s fallback window) and the GE to apply. Assert early to minimise any
	// passive-regen window.
	if (PhaseTime >= 1.5f)
	{
		UAbilitySystemComponent* PlayerASC = Player->GetAbilitySystemComponent();
		if (PlayerASC)
		{
			const float HealthNow = PlayerASC->GetNumericAttribute(UARPGAttributeSet::GetHealthAttribute());
			AssertTrue(HealthNow < PlayerStartHealth,
				FString::Printf(TEXT("player should have taken damage from the enemy: start=%.1f now=%.1f"),
					PlayerStartHealth, HealthNow));
		}
		else
		{
			AssertTrue(false, TEXT("player has no ASC at health check"));
		}

		FinishTest(EFunctionalTestResult::Default, TEXT("enemy attack verified — player took damage"));
	}
}
