#include "UI/VSHUDWidget.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

void UVSHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree)
	{
		return;
	}

	// Build the widget tree entirely in C++ — mirrors UBossHealthBarWidget::BuildWidget().
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), FName(TEXT("VSHUDRoot")));
	WidgetTree->RootWidget = Canvas;
	if (!Canvas)
	{
		return;
	}

	const FSlateFontInfo LabelFont = FCoreStyle::GetDefaultFontStyle("Bold", 16);
	const FSlateFontInfo ValueFont = FCoreStyle::GetDefaultFontStyle("Regular", 13);

	// --- Player block: anchored top-left ---
	PlayerHealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), FName(TEXT("PlayerHealthBar")));
	PlayerHealthBar->SetFillColorAndOpacity(FLinearColor(0.15f, 0.8f, 0.25f));
	PlayerHealthBar->SetPercent(1.f);
	if (UCanvasPanelSlot* PlayerBarSlot = Cast<UCanvasPanelSlot>(Canvas->AddChildToCanvas(PlayerHealthBar)))
	{
		PlayerBarSlot->SetAnchors(FAnchors(0.f, 0.f));
		PlayerBarSlot->SetPosition(FVector2D(40.f, 40.f));
		PlayerBarSlot->SetSize(FVector2D(260.f, 22.f));
	}

	PlayerHealthText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(TEXT("PlayerHealthText")));
	PlayerHealthText->SetFont(ValueFont);
	PlayerHealthText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	PlayerHealthText->SetText(FText::FromString(TEXT("Player")));
	if (UCanvasPanelSlot* PlayerTextSlot = Cast<UCanvasPanelSlot>(Canvas->AddChildToCanvas(PlayerHealthText)))
	{
		PlayerTextSlot->SetAnchors(FAnchors(0.f, 0.f));
		PlayerTextSlot->SetPosition(FVector2D(40.f, 64.f));
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
		EnemyNameSlot->SetPosition(FVector2D(0.f, 32.f));
		EnemyNameSlot->SetAutoSize(true);
	}

	EnemyHealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), FName(TEXT("EnemyHealthBar")));
	EnemyHealthBar->SetFillColorAndOpacity(FLinearColor(0.85f, 0.12f, 0.12f));
	EnemyHealthBar->SetPercent(1.f);
	if (UCanvasPanelSlot* EnemyBarSlot = Cast<UCanvasPanelSlot>(Canvas->AddChildToCanvas(EnemyHealthBar)))
	{
		EnemyBarSlot->SetAnchors(FAnchors(0.5f, 0.f));
		EnemyBarSlot->SetAlignment(FVector2D(0.5f, 0.f));
		EnemyBarSlot->SetPosition(FVector2D(0.f, 54.f));
		EnemyBarSlot->SetSize(FVector2D(360.f, 20.f));
	}

	EnemyHealthText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(TEXT("EnemyHealthText")));
	EnemyHealthText->SetFont(ValueFont);
	EnemyHealthText->SetJustification(ETextJustify::Center);
	EnemyHealthText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	EnemyHealthText->SetText(FText::FromString(TEXT("--- / ---")));
	if (UCanvasPanelSlot* EnemyTextSlot = Cast<UCanvasPanelSlot>(Canvas->AddChildToCanvas(EnemyHealthText)))
	{
		EnemyTextSlot->SetAnchors(FAnchors(0.5f, 0.f));
		EnemyTextSlot->SetAlignment(FVector2D(0.5f, 0.f));
		EnemyTextSlot->SetPosition(FVector2D(0.f, 76.f));
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
