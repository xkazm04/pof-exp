#include "Test/Combat/VSCombatTwoHealthSystemsTest.h"
#include "Player/ARPGPlayerCharacter.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"

AVSCombatTwoHealthSystemsTest::AVSCombatTwoHealthSystemsTest()
{
	PrimaryActorTick.bCanEverTick = true;
	TimeLimit = 15.f;
	LogWarningHandling = EFunctionalTestLogHandling::OutputIgnored;
}

void AVSCombatTwoHealthSystemsTest::PrepareTest()
{
	Super::PrepareTest();
	PhaseTime = 0.f;
	bAsserted = false;
}

void AVSCombatTwoHealthSystemsTest::StartTest()
{
	Super::StartTest();
	Player = Cast<AARPGPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!Player.IsValid())
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("Player missing in the slice level"));
	}
}

void AVSCombatTwoHealthSystemsTest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsRunning() || bAsserted || !Player.IsValid())
	{
		return;
	}

	PhaseTime += DeltaSeconds;
	// Let attribute initialization settle before reading.
	if (PhaseTime < 0.5f)
	{
		return;
	}

	bAsserted = true;

	UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	if (!ASC)
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("Player has no AbilitySystemComponent"));
		return;
	}

	const float GasHealth = ASC->GetNumericAttribute(UARPGAttributeSet::GetHealthAttribute());
	const float GasMaxHealth = ASC->GetNumericAttribute(UARPGAttributeSet::GetMaxHealthAttribute());

	AssertTrue(FMath::IsNearlyEqual(Player->GetHealth(), GasHealth, 0.01f),
		FString::Printf(TEXT("two-health: GetHealth() %.2f must equal GAS Health %.2f (§5 resolution)"),
			Player->GetHealth(), GasHealth));

	AssertTrue(FMath::IsNearlyEqual(Player->GetMaxHealth(), GasMaxHealth, 0.01f),
		FString::Printf(TEXT("two-health: GetMaxHealth() %.2f must equal GAS MaxHealth %.2f (§5 resolution)"),
			Player->GetMaxHealth(), GasMaxHealth));

	FinishTest(EFunctionalTestResult::Default, TEXT("player float health getters track GAS"));
}
