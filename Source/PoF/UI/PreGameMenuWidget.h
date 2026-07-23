#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PreGameMenuWidget.generated.h"

class UTextBlock;
class UButton;
class UBorder;

/**
 * Pre-game duel menu — the UE mirror of the browser preview's staging shell.
 * C++-built tree (Initialize, the DialogueWidget pattern — NativeConstruct is too
 * late to root a WidgetTree). Two saber choices (Crimson / Azure) + Enter button;
 * the choice lands in UPoFDuelSelectionSubsystem. Shown by the FeatureLab
 * subsystem at world begin-play. Vendor pricing/wallet stay browser-side for now.
 */
UCLASS()
class POF_API UPreGameMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	/** For the config gate: tree rooted + both choice buttons exist. */
	bool IsMenuBuilt() const { return CrimsonButton != nullptr && AzureButton != nullptr; }

protected:
	void BuildLayout();

	UFUNCTION() void OnCrimson();
	UFUNCTION() void OnAzure();
	UFUNCTION() void OnQuestLords();
	UFUNCTION() void OnQuestEchoes();
	UFUNCTION() void OnEnter();

	void Select(FName Saber);
	void SelectQuestId(FName Quest, const FString& Display);

	UPROPERTY() TObjectPtr<UBorder> RootBorder;
	UPROPERTY() TObjectPtr<UTextBlock> SelectionText;
	UPROPERTY() TObjectPtr<UButton> CrimsonButton;
	UPROPERTY() TObjectPtr<UButton> AzureButton;
	UPROPERTY() TObjectPtr<UButton> QuestLordsButton;
	UPROPERTY() TObjectPtr<UButton> QuestEchoesButton;
	UPROPERTY() TObjectPtr<UTextBlock> QuestText;
	UPROPERTY() TObjectPtr<UButton> EnterButton;
};
