#include "Loot/ARPGLootDropComponent.h"
#include "Loot/ARPGWorldItem.h"
#include "Inventory/ARPGLootTable.h"
#include "Inventory/ARPGItemInstance.h"
#include "Inventory/ARPGItemDefinition.h"
#include "Character/ARPGEnemyCharacter.h"

UARPGLootDropComponent::UARPGLootDropComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UARPGLootDropComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoDropOnDeath)
	{
		if (AARPGEnemyCharacter* Enemy = Cast<AARPGEnemyCharacter>(GetOwner()))
		{
			Enemy->OnEnemyDeath.AddDynamic(this, &UARPGLootDropComponent::OnOwnerDeath);
		}
	}
}

void UARPGLootDropComponent::OnOwnerDeath(AARPGEnemyCharacter* Enemy)
{
	DropLoot();
}

TArray<AARPGWorldItem*> UARPGLootDropComponent::DropLoot()
{
	TArray<AARPGWorldItem*> SpawnedItems;

	// --- Loot-table items ---
	if (LootTable)
	{
		// Collect all rolled items across NumRolls
		TArray<UARPGItemInstance*> AllItems;
		for (int32 Roll = 0; Roll < NumRolls; ++Roll)
		{
			TArray<UARPGItemInstance*> RolledItems = LootTable->RollLoot(GetOwner(), RarityBonusMultiplier);
			AllItems.Append(RolledItems);
		}

		for (int32 i = 0; i < AllItems.Num(); ++i)
		{
			if (AARPGWorldItem* WorldItem = SpawnWorldItem(AllItems[i], i, AllItems.Num()))
			{
				SpawnedItems.Add(WorldItem);
			}
		}

		UE_LOG(LogTemp, Log, TEXT("[LootDrop] %s — rolled %d times, dropped %d loot item(s)"),
			*GetNameSafe(GetOwner()), NumRolls, SpawnedItems.Num());
	}

	// --- Gold pickup (separate from loot-table items) ---
	if (bDropGold)
	{
		int32 EnemyLevel = 1;
		if (const AARPGEnemyCharacter* Enemy = Cast<AARPGEnemyCharacter>(GetOwner()))
		{
			EnemyLevel = Enemy->GetCharacterLevel();
		}

		if (AARPGWorldItem* GoldItem = DropGold(EnemyLevel))
		{
			SpawnedItems.Add(GoldItem);
		}
	}

	return SpawnedItems;
}

AARPGWorldItem* UARPGLootDropComponent::DropGold(int32 EnemyLevel)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UARPGItemDefinition* GoldDef = ResolveGoldDefinition();
	if (!GoldDef)
	{
		return nullptr;
	}

	// Gold scales with enemy level, then a random ± variance band is applied.
	const int32 Level = FMath::Max(1, EnemyLevel);
	const int32 BaseGold = FlatGoldBonus + Level * BaseGoldPerLevel;
	const float VarianceRoll = FMath::FRandRange(1.f - GoldVariance, 1.f + GoldVariance);
	const int32 GoldAmount = FMath::Max(1, FMath::RoundToInt(BaseGold * VarianceRoll));

	// Represent gold as an item instance whose stack count is the gold amount.
	UARPGItemInstance* GoldInstance = NewObject<UARPGItemInstance>(GetOwner() ? (UObject*)GetOwner() : (UObject*)this);
	GoldInstance->Definition = GoldDef;
	GoldInstance->StackCount = GoldAmount;
	GoldInstance->ItemLevel = Level;
	GoldInstance->RolledRarity = EARPGItemRarity::Common;
	GoldInstance->UniqueId = FGuid::NewGuid();

	// Random outward arc so the gold scatters away from the loot items.
	const float Angle = FMath::FRandRange(0.f, 2.f * PI);
	const float HorizontalSpeed = FMath::FRandRange(DropScatterRadius * 0.5f, DropScatterRadius * 1.5f);
	const float VerticalSpeed = FMath::FRandRange(250.f, 450.f);
	const FVector LaunchVelocity(
		FMath::Cos(Angle) * HorizontalSpeed,
		FMath::Sin(Angle) * HorizontalSpeed,
		VerticalSpeed);

	AARPGWorldItem* GoldItem = SpawnWorldItemInternal(GoldInstance, LaunchVelocity);
	if (GoldItem)
	{
		UE_LOG(LogTemp, Log, TEXT("[LootDrop] %s — dropped %d gold (EnemyLv=%d)"),
			*GetNameSafe(GetOwner()), GoldAmount, Level);
	}
	return GoldItem;
}

AARPGWorldItem* UARPGLootDropComponent::SpawnWorldItem(UARPGItemInstance* Instance, int32 Index, int32 Total)
{
	if (!Instance)
	{
		return nullptr;
	}

	// Launch items outward in an even arc so multiple drops fan out for visual pop.
	const float Angle = (Total > 1) ? (2.f * PI * Index / Total) : FMath::FRandRange(0.f, 2.f * PI);
	const float HorizontalSpeed = FMath::FRandRange(DropScatterRadius * 0.5f, DropScatterRadius * 1.5f);
	const float VerticalSpeed = FMath::FRandRange(200.f, 400.f);

	const FVector LaunchVelocity(
		FMath::Cos(Angle) * HorizontalSpeed,
		FMath::Sin(Angle) * HorizontalSpeed,
		VerticalSpeed);

	return SpawnWorldItemInternal(Instance, LaunchVelocity);
}

AARPGWorldItem* UARPGLootDropComponent::SpawnWorldItemInternal(UARPGItemInstance* Instance, FVector LaunchVelocity)
{
	UWorld* World = GetWorld();
	if (!World || !Instance || !GetOwner())
	{
		return nullptr;
	}

	// Spawn at the owner's position + height offset + a slight random XY offset
	// so stacked drops don't all overlap before the launch arc separates them.
	const FVector OwnerLocation = GetOwner()->GetActorLocation();
	const float OffsetRadius = FMath::Min(DropScatterRadius * 0.25f, 50.f);
	FVector SpawnLocation = OwnerLocation;
	SpawnLocation.X += FMath::FRandRange(-OffsetRadius, OffsetRadius);
	SpawnLocation.Y += FMath::FRandRange(-OffsetRadius, OffsetRadius);
	SpawnLocation.Z += DropHeightOffset;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AARPGWorldItem* WorldItem = World->SpawnActor<AARPGWorldItem>(
		AARPGWorldItem::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParams);
	if (!WorldItem)
	{
		return nullptr;
	}

	WorldItem->InitFromItemInstance(Instance);

	// Give it the upward/outward impulse so loot scatters from the corpse.
	WorldItem->LaunchItem(LaunchVelocity);

	// Slice mode: no inventory required — the item self-destructs after a fixed
	// lifetime instead of waiting to be picked up.
	if (bSliceMode && SliceLifeSpan > 0.f)
	{
		WorldItem->SetLifeSpan(SliceLifeSpan);
	}

	return WorldItem;
}

UARPGItemDefinition* UARPGLootDropComponent::ResolveGoldDefinition()
{
	// Designer-assigned definition takes priority.
	if (GoldItemDefinition)
	{
		return GoldItemDefinition;
	}

	// Lazily build a transient gold definition so the slice works without assets.
	if (!RuntimeGoldDefinition)
	{
		RuntimeGoldDefinition = NewObject<UARPGItemDefinition>(this, TEXT("RuntimeGoldDefinition"));
		RuntimeGoldDefinition->DisplayName = NSLOCTEXT("Loot", "GoldName", "Gold");
		RuntimeGoldDefinition->Description = NSLOCTEXT("Loot", "GoldDesc", "A pile of glittering coins.");
		RuntimeGoldDefinition->Type = EARPGItemType::Material;
		RuntimeGoldDefinition->Rarity = EARPGItemRarity::Common;
		RuntimeGoldDefinition->MaxStackSize = MAX_int32;
		RuntimeGoldDefinition->BaseValue = 1.f;
	}

	return RuntimeGoldDefinition;
}
