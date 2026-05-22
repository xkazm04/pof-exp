#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AbilityUnlockWidget.generated.h"

class AARPGPlayerCharacter;
class UARPGAbilityUnlockComponent;
class UButton;
class UScrollBox;
class UTextBlock;
class UVerticalBox;

/**
 * Ability unlock/upgrade screen.
 *
 * Shows all learnable abilities from the data table.
 * Each row displays:
 *   - Ability name, description, required level
 *   - Current rank / max rank
 *   - Skill point cost
 *   - [Learn] or [Upgrade] button
 *
 * Also shows current skill points and a header.
 *
 * Call BindToPlayer() after construction to hook up.
 */
UCLASS()
class POF_API UAbilityUnlockWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Bind this widget to the player to read unlock component data. */
	UFUNCTION(BlueprintCallable, Category = "UI|Skills")
	void BindToPlayer(AARPGPlayerCharacter* Player);

	/** Refresh the entire list (e.g., after leveling up). */
	UFUNCTION(BlueprintCallable, Category = "UI|Skills")
	void RefreshDisplay();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// --- BindWidget slots ---

	/** Header text showing "Skill Points: X" */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SkillPointsText;

	/** Scrollable container for ability rows. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> AbilityListContainer;

	// --- Colors ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Skills|Colors")
	FLinearColor NameColor = FLinearColor(1.f, 0.85f, 0.3f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Skills|Colors")
	FLinearColor DescColor = FLinearColor(0.7f, 0.7f, 0.7f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Skills|Colors")
	FLinearColor InfoColor = FLinearColor(0.9f, 0.9f, 0.9f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Skills|Colors")
	FLinearColor LearnedColor = FLinearColor(0.2f, 0.9f, 0.2f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Skills|Colors")
	FLinearColor LockedColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Skills")
	int32 FontSize = 14;

private:
	TWeakObjectPtr<AARPGPlayerCharacter> BoundPlayer;
	TWeakObjectPtr<UARPGAbilityUnlockComponent> BoundUnlockComp;

	struct FAbilityUIRow
	{
		FName RowName;
		TObjectPtr<UTextBlock> NameText;
		TObjectPtr<UTextBlock> DescText;
		TObjectPtr<UTextBlock> RankText;
		TObjectPtr<UTextBlock> CostText;
		TObjectPtr<UTextBlock> ReqText;
		TObjectPtr<UButton> ActionButton;
		TObjectPtr<UTextBlock> ButtonLabel;
	};

	TArray<FAbilityUIRow> UIRows;

	/** Map from button pointer to row index for shared click handler. */
	TMap<UButton*, int32> ButtonToRowIndex;

	void BuildList();
	void RefreshRow(int32 Index);
	void UpdateSkillPointsText();

	void OnActionButtonClicked(int32 RowIndex);

	/** Shared click handler — resolves which button via hover/focus state. */
	UFUNCTION() void OnAnyButtonClicked();

	// Delegate callbacks
	UFUNCTION() void OnSkillPointsChanged(int32 NewPoints);
	UFUNCTION() void OnAbilityLearned(FName RowName, int32 Rank);
	UFUNCTION() void OnAbilityUpgraded(FName RowName, int32 NewRank);

	void UnbindAll();
};
