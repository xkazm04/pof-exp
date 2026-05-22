#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayEffectTypes.h"
#include "ARPGHUDWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UBorder;
class UAbilitySystemComponent;
class AARPGPlayerCharacter;
struct FGameplayAttribute;

/**
 * HUD widget that binds health/mana/stamina bars directly to GAS AttributeSet via
 * GetGameplayAttributeValueChangeDelegate(). Features smooth bar interpolation,
 * a pulsing red health bar when below 25%, XP bar, level display, and minimap placeholder.
 */
UCLASS()
class POF_API UARPGHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Call once after the owning player's ASC is available to bind attribute
	 * change delegates. Typically called from the HUD class.
	 */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void BindToAbilitySystem(UAbilitySystemComponent* ASC);

	/** Bind to the player character for stamina and XP updates (non-GAS attributes). */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void BindToPlayerCharacter(AARPGPlayerCharacter* Player);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	// --- Widget bindings (bind in UMG designer by matching names) ---

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ManaBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ManaText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> StaminaBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> XPBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LevelText;

	/** Minimap placeholder — empty border that designers fill in later. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> MinimapPlaceholder;

	// --- Tuning ---

	/** How fast bars interpolate toward their target value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Interpolation")
	float BarInterpSpeed = 8.f;

	/** Health percentage threshold below which the bar pulses red. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|LowHealth")
	float LowHealthThreshold = 0.25f;

	/** Speed of the low-health pulse (full cycles per second). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|LowHealth")
	float LowHealthPulseSpeed = 2.f;

	/** Normal health bar color. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Colors")
	FLinearColor HealthBarColor = FLinearColor(0.1f, 0.8f, 0.1f, 1.f);

	/** Color the health bar pulses to at low health. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Colors")
	FLinearColor LowHealthColor = FLinearColor(0.9f, 0.1f, 0.1f, 1.f);

	/** Normal mana bar color. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Colors")
	FLinearColor ManaBarColor = FLinearColor(0.1f, 0.2f, 0.9f, 1.f);

	/** Stamina bar color. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Colors")
	FLinearColor StaminaBarColor = FLinearColor(0.9f, 0.7f, 0.1f, 1.f);

	/** XP bar color. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Colors")
	FLinearColor XPBarColor = FLinearColor(0.6f, 0.2f, 0.9f, 1.f);

private:
	// GAS callback — signature required by GetGameplayAttributeValueChangeDelegate
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void OnMaxHealthChanged(const FOnAttributeChangeData& Data);
	void OnManaChanged(const FOnAttributeChangeData& Data);
	void OnMaxManaChanged(const FOnAttributeChangeData& Data);

	// Player character callbacks (non-GAS)
	UFUNCTION()
	void OnPlayerLevelUp(int32 NewLevel);

	// Cached references
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	TWeakObjectPtr<AARPGPlayerCharacter> BoundPlayer;

	// Current attribute values (updated by GAS callbacks)
	float CurrentHealth = 0.f;
	float CurrentMaxHealth = 1.f;
	float CurrentMana = 0.f;
	float CurrentMaxMana = 1.f;

	// Displayed bar percentages (interpolated toward target)
	float DisplayedHealthPercent = 1.f;
	float DisplayedManaPercent = 1.f;
	float DisplayedStaminaPercent = 1.f;
	float DisplayedXPPercent = 0.f;

	// Low-health pulse accumulator
	float PulseTime = 0.f;

	void UpdateHealthDisplay();
	void UpdateManaDisplay();
	void UpdateLevelDisplay();
	void UnbindFromAbilitySystem();
};
