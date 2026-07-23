#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Dialogue/ARPGDialogueTypes.h"
#include "DialogueWidget.generated.h"

class UARPGDialogueComponent;
class UTextBlock;
class UImage;
class UVerticalBox;
class UButton;
class UBorder;

/**
 * Dialogue UI widget: displays speaker portrait, name, dialogue text,
 * and choice buttons. Binds to a UARPGDialogueComponent to drive state.
 */
UCLASS()
class POF_API UDialogueWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Bind to a dialogue component. Call when dialogue starts. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue|UI")
	void BindToDialogue(UARPGDialogueComponent* DialogueComp);

	/** Unbind and hide. Call when dialogue ends. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue|UI")
	void UnbindFromDialogue();

protected:
	virtual bool Initialize() override;
	virtual void NativeConstruct() override;

private:
	UPROPERTY()
	TWeakObjectPtr<UARPGDialogueComponent> BoundDialogue;

	// --- Programmatic UI elements ---
	UPROPERTY()
	TObjectPtr<UBorder> RootBorder;

	UPROPERTY()
	TObjectPtr<UTextBlock> SpeakerNameText;

	UPROPERTY()
	TObjectPtr<UTextBlock> DialogueBodyText;

	UPROPERTY()
	TObjectPtr<UImage> SpeakerPortraitImage;

	UPROPERTY()
	TObjectPtr<UVerticalBox> ChoiceContainer;

	UPROPERTY()
	TObjectPtr<UButton> ContinueButton;

	UPROPERTY()
	TObjectPtr<UTextBlock> ContinueButtonText;

	/** Dynamically created choice buttons. */
	UPROPERTY()
	TArray<TObjectPtr<UButton>> ChoiceButtons;

	void BuildLayout();
	void RefreshNode(int32 NodeIndex, const FARPGDialogueNode& Node);
	void ClearChoices();
	void CreateChoiceButton(int32 Index, const FText& Text);

	UFUNCTION()
	void OnDialogueNodeEntered(int32 NodeIndex, const FARPGDialogueNode& Node);

	UFUNCTION()
	void OnDialogueEnded();

	UFUNCTION()
	void OnContinueClicked();

	/** Shared handler for all choice buttons — resolves via hover check. */
	UFUNCTION()
	void OnChoiceButtonClicked();

	void OnChoiceClicked(int32 ChoiceIndex);
};
