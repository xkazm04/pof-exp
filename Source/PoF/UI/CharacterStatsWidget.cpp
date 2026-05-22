#include "UI/CharacterStatsWidget.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

void UCharacterStatsWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UCharacterStatsWidget::NativeDestruct()
{
	UnbindFromAbilitySystem();
	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------
// Binding
// ---------------------------------------------------------------------------

void UCharacterStatsWidget::BindToAbilitySystem(UAbilitySystemComponent* ASC)
{
	UnbindFromAbilitySystem();

	if (!ASC) return;

	BoundASC = ASC;

	// Clear any previously created rows
	StatRows.Empty();
	if (VitalsContainer) VitalsContainer->ClearChildren();
	if (PrimaryStatsContainer) PrimaryStatsContainer->ClearChildren();
	if (CombatStatsContainer) CombatStatsContainer->ClearChildren();

	// --- Vitals ---
	CreateStatRow(VitalsContainer, UARPGAttributeSet::GetHealthAttribute(), TEXT("Health"));
	CreateStatRow(VitalsContainer, UARPGAttributeSet::GetMaxHealthAttribute(), TEXT("Max Health"));
	CreateStatRow(VitalsContainer, UARPGAttributeSet::GetManaAttribute(), TEXT("Mana"));
	CreateStatRow(VitalsContainer, UARPGAttributeSet::GetMaxManaAttribute(), TEXT("Max Mana"));

	// --- Primary Stats ---
	CreateStatRow(PrimaryStatsContainer, UARPGAttributeSet::GetStrengthAttribute(), TEXT("Strength"));
	CreateStatRow(PrimaryStatsContainer, UARPGAttributeSet::GetDexterityAttribute(), TEXT("Dexterity"));
	CreateStatRow(PrimaryStatsContainer, UARPGAttributeSet::GetIntelligenceAttribute(), TEXT("Intelligence"));

	// --- Combat Stats ---
	CreateStatRow(CombatStatsContainer, UARPGAttributeSet::GetArmorAttribute(), TEXT("Armor"));
	CreateStatRow(CombatStatsContainer, UARPGAttributeSet::GetAttackPowerAttribute(), TEXT("Attack Power"));
	CreateStatRow(CombatStatsContainer, UARPGAttributeSet::GetCriticalChanceAttribute(), TEXT("Crit Chance"), true);
	CreateStatRow(CombatStatsContainer, UARPGAttributeSet::GetCriticalDamageAttribute(), TEXT("Crit Damage"), true);

	// Bind delegates for all tracked attributes
	for (const FStatRow& Row : StatRows)
	{
		ASC->GetGameplayAttributeValueChangeDelegate(Row.Attribute)
			.AddUObject(this, &UCharacterStatsWidget::OnAttributeChanged);
	}

	// Initial refresh
	RefreshAllStats();
	UpdateDamageReduction();
}

void UCharacterStatsWidget::UnbindFromAbilitySystem()
{
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		for (const FStatRow& Row : StatRows)
		{
			ASC->GetGameplayAttributeValueChangeDelegate(Row.Attribute)
				.RemoveAll(this);
		}
	}

	BoundASC.Reset();
}

// ---------------------------------------------------------------------------
// Row creation
// ---------------------------------------------------------------------------

void UCharacterStatsWidget::CreateStatRow(UVerticalBox* Container, const FGameplayAttribute& Attribute, const FName& DisplayName, bool bIsPercentage)
{
	if (!Container) return;

	const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", StatFontSize);

	// Horizontal layout: [Label] [BaseValue] [(+Bonus)]
	UHorizontalBox* HBox = NewObject<UHorizontalBox>(this);

	// Label
	UTextBlock* Label = NewObject<UTextBlock>(this);
	Label->SetText(FText::FromName(DisplayName));
	Label->SetFont(Font);
	Label->SetColorAndOpacity(FSlateColor(LabelColor));

	UHorizontalBoxSlot* LabelSlot = HBox->AddChildToHorizontalBox(Label);
	LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	LabelSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Left);
	LabelSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);

	// Base value
	UTextBlock* ValueBlock = NewObject<UTextBlock>(this);
	ValueBlock->SetFont(Font);
	ValueBlock->SetColorAndOpacity(FSlateColor(BaseValueColor));

	UHorizontalBoxSlot* ValueSlot = HBox->AddChildToHorizontalBox(ValueBlock);
	ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	ValueSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
	ValueSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
	ValueSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));

	// Bonus text
	UTextBlock* BonusBlock = NewObject<UTextBlock>(this);
	BonusBlock->SetFont(Font);

	UHorizontalBoxSlot* BonusSlot = HBox->AddChildToHorizontalBox(BonusBlock);
	BonusSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	BonusSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
	BonusSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
	BonusSlot->SetPadding(FMargin(4.f, 0.f, 0.f, 0.f));

	// Add row to container
	UVerticalBoxSlot* RowSlot = Container->AddChildToVerticalBox(HBox);
	RowSlot->SetPadding(FMargin(4.f, 2.f));

	// Track
	FStatRow& Row = StatRows.AddDefaulted_GetRef();
	Row.Attribute = Attribute;
	Row.StatName = DisplayName;
	Row.LabelText = Label;
	Row.ValueText = ValueBlock;
	Row.BonusText = BonusBlock;
	Row.bIsPercentage = bIsPercentage;
}

// ---------------------------------------------------------------------------
// Refresh
// ---------------------------------------------------------------------------

void UCharacterStatsWidget::RefreshStatRow(FStatRow& Row)
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC) return;

	bool bFound = false;
	float CurrentValue = ASC->GetGameplayAttributeValue(Row.Attribute, bFound);
	if (!bFound) return;

	// Get base value from the attribute data on the attribute set
	float BaseValue = CurrentValue;
	const UAttributeSet* AttrSet = ASC->GetAttributeSet(UARPGAttributeSet::StaticClass());
	if (AttrSet)
	{
		if (const FGameplayAttributeData* AttrData = Row.Attribute.GetGameplayAttributeData(AttrSet))
		{
			BaseValue = AttrData->GetBaseValue();
		}
	}

	float Bonus = CurrentValue - BaseValue;

	// Format base value
	if (Row.bIsPercentage)
	{
		Row.ValueText->SetText(FText::FromString(FString::Printf(TEXT("%.1f%%"), BaseValue)));
	}
	else
	{
		Row.ValueText->SetText(FText::FromString(FString::Printf(TEXT("%.0f"), BaseValue)));
	}

	// Format bonus
	if (FMath::Abs(Bonus) > KINDA_SMALL_NUMBER)
	{
		FLinearColor BonusColor = Bonus > 0.f ? PositiveBonusColor : NegativeBonusColor;
		Row.BonusText->SetColorAndOpacity(FSlateColor(BonusColor));

		if (Row.bIsPercentage)
		{
			Row.BonusText->SetText(FText::FromString(
				FString::Printf(TEXT("(%s%.1f%%)"), Bonus > 0.f ? TEXT("+") : TEXT(""), Bonus)));
		}
		else
		{
			Row.BonusText->SetText(FText::FromString(
				FString::Printf(TEXT("(%s%.0f)"), Bonus > 0.f ? TEXT("+") : TEXT(""), Bonus)));
		}
		Row.BonusText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		Row.BonusText->SetText(FText::GetEmpty());
		Row.BonusText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UCharacterStatsWidget::RefreshAllStats()
{
	for (FStatRow& Row : StatRows)
	{
		RefreshStatRow(Row);
	}
}

void UCharacterStatsWidget::UpdateDamageReduction()
{
	if (!DamageReductionText) return;

	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC) return;

	bool bFound = false;
	float ArmorValue = ASC->GetGameplayAttributeValue(UARPGAttributeSet::GetArmorAttribute(), bFound);
	if (!bFound) ArmorValue = 0.f;

	// DR = Armor / (Armor + Constant), clamped to [0, 100]
	float DR = 0.f;
	if (ArmorValue > 0.f && ArmorConstant > 0.f)
	{
		DR = (ArmorValue / (ArmorValue + ArmorConstant)) * 100.f;
	}
	DR = FMath::Clamp(DR, 0.f, 100.f);

	DamageReductionText->SetText(FText::FromString(FString::Printf(TEXT("Damage Reduction: %.1f%%"), DR)));
	DamageReductionText->SetColorAndOpacity(FSlateColor(DamageReductionColor));
}

// ---------------------------------------------------------------------------
// GAS callback
// ---------------------------------------------------------------------------

void UCharacterStatsWidget::OnAttributeChanged(const FOnAttributeChangeData& Data)
{
	// Refresh the matching row
	for (FStatRow& Row : StatRows)
	{
		if (Row.Attribute == Data.Attribute)
		{
			RefreshStatRow(Row);
			break;
		}
	}

	// Armor changes also update DR
	if (Data.Attribute == UARPGAttributeSet::GetArmorAttribute())
	{
		UpdateDamageReduction();
	}
}
