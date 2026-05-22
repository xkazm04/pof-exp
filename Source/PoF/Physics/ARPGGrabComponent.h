#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "ARPGGrabComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnObjectGrabbed, UPrimitiveComponent*, GrabbedComponent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnObjectReleased, UPrimitiveComponent*, ReleasedComponent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnObjectThrown, UPrimitiveComponent*, ThrownComponent, FVector, ThrowVelocity);

/**
 * Physics grab/throw component. Attach to the player character.
 * Uses a UPhysicsHandleComponent internally to smoothly hold objects.
 */
UCLASS(ClassGroup = "Physics", meta = (BlueprintSpawnableComponent))
class POF_API UARPGGrabComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARPGGrabComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Try to grab the physics object in front of the character. */
	UFUNCTION(BlueprintCallable, Category = "Physics|Grab")
	bool TryGrab();

	/** Release the currently held object (no throw). */
	UFUNCTION(BlueprintCallable, Category = "Physics|Grab")
	void Release();

	/** Throw the held object in the aim direction. */
	UFUNCTION(BlueprintCallable, Category = "Physics|Grab")
	void Throw();

	/** Whether an object is currently being held. */
	UFUNCTION(BlueprintPure, Category = "Physics|Grab")
	bool IsHolding() const { return GrabbedComponent != nullptr; }

	/** Get the currently held component. */
	UFUNCTION(BlueprintPure, Category = "Physics|Grab")
	UPrimitiveComponent* GetGrabbedComponent() const { return GrabbedComponent; }

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnObjectGrabbed OnObjectGrabbed;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnObjectReleased OnObjectReleased;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnObjectThrown OnObjectThrown;

protected:
	/** Max distance to reach for grabbable objects (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics|Grab", meta = (ClampMin = "50"))
	float GrabReach = 300.f;

	/** Distance in front of the character to hold the object. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics|Grab", meta = (ClampMin = "50"))
	float HoldDistance = 200.f;

	/** Height offset for the held object relative to character center. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics|Grab")
	float HoldHeightOffset = 50.f;

	/** Max mass of objects that can be grabbed (kg). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics|Grab", meta = (ClampMin = "1"))
	float MaxGrabMass = 100.f;

	/** Throw impulse strength. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics|Grab", meta = (ClampMin = "100"))
	float ThrowImpulse = 1500.f;

	/** How quickly the physics handle interpolates to the target location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics|Grab", meta = (ClampMin = "1"))
	float GrabInterpSpeed = 10.f;

private:
	UPROPERTY()
	TObjectPtr<UPhysicsHandleComponent> PhysicsHandle;

	UPROPERTY()
	TObjectPtr<UPrimitiveComponent> GrabbedComponent;

	FVector GetHoldLocation() const;
};
