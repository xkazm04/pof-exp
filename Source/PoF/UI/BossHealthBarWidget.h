#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BossHealthBarWidget.generated.h"

class AARPGBossCharacter;
class UProgressBar;
class UTextBlock;
class UHorizontalBox;

/**
 * Boss health bar widget shown at the top of screen during boss encounters.
 * Displays boss name, health bar, and current phase indicator.
 */
UCLASS()
class POF_API UBossHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UI|Boss")
	void BindToBoss(AARPGBossCharacter* InBoss);

	UFUNCTION(BlueprintCallable, Category = "UI|Boss")
	void UnbindFromBoss();

	UFUNCTION(BlueprintPure, Category = "UI|Boss")
	AARPGBossCharacter* GetBoundBoss() const { return BoundBoss.Get(); }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	TWeakObjectPtr<AARPGBossCharacter> BoundBoss;

	UPROPERTY()
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY()
	TObjectPtr<UTextBlock> BossNameText;

	UPROPERTY()
	TObjectPtr<UTextBlock> PhaseText;

	UPROPERTY()
	TObjectPtr<UTextBlock> HealthPercentText;

	void BuildWidget();
	void UpdateDisplay();

	UFUNCTION()
	void OnPhaseChanged(int32 NewPhase, int32 TotalPhases);
};
