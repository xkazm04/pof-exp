#include "UI/AbilityBarWidget.h"
#include "UI/AbilitySlotWidget.h"
#include "Player/ARPGPlayerCharacter.h"
#include "AbilitySystem/ARPGGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"

void UAbilityBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Default 4 slots with keybind labels 1-4 if not configured
	if (Slots.Num() == 0)
	{
		Slots.SetNum(4);
		for (int32 i = 0; i < 4; ++i)
		{
			Slots[i].KeybindLabel = FText::FromString(FString::FromInt(i + 1));
		}
	}

	CreateSlotWidgets();
}

void UAbilityBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateCooldowns();
}

void UAbilityBarWidget::BindToAbilitySystem(UAbilitySystemComponent* ASC)
{
	BoundASC = ASC;
}

void UAbilityBarWidget::SetSlotData(int32 SlotIndex, const FAbilitySlotData& Data)
{
	if (!Slots.IsValidIndex(SlotIndex)) return;

	Slots[SlotIndex] = Data;

	if (SlotWidgets.IsValidIndex(SlotIndex) && SlotWidgets[SlotIndex])
	{
		SlotWidgets[SlotIndex]->SetIcon(Data.Icon);
		SlotWidgets[SlotIndex]->SetManaCost(Data.ManaCost);
		SlotWidgets[SlotIndex]->SetKeybindLabel(Data.KeybindLabel);
	}
}

void UAbilityBarWidget::CreateSlotWidgets()
{
	if (!SlotContainer || !SlotWidgetClass) return;

	SlotContainer->ClearChildren();
	SlotWidgets.Empty();

	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		UAbilitySlotWidget* SlotWidget = CreateWidget<UAbilitySlotWidget>(this, SlotWidgetClass);
		if (!SlotWidget) continue;

		UHorizontalBoxSlot* BoxSlot = SlotContainer->AddChildToHorizontalBox(SlotWidget);
		if (BoxSlot)
		{
			BoxSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
			BoxSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
		}

		SlotWidget->SetIcon(Slots[i].Icon);
		SlotWidget->SetManaCost(Slots[i].ManaCost);
		SlotWidget->SetKeybindLabel(Slots[i].KeybindLabel);
		SlotWidget->SetCooldownPercent(0.f, 0.f);

		SlotWidgets.Add(SlotWidget);
	}
}

void UAbilityBarWidget::UpdateCooldowns()
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC) return;

	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		if (!SlotWidgets.IsValidIndex(i) || !SlotWidgets[i]) continue;
		if (!Slots[i].CooldownTag.IsValid())
		{
			SlotWidgets[i]->SetCooldownPercent(0.f, 0.f);
			continue;
		}

		// Query the ASC for active effects that grant this cooldown tag
		float TimeRemaining = 0.f;
		float TotalDuration = 0.f;

		FGameplayTagContainer CooldownTags;
		CooldownTags.AddTag(Slots[i].CooldownTag);

		TArray<float> Durations = ASC->GetActiveEffectsTimeRemaining(
			FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(CooldownTags));

		if (Durations.Num() > 0)
		{
			// Take the longest remaining cooldown
			for (float D : Durations)
			{
				if (D > TimeRemaining) TimeRemaining = D;
			}

			// Get the original duration
			TArray<float> TotalDurations = ASC->GetActiveEffectsDuration(
				FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(CooldownTags));
			for (float D : TotalDurations)
			{
				if (D > TotalDuration) TotalDuration = D;
			}
		}

		const float Percent = TotalDuration > 0.f ? TimeRemaining / TotalDuration : 0.f;
		SlotWidgets[i]->SetCooldownPercent(Percent, TimeRemaining);
	}
}

void UAbilityBarWidget::RefreshFromLoadout(AARPGPlayerCharacter* Player)
{
	if (!Player) return;

	const int32 SlotCount = Player->GetAbilitySlotCount();

	for (int32 i = 0; i < SlotCount && i < Slots.Num(); ++i)
	{
		TSubclassOf<UGameplayAbility> AbilityClass = Player->GetSlotAbility(i);

		FAbilitySlotData Data;
		Data.KeybindLabel = FText::FromString(FString::FromInt(i + 1));

		if (AbilityClass)
		{
			// Read metadata from the ability CDO
			if (const UARPGGameplayAbility* AbilityCDO = AbilityClass->GetDefaultObject<UARPGGameplayAbility>())
			{
				Data.Icon = AbilityCDO->GetAbilityIcon();
				Data.ManaCost = AbilityCDO->GetAbilityManaCost();
				Data.CooldownTag = AbilityCDO->GetAbilityCooldownTag();
			}
		}

		SetSlotData(i, Data);
	}
}
