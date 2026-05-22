#include "UI/QuestTrackerWidget.h"
#include "Quest/ARPGQuestSubsystem.h"
#include "Quest/ARPGQuestDefinition.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"
#include "Styling/CoreStyle.h"
#include "Blueprint/WidgetTree.h"

void UQuestTrackerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildLayout();
}

void UQuestTrackerWidget::BuildLayout()
{
	UBorder* RootBorder = NewObject<UBorder>(this);
	RootBorder->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.4f));
	RootBorder->SetPadding(FMargin(10.f));

	UVerticalBox* MainVBox = NewObject<UVerticalBox>(this);
	RootBorder->SetContent(MainVBox);

	// Header
	HeaderText = NewObject<UTextBlock>(this);
	HeaderText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 16));
	HeaderText->SetText(FText::FromString(TEXT("Active Quests")));
	HeaderText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.85f, 0.3f)));
	auto* HeaderSlot = MainVBox->AddChildToVerticalBox(HeaderText);
	HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));

	// Quest list
	QuestListContainer = NewObject<UVerticalBox>(this);
	MainVBox->AddChildToVerticalBox(QuestListContainer);

	if (WidgetTree)
	{
		WidgetTree->RootWidget = RootBorder;
	}
}

void UQuestTrackerWidget::BindToQuestSubsystem(UARPGQuestSubsystem* QuestSubsystem)
{
	if (!QuestSubsystem) return;

	BoundSubsystem = QuestSubsystem;
	QuestSubsystem->OnQuestAccepted.AddDynamic(this, &UQuestTrackerWidget::OnQuestAccepted);
	QuestSubsystem->OnQuestCompleted.AddDynamic(this, &UQuestTrackerWidget::OnQuestCompleted);
	QuestSubsystem->OnQuestFailed.AddDynamic(this, &UQuestTrackerWidget::OnQuestFailed);
	QuestSubsystem->OnObjectiveUpdated.AddDynamic(this, &UQuestTrackerWidget::OnObjectiveUpdated);
	QuestSubsystem->OnQuestPinChanged.AddDynamic(this, &UQuestTrackerWidget::OnQuestPinChanged);

	RefreshDisplay();
}

void UQuestTrackerWidget::OnQuestAccepted(FName QuestID)
{
	RefreshDisplay();
}

void UQuestTrackerWidget::OnQuestCompleted(FName QuestID)
{
	RefreshDisplay();
}

void UQuestTrackerWidget::OnQuestFailed(FName QuestID)
{
	RefreshDisplay();
}

void UQuestTrackerWidget::OnObjectiveUpdated(FName QuestID, int32 ObjectiveIndex, int32 NewCount)
{
	RefreshDisplay();
}

void UQuestTrackerWidget::OnQuestPinChanged(FName QuestID)
{
	RefreshDisplay();
}

void UQuestTrackerWidget::RefreshDisplay()
{
	if (!QuestListContainer) return;
	QuestListContainer->ClearChildren();

	UARPGQuestSubsystem* QS = BoundSubsystem.Get();
	if (!QS) return;

	TArray<FName> ActiveIDs = QS->GetActiveQuestIDs();

	// Move pinned quest to front if present
	FName PinnedID = QS->GetPinnedQuestID();
	if (!PinnedID.IsNone())
	{
		int32 PinIdx = ActiveIDs.Find(PinnedID);
		if (PinIdx > 0)
		{
			ActiveIDs.RemoveAt(PinIdx);
			ActiveIDs.Insert(PinnedID, 0);
		}
	}

	int32 DisplayCount = FMath::Min(ActiveIDs.Num(), MaxDisplayedQuests);

	if (DisplayCount == 0)
	{
		UTextBlock* EmptyText = NewObject<UTextBlock>(this);
		EmptyText->SetFont(FCoreStyle::GetDefaultFontStyle("Italic", 12));
		EmptyText->SetText(FText::FromString(TEXT("No active quests")));
		EmptyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)));
		QuestListContainer->AddChildToVerticalBox(EmptyText);
		return;
	}

	for (int32 q = 0; q < DisplayCount; ++q)
	{
		FName QuestID = ActiveIDs[q];
		UARPGQuestDefinition* Def = QS->FindQuestDefinition(QuestID);
		const FARPGQuestState* State = QS->GetQuestRuntimeState(QuestID);
		if (!State) continue;

		// Quest name (with pin indicator)
		UTextBlock* QuestNameText = NewObject<UTextBlock>(this);
		QuestNameText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 14));
		FText QName = Def ? Def->QuestName : FText::FromName(QuestID);
		bool bIsPinned = (QuestID == PinnedID);
		if (bIsPinned)
		{
			QName = FText::Format(NSLOCTEXT("Quest", "PinFmt", "[*] {0}"), QName);
		}
		QuestNameText->SetText(QName);
		QuestNameText->SetColorAndOpacity(FSlateColor(bIsPinned ? FLinearColor(1.f, 0.85f, 0.3f) : FLinearColor::White));
		auto* QNameSlot = QuestListContainer->AddChildToVerticalBox(QuestNameText);
		QNameSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 2.f));

		// Objectives
		if (Def)
		{
			for (int32 i = 0; i < Def->Objectives.Num(); ++i)
			{
				const FARPGQuestObjective& Obj = Def->Objectives[i];
				bool bComplete = State->ObjectiveStates.IsValidIndex(i) && State->ObjectiveStates[i].bComplete;
				int32 Current = State->ObjectiveStates.IsValidIndex(i) ? State->ObjectiveStates[i].CurrentCount : 0;

				FString ObjStr;
				if (Obj.RequiredCount > 1)
				{
					ObjStr = FString::Printf(TEXT("  %s %s (%d/%d)"),
						bComplete ? TEXT("[X]") : TEXT("[ ]"),
						*Obj.Description.ToString(),
						Current, Obj.RequiredCount);
				}
				else
				{
					ObjStr = FString::Printf(TEXT("  %s %s"),
						bComplete ? TEXT("[X]") : TEXT("[ ]"),
						*Obj.Description.ToString());
				}

				UTextBlock* ObjText = NewObject<UTextBlock>(this);
				ObjText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 12));
				ObjText->SetText(FText::FromString(ObjStr));
				ObjText->SetColorAndOpacity(FSlateColor(
					bComplete ? FLinearColor(0.4f, 0.8f, 0.4f) : FLinearColor(0.8f, 0.8f, 0.8f)));
				QuestListContainer->AddChildToVerticalBox(ObjText);
			}
		}
	}

	if (ActiveIDs.Num() > MaxDisplayedQuests)
	{
		UTextBlock* MoreText = NewObject<UTextBlock>(this);
		MoreText->SetFont(FCoreStyle::GetDefaultFontStyle("Italic", 11));
		MoreText->SetText(FText::Format(
			NSLOCTEXT("Quest", "MoreFmt", "  +{0} more..."),
			FText::AsNumber(ActiveIDs.Num() - MaxDisplayedQuests)));
		MoreText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)));
		QuestListContainer->AddChildToVerticalBox(MoreText);
	}
}
