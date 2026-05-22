#include "Inventory/ARPGItemDefinition.h"

FLinearColor UARPGItemDefinition::GetRarityColor(EARPGItemRarity InRarity)
{
	switch (InRarity)
	{
	case EARPGItemRarity::Common:     return FLinearColor(0.7f, 0.7f, 0.7f, 1.f);  // Gray
	case EARPGItemRarity::Uncommon:   return FLinearColor(0.1f, 0.8f, 0.1f, 1.f);  // Green
	case EARPGItemRarity::Rare:       return FLinearColor(0.2f, 0.4f, 1.0f, 1.f);  // Blue
	case EARPGItemRarity::Epic:       return FLinearColor(0.6f, 0.2f, 0.9f, 1.f);  // Purple
	case EARPGItemRarity::Legendary:  return FLinearColor(1.0f, 0.6f, 0.0f, 1.f);  // Orange
	default:                          return FLinearColor::White;
	}
}

FText UARPGItemDefinition::GetRarityDisplayName(EARPGItemRarity InRarity)
{
	switch (InRarity)
	{
	case EARPGItemRarity::Common:     return FText::FromString(TEXT("Common"));
	case EARPGItemRarity::Uncommon:   return FText::FromString(TEXT("Uncommon"));
	case EARPGItemRarity::Rare:       return FText::FromString(TEXT("Rare"));
	case EARPGItemRarity::Epic:       return FText::FromString(TEXT("Epic"));
	case EARPGItemRarity::Legendary:  return FText::FromString(TEXT("Legendary"));
	default:                          return FText::FromString(TEXT("Unknown"));
	}
}

FPrimaryAssetId UARPGItemDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ItemDefinition"), GetFName());
}
