#include "UI/AbilityUnlockWidget.h"
#include "Player/ARPGPlayerCharacter.h"
#include "AbilitySystem/ARPGAbilityUnlockComponent.h"
#include "AbilitySystem/ARPGAbilityUnlockData.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Styling/CoreStyle.h"

void UAbilityUnlockWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UAbilityUnlockWidget::NativeDestruct()
{
	UnbindAll();
	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------
// Binding
// ---------------------------------------------------------------------------

void UAbilityUnlockWidget::BindToPlayer(AARPGPlayerCharacter* Player)
{
	UnbindAll();

	if (!Player) return;

	BoundPlayer = Player;
	UARPGAbilityUnlockComponent* UnlockComp = Player->GetAbilityUnlockComponent();
	if (!UnlockComp) return;

	BoundUnlockComp = UnlockComp;

	UnlockComp->OnSkillPointsChanged.AddDynamic(this, &UAbilityUnlockWidget::OnSkillPointsChanged);
	UnlockComp->OnAbilityLearned.AddDynamic(this, &UAbilityUnlockWidget::OnAbilityLearned);
	UnlockComp->OnAbilityUpgraded.AddDynamic(this, &UAbilityUnlockWidget::OnAbilityUpgraded);

	BuildList();
	RefreshDisplay();
}

void UAbilityUnlockWidget::UnbindAll()
{
	if (UARPGAbilityUnlockComponent* Comp = BoundUnlockComp.Get())
	{
		Comp->OnSkillPointsChanged.RemoveDynamic(this, &UAbilityUnlockWidget::OnSkillPointsChanged);
		Comp->OnAbilityLearned.RemoveDynamic(this, &UAbilityUnlockWidget::OnAbilityLearned);
		Comp->OnAbilityUpgraded.RemoveDynamic(this, &UAbilityUnlockWidget::OnAbilityUpgraded);
	}

	BoundPlayer.Reset();
	BoundUnlockComp.Reset();
}

// ---------------------------------------------------------------------------
// List construction
// ---------------------------------------------------------------------------

void UAbilityUnlockWidget::BuildList()
{
	UIRows.Empty();
	ButtonToRowIndex.Empty();
	if (AbilityListContainer)
	{
		AbilityListContainer->ClearChildren();
	}

	UARPGAbilityUnlockComponent* Comp = BoundUnlockComp.Get();
	if (!Comp || !AbilityListContainer) return;

	const TArray<FName> AllRows = Comp->GetAllAbilityRowNames();
	const FSlateFontInfo BoldFont = FCoreStyle::GetDefaultFontStyle("Bold", FontSize);
	const FSlateFontInfo NormalFont = FCoreStyle::GetDefaultFontStyle("Regular", FontSize);
	const FSlateFontInfo SmallFont = FCoreStyle::GetDefaultFontStyle("Regular", FontSize - 2);

	for (int32 i = 0; i < AllRows.Num(); ++i)
	{
		const FName& RowName = AllRows[i];

		FAbilityUnlockRow RowData;
		if (!Comp->GetAbilityUnlockInfo(RowName, RowData)) continue;

		// Outer vertical box for this ability entry
		UVerticalBox* EntryBox = NewObject<UVerticalBox>(this);

		// --- Row 1: Name + Rank ---
		UHorizontalBox* TopRow = NewObject<UHorizontalBox>(this);

		UTextBlock* NameBlock = NewObject<UTextBlock>(this);
		NameBlock->SetText(RowData.DisplayName.IsEmpty() ? FText::FromName(RowName) : RowData.DisplayName);
		NameBlock->SetFont(BoldFont);
		NameBlock->SetColorAndOpacity(FSlateColor(NameColor));

		UHorizontalBoxSlot* NameSlot = TopRow->AddChildToHorizontalBox(NameBlock);
		NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		NameSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);

		UTextBlock* RankBlock = NewObject<UTextBlock>(this);
		RankBlock->SetFont(NormalFont);
		RankBlock->SetColorAndOpacity(FSlateColor(InfoColor));

		UHorizontalBoxSlot* RankSlot = TopRow->AddChildToHorizontalBox(RankBlock);
		RankSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		RankSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
		RankSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));

		UVerticalBoxSlot* TopSlot = EntryBox->AddChildToVerticalBox(TopRow);
		TopSlot->SetPadding(FMargin(0.f, 2.f));

		// --- Row 2: Description ---
		UTextBlock* DescBlock = NewObject<UTextBlock>(this);
		DescBlock->SetText(RowData.Description.IsEmpty() ? FText::FromString(TEXT("No description.")) : RowData.Description);
		DescBlock->SetFont(SmallFont);
		DescBlock->SetColorAndOpacity(FSlateColor(DescColor));
		DescBlock->SetAutoWrapText(true);

		UVerticalBoxSlot* DescSlot = EntryBox->AddChildToVerticalBox(DescBlock);
		DescSlot->SetPadding(FMargin(4.f, 0.f, 0.f, 2.f));

		// --- Row 3: Req Level + Cost + Button ---
		UHorizontalBox* BottomRow = NewObject<UHorizontalBox>(this);

		UTextBlock* ReqBlock = NewObject<UTextBlock>(this);
		ReqBlock->SetFont(SmallFont);
		ReqBlock->SetColorAndOpacity(FSlateColor(InfoColor));

		UHorizontalBoxSlot* ReqSlot = BottomRow->AddChildToHorizontalBox(ReqBlock);
		ReqSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		ReqSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);

		UTextBlock* CostBlock = NewObject<UTextBlock>(this);
		CostBlock->SetFont(SmallFont);
		CostBlock->SetColorAndOpacity(FSlateColor(InfoColor));

		UHorizontalBoxSlot* CostSlot = BottomRow->AddChildToHorizontalBox(CostBlock);
		CostSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		CostSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
		CostSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));

		UButton* ActionBtn = NewObject<UButton>(this);
		UTextBlock* BtnLabel = NewObject<UTextBlock>(this);
		BtnLabel->SetFont(NormalFont);
		BtnLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		ActionBtn->AddChild(BtnLabel);

		UHorizontalBoxSlot* BtnSlot = BottomRow->AddChildToHorizontalBox(ActionBtn);
		BtnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		BtnSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
		BtnSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));

		UVerticalBoxSlot* BottomSlot = EntryBox->AddChildToVerticalBox(BottomRow);
		BottomSlot->SetPadding(FMargin(0.f, 2.f));

		// Add entry to scroll box
		AbilityListContainer->AddChild(EntryBox);

		// Track the row
		FAbilityUIRow& UIRow = UIRows.AddDefaulted_GetRef();
		UIRow.RowName = RowName;
		UIRow.NameText = NameBlock;
		UIRow.DescText = DescBlock;
		UIRow.RankText = RankBlock;
		UIRow.CostText = CostBlock;
		UIRow.ReqText = ReqBlock;
		UIRow.ActionButton = ActionBtn;
		UIRow.ButtonLabel = BtnLabel;

		// Map button to index, bind to shared handler
		ButtonToRowIndex.Add(ActionBtn, i);
		ActionBtn->OnClicked.AddDynamic(this, &UAbilityUnlockWidget::OnAnyButtonClicked);
	}
}

// ---------------------------------------------------------------------------
// Refresh
// ---------------------------------------------------------------------------

void UAbilityUnlockWidget::RefreshDisplay()
{
	UpdateSkillPointsText();
	for (int32 i = 0; i < UIRows.Num(); ++i)
	{
		RefreshRow(i);
	}
}

void UAbilityUnlockWidget::RefreshRow(int32 Index)
{
	if (!UIRows.IsValidIndex(Index)) return;

	FAbilityUIRow& UIRow = UIRows[Index];
	UARPGAbilityUnlockComponent* Comp = BoundUnlockComp.Get();
	AARPGPlayerCharacter* Player = BoundPlayer.Get();
	if (!Comp || !Player) return;

	FAbilityUnlockRow RowData;
	if (!Comp->GetAbilityUnlockInfo(UIRow.RowName, RowData)) return;

	const int32 CurrentRank = Comp->GetAbilityRank(UIRow.RowName);
	const bool bLearned = CurrentRank > 0;
	const int32 PlayerLevel = Player->GetPlayerLevel();

	// Rank text
	if (bLearned)
	{
		UIRow.RankText->SetText(FText::FromString(
			FString::Printf(TEXT("Rank %d / %d"), CurrentRank, RowData.MaxRank)));
		UIRow.RankText->SetColorAndOpacity(FSlateColor(LearnedColor));
	}
	else
	{
		UIRow.RankText->SetText(FText::FromString(TEXT("Not Learned")));
		UIRow.RankText->SetColorAndOpacity(FSlateColor(LockedColor));
	}

	// Requirement text
	if (PlayerLevel >= RowData.RequiredLevel)
	{
		UIRow.ReqText->SetText(FText::FromString(
			FString::Printf(TEXT("Req Lv %d"), RowData.RequiredLevel)));
		UIRow.ReqText->SetColorAndOpacity(FSlateColor(InfoColor));
	}
	else
	{
		UIRow.ReqText->SetText(FText::FromString(
			FString::Printf(TEXT("Req Lv %d (need %d more)"), RowData.RequiredLevel, RowData.RequiredLevel - PlayerLevel)));
		UIRow.ReqText->SetColorAndOpacity(FSlateColor(LockedColor));
	}

	// Cost and button
	if (!bLearned)
	{
		UIRow.CostText->SetText(FText::FromString(
			FString::Printf(TEXT("Cost: %d SP"), RowData.SkillPointCost)));
		UIRow.ButtonLabel->SetText(FText::FromString(TEXT("Learn")));

		const bool bCanLearn = Comp->CanLearnAbility(UIRow.RowName);
		UIRow.ActionButton->SetIsEnabled(bCanLearn);
		UIRow.ButtonLabel->SetColorAndOpacity(FSlateColor(bCanLearn ? FLinearColor::White : FLinearColor(0.4f, 0.4f, 0.4f)));
	}
	else if (CurrentRank < RowData.MaxRank)
	{
		UIRow.CostText->SetText(FText::FromString(
			FString::Printf(TEXT("Upgrade: %d SP"), RowData.UpgradeSkillPointCost)));
		UIRow.ButtonLabel->SetText(FText::FromString(TEXT("Upgrade")));

		const bool bCanUpgrade = Comp->CanUpgradeAbility(UIRow.RowName);
		UIRow.ActionButton->SetIsEnabled(bCanUpgrade);
		UIRow.ButtonLabel->SetColorAndOpacity(FSlateColor(bCanUpgrade ? FLinearColor::White : FLinearColor(0.4f, 0.4f, 0.4f)));
	}
	else
	{
		UIRow.CostText->SetText(FText::FromString(TEXT("MAX RANK")));
		UIRow.CostText->SetColorAndOpacity(FSlateColor(LearnedColor));
		UIRow.ButtonLabel->SetText(FText::FromString(TEXT("Maxed")));
		UIRow.ActionButton->SetIsEnabled(false);
		UIRow.ButtonLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.4f, 0.4f)));
	}
}

void UAbilityUnlockWidget::UpdateSkillPointsText()
{
	if (!SkillPointsText) return;

	UARPGAbilityUnlockComponent* Comp = BoundUnlockComp.Get();
	const int32 Points = Comp ? Comp->GetSkillPoints() : 0;

	SkillPointsText->SetText(FText::FromString(
		FString::Printf(TEXT("Skill Points: %d"), Points)));
	SkillPointsText->SetColorAndOpacity(FSlateColor(Points > 0 ? NameColor : LockedColor));
}

// ---------------------------------------------------------------------------
// Shared button click handler
// ---------------------------------------------------------------------------

void UAbilityUnlockWidget::OnAnyButtonClicked()
{
	// Determine which button was clicked by finding the hovered/focused one
	for (const auto& Pair : ButtonToRowIndex)
	{
		UButton* Btn = Pair.Key;
		if (Btn && Btn->HasKeyboardFocus())
		{
			OnActionButtonClicked(Pair.Value);
			return;
		}
	}

	// Fallback: check IsHovered
	for (const auto& Pair : ButtonToRowIndex)
	{
		UButton* Btn = Pair.Key;
		if (Btn && Btn->IsHovered())
		{
			OnActionButtonClicked(Pair.Value);
			return;
		}
	}
}

void UAbilityUnlockWidget::OnActionButtonClicked(int32 RowIndex)
{
	if (!UIRows.IsValidIndex(RowIndex)) return;

	UARPGAbilityUnlockComponent* Comp = BoundUnlockComp.Get();
	if (!Comp) return;

	const FName& RowName = UIRows[RowIndex].RowName;

	if (!Comp->HasLearnedAbility(RowName))
	{
		Comp->LearnAbility(RowName);
	}
	else
	{
		Comp->UpgradeAbility(RowName);
	}
}

// ---------------------------------------------------------------------------
// Delegate callbacks
// ---------------------------------------------------------------------------

void UAbilityUnlockWidget::OnSkillPointsChanged(int32 NewPoints)
{
	RefreshDisplay();
}

void UAbilityUnlockWidget::OnAbilityLearned(FName RowName, int32 Rank)
{
	RefreshDisplay();
}

void UAbilityUnlockWidget::OnAbilityUpgraded(FName RowName, int32 NewRank)
{
	RefreshDisplay();
}
