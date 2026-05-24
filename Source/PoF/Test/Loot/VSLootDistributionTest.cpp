#include "Test/Loot/VSLootDistributionTest.h"
#include "Inventory/ARPGItemDefinition.h"
#include "Inventory/ARPGLootTable.h"

AVSLootDistributionTest::AVSLootDistributionTest()
{
	PrimaryActorTick.bCanEverTick = true;
	TimeLimit = 15.f;
	LogWarningHandling = EFunctionalTestLogHandling::OutputIgnored;
}

void AVSLootDistributionTest::StartTest()
{
	Super::StartTest();
	PhaseTime = 0.f;
	bDone = false;
}

void AVSLootDistributionTest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsRunning() || bDone)
	{
		return;
	}

	PhaseTime += DeltaSeconds;
	if (PhaseTime < 0.2f) return;
	bDone = true;

	// Build the drop item.
	UARPGItemDefinition* DropItem = NewObject<UARPGItemDefinition>(this);
	if (!DropItem)
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("NewObject<UARPGItemDefinition>(DropItem) returned null"));
		return;
	}
	DropItem->DisplayName = FText::FromString(TEXT("VS09 Loot Drop"));
	DropItem->Type = EARPGItemType::Material;
	DropItem->Rarity = EARPGItemRarity::Common;
	DropItem->MaxStackSize = 99;
	DropItem->BaseValue = 1.f;

	// Build the deterministic single-entry table — NothingWeight=0 ⇒ every roll drops something.
	UARPGLootTable* Table = NewObject<UARPGLootTable>(this);
	if (!Table)
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("NewObject<UARPGLootTable>(Table) returned null"));
		return;
	}
	Table->NothingWeight = 0.f;
	FLootEntry Entry;
	Entry.Item = DropItem;
	Entry.DropWeight = 1.f;
	Entry.MinQuantity = 1;
	Entry.MaxQuantity = 1;
	Entry.MinRarity = EARPGItemRarity::Common;
	Entry.MaxRarity = EARPGItemRarity::Common;
	Table->Entries.Add(Entry);

	// Roll N times.
	int32 Successes = 0;
	int32 InstancesObserved = 0;
	for (int32 i = 0; i < RollCount; ++i)
	{
		TArray<UARPGItemInstance*> Roll = Table->RollLoot(this, 1.0f);
		if (Roll.Num() >= 1)
		{
			++Successes;
			InstancesObserved += Roll.Num();
		}
	}

	const float Rate = RollCount > 0 ? static_cast<float>(Successes) / static_cast<float>(RollCount) : 0.f;

	UE_LOG(LogTemp, Log, TEXT("[VSLootDistributionTest] rolls=%d successes=%d (rate=%.3f) instances=%d"),
		RollCount, Successes, Rate, InstancesObserved);

	AssertTrue(Rate >= MinSuccessRate,
		FString::Printf(TEXT("RollLoot success rate %.3f should be >= %.3f over %d rolls (single-entry table, NothingWeight=0)"),
			Rate, MinSuccessRate, RollCount));

	FinishTest(EFunctionalTestResult::Default,
		FString::Printf(TEXT("UARPGLootTable.RollLoot verified — %d/%d rolls dropped (rate %.3f)"),
			Successes, RollCount, Rate));
}
