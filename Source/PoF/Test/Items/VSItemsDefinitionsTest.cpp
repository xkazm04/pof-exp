#include "Test/Items/VSItemsDefinitionsTest.h"
#include "Inventory/ARPGItemDefinition.h"

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

	FinishTest(EFunctionalTestResult::Default, TEXT("UARPGItemDefinition schema verified"));
}
