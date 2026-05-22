#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARPGPhysicsLever.generated.h"

class UStaticMeshComponent;
class UPhysicsConstraintComponent;
class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLeverActivated, AARPGPhysicsLever*, Lever, bool, bIsOn);

/**
 * Physics-based lever that toggles between on/off positions.
 * Uses a hinge constraint limited to a small angular range.
 * Broadcasts an activation event when toggled.
 */
UCLASS(BlueprintType, Blueprintable)
class POF_API AARPGPhysicsLever : public AActor
{
	GENERATED_BODY()

public:
	AARPGPhysicsLever();

	/** Interact with the lever — pulls it to the opposite position. */
	UFUNCTION(BlueprintCallable, Category = "Physics|Lever")
	void Interact(AActor* Interactor);

	/** Whether the lever is currently in the ON position. */
	UFUNCTION(BlueprintPure, Category = "Physics|Lever")
	bool IsActivated() const { return bIsActivated; }

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnLeverActivated OnLeverActivated;

protected:
	/** Base/mount mesh (static). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lever")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	/** Lever handle mesh (rotates). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lever")
	TObjectPtr<UStaticMeshComponent> HandleMesh;

	/** Hinge constraint for the lever rotation. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lever")
	TObjectPtr<UPhysicsConstraintComponent> HingeConstraint;

	/** Interaction trigger. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lever")
	TObjectPtr<UBoxComponent> InteractionVolume;

	/** Angular range of the lever movement (degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lever", meta = (ClampMin = "15", ClampMax = "90"))
	float LeverAngle = 45.f;

	/** Impulse to apply when toggling the lever. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lever", meta = (ClampMin = "10"))
	float ToggleImpulse = 100.f;

	/** Sound played on lever toggle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lever|SFX")
	TObjectPtr<class USoundBase> ToggleSound;

	virtual void BeginPlay() override;

private:
	bool bIsActivated = false;
};
