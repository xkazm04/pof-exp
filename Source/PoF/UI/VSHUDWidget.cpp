#include "UI/VSHUDWidget.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"



void UVSHUDWidget::BuildTree()
{
	if (!WidgetTree || PlayerHealthBar)
	{
		// Already built, or no tree to build into.
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), FName(TEXT("VSHUDRoot")));
	WidgetTree->RootWidget = Canvas;
	if (!Canvas)
	{
		return;
	}

	const FSlateFontInfo LabelFont = FCoreStyle::GetDefaultFontStyle("Bold", 16);
	const FSlateFontInfo ValueFont = FCoreStyle::GetDefaultFontStyle("Bold", 14);

	const FLinearColor PlayerFill(0.15f, 0.85f, 0.25f);
	const FLinearColor EnemyFill(0.9f, 0.13f, 0.13f);
	const FSlateBrush FrameBrush = MakeSolidBrush(FLinearColor(0.f, 0.f, 0.f, 0.7f));

	// --- Player block: anchored top-left ---
	// A Border gives the bar a dark frame so its bounds are obvious on any scene.
	UBorder* PlayerFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(TEXT("PlayerFrame")));
	PlayerFrame->SetBrush(FrameBrush);
	PlayerFrame->SetPadding(FMargin(3.f));
	if (UCanvasPanelSlot* FrameSlot = Cast<UCanvasPanelSlot>(Canvas->AddChildToCanvas(PlayerFrame)))
	{
		FrameSlot->SetAnchors(FAnchors(0.f, 0.f));
		FrameSlot->SetPosition(FVector2D(40.f, 90.f));
		FrameSlot->SetSize(FVector2D(286.f, 28.f));
	}

	PlayerHealthBar = CreateStyledProgressBar(FName(TEXT("PlayerHealthBar")), PlayerFill);
	PlayerFrame->SetContent(PlayerHealthBar);

	PlayerHealthText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(TEXT("PlayerHealthText")));
	PlayerHealthText->SetFont(ValueFont);
	PlayerHealthText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	PlayerHealthText->SetText(FText::FromString(TEXT("Player")));
	if (UCanvasPanelSlot* PlayerTextSlot = Cast<UCanvasPanelSlot>(Canvas->AddChildToCanvas(PlayerHealthText)))
	{
		PlayerTextSlot->SetAnchors(FAnchors(0.f, 0.f));
		PlayerTextSlot->SetPosition(FVector2D(44.f, 122.f));
		PlayerTextSlot->SetAutoSize(true);
	}

	// --- Target / enemy block: anchored top-centre ---
	EnemyNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(TEXT("EnemyNameText")));
	EnemyNameText->SetFont(LabelFont);
	EnemyNameText->SetJustification(ETextJustify::Center);
	EnemyNameText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.85f, 0.3f)));
	EnemyNameText->SetText(FText::FromString(TEXT("Enemy")));
	if (UCanvasPanelSlot* EnemyNameSlot = Cast<UCanvasPanelSlot>(Canvas->AddChildToCanvas(EnemyNameText)))
	{
		EnemyNameSlot->SetAnchors(FAnchors(0.5f, 0.f));
		EnemyNameSlot->SetAlignment(FVector2D(0.5f, 0.f));
		EnemyNameSlot->SetPosition(FVector2D(0.f, 36.f));
		EnemyNameSlot->SetAutoSize(true);
	}

	UBorder* EnemyFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(TEXT("EnemyFrame")));
	EnemyFrame->SetBrush(FrameBrush);
	EnemyFrame->SetPadding(FMargin(3.f));
	if (UCanvasPanelSlot* EnemyFrameSlot = Cast<UCanvasPanelSlot>(Canvas->AddChildToCanvas(EnemyFrame)))
	{
		EnemyFrameSlot->SetAnchors(FAnchors(0.5f, 0.f));
		EnemyFrameSlot->SetAlignment(FVector2D(0.5f, 0.f));
		EnemyFrameSlot->SetPosition(FVector2D(0.f, 62.f));
		EnemyFrameSlot->SetSize(FVector2D(380.f, 28.f));
	}

	EnemyHealthBar = CreateStyledProgressBar(FName(TEXT("EnemyHealthBar")), EnemyFill);
	EnemyFrame->SetContent(EnemyHealthBar);

	EnemyHealthText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(TEXT("EnemyHealthText")));
	EnemyHealthText->SetFont(ValueFont);
	EnemyHealthText->SetJustification(ETextJustify::Center);
	EnemyHealthText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	EnemyHealthText->SetText(FText::FromString(TEXT("--- / ---")));
	if (UCanvasPanelSlot* EnemyTextSlot = Cast<UCanvasPanelSlot>(Canvas->AddChildToCanvas(EnemyHealthText)))
	{
		EnemyTextSlot->SetAnchors(FAnchors(0.5f, 0.f));
		EnemyTextSlot->SetAlignment(FVector2D(0.5f, 0.f));
		EnemyTextSlot->SetPosition(FVector2D(0.f, 94.f));
		EnemyTextSlot->SetAutoSize(true);
	}
}

void UVSHUDWidget::BindPlayer(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	PlayerASC = ASC;

	PlayerHealth = ASC->GetNumericAttribute(UARPGAttributeSet::GetHealthAttribute());
	PlayerMaxHealth = ASC->GetNumericAttribute(UARPGAttributeSet::GetMaxHealthAttribute());

	ASC->GetGameplayAttributeValueChangeDelegate(UARPGAttributeSet::GetHealthAttribute())
		.AddUObject(this, &UVSHUDWidget::OnPlayerHealthChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(UARPGAttributeSet::GetMaxHealthAttribute())
		.AddUObject(this, &UVSHUDWidget::OnPlayerMaxHealthChanged);

	if (PlayerHealthText)
	{
		PlayerHealthText->SetText(FText::FromString(TEXT("Player")));
	}

	RefreshPlayerBar();
}

void UVSHUDWidget::BindEnemy(UAbilitySystemComponent* ASC, const FString& EnemyName)
{
	if (!ASC)
	{
		return;
	}

	EnemyASC = ASC;

	EnemyHealth = ASC->GetNumericAttribute(UARPGAttributeSet::GetHealthAttribute());
	EnemyMaxHealth = ASC->GetNumericAttribute(UARPGAttributeSet::GetMaxHealthAttribute());

	ASC->GetGameplayAttributeValueChangeDelegate(UARPGAttributeSet::GetHealthAttribute())
		.AddUObject(this, &UVSHUDWidget::OnEnemyHealthChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(UARPGAttributeSet::GetMaxHealthAttribute())
		.AddUObject(this, &UVSHUDWidget::OnEnemyMaxHealthChanged);

	if (EnemyNameText)
	{
		EnemyNameText->SetText(FText::FromString(EnemyName));
	}

	RefreshEnemyBar();
}

void UVSHUDWidget::OnPlayerHealthChanged(const FOnAttributeChangeData& Data)
{
	PlayerHealth = Data.NewValue;
	RefreshPlayerBar();
}

void UVSHUDWidget::OnPlayerMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	PlayerMaxHealth = Data.NewValue;
	RefreshPlayerBar();
}

void UVSHUDWidget::OnEnemyHealthChanged(const FOnAttributeChangeData& Data)
{
	EnemyHealth = Data.NewValue;
	RefreshEnemyBar();
}

void UVSHUDWidget::OnEnemyMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	EnemyMaxHealth = Data.NewValue;
	RefreshEnemyBar();
}

void UVSHUDWidget::RefreshPlayerBar()
{
	if (PlayerHealthBar)
	{
		PlayerHealthBar->SetPercent(PlayerMaxHealth > 0.f ? PlayerHealth / PlayerMaxHealth : 0.f);
	}
	if (PlayerHealthText)
	{
		PlayerHealthText->SetText(FText::FromString(
			FString::Printf(TEXT("%.0f / %.0f"), PlayerHealth, PlayerMaxHealth)));
	}
}

void UVSHUDWidget::RefreshEnemyBar()
{
	if (EnemyHealthBar)
	{
		EnemyHealthBar->SetPercent(EnemyMaxHealth > 0.f ? EnemyHealth / EnemyMaxHealth : 0.f);
	}
	if (EnemyHealthText)
	{
		EnemyHealthText->SetText(FText::FromString(
			FString::Printf(TEXT("%.0f / %.0f"), EnemyHealth, EnemyMaxHealth)));
	}
}
