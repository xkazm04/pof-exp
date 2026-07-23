#include "UI/PreGameMenuWidget.h"
#include "World/PoFDuelSelectionSubsystem.h"
#include "World/PoFDuelStaging.h"
#include "GameFramework/Pawn.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/CoreStyle.h"

bool UPreGameMenuWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}
	if (WidgetTree && !WidgetTree->RootWidget && !HasAnyFlags(RF_ClassDefaultObject))
	{
		BuildLayout();
	}
	return true;
}

namespace
{
UTextBlock* MakeText(UObject* Outer, const FString& S, int32 Size, FLinearColor Color)
{
	UTextBlock* T = NewObject<UTextBlock>(Outer);
	T->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", Size));
	T->SetText(FText::FromString(S));
	T->SetColorAndOpacity(FSlateColor(Color));
	return T;
}

UButton* MakeButton(UObject* Outer, UVerticalBox* Parent, const FString& Label, FLinearColor Color)
{
	UButton* B = NewObject<UButton>(Outer);
	B->AddChild(MakeText(Outer, Label, 16, Color));
	if (Parent)
	{
		auto* S = Parent->AddChildToVerticalBox(B);
		S->SetPadding(FMargin(0.f, 4.f));
		S->SetHorizontalAlignment(HAlign_Center);
	}
	return B;
}
} // namespace

void UPreGameMenuWidget::BuildLayout()
{
	RootBorder = NewObject<UBorder>(this);
	RootBorder->SetBrushColor(FLinearColor(0.015f, 0.025f, 0.05f, 0.94f));
	RootBorder->SetPadding(FMargin(40.f));
	RootBorder->SetHorizontalAlignment(HAlign_Center);
	RootBorder->SetVerticalAlignment(VAlign_Center);

	UVerticalBox* VBox = NewObject<UVerticalBox>(this);
	RootBorder->SetContent(VBox);

	auto* TitleSlot = VBox->AddChildToVerticalBox(
		MakeText(this, TEXT("SABER DUEL"), 34, FLinearColor(0.9f, 0.97f, 1.f)));
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 14.f));

	auto* SubSlot = VBox->AddChildToVerticalBox(
		MakeText(this, TEXT("Choose your blade"), 14, FLinearColor(0.6f, 0.72f, 0.9f)));
	SubSlot->SetHorizontalAlignment(HAlign_Center);
	SubSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));

	UHorizontalBox* Choices = NewObject<UHorizontalBox>(this);
	auto* ChoicesSlot = VBox->AddChildToVerticalBox(Choices);
	ChoicesSlot->SetHorizontalAlignment(HAlign_Center);

	CrimsonButton = NewObject<UButton>(this);
	CrimsonButton->AddChild(MakeText(this, TEXT("  CRIMSON  "), 18, FLinearColor(1.f, 0.25f, 0.2f)));
	CrimsonButton->OnClicked.AddDynamic(this, &UPreGameMenuWidget::OnCrimson);
	auto* CSlot = Choices->AddChildToHorizontalBox(CrimsonButton);
	CSlot->SetPadding(FMargin(8.f, 0.f));

	AzureButton = NewObject<UButton>(this);
	AzureButton->AddChild(MakeText(this, TEXT("  AZURE  "), 18, FLinearColor(0.25f, 0.55f, 1.f)));
	AzureButton->OnClicked.AddDynamic(this, &UPreGameMenuWidget::OnAzure);
	auto* ASlot = Choices->AddChildToHorizontalBox(AzureButton);
	ASlot->SetPadding(FMargin(8.f, 0.f));

	SelectionText = MakeText(this, TEXT("No blade chosen - the standard issue will serve."), 12,
		FLinearColor(0.55f, 0.65f, 0.8f));
	auto* SelSlot = VBox->AddChildToVerticalBox(SelectionText);
	SelSlot->SetHorizontalAlignment(HAlign_Center);
	SelSlot->SetPadding(FMargin(0.f, 10.f));

	auto* QuestHeadSlot = VBox->AddChildToVerticalBox(
		MakeText(this, TEXT("Choose your quest"), 14, FLinearColor(0.6f, 0.72f, 0.9f)));
	QuestHeadSlot->SetHorizontalAlignment(HAlign_Center);
	QuestHeadSlot->SetPadding(FMargin(0.f, 6.f));

	UHorizontalBox* QuestRow = NewObject<UHorizontalBox>(this);
	auto* QuestRowSlot = VBox->AddChildToVerticalBox(QuestRow);
	QuestRowSlot->SetHorizontalAlignment(HAlign_Center);

	QuestLordsButton = NewObject<UButton>(this);
	QuestLordsButton->AddChild(MakeText(this, TEXT("  LORD\'S CHALLENGE  "), 14, FLinearColor(1.f, 0.45f, 0.25f)));
	QuestLordsButton->OnClicked.AddDynamic(this, &UPreGameMenuWidget::OnQuestLords);
	QuestRow->AddChildToHorizontalBox(QuestLordsButton)->SetPadding(FMargin(6.f, 0.f));

	QuestEchoesButton = NewObject<UButton>(this);
	QuestEchoesButton->AddChild(MakeText(this, TEXT("  ECHOES OF THE ORDER  "), 14, FLinearColor(0.42f, 0.72f, 1.f)));
	QuestEchoesButton->OnClicked.AddDynamic(this, &UPreGameMenuWidget::OnQuestEchoes);
	QuestRow->AddChildToHorizontalBox(QuestEchoesButton)->SetPadding(FMargin(6.f, 0.f));

	QuestText = MakeText(this, TEXT("No quest - a plain duel."), 12, FLinearColor(0.55f, 0.65f, 0.8f));
	auto* QuestTextSlot = VBox->AddChildToVerticalBox(QuestText);
	QuestTextSlot->SetHorizontalAlignment(HAlign_Center);
	QuestTextSlot->SetPadding(FMargin(0.f, 8.f));

	EnterButton = MakeButton(this, VBox, TEXT("  ENTER THE DUEL  "), FLinearColor(0.72f, 1.f, 0.87f));
	EnterButton->OnClicked.AddDynamic(this, &UPreGameMenuWidget::OnEnter);

	if (WidgetTree)
	{
		WidgetTree->RootWidget = RootBorder;
	}
}

void UPreGameMenuWidget::Select(FName Saber)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UPoFDuelSelectionSubsystem* Sel = GI->GetSubsystem<UPoFDuelSelectionSubsystem>())
		{
			Sel->SelectSaber(Saber);
		}
	}
	if (SelectionText)
	{
		SelectionText->SetText(FText::FromString(
			FString::Printf(TEXT("Chosen: %s lightsaber"), *Saber.ToString())));
	}
}

void UPreGameMenuWidget::OnCrimson() { Select(FName(TEXT("Crimson"))); }
void UPreGameMenuWidget::OnAzure() { Select(FName(TEXT("Azure"))); }

void UPreGameMenuWidget::SelectQuestId(FName Quest, const FString& Display)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UPoFDuelSelectionSubsystem* Sel = GI->GetSubsystem<UPoFDuelSelectionSubsystem>())
		{
			Sel->SelectQuest(Quest);
		}
	}
	if (QuestText)
	{
		QuestText->SetText(FText::FromString(FString::Printf(TEXT("Quest: %s"), *Display)));
	}
}

void UPreGameMenuWidget::OnQuestLords() { SelectQuestId(FName(TEXT("lords-challenge")), TEXT("The Lord\'s Challenge")); }
void UPreGameMenuWidget::OnQuestEchoes() { SelectQuestId(FName(TEXT("echoes-order")), TEXT("Echoes of the Order")); }

void UPreGameMenuWidget::OnEnter()
{
	SetVisibility(ESlateVisibility::Collapsed);
	// Apply the selections to the world (same functions the headless args use).
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UPoFDuelSelectionSubsystem* Sel = GI->GetSubsystem<UPoFDuelSelectionSubsystem>())
		{
			if (!Sel->SelectedSaber.IsNone())
			{
				if (APlayerController* PC = GetOwningPlayer())
				{
					PoFDuelStaging::ApplySaberChoice(PC->GetPawn(), Sel->SelectedSaber);
				}
			}
			if (!Sel->SelectedQuest.IsNone())
			{
				PoFDuelStaging::ApplyQuestStaging(GetWorld(), Sel->SelectedQuest);
			}
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[PreGameMenu] entered the duel"));
}
