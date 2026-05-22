#include "UI/ARPGMainHUDWidget.h"
#include "UI/AbilityBarWidget.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "Player/ARPGPlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"

void UARPGMainHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Apply the static fill colors once. The health bar may be recolored each
	// tick by the low-health pulse; mana stays constant.
	if (HealthBar)
	{
		HealthBar->SetFillColorAndOpacity(HealthBarColor);
	}
	if (ManaBar)
	{
		ManaBar->SetFillColorAndOpacity(ManaBarColor);
	}

	if (ZoneNameText && ZoneNameText->GetText().IsEmpty())
	{
		ZoneNameText->SetText(NSLOCTEXT("ARPGHUD", "UnknownZone", "Unknown Zone"));
	}
}

void UARPGMainHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const float TargetHealthPercent = CurrentMaxHealth > 0.f ? CurrentHealth / CurrentMaxHealth : 0.f;
	const float TargetManaPercent = CurrentMaxMana > 0.f ? CurrentMana / CurrentMaxMana : 0.f;

	// Smoothly interpolate the displayed bar values so big hits/heals ease in.
	DisplayedHealthPercent = FMath::FInterpTo(DisplayedHealthPercent, TargetHealthPercent, InDeltaTime, BarInterpSpeed);
	DisplayedManaPercent = FMath::FInterpTo(DisplayedManaPercent, TargetManaPercent, InDeltaTime, BarInterpSpeed);

	if (HealthBar)
	{
		HealthBar->SetPercent(DisplayedHealthPercent);
	}
	if (ManaBar)
	{
		ManaBar->SetPercent(DisplayedManaPercent);
	}

	// Low-health pulse: oscillate the health bar color toward LowHealthColor.
	if (HealthBar)
	{
		if (TargetHealthPercent > 0.f && TargetHealthPercent < LowHealthThreshold)
		{
			PulseTime += InDeltaTime;
			const float Alpha = (FMath::Sin(PulseTime * LowHealthPulseSpeed * 2.f * PI) + 1.f) * 0.5f;
			HealthBar->SetFillColorAndOpacity(FMath::Lerp(HealthBarColor, LowHealthColor, Alpha));
		}
		else
		{
			PulseTime = 0.f;
			HealthBar->SetFillColorAndOpacity(HealthBarColor);
		}
	}
}

void UARPGMainHUDWidget::NativeDestruct()
{
	UnbindFromAbilitySystem();
	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------

void UARPGMainHUDWidget::InitializeForPlayer(AARPGPlayerCharacter* Player)
{
	if (!Player)
	{
		return;
	}

	UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	if (ASC)
	{
		BindToAbilitySystem(ASC);

		// Wire the embedded hotbar to the same ASC for cooldown polling.
		if (AbilityHotbar)
		{
			AbilityHotbar->BindToAbilitySystem(ASC);
		}
	}

	// Populate the hotbar slots from the player's current ability loadout.
	if (AbilityHotbar)
	{
		AbilityHotbar->RefreshFromLoadout(Player);
	}
}

void UARPGMainHUDWidget::BindToAbilitySystem(UAbilitySystemComponent* ASC)
{
	// Drop any previous binding so we never double-register callbacks.
	UnbindFromAbilitySystem();

	if (!ASC)
	{
		return;
	}

	BoundASC = ASC;

	ASC->GetGameplayAttributeValueChangeDelegate(UARPGAttributeSet::GetHealthAttribute())
		.AddUObject(this, &UARPGMainHUDWidget::OnHealthChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(UARPGAttributeSet::GetMaxHealthAttribute())
		.AddUObject(this, &UARPGMainHUDWidget::OnMaxHealthChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(UARPGAttributeSet::GetManaAttribute())
		.AddUObject(this, &UARPGMainHUDWidget::OnManaChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(UARPGAttributeSet::GetMaxManaAttribute())
		.AddUObject(this, &UARPGMainHUDWidget::OnMaxManaChanged);

	// Seed current values so the HUD is correct before the first effect fires.
	bool bFound = false;
	CurrentHealth = ASC->GetGameplayAttributeValue(UARPGAttributeSet::GetHealthAttribute(), bFound);
	if (!bFound) { CurrentHealth = 0.f; }

	CurrentMaxHealth = ASC->GetGameplayAttributeValue(UARPGAttributeSet::GetMaxHealthAttribute(), bFound);
	if (!bFound) { CurrentMaxHealth = 1.f; }

	CurrentMana = ASC->GetGameplayAttributeValue(UARPGAttributeSet::GetManaAttribute(), bFound);
	if (!bFound) { CurrentMana = 0.f; }

	CurrentMaxMana = ASC->GetGameplayAttributeValue(UARPGAttributeSet::GetMaxManaAttribute(), bFound);
	if (!bFound) { CurrentMaxMana = 1.f; }

	CurrentMaxHealth = FMath::Max(CurrentMaxHealth, 1.f);
	CurrentMaxMana = FMath::Max(CurrentMaxMana, 1.f);

	// Snap the bars on init — no interpolation from a stale value.
	DisplayedHealthPercent = CurrentHealth / CurrentMaxHealth;
	DisplayedManaPercent = CurrentMana / CurrentMaxMana;

	RefreshHealthText();
	RefreshManaText();
}

void UARPGMainHUDWidget::SetZoneName(const FText& InZoneName)
{
	if (ZoneNameText)
	{
		ZoneNameText->SetText(InZoneName);
	}
}

// ---------------------------------------------------------------------------
// GAS attribute change callbacks
// ---------------------------------------------------------------------------

void UARPGMainHUDWidget::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	CurrentHealth = Data.NewValue;
	RefreshHealthText();
}

void UARPGMainHUDWidget::OnMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	CurrentMaxHealth = FMath::Max(Data.NewValue, 1.f);
	RefreshHealthText();
}

void UARPGMainHUDWidget::OnManaChanged(const FOnAttributeChangeData& Data)
{
	CurrentMana = Data.NewValue;
	RefreshManaText();
}

void UARPGMainHUDWidget::OnMaxManaChanged(const FOnAttributeChangeData& Data)
{
	CurrentMaxMana = FMath::Max(Data.NewValue, 1.f);
	RefreshManaText();
}

// ---------------------------------------------------------------------------
// Display helpers
// ---------------------------------------------------------------------------

void UARPGMainHUDWidget::RefreshHealthText()
{
	if (HealthText)
	{
		HealthText->SetText(FText::FromString(
			FString::Printf(TEXT("%.0f / %.0f"), CurrentHealth, CurrentMaxHealth)));
	}
}

void UARPGMainHUDWidget::RefreshManaText()
{
	if (ManaText)
	{
		ManaText->SetText(FText::FromString(
			FString::Printf(TEXT("%.0f / %.0f"), CurrentMana, CurrentMaxMana)));
	}
}

void UARPGMainHUDWidget::UnbindFromAbilitySystem()
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC)
	{
		BoundASC = nullptr;
		return;
	}

	ASC->GetGameplayAttributeValueChangeDelegate(UARPGAttributeSet::GetHealthAttribute()).RemoveAll(this);
	ASC->GetGameplayAttributeValueChangeDelegate(UARPGAttributeSet::GetMaxHealthAttribute()).RemoveAll(this);
	ASC->GetGameplayAttributeValueChangeDelegate(UARPGAttributeSet::GetManaAttribute()).RemoveAll(this);
	ASC->GetGameplayAttributeValueChangeDelegate(UARPGAttributeSet::GetMaxManaAttribute()).RemoveAll(this);

	BoundASC = nullptr;
}
