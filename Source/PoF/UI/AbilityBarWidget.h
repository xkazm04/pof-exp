#pragma once

#include "CoreMinimal.h"
#include "UI/ARPGCodeWidgetBase.h"
#include "GameplayTagContainer.h"
#include "AbilityBarWidget.generated.h"

class AARPGPlayerCharacter;
class UAbilitySlotWidget;
class UAbilitySystemComponent;
class UHorizontalBox;
class UTexture2D;

/**
 * Data describing a single ability slot.
 * Configured on the AbilityBarWidget and used to populate each slot.
 */
USTRUCT(BlueprintType)
struct FAbilitySlotData
{
	GENERATED_BODY()

	/** Icon texture for this ability. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AbilitySlot")
	TObjectPtr<UTexture2D> Icon = nullptr;

	/** Mana cost displayed on the slot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AbilitySlot")
	float ManaCost = 0.f;

	/** Keybind label (e.g., "1", "2", "Q"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AbilitySlot")
	FText KeybindLabel;

	/**
	 * Cooldown tag that the ability's cooldown GE grants.
	 * Used to query the ASC for remaining cooldown duration.
	 * Example: "Cooldown.Ability.Fireball"
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AbilitySlot")
	FGameplayTag CooldownTag;
};

/**
 * Ability bar widget containing 4 ability slots.
 * Polls the player's ASC each tick for cooldown state.
 */
UCLASS()
class POF_API UAbilityBarWidget : public UARPGCodeWidgetBase
{
	GENERATED_BODY()

public:
	/** Bind to the player's ASC. Call once after the ASC is available. */
	UFUNCTION(BlueprintCallable, Category = "UI|AbilityBar")
	void BindToAbilitySystem(UAbilitySystemComponent* ASC);

	/** Update a slot's data at runtime (e.g., when equipping a new ability). */
	UFUNCTION(BlueprintCallable, Category = "UI|AbilityBar")
	void SetSlotData(int32 SlotIndex, const FAbilitySlotData& Data);

	/**
	 * Refresh all slots from the player's loadout.
	 * Reads ability icon, mana cost, and cooldown tag from each ability CDO.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI|AbilityBar")
	void RefreshFromLoadout(AARPGPlayerCharacter* Player);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Build the bottom-centre slot container in C++ (no companion Widget Blueprint). */
	virtual void BuildTree() override;

	/** The widget class to instantiate for each slot. Must subclass UAbilitySlotWidget. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|AbilityBar")
	TSubclassOf<UAbilitySlotWidget> SlotWidgetClass;

	/** Data for each slot. Array size determines slot count (default 4). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|AbilityBar")
	TArray<FAbilitySlotData> Slots;

	/** Container that holds the slot widgets (built in BuildTree). */
	UPROPERTY()
	TObjectPtr<UHorizontalBox> SlotContainer;

private:
	UPROPERTY()
	TArray<TObjectPtr<UAbilitySlotWidget>> SlotWidgets;

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;

	void CreateSlotWidgets();
	void UpdateCooldowns();
};
