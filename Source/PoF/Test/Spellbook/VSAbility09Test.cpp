#include "Test/Spellbook/VSAbility09Test.h"
#include "AbilitySystem/GA_VS09Smite.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "Player/ARPGPlayerCharacter.h"
#include "Character/ARPGEnemyCharacter.h"
#include "AbilitySystemComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

AVSAbility09Test::AVSAbility09Test()
{
	PrimaryActorTick.bCanEverTick = true;
	TimeLimit = 20.f;

	// Gray-box slice: empty/missing montages produce incidental engine *warnings*
	// that are NOT failures. The pass/fail criterion is the AssertTrue below.
	LogWarningHandling = EFunctionalTestLogHandling::OutputIgnored;
}

void AVSAbility09Test::PrepareTest()
{
	Super::PrepareTest();
	PhaseTime = 0.f;
	bActivated = false;
}

void AVSAbility09Test::StartTest()
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
	UAbilitySystemComponent* EnemyASC = Enemy->GetAbilitySystemComponent();
	if (!PlayerASC || !EnemyASC)
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("Player or Enemy has no AbilitySystemComponent"));
		return;
	}

	EnemyStartHealth = EnemyASC->GetNumericAttribute(UARPGAttributeSet::GetHealthAttribute());

	// Grant the gray-box ability to the player at runtime — pure C++, no config-BP.
	PlayerASC->GiveAbility(FGameplayAbilitySpec(UGA_VS09Smite::StaticClass(), 1, INDEX_NONE, this));

	// Place the player next to the enemy (well within HitRadius) and face it.
	// The ability is omnidirectional, so facing is incidental — distance is what matters.
	const FVector EnemyLoc = Enemy->GetActorLocation();
	const FVector PlayerLoc = EnemyLoc + FVector(150.f, 0.f, 0.f);
	Player->SetActorLocation(PlayerLoc, false, nullptr, ETeleportType::TeleportPhysics);
	const FVector LookDir = (EnemyLoc - PlayerLoc).GetSafeNormal();
	if (!LookDir.IsNearlyZero())
	{
		Player->SetActorRotation(LookDir.Rotation());
	}
}

void AVSAbility09Test::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsRunning() || !Player.IsValid() || !Enemy.IsValid())
	{
		return;
	}

	PhaseTime += DeltaSeconds;

	// Activate once, after a short settle so the player pawn + ASC are live.
	if (!bActivated && PhaseTime >= 0.5f)
	{
		bActivated = true;
		UAbilitySystemComponent* PlayerASC = Player->GetAbilitySystemComponent();
		const bool bTried = PlayerASC && PlayerASC->TryActivateAbilityByClass(UGA_VS09Smite::StaticClass());
		UE_LOG(LogTemp, Log, TEXT("[VSAbility09Test] TryActivateAbilityByClass(GA_VS09Smite) -> %s"),
			bTried ? TEXT("true") : TEXT("false"));
	}

	// Assert the enemy took damage (synchronous radial GE_Damage applies on activation).
	if (PhaseTime >= 1.5f)
	{
		UAbilitySystemComponent* EnemyASC = Enemy->GetAbilitySystemComponent();
		if (EnemyASC)
		{
			const float HealthNow = EnemyASC->GetNumericAttribute(UARPGAttributeSet::GetHealthAttribute());
			AssertTrue(HealthNow < EnemyStartHealth,
				FString::Printf(TEXT("enemy should have taken damage from GA_VS09Smite: start=%.1f now=%.1f"),
					EnemyStartHealth, HealthNow));
		}
		else
		{
			AssertTrue(false, TEXT("enemy has no ASC at health check"));
		}

		FinishTest(EFunctionalTestResult::Default, TEXT("generated ability verified — enemy took damage"));
	}
}
