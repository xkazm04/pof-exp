#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARPGPhysicsDoor.generated.h"

class UStaticMeshComponent;
class UPhysicsConstraintComponent;
class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDoorStateChanged, bool, bIsOpen);

/**
 * Physics-based door using a hinge constraint.
 * Has an interaction trigger volume — player overlaps to toggle open/close.
 * Can also be kicked open by applying impulse.
 */
UCLASS(BlueprintType, Blueprintable)
class POF_API AARPGPhysicsDoor : public AActor
{
	GENERATED_BODY()

public:
	AARPGPhysicsDoor();

	/** Toggle the door open or closed. */
	UFUNCTION(BlueprintCallable, Category = "Physics|Door")
	void ToggleDoor(AActor* Interactor);

	/** Force the door open with an impulse. */
	UFUNCTION(BlueprintCallable, Category = "Physics|Door")
	void KickOpen(const FVector& KickDirection, float KickStrength = 500.f);

	/** Whether the door is currently open (past the open angle threshold). */
	UFUNCTION(BlueprintPure, Category = "Physics|Door")
	bool IsOpen() const { return bIsOpen; }

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnDoorStateChanged OnDoorStateChanged;

protected:
	/** Static frame/doorjamb mesh (does not move). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
	TObjectPtr<UStaticMeshComponent> DoorFrame;

	/** The door panel mesh (swings on hinge). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
	TObjectPtr<UStaticMeshComponent> DoorPanel;

	/** Hinge constraint between frame and panel. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
	TObjectPtr<UPhysicsConstraintComponent> HingeConstraint;

	/** Interaction trigger volume. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
	TObjectPtr<UBoxComponent> InteractionVolume;

	/** Impulse applied when opening/closing via ToggleDoor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (ClampMin = "100"))
	float OpenImpulse = 400.f;

	/** Angular limit of the hinge in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (ClampMin = "10", ClampMax = "170"))
	float HingeAngleLimit = 90.f;

	/** Door mass in kg. Heavier doors swing slower. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (ClampMin = "1"))
	float DoorMass = 30.f;

	/** Angular damping on the door panel. Higher = less swinging. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (ClampMin = "0"))
	float AngularDamping = 5.f;

	/** Sound played when the door opens. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|SFX")
	TObjectPtr<class USoundBase> OpenSound;

	/** Sound played when the door closes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|SFX")
	TObjectPtr<class USoundBase> CloseSound;

	virtual void BeginPlay() override;

private:
	bool bIsOpen = false;
};
