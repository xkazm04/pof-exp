#include "UI/QuestLogWidget.h"
#include "Quest/ARPGQuestSubsystem.h"
#include "Quest/ARPGQuestDefinition.h"
#include "Quest/ARPGQuestTypes.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Styling/CoreStyle.h"
#include "Blueprint/WidgetTree.h"

void UQuestLogWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildLayout();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UQuestLogWidget::BuildLayout()
{
	RootBorder = NewObject<UBorder>(this);
	RootBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.05f, 0.92f));
	RootBorder->SetPadding(FMargin(20.f));

	UVerticalBox* MainVBox = NewObject<UVerticalBox>(this);
	RootBorder->SetContent(MainVBox);

	// Title
	UTextBlock* Title = NewObject<UTextBlock>(this);
	Title->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 24));
	Title->SetText(FText::FromString(TEXT("Quest Log")));
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.85f, 0.3f)));
	auto* TitleSlot = MainVBox->AddChildToVerticalBox(Title);
	TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	TitleSlot->SetHorizontalAlignment(HAlign_Center);

	// Tab row
	UHorizontalBox* TabRow = NewObject<UHorizontalBox>(this);
	auto* TabRowSlot = MainVBox->AddChildToVerticalBox(TabRow);
	TabRowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));

	ActiveTabButton = NewObject<UButton>(this);
	ActiveTabButton->OnClicked.AddDynamic(this, &UQuestLogWidget::OnActiveTabClicked);
	ActiveTabText = NewObject<UTextBlock>(this);
	ActiveTabText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 16));
	ActiveTabText->SetText(FText::FromString(TEXT("Active")));
	ActiveTabButton->AddChild(ActiveTabText);
	auto* ActiveSlot = TabRow->AddChildToHorizontalBox(ActiveTabButton);
	ActiveSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));

	CompletedTabButton = NewObject<UButton>(this);
	CompletedTabButton->OnClicked.AddDynamic(this, &UQuestLogWidget::OnCompletedTabClicked);
	CompletedTabText = NewObject<UTextBlock>(this);
	CompletedTabText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 16));
	CompletedTabText->SetText(FText::FromString(TEXT("Completed")));
	CompletedTabButton->AddChild(CompletedTabText);
	auto* CompSlot = TabRow->AddChildToHorizontalBox(CompletedTabButton);
	CompSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));

	FailedTabButton = NewObject<UButton>(this);
	FailedTabButton->OnClicked.AddDynamic(this, &UQuestLogWidget::OnFailedTabClicked);
	FailedTabText = NewObject<UTextBlock>(this);
	FailedTabText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 16));
	FailedTabText->SetText(FText::FromString(TEXT("Failed")));
	FailedTabButton->AddChild(FailedTabText);
	auto* FailSlot = TabRow->AddChildToHorizontalBox(FailedTabButton);
	FailSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));

	// Content area: quest list (left) + detail panel (right)
	UHorizontalBox* ContentRow = NewObject<UHorizontalBox>(this);
	auto* ContentSlot = MainVBox->AddChildToVerticalBox(ContentRow);
	ContentSlot->SetSize(ESlateSizeRule::Fill);

	// Left: scrollable quest list
	QuestListScroll = NewObject<UScrollBox>(this);
	auto* ScrollSlot = ContentRow->AddChildToHorizontalBox(QuestListScroll);
	ScrollSlot->SetSize(ESlateSizeRule::Fill);
	ScrollSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));

	QuestListContainer = NewObject<UVerticalBox>(this);
	QuestListScroll->AddChild(QuestListContainer);

	// Right: detail panel
	UBorder* DetailBorder = NewObject<UBorder>(this);
	DetailBorder->SetBrushColor(FLinearColor(0.05f, 0.05f, 0.08f, 0.8f));
	DetailBorder->SetPadding(FMargin(12.f));
	auto* DetailSlot = ContentRow->AddChildToHorizontalBox(DetailBorder);
	DetailSlot->SetSize(ESlateSizeRule::Fill);

	DetailPanel = NewObject<UVerticalBox>(this);
	DetailBorder->SetContent(DetailPanel);

	DetailQuestName = NewObject<UTextBlock>(this);
	DetailQuestName->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 20));
	DetailQuestName->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	auto* NameSlot = DetailPanel->AddChildToVerticalBox(DetailQuestName);
	NameSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));

	DetailQuestDescription = NewObject<UTextBlock>(this);
	DetailQuestDescription->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 14));
	DetailQuestDescription->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f)));
	DetailQuestDescription->SetAutoWrapText(true);
	auto* DescSlot = DetailPanel->AddChildToVerticalBox(DetailQuestDescription);
	DescSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

	// Objectives header
	UTextBlock* ObjHeader = NewObject<UTextBlock>(this);
	ObjHeader->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 16));
	ObjHeader->SetText(FText::FromString(TEXT("Objectives")));
	ObjHeader->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.85f, 0.3f)));
	auto* ObjHSlot = DetailPanel->AddChildToVerticalBox(ObjHeader);
	ObjHSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));

	DetailObjectivesContainer = NewObject<UVerticalBox>(this);
	auto* ObjSlot = DetailPanel->AddChildToVerticalBox(DetailObjectivesContainer);
	ObjSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

	// Rewards header
	UTextBlock* RewHeader = NewObject<UTextBlock>(this);
	RewHeader->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 16));
	RewHeader->SetText(FText::FromString(TEXT("Rewards")));
	RewHeader->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.85f, 0.3f)));
	auto* RewHSlot = DetailPanel->AddChildToVerticalBox(RewHeader);
	RewHSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));

	DetailRewardsContainer = NewObject<UVerticalBox>(this);
	auto* RewSlot = DetailPanel->AddChildToVerticalBox(DetailRewardsContainer);
	RewSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

	// Button row
	UHorizontalBox* ButtonRow = NewObject<UHorizontalBox>(this);
	auto* ButtonRowSlot = DetailPanel->AddChildToVerticalBox(ButtonRow);
	ButtonRowSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));

	// Pin button
	PinButton = NewObject<UButton>(this);
	PinButton->OnClicked.AddDynamic(this, &UQuestLogWidget::OnPinClicked);
	PinButtonText = NewObject<UTextBlock>(this);
	PinButtonText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 14));
	PinButtonText->SetText(FText::FromString(TEXT("Pin to HUD")));
	PinButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.85f, 0.3f)));
	PinButton->AddChild(PinButtonText);
	auto* PinSlot = ButtonRow->AddChildToHorizontalBox(PinButton);
	PinSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
	PinButton->SetVisibility(ESlateVisibility::Collapsed);

	// Abandon button
	AbandonButton = NewObject<UButton>(this);
	AbandonButton->OnClicked.AddDynamic(this, &UQuestLogWidget::OnAbandonClicked);
	AbandonButtonText = NewObject<UTextBlock>(this);
	AbandonButtonText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 14));
	AbandonButtonText->SetText(FText::FromString(TEXT("Abandon Quest")));
	AbandonButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.3f, 0.3f)));
	AbandonButton->AddChild(AbandonButtonText);
	auto* AbandonSlot = ButtonRow->AddChildToHorizontalBox(AbandonButton);
	AbandonSlot->SetPadding(FMargin(0.f));
	AbandonButton->SetVisibility(ESlateVisibility::Collapsed);

	if (WidgetTree)
	{
		WidgetTree->RootWidget = RootBorder;
	}

	UpdateTabStyles();
}

void UQuestLogWidget::BindToQuestSubsystem(UARPGQuestSubsystem* QuestSubsystem)
{
	if (!QuestSubsystem) return;
	BoundSubsystem = QuestSubsystem;

	QuestSubsystem->OnQuestAccepted.AddDynamic(this, &UQuestLogWidget::OnQuestAccepted);
	QuestSubsystem->OnQuestCompleted.AddDynamic(this, &UQuestLogWidget::OnQuestCompleted);
	QuestSubsystem->OnQuestFailed.AddDynamic(this, &UQuestLogWidget::OnQuestFailed);
	QuestSubsystem->OnObjectiveUpdated.AddDynamic(this, &UQuestLogWidget::OnObjectiveUpdated);
}

void UQuestLogWidget::RefreshDisplay()
{
	PopulateQuestList();
	PopulateDetail();
}

void UQuestLogWidget::ShowActiveTab()
{
	ActiveTabIndex = 0;
	SelectedQuestID = NAME_None;
	UpdateTabStyles();
	RefreshDisplay();
}

void UQuestLogWidget::ShowCompletedTab()
{
	ActiveTabIndex = 1;
	SelectedQuestID = NAME_None;
	UpdateTabStyles();
	RefreshDisplay();
}

void UQuestLogWidget::ShowFailedTab()
{
	ActiveTabIndex = 2;
	SelectedQuestID = NAME_None;
	UpdateTabStyles();
	RefreshDisplay();
}

void UQuestLogWidget::SelectQuest(FName QuestID)
{
	SelectedQuestID = QuestID;
	PopulateDetail();
}

void UQuestLogWidget::AbandonSelectedQuest()
{
	if (SelectedQuestID.IsNone()) return;

	UARPGQuestSubsystem* QS = BoundSubsystem.Get();
	if (!QS) return;

	if (QS->AbandonQuest(SelectedQuestID))
	{
		SelectedQuestID = NAME_None;
		RefreshDisplay();
	}
}

void UQuestLogWidget::PinSelectedQuest()
{
	if (SelectedQuestID.IsNone()) return;

	UARPGQuestSubsystem* QS = BoundSubsystem.Get();
	if (!QS) return;

	if (QS->GetPinnedQuestID() == SelectedQuestID)
	{
		QS->UnpinQuest();
	}
	else
	{
		QS->PinQuest(SelectedQuestID);
	}

	PopulateDetail(); // Refresh pin button text
}

void UQuestLogWidget::UpdateTabStyles()
{
	const FLinearColor ActiveColor(1.f, 0.85f, 0.3f);
	const FLinearColor InactiveColor(0.5f, 0.5f, 0.5f);

	if (ActiveTabText)
	{
		ActiveTabText->SetColorAndOpacity(FSlateColor(ActiveTabIndex == 0 ? ActiveColor : InactiveColor));
	}
	if (CompletedTabText)
	{
		CompletedTabText->SetColorAndOpacity(FSlateColor(ActiveTabIndex == 1 ? ActiveColor : InactiveColor));
	}
	if (FailedTabText)
	{
		FailedTabText->SetColorAndOpacity(FSlateColor(ActiveTabIndex == 2 ? FLinearColor(0.9f, 0.3f, 0.3f) : InactiveColor));
	}
}

void UQuestLogWidget::PopulateQuestList()
{
	if (!QuestListContainer) return;
	QuestListContainer->ClearChildren();
	QuestListButtons.Empty();
	QuestListIDs.Empty();

	UARPGQuestSubsystem* QS = BoundSubsystem.Get();
	if (!QS) return;

	TArray<FName> IDs;
	FString EmptyMsg;
	switch (ActiveTabIndex)
	{
	case 0:  IDs = QS->GetActiveQuestIDs();    EmptyMsg = TEXT("No active quests"); break;
	case 1:  IDs = QS->GetCompletedQuestIDs(); EmptyMsg = TEXT("No completed quests"); break;
	case 2:  IDs = QS->GetFailedQuestIDs();    EmptyMsg = TEXT("No failed quests"); break;
	default: break;
	}

	if (IDs.Num() == 0)
	{
		UTextBlock* EmptyText = NewObject<UTextBlock>(this);
		EmptyText->SetFont(FCoreStyle::GetDefaultFontStyle("Italic", 13));
		EmptyText->SetText(FText::FromString(EmptyMsg));
		EmptyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)));
		QuestListContainer->AddChildToVerticalBox(EmptyText);
		return;
	}

	// Sort by category (Main first, then Side, etc.)
	IDs.Sort([QS](const FName& A, const FName& B)
	{
		UARPGQuestDefinition* DefA = QS->FindQuestDefinition(A);
		UARPGQuestDefinition* DefB = QS->FindQuestDefinition(B);
		int32 CatA = DefA ? static_cast<int32>(DefA->Category) : 999;
		int32 CatB = DefB ? static_cast<int32>(DefB->Category) : 999;
		return CatA < CatB;
	});

	int32 LastCategory = -1;
	for (FName QuestID : IDs)
	{
		UARPGQuestDefinition* Def = QS->FindQuestDefinition(QuestID);

		// Insert category header when category changes
		if (Def)
		{
			int32 Cat = static_cast<int32>(Def->Category);
			if (Cat != LastCategory)
			{
				LastCategory = Cat;
				FString CatLabel;
				switch (Def->Category)
				{
				case EARPGQuestCategory::Main:        CatLabel = TEXT("-- Main Quests --"); break;
				case EARPGQuestCategory::Side:        CatLabel = TEXT("-- Side Quests --"); break;
				case EARPGQuestCategory::Bounty:      CatLabel = TEXT("-- Bounties --"); break;
				case EARPGQuestCategory::Exploration:  CatLabel = TEXT("-- Exploration --"); break;
				case EARPGQuestCategory::Crafting:     CatLabel = TEXT("-- Crafting --"); break;
				}

				UTextBlock* CatText = NewObject<UTextBlock>(this);
				CatText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 12));
				CatText->SetText(FText::FromString(CatLabel));
				CatText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)));
				auto* CatSlot = QuestListContainer->AddChildToVerticalBox(CatText);
				CatSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 2.f));
			}
		}

		FText QName = Def ? Def->QuestName : FText::FromName(QuestID);
		CreateQuestListEntry(QuestID, QName, ActiveTabIndex == 0);
	}

	// Auto-select first quest if none selected
	if (SelectedQuestID.IsNone() && IDs.Num() > 0)
	{
		SelectedQuestID = IDs[0];
		PopulateDetail();
	}
}

void UQuestLogWidget::OnQuestListButtonClicked()
{
	for (int32 i = 0; i < QuestListButtons.Num(); ++i)
	{
		if (QuestListButtons[i] && QuestListButtons[i]->IsHovered())
		{
			if (QuestListIDs.IsValidIndex(i))
			{
				SelectedQuestID = QuestListIDs[i];
				PopulateQuestList(); // refresh highlight
				PopulateDetail();
			}
			return;
		}
	}
}

void UQuestLogWidget::CreateQuestListEntry(FName QuestID, const FText& QuestName, bool bIsActive)
{
	UButton* Btn = NewObject<UButton>(this);

	UTextBlock* BtnText = NewObject<UTextBlock>(this);
	BtnText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 14));
	BtnText->SetText(QuestName);

	bool bSelected = (QuestID == SelectedQuestID);
	BtnText->SetColorAndOpacity(FSlateColor(
		bSelected ? FLinearColor(1.f, 0.85f, 0.3f) : FLinearColor(0.85f, 0.85f, 0.85f)));
	Btn->AddChild(BtnText);

	Btn->OnClicked.AddDynamic(this, &UQuestLogWidget::OnQuestListButtonClicked);

	QuestListButtons.Add(Btn);
	QuestListIDs.Add(QuestID);

	auto* BtnSlot = QuestListContainer->AddChildToVerticalBox(Btn);
	BtnSlot->SetPadding(FMargin(0.f, 1.f));
}

void UQuestLogWidget::PopulateDetail()
{
	if (!DetailPanel) return;

	UARPGQuestSubsystem* QS = BoundSubsystem.Get();

	if (SelectedQuestID.IsNone() || !QS)
	{
		if (DetailQuestName) DetailQuestName->SetText(FText::GetEmpty());
		if (DetailQuestDescription) DetailQuestDescription->SetText(FText::FromString(TEXT("Select a quest from the list.")));
		if (DetailObjectivesContainer) DetailObjectivesContainer->ClearChildren();
		if (DetailRewardsContainer) DetailRewardsContainer->ClearChildren();
		if (AbandonButton) AbandonButton->SetVisibility(ESlateVisibility::Collapsed);
		if (PinButton) PinButton->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	UARPGQuestDefinition* Def = QS->FindQuestDefinition(SelectedQuestID);
	const FARPGQuestState* State = QS->GetQuestRuntimeState(SelectedQuestID);

	if (DetailQuestName)
	{
		DetailQuestName->SetText(Def ? Def->QuestName : FText::FromName(SelectedQuestID));
	}

	if (DetailQuestDescription)
	{
		FString DescStr;

		// Category + level info line
		if (Def)
		{
			FString CatStr;
			switch (Def->Category)
			{
			case EARPGQuestCategory::Main:        CatStr = TEXT("Main Quest"); break;
			case EARPGQuestCategory::Side:        CatStr = TEXT("Side Quest"); break;
			case EARPGQuestCategory::Bounty:      CatStr = TEXT("Bounty"); break;
			case EARPGQuestCategory::Exploration:  CatStr = TEXT("Exploration"); break;
			case EARPGQuestCategory::Crafting:     CatStr = TEXT("Crafting"); break;
			}

			DescStr = FString::Printf(TEXT("[%s]"), *CatStr);
			if (Def->RequiredLevel > 0)
			{
				DescStr += FString::Printf(TEXT("  Lv. %d+"), Def->RequiredLevel);
			}
			DescStr += TEXT("\n\n");
			DescStr += Def->Description.ToString();
		}

		DetailQuestDescription->SetText(FText::FromString(DescStr));
	}

	// Objectives
	if (DetailObjectivesContainer)
	{
		DetailObjectivesContainer->ClearChildren();

		if (Def && State)
		{
			for (int32 i = 0; i < Def->Objectives.Num(); ++i)
			{
				const FARPGQuestObjective& Obj = Def->Objectives[i];
				bool bComplete = State->ObjectiveStates.IsValidIndex(i) && State->ObjectiveStates[i].bComplete;
				int32 Current = State->ObjectiveStates.IsValidIndex(i) ? State->ObjectiveStates[i].CurrentCount : 0;

				FString ObjStr;
				if (Obj.bOptional)
				{
					ObjStr += TEXT("(Optional) ");
				}

				if (Obj.RequiredCount > 1)
				{
					ObjStr += FString::Printf(TEXT("%s %s (%d/%d)"),
						bComplete ? TEXT("[X]") : TEXT("[ ]"),
						*Obj.Description.ToString(),
						Current, Obj.RequiredCount);
				}
				else
				{
					ObjStr += FString::Printf(TEXT("%s %s"),
						bComplete ? TEXT("[X]") : TEXT("[ ]"),
						*Obj.Description.ToString());
				}

				UTextBlock* ObjText = NewObject<UTextBlock>(this);
				ObjText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 13));
				ObjText->SetText(FText::FromString(ObjStr));
				ObjText->SetColorAndOpacity(FSlateColor(
					bComplete ? FLinearColor(0.4f, 0.8f, 0.4f) : FLinearColor(0.85f, 0.85f, 0.85f)));
				ObjText->SetAutoWrapText(true);
				DetailObjectivesContainer->AddChildToVerticalBox(ObjText);
			}
		}
	}

	// Rewards
	if (DetailRewardsContainer)
	{
		DetailRewardsContainer->ClearChildren();

		if (Def)
		{
			for (const FARPGQuestReward& Reward : Def->Rewards)
			{
				FString RewardStr;
				if (Reward.XP > 0)
				{
					RewardStr += FString::Printf(TEXT("+%d XP  "), Reward.XP);
				}
				if (!Reward.ItemID.IsNone())
				{
					RewardStr += FString::Printf(TEXT("%s x%d  "), *Reward.ItemID.ToString(), Reward.ItemQuantity);
				}

				if (!RewardStr.IsEmpty())
				{
					UTextBlock* RewText = NewObject<UTextBlock>(this);
					RewText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 13));
					RewText->SetText(FText::FromString(RewardStr));
					RewText->SetColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.8f, 1.f)));
					DetailRewardsContainer->AddChildToVerticalBox(RewText);
				}
			}

			if (Def->Rewards.Num() == 0)
			{
				UTextBlock* NoRew = NewObject<UTextBlock>(this);
				NoRew->SetFont(FCoreStyle::GetDefaultFontStyle("Italic", 12));
				NoRew->SetText(FText::FromString(TEXT("No rewards listed")));
				NoRew->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)));
				DetailRewardsContainer->AddChildToVerticalBox(NoRew);
			}
		}
	}

	// Pin button: only for active quests
	if (PinButton && PinButtonText)
	{
		bool bIsActive = State && State->State == EARPGQuestState::Active;
		PinButton->SetVisibility(bIsActive ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

		if (bIsActive)
		{
			bool bIsPinned = (QS->GetPinnedQuestID() == SelectedQuestID);
			PinButtonText->SetText(FText::FromString(bIsPinned ? TEXT("Unpin") : TEXT("Pin to HUD")));
		}
	}

	// Abandon button: only for active, abandonable quests
	if (AbandonButton)
	{
		bool bCanAbandon = false;
		if (State && State->State == EARPGQuestState::Active && Def && Def->bAbandonable)
		{
			bCanAbandon = true;
		}
		AbandonButton->SetVisibility(bCanAbandon ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UQuestLogWidget::OnActiveTabClicked()
{
	ShowActiveTab();
}

void UQuestLogWidget::OnCompletedTabClicked()
{
	ShowCompletedTab();
}

void UQuestLogWidget::OnFailedTabClicked()
{
	ShowFailedTab();
}

void UQuestLogWidget::OnPinClicked()
{
	PinSelectedQuest();
}

void UQuestLogWidget::OnAbandonClicked()
{
	AbandonSelectedQuest();
}

void UQuestLogWidget::OnQuestAccepted(FName QuestID)
{
	if (GetVisibility() == ESlateVisibility::Visible)
	{
		RefreshDisplay();
	}
}

void UQuestLogWidget::OnQuestCompleted(FName QuestID)
{
	if (GetVisibility() == ESlateVisibility::Visible)
	{
		RefreshDisplay();
	}
}

void UQuestLogWidget::OnQuestFailed(FName QuestID)
{
	if (GetVisibility() == ESlateVisibility::Visible)
	{
		RefreshDisplay();
	}
}

void UQuestLogWidget::OnObjectiveUpdated(FName QuestID, int32 ObjectiveIndex, int32 NewCount)
{
	if (GetVisibility() == ESlateVisibility::Visible && QuestID == SelectedQuestID)
	{
		PopulateDetail();
	}
}
