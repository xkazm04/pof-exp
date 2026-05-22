#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AbilitySlotWidget.generated.h"

class UImage;
class UTextBlock;

/**
 * Single ability slot in the ability bar.
 * Shows icon, cooldown sweep overlay, mana cost, and keybind label.
 * Cooldown sweep is driven externally by the parent AbilityBarWidget.
 */
UCLASS()
class POF_API UAbilitySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Set the ability icon texture. Pass nullptr to show an empty slot. */
	UFUNCTION(BlueprintCallable, Category = "UI|AbilitySlot")
	void SetIcon(UTexture2D* InIcon);

	/** Set the keybind label text (e.g., "1", "2", "Q"). */
	UFUNCTION(BlueprintCallable, Category = "UI|AbilitySlot")
	void SetKeybindLabel(const FText& InLabel);

	/** Set the mana cost display. 0 hides the text. */
	UFUNCTION(BlueprintCallable, Category = "UI|AbilitySlot")
	void SetManaCost(float InCost);

	/**
	 * Update the cooldown sweep overlay.
	 * @param Percent  0.0 = ready (no overlay), 1.0 = fully on cooldown.
	 * @param RemainingSeconds  Seconds left (shown as text on the overlay).
	 */
	UFUNCTION(BlueprintCallable, Category = "UI|AbilitySlot")
	void SetCooldownPercent(float Percent, float RemainingSeconds);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> AbilityIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CooldownSweep;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CooldownText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ManaCostText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> KeybindLabel;

	/** Material instance used for the circular sweep. Must have a scalar param "Percent". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|AbilitySlot")
	TObjectPtr<UMaterialInterface> CooldownSweepMaterial;

private:
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> SweepMID;
};
