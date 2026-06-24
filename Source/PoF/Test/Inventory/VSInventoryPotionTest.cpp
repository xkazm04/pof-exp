#include "Test/Inventory/VSInventoryPotionTest.h"
#include "Inventory/ARPGItemDefinition.h"
#include "GameplayEffect.h"
#include "AbilitySystem/ARPGAttributeSet.h"

AVSInventoryPotionTest::AVSInventoryPotionTest()
{
	PrimaryActorTick.bCanEverTick = true;
	TimeLimit = 10.f;
	LogWarningHandling = EFunctionalTestLogHandling::OutputIgnored;
}

void AVSInventoryPotionTest::StartTest()
{
	Super::StartTest();
	PhaseTime = 0.f;
	bDone = false;
}

void AVSInventoryPotionTest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsRunning() || bDone)
	{
		return;
	}

	PhaseTime += DeltaSeconds;
	if (PhaseTime < 0.2f) return;
	bDone = true;

	// Real authored asset: the Minor Health Potion (Stream 4, app entity item-7),
	// authored by Content/Python/inventory_stream/author_inventory_assets.py.
	UARPGItemDefinition* Potion = LoadObject<UARPGItemDefinition>(
		nullptr, TEXT("/Game/Inventory/DA_HealthPotion.DA_HealthPotion"));
	if (!Potion)
	{
		FinishTest(EFunctionalTestResult::Failed,
			TEXT("DA_HealthPotion not found at /Game/Inventory/ — run author_inventory_assets.py"));
		return;
	}

	AssertTrue(Potion->DisplayName.ToString() == TEXT("Minor Health Potion"),
		TEXT("Potion DisplayName matches the catalog entity (item-7)"));
	AssertTrue(Potion->Type == EARPGItemType::Consumable,
		TEXT("Potion Type is Consumable"));
	AssertTrue(Potion->MaxStackSize > 1,
		TEXT("Potion is stackable (MaxStackSize > 1)"));
	AssertTrue(Potion->OnUseEffect != nullptr,
		TEXT("Potion has an OnUseEffect (the heal GE)"));

	// The OnUseEffect must heal +50 (the 50->100 contract): assert it carries an additive
	// IncomingHeal modifier with a static magnitude of 50 — the config gate behind the
	// runtime heal proof.
	if (Potion->OnUseEffect)
	{
		const UGameplayEffect* HealGE = Potion->OnUseEffect.GetDefaultObject();
		if (AssertTrue(HealGE != nullptr, TEXT("OnUseEffect CDO resolves")))
		{
			bool bFoundHeal = false;
			for (const FGameplayModifierInfo& Mod : HealGE->Modifiers)
			{
				if (Mod.Attribute == UARPGAttributeSet::GetIncomingHealAttribute()
					&& Mod.ModifierOp == EGameplayModOp::Additive)
				{
					float Magnitude = 0.f;
					const bool bStatic = Mod.ModifierMagnitude.GetStaticMagnitudeIfPossible(1.f, Magnitude);
					AssertTrue(bStatic, TEXT("Heal IncomingHeal modifier has a static magnitude"));
					AssertTrue(FMath::IsNearlyEqual(Magnitude, 50.f),
						FString::Printf(TEXT("OnUseEffect heals +50 IncomingHeal, got %.1f"), Magnitude));
					bFoundHeal = true;
				}
			}
			AssertTrue(bFoundHeal,
				TEXT("OnUseEffect has an additive IncomingHeal modifier"));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[VSInventoryPotionTest] potion verified — '%s' Type=Consumable stack=%d onUse=%s"),
		*Potion->DisplayName.ToString(), Potion->MaxStackSize,
		Potion->OnUseEffect ? TEXT("set") : TEXT("null"));

	FinishTest(EFunctionalTestResult::Default,
		TEXT("DA_HealthPotion + GE_HealthPotion (+50 heal) config verified"));
}
