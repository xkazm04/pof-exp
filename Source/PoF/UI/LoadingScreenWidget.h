#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LoadingScreenWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UImage;

/**
 * Loading screen widget displayed during async level transitions.
 * Shows a progress bar, loading tip, and optional background image.
 * Managed by the GameInstance during TravelToMap.
 */
UCLASS()
class POF_API ULoadingScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Set the progress bar value (0.0 to 1.0). */
	UFUNCTION(BlueprintCallable, Category = "UI|Loading")
	void SetProgress(float Progress);

	/** Set the status text (e.g., "Loading World..."). */
	UFUNCTION(BlueprintCallable, Category = "UI|Loading")
	void SetStatusText(const FText& InText);

	/** Display a random loading tip from the configured list. */
	UFUNCTION(BlueprintCallable, Category = "UI|Loading")
	void ShowRandomTip();

	/** Show the loading screen with a fade-in. */
	UFUNCTION(BlueprintCallable, Category = "UI|Loading")
	void Show();

	/** Begin fading out the loading screen. Removes from parent when done. */
	UFUNCTION(BlueprintCallable, Category = "UI|Loading")
	void Hide();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> LoadingProgressBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TipText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> BackgroundImage;

	/** Loading tips displayed randomly during load screens. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loading|Tips")
	TArray<FText> LoadingTips;

	/** Fade-in duration in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loading|Fade")
	float FadeInDuration = 0.3f;

	/** Fade-out duration in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loading|Fade")
	float FadeOutDuration = 0.5f;

	/** Progress bar fill color. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loading|Colors")
	FLinearColor ProgressBarColor = FLinearColor(0.2f, 0.6f, 1.f, 1.f);

private:
	enum class ELoadingFadeState : uint8 { Idle, FadingIn, Visible, FadingOut };
	ELoadingFadeState FadeState = ELoadingFadeState::Idle;
	float FadeAlpha = 0.f;
};
