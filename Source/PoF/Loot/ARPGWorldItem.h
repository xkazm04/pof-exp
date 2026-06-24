#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Inventory/ARPGItemDefinition.h"
#include "ARPGWorldItem.generated.h"

class UARPGItemInstance;
class UStaticMeshComponent;
class UPointLightComponent;
class USphereComponent;
class UWidgetComponent;
class USoundBase;
class UNiagaraSystem;

// --- Delegates for HUD/UI pickup notifications ---

/** Broadcast when an item is picked up: (ItemName, Rarity, Quantity) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnItemPickedUp, const FText&, ItemName, EARPGItemRarity, ItemRarity, int32, Quantity);

/** Static multicast so any HUD can listen without a per-actor reference */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnItemPickedUpStatic, const FText& /*ItemName*/, EARPGItemRarity /*Rarity*/, int32 /*Quantity*/);

/**
 * A physical item in the world — dropped from enemies or chests.
 * Shows the item's mesh, a rarity-colored light beam, and a nameplate.
 * Players can pick up via overlap (auto-pickup) or interact.
 */
UCLASS()
class POF_API AARPGWorldItem : public AActor
{
	GENERATED_BODY()

public:
	AARPGWorldItem();

	/** Initialize this world item with an item instance. Call after spawning.
	 *  @param bRouteToInventory  If true, this item routes pickups into the looter's
	 *         UARPGInventoryComponent (sets bSliceMode=false). Default false preserves
	 *         the vertical-slice VFX-and-destroy pickup for all existing callers. */
	UFUNCTION(BlueprintCallable, Category = "Loot")
	void InitFromItemInstance(UARPGItemInstance* InInstance, bool bRouteToInventory = false);

	/** Get the item instance this world item holds. */
	UFUNCTION(BlueprintPure, Category = "Loot")
	UARPGItemInstance* GetItemInstance() const { return ItemInstance; }

	/** Attempt pickup by the given actor. Returns true if successfully added to inventory. */
	UFUNCTION(BlueprintCallable, Category = "Loot")
	bool TryPickup(AActor* PickupActor);

	/** Launch the item with an initial velocity (for spawn pop-out arc). */
	UFUNCTION(BlueprintCallable, Category = "Loot")
	void LaunchItem(FVector LaunchVelocity);

	/** Per-actor delegate for Blueprint binding. */
	UPROPERTY(BlueprintAssignable, Category = "Events|Loot")
	FOnItemPickedUp OnItemPickedUp;

	/** Global static delegate — bind once from HUD to receive all pickup notifications. */
	static FOnItemPickedUpStatic OnAnyItemPickedUp;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ItemMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> RarityLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> PickupTrigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> NameplateWidget;

	/** The item instance held by this world item. */
	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UARPGItemInstance> ItemInstance;

	// =====================================================================
	// Pickup Settings
	// =====================================================================

	/** Pickup radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Pickup", meta = (ClampMin = "10"))
	float PickupRadius = 100.f;

	/** If true, common items are auto-picked up on overlap (no interact needed). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Pickup")
	bool bAutoPickupCommon = true;

	/** Maximum rarity tier for auto-pickup (inclusive). Items above this require interaction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Pickup")
	EARPGItemRarity AutoPickupMaxRarity = EARPGItemRarity::Common;

	/** Delay before auto-pickup is enabled (seconds). Prevents instant grab on spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Pickup", meta = (ClampMin = "0"))
	float AutoPickupDelay = 0.5f;

	/** Sound to play when picked up. Null = no sound. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Pickup")
	TObjectPtr<USoundBase> PickupSound;

	// =====================================================================
	// Slice Mode (vertical slice — no inventory)
	// =====================================================================

	/**
	 * If true, overlap/interact triggers the simplified vertical-slice pickup:
	 * spawn the pickup VFX, play the sound, fire the notification, and Destroy()
	 * the actor — no inventory component or item storage required.
	 * If false, pickup routes through the full inventory variant (LATER follow-up).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Slice")
	bool bSliceMode = true;

	/** Niagara effect spawned at the pickup location on a slice-mode pickup (e.g. a "+gold" burst). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Slice")
	TObjectPtr<UNiagaraSystem> PickupVFX;

	/** Display name used for the slice-mode pickup notification when there is no item instance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Slice")
	FText SlicePickupName = NSLOCTEXT("Loot", "SlicePickupGold", "Gold");

	// =====================================================================
	// Despawn
	// =====================================================================

	/** If > 0, the item despawns after this many seconds. 0 = never despawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Despawn", meta = (ClampMin = "0"))
	float DespawnTime = 300.f;

	/** Duration of the fade-out blink before despawn (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Despawn", meta = (ClampMin = "0"))
	float DespawnWarningTime = 5.f;

	/** Blink frequency during despawn warning (blinks per second). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Despawn", meta = (ClampMin = "1"))
	float DespawnBlinkRate = 4.f;

	// =====================================================================
	// Visual Settings
	// =====================================================================

	/** Vertical bob amplitude. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Visual")
	float BobAmplitude = 10.f;

	/** Vertical bob speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Visual")
	float BobSpeed = 2.f;

	/** Rotation speed in degrees/second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Visual")
	float RotationSpeed = 45.f;

	/** Light intensity for rarity beam. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Visual")
	float LightIntensity = 3000.f;

	/** Light attenuation radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Visual")
	float LightRadius = 400.f;

	/** Mesh scale multiplier per rarity tier (applied additively: base + tier * this). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Visual", meta = (ClampMin = "0"))
	float MeshScalePerRarity = 0.05f;

private:
	/** Apply rarity-based visuals (light color, mesh scale, etc). */
	void ApplyRarityVisuals();

	/** Slice-mode pickup: spawn the "+gold" VFX, play the sound, notify, and destroy the actor. */
	void PickupSlice(AActor* PickupActor);

	/** Handle overlap for auto-pickup. */
	UFUNCTION()
	void OnPickupTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	FVector InitialLocation = FVector::ZeroVector;
	float BobTimer = 0.f;

	// Launch arc state
	bool bIsLaunching = false;
	FVector LaunchVel = FVector::ZeroVector;
	float LaunchGravity = 980.f;
	float LaunchTimer = 0.f;

	// Despawn state
	float DespawnTimer = 0.f;
	bool bDespawnWarning = false;

	// Auto-pickup delay
	float AutoPickupTimer = 0.f;
	bool bAutoPickupReady = false;
};
