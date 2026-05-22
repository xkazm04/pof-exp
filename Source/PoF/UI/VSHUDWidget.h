#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "VSHUDWidget.generated.h"

struct FOnAttributeChangeData;

/**
 * Vertical-slice combat HUD widget. Builds its own widget tree in C++ (no
 * BindWidget, no companion Widget Blueprint) — see UBossHealthBarWidget.
 * A player health bar (top-left) and a target/enemy health bar (top-centre),
 * each bound to a GAS AbilitySystemComponent's Health attribute.
 */
UCLASS()
class POF_API UVSHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Bind the player health bar to a player ASC. */
	void BindPlayer(class UAbilitySystemComponent* ASC);
	/** Bind the target health bar to an enemy ASC; label it with EnemyName. */
	void BindEnemy(class UAbilitySystemComponent* ASC, const FString& EnemyName);

protected:
	virtual void NativeConstruct() override;

private:
	void OnPlayerHealthChanged(const FOnAttributeChangeData& Data);
	void OnPlayerMaxHealthChanged(const FOnAttributeChangeData& Data);
	void OnEnemyHealthChanged(const FOnAttributeChangeData& Data);
	void OnEnemyMaxHealthChanged(const FOnAttributeChangeData& Data);
	void RefreshPlayerBar();
	void RefreshEnemyBar();

	UPROPERTY() class UProgressBar* PlayerHealthBar = nullptr;
	UPROPERTY() class UTextBlock*   PlayerHealthText = nullptr;
	UPROPERTY() class UTextBlock*   EnemyNameText = nullptr;
	UPROPERTY() class UProgressBar* EnemyHealthBar = nullptr;
	UPROPERTY() class UTextBlock*   EnemyHealthText = nullptr;

	float PlayerHealth = 0.f, PlayerMaxHealth = 1.f;
	float EnemyHealth  = 0.f, EnemyMaxHealth  = 1.f;
	TWeakObjectPtr<class UAbilitySystemComponent> PlayerASC;
	TWeakObjectPtr<class UAbilitySystemComponent> EnemyASC;
};
