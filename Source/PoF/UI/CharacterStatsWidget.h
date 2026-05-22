#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayEffectTypes.h"
#include "CharacterStatsWidget.generated.h"

class UAbilitySystemComponent;
class UVerticalBox;
class UTextBlock;

/**
 * Character stats panel showing all GAS attributes with base value + bonus.
 * Each stat row displays: "StatName: BaseValue (+Bonus)" where bonus is
 * colored differently (green for positive, red for negative).
 * Includes a derived damage reduction % from Armor.
 */
UCLASS()
class POF_API UCharacterStatsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Bind to the player's ASC to listen for attribute changes. */
	UFUNCTION(BlueprintCallable, Category = "UI|Stats")
	void BindToAbilitySystem(UAbilitySystemComponent* ASC);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// --- Widget bindings ---

	/** Container for vital stat rows (Health, Mana). */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VitalsContainer;

	/** Container for primary stat rows (Str, Dex, Int). */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> PrimaryStatsContainer;

	/** Container for combat stat rows (Armor, AttackPower, Crit, etc.). */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> CombatStatsContainer;

	/** Displays the damage reduction percentage derived from Armor. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DamageReductionText;

	// --- Colors ---

	/** Color for base stat values. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Stats|Colors")
	FLinearColor BaseValueColor = FLinearColor(0.9f, 0.9f, 0.9f, 1.f);

	/** Color for positive bonus values. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Stats|Colors")
	FLinearColor PositiveBonusColor = FLinearColor(0.2f, 0.9f, 0.2f, 1.f);

	/** Color for negative bonus values. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Stats|Colors")
	FLinearColor NegativeBonusColor = FLinearColor(0.9f, 0.2f, 0.2f, 1.f);

	/** Color for stat labels. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Stats|Colors")
	FLinearColor LabelColor = FLinearColor(0.7f, 0.7f, 0.7f, 1.f);

	/** Color for the damage reduction text. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Stats|Colors")
	FLinearColor DamageReductionColor = FLinearColor(0.3f, 0.6f, 1.f, 1.f);

	/** Font size for stat rows. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Stats")
	int32 StatFontSize = 14;

	/** Armor constant used in damage reduction formula: DR = Armor / (Armor + ArmorConstant). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Stats")
	float ArmorConstant = 100.f;

private:
	// Cached ASC
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;

	// Stat row text blocks for dynamic updates
	struct FStatRow
	{
		FGameplayAttribute Attribute;
		FName StatName;
		TObjectPtr<UTextBlock> LabelText;
		TObjectPtr<UTextBlock> ValueText;
		TObjectPtr<UTextBlock> BonusText;
		bool bIsPercentage = false;
	};

	TArray<FStatRow> StatRows;

	void CreateStatRow(UVerticalBox* Container, const FGameplayAttribute& Attribute, const FName& DisplayName, bool bIsPercentage = false);
	void RefreshStatRow(FStatRow& Row);
	void RefreshAllStats();
	void UpdateDamageReduction();
	void UnbindFromAbilitySystem();

	// GAS callback
	void OnAttributeChanged(const FOnAttributeChangeData& Data);
};
