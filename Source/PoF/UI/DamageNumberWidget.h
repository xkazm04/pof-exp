#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "DamageNumberWidget.generated.h"

class UTextBlock;

/**
 * Floating damage number widget. Spawned at a world location, animates
 * upward and fades out over its lifetime. Removes itself when done.
 */
UCLASS()
class POF_API UDamageNumberWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Initialize the damage number display.
	 * @param Amount       Absolute damage/heal value.
	 * @param bIsCrit      Whether this was a critical hit.
	 * @param bIsHeal      True for heals, false for damage.
	 * @param DamageType   Gameplay tag for damage element color.
	 * @param WorldLocation  World-space origin for the number.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI|DamageNumber")
	void InitDamageNumber(float Amount, bool bIsCrit, bool bIsHeal, FGameplayTag DamageType, FVector WorldLocation);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/**
	 * Builds a default UI tree (canvas + centered text) when no Blueprint
	 * subclass supplies one, so the widget is fully usable from pure C++.
	 */
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/**
	 * The text block displaying the number. Bound from a Blueprint subclass
	 * if one exists; otherwise created in RebuildWidget().
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DamageText;

	/** Total lifetime before the widget removes itself. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageNumber")
	float Lifetime = 1.0f;

	/** How far the number floats upward (in screen pixels) over its lifetime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageNumber")
	float FloatDistance = 80.f;

	/** Random horizontal spread range (pixels). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageNumber")
	float HorizontalSpread = 30.f;

	/** Normal damage font size. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageNumber")
	int32 NormalFontSize = 18;

	/** Crit damage font size. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageNumber")
	int32 CritFontSize = 26;

	// --- Colors ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageNumber|Colors")
	FLinearColor PhysicalColor = FLinearColor(1.f, 1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageNumber|Colors")
	FLinearColor FireColor = FLinearColor(1.f, 0.3f, 0.1f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageNumber|Colors")
	FLinearColor IceColor = FLinearColor(0.3f, 0.6f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageNumber|Colors")
	FLinearColor LightningColor = FLinearColor(1.f, 1.f, 0.2f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageNumber|Colors")
	FLinearColor HealColor = FLinearColor(0.2f, 1.f, 0.3f, 1.f);

private:
	float ElapsedTime = 0.f;
	FVector OriginWorldLocation = FVector::ZeroVector;
	float RandomOffsetX = 0.f;
	bool bInitialized = false;
	bool bCrit = false;
};
