#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayEffectTypes.h"
#include "AttributeAllocationWidget.generated.h"

class AARPGPlayerCharacter;
class UAbilitySystemComponent;
class UButton;
class UTextBlock;
class UVerticalBox;

/**
 * Attribute point allocation screen.
 *
 * Shows unspent points and three allocation rows (Strength, Dexterity, Intelligence).
 * Each row displays:
 *   - Stat name and current value
 *   - Points allocated so far
 *   - What the next point will add (preview)
 *   - A [+] button to allocate
 *
 * Requires BindWidget slots in the UMG Blueprint:
 *   - UnspentPointsText (UTextBlock)
 *   - AllocationContainer (UVerticalBox)
 *
 * Call BindToPlayer() after construction to hook up.
 */
UCLASS()
class POF_API UAttributeAllocationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Bind this widget to the player character to read/write attribute points. */
	UFUNCTION(BlueprintCallable, Category = "UI|Attributes")
	void BindToPlayer(AARPGPlayerCharacter* Player);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// --- BindWidget slots ---

	/** Text showing "Unspent Points: X" */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> UnspentPointsText;

	/** Container for the three attribute rows. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> AllocationContainer;

	// --- Colors ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Attributes|Colors")
	FLinearColor LabelColor = FLinearColor(0.9f, 0.9f, 0.9f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Attributes|Colors")
	FLinearColor ValueColor = FLinearColor(1.f, 1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Attributes|Colors")
	FLinearColor PreviewColor = FLinearColor(0.2f, 0.9f, 0.2f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Attributes|Colors")
	FLinearColor DisabledColor = FLinearColor(0.4f, 0.4f, 0.4f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Attributes|Colors")
	FLinearColor HeaderColor = FLinearColor(1.f, 0.85f, 0.3f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Attributes")
	int32 FontSize = 16;

private:
	TWeakObjectPtr<AARPGPlayerCharacter> BoundPlayer;
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;

	// Per-row UI references
	struct FAttributeRow
	{
		FName StatName;
		TObjectPtr<UTextBlock> NameText;
		TObjectPtr<UTextBlock> CurrentValueText;
		TObjectPtr<UTextBlock> AllocatedText;
		TObjectPtr<UTextBlock> PreviewText;
		TObjectPtr<UButton> AllocateButton;
		TObjectPtr<UTextBlock> ButtonLabel;
		FGameplayAttribute Attribute;        // primary stat attribute
		FGameplayAttribute DerivedAttribute; // derived combat attribute
	};

	TArray<FAttributeRow> Rows;

	void BuildRows();
	void CreateRow(
		const FName& StatName,
		const FText& DisplayName,
		const FGameplayAttribute& PrimaryAttribute,
		const FGameplayAttribute& DerivedAttribute,
		int32 RowIndex);
	void RefreshAll();
	void RefreshRow(int32 RowIndex);
	void UpdateUnspentText();

	// Button callbacks
	UFUNCTION() void OnAllocateStrength();
	UFUNCTION() void OnAllocateDexterity();
	UFUNCTION() void OnAllocateIntelligence();

	// GAS callbacks
	void OnAttributeChanged(const FOnAttributeChangeData& Data);

	// Player delegate callback
	UFUNCTION() void OnAttributePointsChanged(int32 NewPoints);

	void UnbindAll();
};
