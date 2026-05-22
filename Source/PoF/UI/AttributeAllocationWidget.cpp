#include "UI/AttributeAllocationWidget.h"
#include "Player/ARPGPlayerCharacter.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Styling/CoreStyle.h"

void UAttributeAllocationWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UAttributeAllocationWidget::NativeDestruct()
{
	UnbindAll();
	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------
// Binding
// ---------------------------------------------------------------------------

void UAttributeAllocationWidget::BindToPlayer(AARPGPlayerCharacter* Player)
{
	UnbindAll();

	if (!Player) return;

	BoundPlayer = Player;
	BoundASC = Player->GetAbilitySystemComponent();

	// Listen for attribute point changes
	Player->OnAttributePointsChanged.AddDynamic(this, &UAttributeAllocationWidget::OnAttributePointsChanged);

	// Build the rows
	BuildRows();

	// Bind GAS attribute change delegates for live updates
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		for (const FAttributeRow& Row : Rows)
		{
			ASC->GetGameplayAttributeValueChangeDelegate(Row.Attribute)
				.AddUObject(this, &UAttributeAllocationWidget::OnAttributeChanged);
			ASC->GetGameplayAttributeValueChangeDelegate(Row.DerivedAttribute)
				.AddUObject(this, &UAttributeAllocationWidget::OnAttributeChanged);
		}
	}

	RefreshAll();
}

void UAttributeAllocationWidget::UnbindAll()
{
	if (AARPGPlayerCharacter* Player = BoundPlayer.Get())
	{
		Player->OnAttributePointsChanged.RemoveDynamic(this, &UAttributeAllocationWidget::OnAttributePointsChanged);
	}

	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		for (const FAttributeRow& Row : Rows)
		{
			ASC->GetGameplayAttributeValueChangeDelegate(Row.Attribute).RemoveAll(this);
			ASC->GetGameplayAttributeValueChangeDelegate(Row.DerivedAttribute).RemoveAll(this);
		}
	}

	BoundPlayer.Reset();
	BoundASC.Reset();
}

// ---------------------------------------------------------------------------
// Row creation
// ---------------------------------------------------------------------------

void UAttributeAllocationWidget::BuildRows()
{
	Rows.Empty();
	if (AllocationContainer)
	{
		AllocationContainer->ClearChildren();
	}

	CreateRow(TEXT("Strength"),
		FText::FromString(TEXT("Strength")),
		UARPGAttributeSet::GetStrengthAttribute(),
		UARPGAttributeSet::GetAttackPowerAttribute(),
		0);

	CreateRow(TEXT("Dexterity"),
		FText::FromString(TEXT("Dexterity")),
		UARPGAttributeSet::GetDexterityAttribute(),
		UARPGAttributeSet::GetCriticalChanceAttribute(),
		1);

	CreateRow(TEXT("Intelligence"),
		FText::FromString(TEXT("Intelligence")),
		UARPGAttributeSet::GetIntelligenceAttribute(),
		UARPGAttributeSet::GetMaxManaAttribute(),
		2);
}

void UAttributeAllocationWidget::CreateRow(
	const FName& StatName,
	const FText& DisplayName,
	const FGameplayAttribute& PrimaryAttribute,
	const FGameplayAttribute& DerivedAttribute,
	int32 RowIndex)
{
	if (!AllocationContainer) return;

	const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", FontSize);
	const FSlateFontInfo BoldFont = FCoreStyle::GetDefaultFontStyle("Bold", FontSize);
	const FSlateFontInfo SmallFont = FCoreStyle::GetDefaultFontStyle("Regular", FontSize - 2);

	// Outer horizontal box: [StatName] [CurrentValue] [Allocated] [Preview] [+Button]
	UHorizontalBox* HBox = NewObject<UHorizontalBox>(this);

	// --- Stat Name ---
	UTextBlock* NameBlock = NewObject<UTextBlock>(this);
	NameBlock->SetText(DisplayName);
	NameBlock->SetFont(BoldFont);
	NameBlock->SetColorAndOpacity(FSlateColor(LabelColor));

	UHorizontalBoxSlot* NameSlot = HBox->AddChildToHorizontalBox(NameBlock);
	NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	NameSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Left);
	NameSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);

	// --- Current Value (primary stat: derived stat) ---
	UTextBlock* ValueBlock = NewObject<UTextBlock>(this);
	ValueBlock->SetFont(Font);
	ValueBlock->SetColorAndOpacity(FSlateColor(ValueColor));

	UHorizontalBoxSlot* ValueSlot = HBox->AddChildToHorizontalBox(ValueBlock);
	ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	ValueSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
	ValueSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
	ValueSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));

	// --- Allocated count ---
	UTextBlock* AllocBlock = NewObject<UTextBlock>(this);
	AllocBlock->SetFont(SmallFont);
	AllocBlock->SetColorAndOpacity(FSlateColor(LabelColor));

	UHorizontalBoxSlot* AllocSlot = HBox->AddChildToHorizontalBox(AllocBlock);
	AllocSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	AllocSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
	AllocSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
	AllocSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));

	// --- Preview (what next point adds) ---
	UTextBlock* PreviewBlock = NewObject<UTextBlock>(this);
	PreviewBlock->SetFont(SmallFont);
	PreviewBlock->SetColorAndOpacity(FSlateColor(PreviewColor));

	UHorizontalBoxSlot* PreviewSlot = HBox->AddChildToHorizontalBox(PreviewBlock);
	PreviewSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	PreviewSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
	PreviewSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
	PreviewSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));

	// --- [+] Button ---
	UButton* AllocButton = NewObject<UButton>(this);
	UTextBlock* BtnLabel = NewObject<UTextBlock>(this);
	BtnLabel->SetText(FText::FromString(TEXT("+")));
	BtnLabel->SetFont(BoldFont);
	BtnLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	AllocButton->AddChild(BtnLabel);

	UHorizontalBoxSlot* BtnSlot = HBox->AddChildToHorizontalBox(AllocButton);
	BtnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	BtnSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
	BtnSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
	BtnSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));

	// Bind button click
	switch (RowIndex)
	{
	case 0: AllocButton->OnClicked.AddDynamic(this, &UAttributeAllocationWidget::OnAllocateStrength); break;
	case 1: AllocButton->OnClicked.AddDynamic(this, &UAttributeAllocationWidget::OnAllocateDexterity); break;
	case 2: AllocButton->OnClicked.AddDynamic(this, &UAttributeAllocationWidget::OnAllocateIntelligence); break;
	}

	// Add row to container
	UVerticalBoxSlot* RowSlot = AllocationContainer->AddChildToVerticalBox(HBox);
	RowSlot->SetPadding(FMargin(8.f, 4.f));

	// Track
	FAttributeRow& Row = Rows.AddDefaulted_GetRef();
	Row.StatName = StatName;
	Row.NameText = NameBlock;
	Row.CurrentValueText = ValueBlock;
	Row.AllocatedText = AllocBlock;
	Row.PreviewText = PreviewBlock;
	Row.AllocateButton = AllocButton;
	Row.ButtonLabel = BtnLabel;
	Row.Attribute = PrimaryAttribute;
	Row.DerivedAttribute = DerivedAttribute;
}

// ---------------------------------------------------------------------------
// Refresh
// ---------------------------------------------------------------------------

void UAttributeAllocationWidget::RefreshAll()
{
	UpdateUnspentText();
	for (int32 i = 0; i < Rows.Num(); ++i)
	{
		RefreshRow(i);
	}
}

void UAttributeAllocationWidget::RefreshRow(int32 RowIndex)
{
	if (!Rows.IsValidIndex(RowIndex)) return;

	FAttributeRow& Row = Rows[RowIndex];
	AARPGPlayerCharacter* Player = BoundPlayer.Get();
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!Player || !ASC) return;

	// Get current attribute values
	bool bFound = false;
	float PrimaryValue = ASC->GetGameplayAttributeValue(Row.Attribute, bFound);
	float DerivedValue = ASC->GetGameplayAttributeValue(Row.DerivedAttribute, bFound);

	// Get allocated count and preview info per stat
	int32 Allocated = 0;
	FString PreviewStr;
	FString DerivedLabel;

	if (RowIndex == 0) // Strength
	{
		Allocated = Player->GetAllocatedStrength();
		PreviewStr = FString::Printf(TEXT("Next: +%.0f AP"), Player->GetAttackPowerPerStrength());
		DerivedLabel = FString::Printf(TEXT("%.0f  (AP: %.0f)"), PrimaryValue, DerivedValue);
	}
	else if (RowIndex == 1) // Dexterity
	{
		Allocated = Player->GetAllocatedDexterity();
		PreviewStr = FString::Printf(TEXT("Next: +%.1f%% Crit"), Player->GetCritChancePerDexterity() * 100.f);
		DerivedLabel = FString::Printf(TEXT("%.0f  (Crit: %.1f%%)"), PrimaryValue, DerivedValue * 100.f);
	}
	else if (RowIndex == 2) // Intelligence
	{
		Allocated = Player->GetAllocatedIntelligence();
		PreviewStr = FString::Printf(TEXT("Next: +%.0f Mana"), Player->GetMaxManaPerIntelligence());
		DerivedLabel = FString::Printf(TEXT("%.0f  (MaxMP: %.0f)"), PrimaryValue, DerivedValue);
	}

	// Update text blocks
	Row.CurrentValueText->SetText(FText::FromString(DerivedLabel));
	Row.AllocatedText->SetText(FText::FromString(FString::Printf(TEXT("[%d pts]"), Allocated)));

	// Preview and button state
	bool bCanAllocate = Player->GetUnspentAttributePoints() > 0;
	if (bCanAllocate)
	{
		Row.PreviewText->SetText(FText::FromString(PreviewStr));
		Row.PreviewText->SetColorAndOpacity(FSlateColor(PreviewColor));
		Row.PreviewText->SetVisibility(ESlateVisibility::HitTestInvisible);
		Row.AllocateButton->SetIsEnabled(true);
		Row.ButtonLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	}
	else
	{
		Row.PreviewText->SetVisibility(ESlateVisibility::Collapsed);
		Row.AllocateButton->SetIsEnabled(false);
		Row.ButtonLabel->SetColorAndOpacity(FSlateColor(DisabledColor));
	}
}

void UAttributeAllocationWidget::UpdateUnspentText()
{
	if (!UnspentPointsText) return;

	AARPGPlayerCharacter* Player = BoundPlayer.Get();
	int32 Points = Player ? Player->GetUnspentAttributePoints() : 0;

	UnspentPointsText->SetText(FText::FromString(
		FString::Printf(TEXT("Unspent Attribute Points: %d"), Points)));
	UnspentPointsText->SetColorAndOpacity(FSlateColor(Points > 0 ? HeaderColor : DisabledColor));
}

// ---------------------------------------------------------------------------
// Button callbacks
// ---------------------------------------------------------------------------

void UAttributeAllocationWidget::OnAllocateStrength()
{
	if (AARPGPlayerCharacter* Player = BoundPlayer.Get())
	{
		Player->AllocateStrength();
		RefreshAll();
	}
}

void UAttributeAllocationWidget::OnAllocateDexterity()
{
	if (AARPGPlayerCharacter* Player = BoundPlayer.Get())
	{
		Player->AllocateDexterity();
		RefreshAll();
	}
}

void UAttributeAllocationWidget::OnAllocateIntelligence()
{
	if (AARPGPlayerCharacter* Player = BoundPlayer.Get())
	{
		Player->AllocateIntelligence();
		RefreshAll();
	}
}

// ---------------------------------------------------------------------------
// GAS callbacks
// ---------------------------------------------------------------------------

void UAttributeAllocationWidget::OnAttributeChanged(const FOnAttributeChangeData& Data)
{
	// Refresh the matching row
	for (int32 i = 0; i < Rows.Num(); ++i)
	{
		if (Rows[i].Attribute == Data.Attribute || Rows[i].DerivedAttribute == Data.Attribute)
		{
			RefreshRow(i);
			break;
		}
	}
}

void UAttributeAllocationWidget::OnAttributePointsChanged(int32 NewPoints)
{
	RefreshAll();
}
