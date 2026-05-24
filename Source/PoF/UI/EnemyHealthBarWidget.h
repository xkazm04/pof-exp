#pragma once

#include "CoreMinimal.h"
#include "UI/ARPGCodeWidgetBase.h"
#include "GameplayEffectTypes.h"
#include "EnemyHealthBarWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UAbilitySystemComponent;

/**
 * Floating health bar widget for enemies. Binds to GAS attribute change
 * delegates for Health/MaxHealth. Fades in on damage, fades out after a
 * configurable idle period, and hides on death.
 */
UCLASS()
class POF_API UEnemyHealthBarWidget : public UARPGCodeWidgetBase
{
	GENERATED_BODY()

public:
	/** Bind to the enemy's ASC. Call once after ASC is initialized. */
	UFUNCTION(BlueprintCallable, Category = "UI|EnemyHealthBar")
	void BindToAbilitySystem(UAbilitySystemComponent* ASC);

	/** Set the display name and level shown above the health bar. */
	UFUNCTION(BlueprintCallable, Category = "UI|EnemyHealthBar")
	void SetEnemyInfo(const FText& InName, int32 InLevel);

	/** Update the level text color based on level difference with the player. */
	UFUNCTION(BlueprintCallable, Category = "UI|EnemyHealthBar")
	void UpdateLevelColor(int32 PlayerLevel);

	/** Force-hide the widget (e.g., on death). Clears all fade state. */
	UFUNCTION(BlueprintCallable, Category = "UI|EnemyHealthBar")
	void HideForDeath();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	/** Build the floating-bar tree in C++ (no companion Widget Blueprint). */
	virtual void BuildTree() override;

	// --- Widgets (constructed in BuildTree) ---

	UPROPERTY()
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY()
	TObjectPtr<UTextBlock> EnemyNameText;

	UPROPERTY()
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY()
	TObjectPtr<UTextBlock> HealthText;

	// --- Tuning ---

	/** Bar interpolation speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyHealthBar|Interpolation")
	float BarInterpSpeed = 10.f;

	/** Seconds after last damage before the bar fades out. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyHealthBar|Fade")
	float FadeOutDelay = 3.f;

	/** Duration of the fade-in animation (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyHealthBar|Fade")
	float FadeInDuration = 0.2f;

	/** Duration of the fade-out animation (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyHealthBar|Fade")
	float FadeOutDuration = 0.5f;

	/** Health bar fill color. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyHealthBar|Colors")
	FLinearColor BarColor = FLinearColor(0.8f, 0.1f, 0.1f, 1.f);

	/** Level text color when enemy is much higher level than player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyHealthBar|Colors")
	FLinearColor DangerLevelColor = FLinearColor(1.f, 0.1f, 0.1f, 1.f);

	/** Level text color when enemy is similar level to player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyHealthBar|Colors")
	FLinearColor NormalLevelColor = FLinearColor(1.f, 1.f, 1.f, 1.f);

	/** Level text color when enemy is much lower level than player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyHealthBar|Colors")
	FLinearColor TrivialLevelColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.f);

private:
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void OnMaxHealthChanged(const FOnAttributeChangeData& Data);

	void UpdateHealthDisplay();
	void UnbindFromAbilitySystem();

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;

	float CurrentHealth = 0.f;
	float CurrentMaxHealth = 1.f;
	float DisplayedPercent = 1.f;
	int32 EnemyLevel = 1;

	// Fade state
	enum class EFadeState : uint8 { Hidden, FadingIn, Visible, FadingOut };
	EFadeState FadeState = EFadeState::Hidden;
	float FadeAlpha = 0.f;
	float TimeSinceLastDamage = 0.f;
	bool bDead = false;
};
