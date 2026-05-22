#include "UI/AbilitySlotWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"

void UAbilitySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Create dynamic material for the sweep overlay
	if (CooldownSweep && CooldownSweepMaterial)
	{
		SweepMID = UMaterialInstanceDynamic::Create(CooldownSweepMaterial, this);
		CooldownSweep->SetBrushFromMaterial(SweepMID);
		SweepMID->SetScalarParameterValue(TEXT("Percent"), 0.f);
	}

	// Start with sweep hidden
	if (CooldownSweep)
	{
		CooldownSweep->SetRenderOpacity(0.f);
	}
	if (CooldownText)
	{
		CooldownText->SetText(FText::GetEmpty());
	}
}

void UAbilitySlotWidget::SetIcon(UTexture2D* InIcon)
{
	if (!AbilityIcon) return;

	if (InIcon)
	{
		AbilityIcon->SetBrushFromTexture(InIcon);
		AbilityIcon->SetRenderOpacity(1.f);
	}
	else
	{
		AbilityIcon->SetRenderOpacity(0.3f);
	}
}

void UAbilitySlotWidget::SetKeybindLabel(const FText& InLabel)
{
	if (KeybindLabel)
	{
		KeybindLabel->SetText(InLabel);
	}
}

void UAbilitySlotWidget::SetManaCost(float InCost)
{
	if (!ManaCostText) return;

	if (InCost > 0.f)
	{
		ManaCostText->SetText(FText::FromString(FString::Printf(TEXT("%.0f"), InCost)));
	}
	else
	{
		ManaCostText->SetText(FText::GetEmpty());
	}
}

void UAbilitySlotWidget::SetCooldownPercent(float Percent, float RemainingSeconds)
{
	const float Clamped = FMath::Clamp(Percent, 0.f, 1.f);

	if (SweepMID)
	{
		SweepMID->SetScalarParameterValue(TEXT("Percent"), Clamped);
	}

	if (CooldownSweep)
	{
		CooldownSweep->SetRenderOpacity(Clamped > 0.01f ? 1.f : 0.f);
	}

	if (CooldownText)
	{
		if (RemainingSeconds > 0.1f)
		{
			CooldownText->SetText(FText::FromString(
				FString::Printf(TEXT("%.1f"), RemainingSeconds)));
		}
		else
		{
			CooldownText->SetText(FText::GetEmpty());
		}
	}
}
