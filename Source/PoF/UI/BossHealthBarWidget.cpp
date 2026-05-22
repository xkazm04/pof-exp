#include "UI/BossHealthBarWidget.h"
#include "Character/ARPGBossCharacter.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Spacer.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/CoreStyle.h"

void UBossHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildWidget();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UBossHealthBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateDisplay();
}

void UBossHealthBarWidget::BuildWidget()
{
	if (!WidgetTree) return;

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName(TEXT("BossRoot")));
	WidgetTree->RootWidget = Root;

	// Boss name
	BossNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(TEXT("BossNameText")));
	BossNameText->SetJustification(ETextJustify::Center);
	FSlateFontInfo NameFont = FCoreStyle::GetDefaultFontStyle("Bold", 22);
	BossNameText->SetFont(NameFont);
	BossNameText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.85f, 0.3f)));
	UVerticalBoxSlot* NameSlot = Root->AddChildToVerticalBox(BossNameText);
	NameSlot->SetHorizontalAlignment(HAlign_Center);
	NameSlot->SetPadding(FMargin(0, 2, 0, 4));

	// Health bar
	HealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), FName(TEXT("BossHealthBar")));
	HealthBar->SetFillColorAndOpacity(FLinearColor(0.8f, 0.1f, 0.1f));
	UVerticalBoxSlot* BarSlot = Root->AddChildToVerticalBox(HealthBar);
	BarSlot->SetHorizontalAlignment(HAlign_Fill);
	BarSlot->SetPadding(FMargin(50, 0, 50, 2));

	// Phase / percent row
	UHorizontalBox* InfoRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), FName(TEXT("InfoRow")));
	UVerticalBoxSlot* InfoSlot = Root->AddChildToVerticalBox(InfoRow);
	InfoSlot->SetHorizontalAlignment(HAlign_Fill);
	InfoSlot->SetPadding(FMargin(50, 0, 50, 0));

	PhaseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(TEXT("PhaseText")));
	FSlateFontInfo SmallFont = FCoreStyle::GetDefaultFontStyle("Regular", 14);
	PhaseText->SetFont(SmallFont);
	PhaseText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.9f, 0.9f)));
	UHorizontalBoxSlot* PhaseSlot = InfoRow->AddChildToHorizontalBox(PhaseText);
	PhaseSlot->SetHorizontalAlignment(HAlign_Left);

	USpacer* SpacerWidget = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), FName(TEXT("InfoSpacer")));
	SpacerWidget->SetSize(FVector2D(1.f, 1.f));
	UHorizontalBoxSlot* SpacerSlot = InfoRow->AddChildToHorizontalBox(SpacerWidget);
	SpacerSlot->SetSize(ESlateSizeRule::Fill);

	HealthPercentText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(TEXT("HealthPctText")));
	HealthPercentText->SetFont(SmallFont);
	HealthPercentText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.9f, 0.9f)));
	UHorizontalBoxSlot* PctSlot = InfoRow->AddChildToHorizontalBox(HealthPercentText);
	PctSlot->SetHorizontalAlignment(HAlign_Right);
}

void UBossHealthBarWidget::BindToBoss(AARPGBossCharacter* InBoss)
{
	if (!InBoss) return;

	BoundBoss = InBoss;
	InBoss->OnBossPhaseChanged.AddDynamic(this, &UBossHealthBarWidget::OnPhaseChanged);

	if (BossNameText)
	{
		BossNameText->SetText(InBoss->GetBossName());
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	UpdateDisplay();
}

void UBossHealthBarWidget::UnbindFromBoss()
{
	if (BoundBoss.IsValid())
	{
		BoundBoss->OnBossPhaseChanged.RemoveDynamic(this, &UBossHealthBarWidget::OnPhaseChanged);
	}
	BoundBoss = nullptr;
	SetVisibility(ESlateVisibility::Collapsed);
}

void UBossHealthBarWidget::UpdateDisplay()
{
	if (!BoundBoss.IsValid())
	{
		if (GetVisibility() != ESlateVisibility::Collapsed)
		{
			SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	AARPGBossCharacter* Boss = BoundBoss.Get();
	const float HealthPct = Boss->GetHealthPercentage();

	if (HealthBar)
	{
		HealthBar->SetPercent(HealthPct);

		// Color shifts based on health
		FLinearColor BarColor;
		if (HealthPct > 0.5f)
		{
			BarColor = FLinearColor(0.8f, 0.1f, 0.1f);
		}
		else if (HealthPct > 0.25f)
		{
			BarColor = FLinearColor(0.9f, 0.5f, 0.1f);
		}
		else
		{
			BarColor = FLinearColor(0.9f, 0.2f, 0.2f);
		}
		HealthBar->SetFillColorAndOpacity(BarColor);
	}

	if (HealthPercentText)
	{
		HealthPercentText->SetText(FText::FromString(
			FString::Printf(TEXT("%.0f%%"), HealthPct * 100.f)));
	}

	// Hide if boss is dead
	if (Boss->IsDead() && GetVisibility() != ESlateVisibility::Collapsed)
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UBossHealthBarWidget::OnPhaseChanged(int32 NewPhase, int32 TotalPhases)
{
	if (PhaseText)
	{
		PhaseText->SetText(FText::FromString(
			FString::Printf(TEXT("Phase %d/%d"), NewPhase + 1, TotalPhases)));
	}
}
