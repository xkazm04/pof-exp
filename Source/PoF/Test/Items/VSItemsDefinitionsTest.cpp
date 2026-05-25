#include "Test/Items/VSItemsDefinitionsTest.h"
#include "Inventory/ARPGItemDefinition.h"
#include "GameplayEffect.h"
#include "AbilitySystem/ARPGAttributeSet.h"

AVSItemsDefinitionsTest::AVSItemsDefinitionsTest()
{
	PrimaryActorTick.bCanEverTick = true;
	TimeLimit = 10.f;
	LogWarningHandling = EFunctionalTestLogHandling::OutputIgnored;
}

void AVSItemsDefinitionsTest::StartTest()
{
	Super::StartTest();
	PhaseTime = 0.f;
	bDone = false;
}

void AVSItemsDefinitionsTest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsRunning() || bDone)
	{
		return;
	}

	PhaseTime += DeltaSeconds;
	if (PhaseTime < 0.2f) return;
	bDone = true;

	// Instantiate a transient item definition (no .uasset needed for the schema gate).
	UARPGItemDefinition* Item = NewObject<UARPGItemDefinition>(this);
	if (!Item)
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("NewObject<UARPGItemDefinition> returned null"));
		return;
	}

	Item->DisplayName = FText::FromString(TEXT("VS09 Test Sword"));
	Item->Description = FText::FromString(TEXT("Gray-box test item for the items catalog gate."));
	Item->Type = EARPGItemType::Weapon;
	Item->Rarity = EARPGItemRarity::Rare;
	Item->MaxStackSize = 1;
	Item->BaseValue = 100.f;
	Item->Weight = 2.5f;
	Item->RequiredLevel = 5;
	Item->AllowedSlots.Add(EEquipmentSlot::Weapon);

	// 1. Property roundtrip.
	AssertTrue(Item->DisplayName.ToString() == TEXT("VS09 Test Sword"),
		TEXT("DisplayName should roundtrip"));
	AssertTrue(Item->Type == EARPGItemType::Weapon,
		TEXT("Type should be Weapon"));
	AssertTrue(Item->Rarity == EARPGItemRarity::Rare,
		TEXT("Rarity should be Rare"));
	AssertTrue(FMath::IsNearlyEqual(Item->BaseValue, 100.f),
		TEXT("BaseValue should roundtrip"));
	AssertTrue(Item->AllowedSlots.Num() == 1 && Item->AllowedSlots[0] == EEquipmentSlot::Weapon,
		TEXT("AllowedSlots should contain Weapon"));

	// 2. Rarity helpers behave (color non-black, display name non-empty).
	const FLinearColor RareColor = UARPGItemDefinition::GetRarityColor(EARPGItemRarity::Rare);
	AssertTrue(!RareColor.Equals(FLinearColor::Black),
		TEXT("GetRarityColor(Rare) should not be black"));

	const FString RareName = UARPGItemDefinition::GetRarityDisplayName(EARPGItemRarity::Rare).ToString();
	AssertTrue(!RareName.IsEmpty(),
		FString::Printf(TEXT("GetRarityDisplayName(Rare) should be non-empty, got '%s'"), *RareName));

	// 3. PrimaryAssetId valid.
	const FPrimaryAssetId Id = Item->GetPrimaryAssetId();
	AssertTrue(Id.IsValid(),
		TEXT("GetPrimaryAssetId() should be valid for a transient UARPGItemDefinition"));

	UE_LOG(LogTemp, Log, TEXT("[VSItemsDefinitionsTest] item schema verified — rarity=%s, color=%s, slots=%d"),
		*RareName, *RareColor.ToString(), Item->AllowedSlots.Num());

	// ── Real authored asset: the catalog target Iron Longsword (entity item-1). ──
	// Closes the "fixture, not the entity" gap (Catalog Pipeline / Fireball finding):
	// the gate now asserts the actual DA_IronLongsword.uasset authored by
	// Content/Python/author_items.py matches the canonical catalog data — not just a
	// transient schema fixture. If the asset is missing, run author_items.py first.
	UARPGItemDefinition* Sword = LoadObject<UARPGItemDefinition>(
		nullptr, TEXT("/Game/Data/Items/DA_IronLongsword.DA_IronLongsword"));
	if (!Sword)
	{
		FinishTest(EFunctionalTestResult::Failed,
			TEXT("DA_IronLongsword not found at /Game/Data/Items/ — run author_items.py to author it"));
		return;
	}

	AssertTrue(Sword->DisplayName.ToString() == TEXT("Iron Longsword"),
		TEXT("Iron Longsword DisplayName matches the catalog entity"));
	AssertTrue(Sword->Type == EARPGItemType::Weapon,
		TEXT("Iron Longsword Type is Weapon"));
	AssertTrue(Sword->Rarity == EARPGItemRarity::Common,
		TEXT("Iron Longsword Rarity is Common"));
	AssertTrue(Sword->MaxStackSize == 1,
		TEXT("Iron Longsword is non-stackable (MaxStackSize == 1)"));
	AssertTrue(Sword->AllowedSlots.Num() == 1 && Sword->AllowedSlots[0] == EEquipmentSlot::Weapon,
		TEXT("Iron Longsword equips to the Weapon slot"));

	// Equip effect carries the weapon's offense: assert it grants +15 AttackPower
	// (canonical avg of the 12-18 damage) — the same modifier-config gate the
	// Fireball test uses, so the GE stays in lockstep with the catalog entity.
	AssertTrue(Sword->OnEquipEffect != nullptr,
		TEXT("Iron Longsword has an OnEquipEffect"));
	if (Sword->OnEquipEffect)
	{
		const UGameplayEffect* EquipGE = Sword->OnEquipEffect.GetDefaultObject();
		if (AssertTrue(EquipGE != nullptr, TEXT("OnEquipEffect CDO resolves")))
		{
			bool bFoundAttackPower = false;
			for (const FGameplayModifierInfo& Mod : EquipGE->Modifiers)
			{
				if (Mod.Attribute == UARPGAttributeSet::GetAttackPowerAttribute()
					&& Mod.ModifierOp == EGameplayModOp::Additive)
				{
					float Magnitude = 0.f;
					const bool bStatic = Mod.ModifierMagnitude.GetStaticMagnitudeIfPossible(1.f, Magnitude);
					AssertTrue(bStatic, TEXT("Equip AttackPower modifier has a static magnitude"));
					AssertTrue(FMath::IsNearlyEqual(Magnitude, 15.f),
						FString::Printf(TEXT("Equip GE grants +15 AttackPower (canonical), got %.1f"), Magnitude));
					bFoundAttackPower = true;
				}
			}
			AssertTrue(bFoundAttackPower,
				TEXT("Equip GE has an additive AttackPower modifier"));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[VSItemsDefinitionsTest] real asset verified — DA_IronLongsword '%s' (slots=%d, equipGE=%s)"),
		*Sword->DisplayName.ToString(), Sword->AllowedSlots.Num(),
		Sword->OnEquipEffect ? TEXT("set") : TEXT("null"));

	FinishTest(EFunctionalTestResult::Default,
		TEXT("UARPGItemDefinition schema + DA_IronLongsword catalog asset verified"));
}
