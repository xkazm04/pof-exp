#include "UI/ItemTooltipWidget.h"
#include "Inventory/ARPGItemInstance.h"
#include "Inventory/ARPGItemDefinition.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/Border.h"

void UItemTooltipWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!AffixFont.HasValidFont())
	{
		AffixFont = FCoreStyle::GetDefaultFontStyle("Regular", 12);
	}
}

void UItemTooltipWidget::SetItem(UARPGItemInstance* Item)
{
	if (!Item || !Item->Definition)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);

	const UARPGItemDefinition* Def = Item->Definition;
	const FLinearColor RarityColor = GetRarityColor(static_cast<uint8>(Def->Rarity));

	// Name (rarity-colored, includes affix prefix/suffix decorations)
	if (ItemNameText)
	{
		ItemNameText->SetText(Item->GetDisplayName());
		ItemNameText->SetColorAndOpacity(FSlateColor(RarityColor));
	}

	// Item level
	if (ItemLevelText)
	{
		ItemLevelText->SetText(FText::FromString(
			FString::Printf(TEXT("Item Level %d"), Item->ItemLevel)));
	}

	// Type + Rarity
	if (ItemTypeText)
	{
		const FText RarityName = GetRarityName(static_cast<uint8>(Def->Rarity));
		const UEnum* TypeEnum = StaticEnum<EARPGItemType>();
		const FText TypeName = TypeEnum
			? TypeEnum->GetDisplayNameTextByValue(static_cast<int64>(Def->Type))
			: FText::FromString(TEXT("Item"));
		ItemTypeText->SetText(FText::Format(NSLOCTEXT("Tooltip", "TypeFmt", "{0} {1}"), RarityName, TypeName));
		ItemTypeText->SetColorAndOpacity(FSlateColor(RarityColor));
	}

	// Description
	if (DescriptionText)
	{
		if (Def->Description.IsEmpty())
		{
			DescriptionText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			DescriptionText->SetVisibility(ESlateVisibility::HitTestInvisible);
			DescriptionText->SetText(Def->Description);
		}
	}

	// Border tint
	if (TooltipBorder)
	{
		FLinearColor BorderColor = RarityColor;
		BorderColor.A = 0.8f;
		TooltipBorder->SetBrushColor(BorderColor);
	}

	// Affixes
	if (AffixContainer)
	{
		AffixContainer->ClearChildren();

		for (const FItemAffix& Affix : Item->Affixes)
		{
			UTextBlock* AffixLine = NewObject<UTextBlock>(this);
			AffixLine->SetFont(AffixFont);

			// Use affix DisplayName if available, otherwise fall back to tag leaf name
			FString DisplayStat;
			if (!Affix.DisplayName.IsEmpty())
			{
				DisplayStat = Affix.DisplayName.ToString();
			}
			else
			{
				const FString TagName = Affix.AffixTag.IsValid()
					? Affix.AffixTag.GetTagName().ToString()
					: TEXT("Unknown");

				DisplayStat = TagName;
				int32 DotIndex;
				if (TagName.FindLastChar('.', DotIndex))
				{
					DisplayStat = TagName.RightChop(DotIndex + 1);
				}
			}

			const FString Sign = Affix.Magnitude >= 0.f ? TEXT("+") : TEXT("");
			AffixLine->SetText(FText::FromString(
				FString::Printf(TEXT("%s%.0f %s"), *Sign, Affix.Magnitude, *DisplayStat)));
			AffixLine->SetColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.8f, 1.f)));

			AffixContainer->AddChild(AffixLine);
		}
	}
}

FLinearColor UItemTooltipWidget::GetRarityColor(uint8 Rarity)
{
	switch (static_cast<EARPGItemRarity>(Rarity))
	{
	case EARPGItemRarity::Common:    return FLinearColor(0.7f, 0.7f, 0.7f);
	case EARPGItemRarity::Uncommon:  return FLinearColor(0.3f, 0.8f, 0.3f);
	case EARPGItemRarity::Rare:      return FLinearColor(0.3f, 0.5f, 1.0f);
	case EARPGItemRarity::Epic:      return FLinearColor(0.6f, 0.2f, 0.9f);
	case EARPGItemRarity::Legendary: return FLinearColor(1.0f, 0.5f, 0.0f);
	default:                         return FLinearColor::White;
	}
}

FText UItemTooltipWidget::GetRarityName(uint8 Rarity)
{
	switch (static_cast<EARPGItemRarity>(Rarity))
	{
	case EARPGItemRarity::Common:    return FText::FromString(TEXT("Common"));
	case EARPGItemRarity::Uncommon:  return FText::FromString(TEXT("Uncommon"));
	case EARPGItemRarity::Rare:      return FText::FromString(TEXT("Rare"));
	case EARPGItemRarity::Epic:      return FText::FromString(TEXT("Epic"));
	case EARPGItemRarity::Legendary: return FText::FromString(TEXT("Legendary"));
	default:                         return FText::FromString(TEXT("Unknown"));
	}
}
