#include "Inventory/ARPGItemInstance.h"
#include "Inventory/ARPGItemDefinition.h"
#include "Net/UnrealNetwork.h"

UARPGItemInstance::UARPGItemInstance()
{
	UniqueId = FGuid::NewGuid();
}

// ---------------------------------------------------------------------------
// Replication
// ---------------------------------------------------------------------------

void UARPGItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UARPGItemInstance, Definition);
	DOREPLIFETIME(UARPGItemInstance, StackCount);
	DOREPLIFETIME(UARPGItemInstance, ItemLevel);
	DOREPLIFETIME(UARPGItemInstance, Affixes);
	DOREPLIFETIME(UARPGItemInstance, UniqueId);
	DOREPLIFETIME(UARPGItemInstance, RolledRarity);
	DOREPLIFETIME(UARPGItemInstance, bHasRolledRarity);
}

// ---------------------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------------------

void UARPGItemInstance::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	// When saving/loading via SaveGame, use the proxy archive to resolve UObject
	// references as string paths (asset paths). This lets the Definition TObjectPtr
	// round-trip through saves without requiring identical pointer values.
	if (Ar.IsSaveGame())
	{
		FObjectAndNameAsStringProxyArchive ProxyAr(Ar, true);
		ProxyAr.SetFilterEditorOnly(true);

		// Serialize all UPROPERTY(SaveGame) fields through the proxy
		UClass* ThisClass = GetClass();
		ThisClass->SerializeTaggedProperties(ProxyAr, reinterpret_cast<uint8*>(this),
			ThisClass, nullptr);
	}
}

FARPGItemSaveData UARPGItemInstance::ToSaveData() const
{
	FARPGItemSaveData Data;
	Data.StackCount = StackCount;
	Data.ItemLevel = ItemLevel;
	Data.UniqueId = UniqueId;
	Data.Affixes = Affixes;

	Data.RolledRarity = RolledRarity;
	Data.bHasRolledRarity = bHasRolledRarity;

	if (Definition)
	{
		Data.DefinitionId = Definition->GetPrimaryAssetId();
	}

	return Data;
}

void UARPGItemInstance::FromSaveData(const FARPGItemSaveData& SaveData, UARPGItemDefinition* InDefinition)
{
	Definition = InDefinition;
	StackCount = FMath::Max(1, SaveData.StackCount);
	ItemLevel = FMath::Max(1, SaveData.ItemLevel);
	UniqueId = SaveData.UniqueId.IsValid() ? SaveData.UniqueId : FGuid::NewGuid();
	Affixes = SaveData.Affixes;
	RolledRarity = SaveData.RolledRarity;
	bHasRolledRarity = SaveData.bHasRolledRarity;

	// Clear any stale active handles from saved affix data
	for (FItemAffix& Affix : Affixes)
	{
		Affix.ActiveHandle.Invalidate();
	}
}

// ---------------------------------------------------------------------------
// Active Effect Management
// ---------------------------------------------------------------------------

bool UARPGItemInstance::HasActiveEffects() const
{
	if (EquipEffectHandle.IsValid())
	{
		return true;
	}

	for (const FItemAffix& Affix : Affixes)
	{
		if (Affix.ActiveHandle.IsValid())
		{
			return true;
		}
	}

	return false;
}

void UARPGItemInstance::ClearActiveHandles()
{
	EquipEffectHandle.Invalidate();

	for (FItemAffix& Affix : Affixes)
	{
		Affix.ActiveHandle.Invalidate();
	}
}

// ---------------------------------------------------------------------------
// Display
// ---------------------------------------------------------------------------

FText UARPGItemInstance::GetDisplayName() const
{
	if (!Definition)
	{
		return FText::FromString(TEXT("Unknown"));
	}

	if (Affixes.IsEmpty())
	{
		return Definition->DisplayName;
	}

	return BuildAffixName();
}

FText UARPGItemInstance::BuildAffixName() const
{
	if (!Definition)
	{
		return FText::GetEmpty();
	}

	// Collect prefixes and suffixes from affixes
	FString Prefixes;
	FString Suffixes;

	for (const FItemAffix& Affix : Affixes)
	{
		if (Affix.DisplayName.IsEmpty())
		{
			continue;
		}

		const FString Name = Affix.DisplayName.ToString();

		if (Affix.bIsPrefix)
		{
			if (!Prefixes.IsEmpty())
			{
				Prefixes += TEXT(" ");
			}
			Prefixes += Name;
		}
		else
		{
			if (!Suffixes.IsEmpty())
			{
				Suffixes += TEXT(" ");
			}
			Suffixes += Name;
		}
	}

	// Build: "[Prefixes] BaseName [Suffixes]"
	const FString BaseName = Definition->DisplayName.ToString();
	FString FullName;

	if (!Prefixes.IsEmpty())
	{
		FullName = Prefixes + TEXT(" ") + BaseName;
	}
	else
	{
		FullName = BaseName;
	}

	if (!Suffixes.IsEmpty())
	{
		FullName += TEXT(" ") + Suffixes;
	}

	return FText::FromString(FullName);
}

float UARPGItemInstance::GetTotalStats() const
{
	float Total = 0.f;
	for (const FItemAffix& Affix : Affixes)
	{
		Total += Affix.Magnitude;
	}
	return Total;
}

bool UARPGItemInstance::CanStack() const
{
	if (!Definition)
	{
		return false;
	}
	return StackCount < Definition->MaxStackSize;
}

EARPGItemRarity UARPGItemInstance::GetEffectiveRarity() const
{
	if (bHasRolledRarity)
	{
		return RolledRarity;
	}
	return Definition ? Definition->Rarity : EARPGItemRarity::Common;
}
