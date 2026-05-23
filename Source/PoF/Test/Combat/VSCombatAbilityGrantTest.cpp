#include "Test/Combat/VSCombatAbilityGrantTest.h"
#include "Player/ARPGPlayerCharacter.h"
#include "AbilitySystem/GA_MeleeAttack.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Kismet/GameplayStatics.h"

AVSCombatAbilityGrantTest::AVSCombatAbilityGrantTest()
{
	TimeLimit = 10.f;
	LogWarningHandling = EFunctionalTestLogHandling::OutputIgnored;
}

void AVSCombatAbilityGrantTest::StartTest()
{
	Super::StartTest();

	AARPGPlayerCharacter* PlayerChar = Cast<AARPGPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!PlayerChar)
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("No player character"));
		return;
	}

	UAbilitySystemComponent* ASC = PlayerChar->GetAbilitySystemComponent();
	if (!ASC)
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("Player has no AbilitySystemComponent"));
		return;
	}

	const TArray<FGameplayAbilitySpec>& Specs = ASC->GetActivatableAbilities();
	AssertTrue(Specs.Num() > 0,
		FString::Printf(TEXT("ability grant: player should have >=1 ability granted, has %d"), Specs.Num()));

	bool bHasMelee = false;
	for (const FGameplayAbilitySpec& Spec : Specs)
	{
		if (Spec.Ability && Spec.Ability->IsA(UGA_MeleeAttack::StaticClass()))
		{
			bHasMelee = true;
			break;
		}
	}
	AssertTrue(bHasMelee, TEXT("ability grant: GA_MeleeAttack should be granted to the player ASC"));

	FinishTest(EFunctionalTestResult::Default, TEXT("ability grant verified"));
}
