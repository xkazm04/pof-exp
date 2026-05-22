#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemTooltipWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class UARPGItemInstance;
class UBorder;

/**
 * Tooltip popup for inventory items. Shows name (rarity-colored),
 * item level, description, base stats, and rolled affixes.
 */
UCLASS()
class POF_API UItemTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Populate the tooltip from an item instance. */
	UFUNCTION(BlueprintCallable, Category = "UI|Tooltip")
	void SetItem(UARPGItemInstance* Item);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemLevelText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemTypeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DescriptionText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> AffixContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> TooltipBorder;

	/** Text class used for affix lines. If null, plain UTextBlock is created. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Tooltip")
	FSlateFontInfo AffixFont;

private:
	static FLinearColor GetRarityColor(uint8 Rarity);
	static FText GetRarityName(uint8 Rarity);
};
