#include "Test/HUD/ARPGHUDWidgetTest.h"
#include "UI/ARPGHUDWidget.h"
#include "UI/AbilityBarWidget.h"
#include "UI/ARPGCodeWidgetBase.h"
#include "Player/ARPGPlayerCharacter.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "AbilitySystemComponent.h"

AARPGHUDWidgetTest::AARPGHUDWidgetTest()
{
	TimeLimit = 30.f;
	Phases = { TEXT("HUDStructure"), TEXT("Hotbar"), TEXT("HitVignette"), TEXT("HUDBinding") };
}

void AARPGHUDWidgetTest::OnTestStarted()
{
	// Build the reparented real HUD exactly as AARPGHUD does (pure C++, no WBP).
	HUD = CreateWidget<UARPGHUDWidget>(GetWorld(), UARPGHUDWidget::StaticClass());
	if (HUD)
	{
		HUD->AddToViewport(UARPGCodeWidgetBase::DefaultHUDZOrder);
	}

	Bar = CreateWidget<UAbilityBarWidget>(GetWorld(), UAbilityBarWidget::StaticClass());
	if (Bar)
	{
		Bar->AddToViewport(UARPGCodeWidgetBase::DefaultHUDZOrder);
	}
}

EARPGPhaseResult AARPGHUDWidgetTest::RunPhase(int32 /*PhaseIndex*/, FName PhaseName, float DeltaSeconds)
{
	if (PhaseName == TEXT("HUDStructure"))
	{
		if (!AssertTrue(HUD != nullptr, TEXT("HUD widget created")))
		{
			return EARPGPhaseResult::Fail;
		}
		AssertTrue(HUD->GetWidgetFromName(TEXT("HealthBar"))   != nullptr, TEXT("HealthBar exists"));
		AssertTrue(HUD->GetWidgetFromName(TEXT("ManaBar"))     != nullptr, TEXT("ManaBar exists"));
		AssertTrue(HUD->GetWidgetFromName(TEXT("StaminaBar"))  != nullptr, TEXT("StaminaBar exists"));
		AssertTrue(HUD->GetWidgetFromName(TEXT("XPBar"))       != nullptr, TEXT("XPBar exists"));
		AssertTrue(HUD->GetWidgetFromName(TEXT("LevelText"))   != nullptr, TEXT("LevelText exists"));
		AssertTrue(HUD->GetWidgetFromName(TEXT("HitVignette")) != nullptr, TEXT("HitVignette exists"));
		return EARPGPhaseResult::Advance;
	}

	if (PhaseName == TEXT("Hotbar"))
	{
		if (!AssertTrue(Bar != nullptr, TEXT("ability bar created")))
		{
			return EARPGPhaseResult::Fail;
		}
		if (IsFirstTickOfPhase(DeltaSeconds))
		{
			// Drives off the player's AbilityLoadout; slot widgets are created in
			// NativeConstruct regardless, so the container is non-empty either way.
			Bar->RefreshFromLoadout(GetPlayerCharacter());
		}
		UPanelWidget* Container = Cast<UPanelWidget>(Bar->GetWidgetFromName(TEXT("SlotContainer")));
		const EARPGWait W = WaitForCondition(
			[Container]() { return Container != nullptr && Container->GetChildrenCount() > 0; }, 2.f);
		if (W == EARPGWait::Satisfied)
		{
			AssertTrue(true, TEXT("hotbar produced slot widgets"));
			return EARPGPhaseResult::Advance;
		}
		if (W == EARPGWait::TimedOut)
		{
			AssertTrue(false, TEXT("hotbar produced no slots within 2s"));
			return EARPGPhaseResult::Fail;
		}
		return EARPGPhaseResult::Running;
	}

	if (PhaseName == TEXT("HitVignette"))
	{
		if (IsFirstTickOfPhase(DeltaSeconds))
		{
			if (UAbilitySystemComponent* ASC = GetPlayerASC())
			{
				HUD->BindToAbilitySystem(ASC);
			}
			ApplyDamage(GetPlayerCharacter(), 10.f); // non-lethal
		}
		UImage* Vignette = Cast<UImage>(HUD->GetWidgetFromName(TEXT("HitVignette")));
		const EARPGWait W = WaitForCondition(
			[Vignette]() { return Vignette != nullptr && Vignette->GetRenderOpacity() > 0.01f; }, 1.5f);
		if (W == EARPGWait::Satisfied)
		{
			AssertTrue(true, TEXT("hit vignette flashed on player damage"));
			return EARPGPhaseResult::Advance;
		}
		if (W == EARPGWait::TimedOut)
		{
			AssertTrue(false, TEXT("hit vignette did not flash within 1.5s"));
			return EARPGPhaseResult::Fail;
		}
		return EARPGPhaseResult::Running;
	}

	if (PhaseName == TEXT("HUDBinding"))
	{
		if (IsFirstTickOfPhase(DeltaSeconds))
		{
			ApplyDamage(GetPlayerCharacter(), 15.f); // non-lethal
		}
		UProgressBar* HealthBar = Cast<UProgressBar>(HUD->GetWidgetFromName(TEXT("HealthBar")));
		const EARPGWait W = WaitForCondition(
			[HealthBar]() { return HealthBar != nullptr && HealthBar->GetPercent() < 0.999f; }, 3.f);
		if (W == EARPGWait::Satisfied)
		{
			AssertTrue(true, TEXT("player health bar updated on damage"));
			return EARPGPhaseResult::Advance;
		}
		if (W == EARPGWait::TimedOut)
		{
			AssertTrue(false, TEXT("player health bar did not update within 3s"));
			return EARPGPhaseResult::Fail;
		}
		return EARPGPhaseResult::Running;
	}

	return EARPGPhaseResult::Advance;
}
