#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayEffectTypes.h"
#include "ARPGMainHUDWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UBorder;
class UAbilityBarWidget;
class UAbilitySystemComponent;
class AARPGPlayerCharacter;

/**
 * Main in-game HUD widget for the aRPG.
 *
 * Layout (anchors set up in the UMG designer):
 *   - Bottom-left  : health bar (red) + mana bar (blue), each with a "current / max" label.
 *   - Bottom-center: ability hotbar (reuses UAbilityBarWidget — 4 slots with cooldown overlays).
 *   - Top-right    : minimap placeholder (an empty UBorder designers fill in later).
 *   - Top-left     : zone name label.
 *
 * Health/mana bars bind directly to the GAS AttributeSet via
 * GetGameplayAttributeValueChangeDelegate(), so they update in real time as
 * gameplay effects modify the attributes. Bars interpolate smoothly toward
 * their target value each tick.
 */
UCLASS()
class POF_API UARPGMainHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * One-call setup: binds the vitals bars to the player's ASC, wires the
	 * ability hotbar to the same ASC, and refreshes the hotbar from the
	 * player's current loadout. Call once after the player's ASC is ready.
	 */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void InitializeForPlayer(AARPGPlayerCharacter* Player);

	/** Bind the health/mana bars to GAS attribute change delegates. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void BindToAbilitySystem(UAbilitySystemComponent* ASC);

	/** Set the zone name shown in the top-left corner. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetZoneName(const FText& InZoneName);

	/** Access the embedded ability hotbar (e.g. to push slot data). */
	UFUNCTION(BlueprintPure, Category = "HUD")
	UAbilityBarWidget* GetAbilityHotbar() const { return AbilityHotbar; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	// --- Bottom-left: vitals (bind in UMG designer by matching names) ---

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ManaBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ManaText;

	// --- Bottom-center: ability hotbar (4 slots with cooldown overlays) ---

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAbilityBarWidget> AbilityHotbar;

	// --- Top-right: minimap placeholder ---

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> MinimapPlaceholder;

	// --- Top-left: zone name ---

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ZoneNameText;

	// --- Tuning ---

	/** How fast the bars interpolate toward their target percentage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Interpolation")
	float BarInterpSpeed = 8.f;

	/** Health ratio below which the health bar pulses toward LowHealthColor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|LowHealth")
	float LowHealthThreshold = 0.25f;

	/** Low-health pulse rate, in full cycles per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|LowHealth")
	float LowHealthPulseSpeed = 2.f;

	/** Health bar fill color (red). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Colors")
	FLinearColor HealthBarColor = FLinearColor(0.85f, 0.12f, 0.12f, 1.f);

	/** Color the health bar pulses to while at low health. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Colors")
	FLinearColor LowHealthColor = FLinearColor(1.f, 0.85f, 0.2f, 1.f);

	/** Mana bar fill color (blue). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Colors")
	FLinearColor ManaBarColor = FLinearColor(0.15f, 0.35f, 0.95f, 1.f);

private:
	// GAS callbacks — signature required by GetGameplayAttributeValueChangeDelegate.
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void OnMaxHealthChanged(const FOnAttributeChangeData& Data);
	void OnManaChanged(const FOnAttributeChangeData& Data);
	void OnMaxManaChanged(const FOnAttributeChangeData& Data);

	void RefreshHealthText();
	void RefreshManaText();
	void UnbindFromAbilitySystem();

	/** ASC the vitals bars are currently bound to. */
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;

	// Latest attribute values, updated by GAS callbacks.
	float CurrentHealth = 0.f;
	float CurrentMaxHealth = 1.f;
	float CurrentMana = 0.f;
	float CurrentMaxMana = 1.f;

	// Displayed bar percentages, interpolated toward the target each tick.
	float DisplayedHealthPercent = 1.f;
	float DisplayedManaPercent = 1.f;

	/** Accumulator driving the low-health pulse. */
	float PulseTime = 0.f;
};
