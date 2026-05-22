#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "DamageNumberManagerComponent.generated.h"

class UDamageNumberWidget;

/**
 * Spawns and manages floating damage number widgets. Attach to the
 * PlayerController. Automatically subscribes to the global static
 * delegate on UARPGAttributeSet so damage/heal numbers from ALL
 * characters (player + enemies) are displayed.
 */
UCLASS(ClassGroup = "UI", meta = (BlueprintSpawnableComponent))
class POF_API UDamageNumberManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDamageNumberManagerComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** The widget class to spawn for each damage number. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageNumbers")
	TSubclassOf<UDamageNumberWidget> DamageNumberWidgetClass;

	/** Max simultaneous damage numbers on screen. Oldest are removed first. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageNumbers")
	int32 MaxActiveNumbers = 20;

	/**
	 * Spawn a floating damage number at the given world location.
	 * Called automatically via the global delegate, but can also be called manually.
	 */
	UFUNCTION(BlueprintCallable, Category = "DamageNumbers")
	void SpawnDamageNumber(AActor* TargetActor, float DamageAmount, bool bIsCrit,
		bool bIsHeal, FGameplayTag DamageType, FVector HitLocation);

private:
	void HandleDamageNumber(AActor* TargetActor, float DamageAmount, bool bIsCrit,
		bool bIsHeal, FGameplayTag DamageType, FVector HitLocation);

	FDelegateHandle GlobalDelegateHandle;

	TArray<TWeakObjectPtr<UDamageNumberWidget>> ActiveNumbers;
};
