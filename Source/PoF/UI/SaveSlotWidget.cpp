#include "SaveSlotWidget.h"
#include "Framework/ARPGSaveSubsystem.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Spacer.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetTree.h"

// =========================================================================
// Lifecycle
// =========================================================================

void USaveSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CloseButton->OnClicked.AddDynamic(this, &USaveSlotWidget::OnCloseClicked);
	ConfirmYesButton->OnClicked.AddDynamic(this, &USaveSlotWidget::OnConfirmYes);
	ConfirmNoButton->OnClicked.AddDynamic(this, &USaveSlotWidget::OnConfirmNo);

	HideConfirmation();
}

void USaveSlotWidget::NativeDestruct()
{
	if (UARPGSaveSubsystem* Sub = GetSaveSubsystem())
	{
		Sub->OnSaveCompleted.RemoveDynamic(this, &USaveSlotWidget::OnSaveCompleted);
		Sub->OnLoadCompleted.RemoveDynamic(this, &USaveSlotWidget::OnLoadCompleted);
	}

	Super::NativeDestruct();
}

// =========================================================================
// Open / Close
// =========================================================================

void USaveSlotWidget::OpenScreen(ESaveScreenMode InMode)
{
	ScreenMode = InMode;
	TitleText->SetText(ScreenMode == ESaveScreenMode::Save
		? FText::FromString(TEXT("Save Game"))
		: FText::FromString(TEXT("Load Game")));

	// Bind to save/load completion for UI refresh
	if (UARPGSaveSubsystem* Sub = GetSaveSubsystem())
	{
		Sub->OnSaveCompleted.AddUniqueDynamic(this, &USaveSlotWidget::OnSaveCompleted);
		Sub->OnLoadCompleted.AddUniqueDynamic(this, &USaveSlotWidget::OnLoadCompleted);
	}

	HideConfirmation();
	RefreshSlotList();
	SetVisibility(ESlateVisibility::Visible);
}

void USaveSlotWidget::CloseScreen()
{
	SetVisibility(ESlateVisibility::Collapsed);
	HideConfirmation();
}

// =========================================================================
// Slot list
// =========================================================================

void USaveSlotWidget::RefreshSlotList()
{
	SlotListBox->ClearChildren();
	SlotButtonToIndex.Empty();
	DeleteButtonToIndex.Empty();

	UARPGSaveSubsystem* Sub = GetSaveSubsystem();
	if (!Sub) return;

	TArray<FARPGSaveSlotInfo> AllSlots = Sub->GetAllSlotInfo();

	for (int32 i = 0; i < AllSlots.Num(); ++i)
	{
		const FARPGSaveSlotInfo& Info = AllSlots[i];

		// Row: [Slot Info Text] [Delete Button] [Action Button]
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), *FString::Printf(TEXT("SlotRow_%d"), i));

		// --- Slot info text ---
		UTextBlock* SlotText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), *FString::Printf(TEXT("SlotText_%d"), i));

		FString Label;
		if (Info.bIsAutoSave)
		{
			Label = TEXT("Auto-Save");
		}
		else
		{
			Label = FString::Printf(TEXT("Slot %d"), i + 1);
		}

		if (Info.bExists)
		{
			Label += FString::Printf(TEXT("  |  %s  Lv.%d  %s  %s"),
				*Info.CharacterName,
				Info.PlayerLevel,
				*FormatPlayTime(Info.PlayTimeSeconds),
				*Info.SaveTimestamp.ToString(TEXT("%Y-%m-%d %H:%M")));
		}
		else
		{
			Label += TEXT("  |  Empty");
		}

		SlotText->SetText(FText::FromString(Label));
		UHorizontalBoxSlot* SlotTextSlot = Row->AddChildToHorizontalBox(SlotText);
		SlotTextSlot->SetHorizontalAlignment(HAlign_Fill);
		SlotTextSlot->SetSize(ESlateSizeRule::Fill);

		// --- Spacer ---
		USpacer* Spacer = WidgetTree->ConstructWidget<USpacer>(
			USpacer::StaticClass(), *FString::Printf(TEXT("Spacer_%d"), i));
		Spacer->SetSize(FVector2D(16.f, 0.f));
		Row->AddChildToHorizontalBox(Spacer);

		// --- Delete button (only for existing slots, not auto-save in load mode) ---
		if (Info.bExists)
		{
			UButton* DeleteBtn = WidgetTree->ConstructWidget<UButton>(
				UButton::StaticClass(), *FString::Printf(TEXT("DeleteBtn_%d"), i));
			UTextBlock* DeleteLabel = WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(), *FString::Printf(TEXT("DeleteLabel_%d"), i));
			DeleteLabel->SetText(FText::FromString(TEXT("Delete")));
			DeleteBtn->AddChild(DeleteLabel);
			DeleteBtn->OnClicked.AddDynamic(this, &USaveSlotWidget::OnDeleteButtonClicked);
			DeleteButtonToIndex.Add(DeleteBtn, i);
			Row->AddChildToHorizontalBox(DeleteBtn);
		}

		// --- Action button (Save / Load) ---
		UButton* ActionBtn = WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(), *FString::Printf(TEXT("ActionBtn_%d"), i));
		UTextBlock* ActionLabel = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), *FString::Printf(TEXT("ActionLabel_%d"), i));

		if (ScreenMode == ESaveScreenMode::Save)
		{
			// Can save to any manual slot; auto-save is read-only
			if (Info.bIsAutoSave)
			{
				ActionLabel->SetText(FText::FromString(TEXT("Auto")));
				ActionBtn->SetIsEnabled(false);
			}
			else
			{
				ActionLabel->SetText(FText::FromString(TEXT("Save")));
			}
		}
		else // Load
		{
			ActionLabel->SetText(FText::FromString(TEXT("Load")));
			ActionBtn->SetIsEnabled(Info.bExists);
		}

		ActionBtn->AddChild(ActionLabel);
		ActionBtn->OnClicked.AddDynamic(this, &USaveSlotWidget::OnSlotButtonClicked);
		SlotButtonToIndex.Add(ActionBtn, i);
		Row->AddChildToHorizontalBox(ActionBtn);

		SlotListBox->AddChildToVerticalBox(Row);
	}
}

FString USaveSlotWidget::FormatPlayTime(float Seconds)
{
	const int32 Hours = FMath::FloorToInt32(Seconds / 3600.f);
	const int32 Minutes = FMath::FloorToInt32(FMath::Fmod(Seconds, 3600.f) / 60.f);
	return FString::Printf(TEXT("%dh %02dm"), Hours, Minutes);
}

// =========================================================================
// Button handlers
// =========================================================================

void USaveSlotWidget::OnSlotButtonClicked()
{
	UARPGSaveSubsystem* Sub = GetSaveSubsystem();
	if (!Sub) return;

	TArray<FARPGSaveSlotInfo> AllSlots = Sub->GetAllSlotInfo();

	// Find which button was clicked via IsHovered
	for (auto& Pair : SlotButtonToIndex)
	{
		if (Pair.Key && Pair.Key->IsHovered())
		{
			const int32 Idx = Pair.Value;
			if (Idx >= 0 && Idx < AllSlots.Num())
			{
				HandleSlotAction(Idx, AllSlots[Idx]);
			}
			return;
		}
	}
}

void USaveSlotWidget::OnDeleteButtonClicked()
{
	UARPGSaveSubsystem* Sub = GetSaveSubsystem();
	if (!Sub) return;

	TArray<FARPGSaveSlotInfo> AllSlots = Sub->GetAllSlotInfo();

	for (auto& Pair : DeleteButtonToIndex)
	{
		if (Pair.Key && Pair.Key->IsHovered())
		{
			const int32 Idx = Pair.Value;
			if (Idx >= 0 && Idx < AllSlots.Num())
			{
				HandleSlotDelete(Idx, AllSlots[Idx]);
			}
			return;
		}
	}
}

void USaveSlotWidget::HandleSlotAction(int32 SlotIndex, const FARPGSaveSlotInfo& Info)
{
	UARPGSaveSubsystem* Sub = GetSaveSubsystem();
	if (!Sub) return;

	if (ScreenMode == ESaveScreenMode::Save)
	{
		if (Info.bExists)
		{
			// Occupied slot — confirm overwrite
			ShowConfirmation(
				FText::FromString(FString::Printf(TEXT("Overwrite save in Slot %d?"), SlotIndex + 1)),
				SlotIndex, false);
		}
		else
		{
			// Empty slot — save directly
			Sub->SaveToManualSlot(SlotIndex);
		}
	}
	else // Load
	{
		if (Info.bExists)
		{
			Sub->LoadFromManualSlot(SlotIndex);
		}
	}
}

void USaveSlotWidget::HandleSlotDelete(int32 SlotIndex, const FARPGSaveSlotInfo& Info)
{
	if (!Info.bExists) return;

	FString SlotLabel = Info.bIsAutoSave ? TEXT("Auto-Save") : FString::Printf(TEXT("Slot %d"), SlotIndex + 1);
	ShowConfirmation(
		FText::FromString(FString::Printf(TEXT("Delete %s? This cannot be undone."), *SlotLabel)),
		SlotIndex, true);
}

// =========================================================================
// Confirmation
// =========================================================================

void USaveSlotWidget::ShowConfirmation(const FText& Message, int32 SlotIndex, bool bIsDelete)
{
	PendingSlotIndex = SlotIndex;
	bPendingDelete = bIsDelete;
	ConfirmText->SetText(Message);
	ConfirmOverlay->SetVisibility(ESlateVisibility::Visible);
}

void USaveSlotWidget::HideConfirmation()
{
	PendingSlotIndex = -1;
	ConfirmOverlay->SetVisibility(ESlateVisibility::Collapsed);
}

void USaveSlotWidget::OnConfirmYes()
{
	if (PendingSlotIndex < 0) return;

	UARPGSaveSubsystem* Sub = GetSaveSubsystem();
	if (!Sub)
	{
		HideConfirmation();
		return;
	}

	TArray<FARPGSaveSlotInfo> AllSlots = Sub->GetAllSlotInfo();
	const bool bIsAutoSlot = (PendingSlotIndex < AllSlots.Num()) && AllSlots[PendingSlotIndex].bIsAutoSave;

	if (bPendingDelete)
	{
		FString SlotName = bIsAutoSlot
			? Sub->GetAutoSaveSlotName()
			: UARPGSaveSubsystem::GetManualSlotName(PendingSlotIndex);
		Sub->DeleteSlot(SlotName);
		RefreshSlotList();
	}
	else
	{
		// Overwrite save
		Sub->SaveToManualSlot(PendingSlotIndex);
	}

	HideConfirmation();
}

void USaveSlotWidget::OnConfirmNo()
{
	HideConfirmation();
}

void USaveSlotWidget::OnCloseClicked()
{
	CloseScreen();
}

// =========================================================================
// Save/Load completion callbacks — refresh UI
// =========================================================================

void USaveSlotWidget::OnSaveCompleted(const FString& SlotName, bool bSuccess)
{
	RefreshSlotList();
}

void USaveSlotWidget::OnLoadCompleted(const FString& SlotName, bool bSuccess)
{
	if (bSuccess)
	{
		CloseScreen();
	}
}

// =========================================================================
// Helpers
// =========================================================================

UARPGSaveSubsystem* USaveSlotWidget::GetSaveSubsystem() const
{
	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	return GI ? GI->GetSubsystem<UARPGSaveSubsystem>() : nullptr;
}
