#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NotificationWidget.generated.h"

class UVerticalBox;
class UTextBlock;

/**
 * Toast notification system. Displays stacked messages that auto-fade
 * after a configurable duration. Used for quest updates, loot pickups,
 * achievements, and general game feedback.
 */
UCLASS()
class POF_API UNotificationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Display a toast notification.
	 * @param Message  The text to display.
	 * @param Duration Seconds before the toast begins fading out.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI|Notification")
	void ShowToast(const FText& Message, float Duration = 3.f);

	/** Show a loot pickup notification with item name and optional rarity color. */
	UFUNCTION(BlueprintCallable, Category = "UI|Notification")
	void ShowLootPickup(const FText& ItemName, FLinearColor RarityColor, int32 Quantity = 1);

	/** Show a quest update notification. */
	UFUNCTION(BlueprintCallable, Category = "UI|Notification")
	void ShowQuestUpdate(const FText& QuestName, const FText& Status);

	/** Show an achievement notification. */
	UFUNCTION(BlueprintCallable, Category = "UI|Notification")
	void ShowAchievement(const FText& AchievementName);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Container for active toast entries (newest at the bottom). */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> ToastContainer;

	/** Maximum number of visible toasts. Oldest are removed when exceeded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notification")
	int32 MaxVisibleToasts = 5;

	/** Fade-out duration in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notification")
	float FadeOutDuration = 0.5f;

	/** Default toast text color. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notification|Colors")
	FLinearColor DefaultColor = FLinearColor(1.f, 1.f, 1.f, 1.f);

	/** Quest update text color. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notification|Colors")
	FLinearColor QuestColor = FLinearColor(1.f, 0.85f, 0.3f, 1.f);

	/** Achievement text color. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notification|Colors")
	FLinearColor AchievementColor = FLinearColor(1.f, 0.5f, 0.0f, 1.f);

	/** Font size for toast text. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notification")
	int32 ToastFontSize = 16;

private:
	struct FToastEntry
	{
		TObjectPtr<UTextBlock> TextWidget;
		float RemainingTime;
		float FadeTime;     // Time spent fading (0 until RemainingTime expires)
		bool bFading;
	};

	TArray<FToastEntry> ActiveToasts;

	void AddToast(const FText& Message, FLinearColor Color, float Duration);
};
