#include "Physics/ARPGGrabComponent.h"
#include "Physics/ARPGCollisionChannels.h"
#include "GameFramework/Character.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

UARPGGrabComponent::UARPGGrabComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UARPGGrabComponent::BeginPlay()
{
	Super::BeginPlay();

	// Create physics handle as a sibling component on the owning actor
	PhysicsHandle = NewObject<UPhysicsHandleComponent>(GetOwner(), TEXT("GrabPhysicsHandle"));
	if (PhysicsHandle)
	{
		PhysicsHandle->RegisterComponent();
		PhysicsHandle->LinearStiffness = 750.f;
		PhysicsHandle->LinearDamping = 65.f;
		PhysicsHandle->AngularStiffness = 1500.f;
		PhysicsHandle->AngularDamping = 500.f;
		PhysicsHandle->InterpolationSpeed = GrabInterpSpeed;
	}
}

void UARPGGrabComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GrabbedComponent || !PhysicsHandle)
	{
		Release();
		return;
	}

	// Check if the grabbed object was destroyed or physics disabled
	if (!GrabbedComponent->IsSimulatingPhysics())
	{
		Release();
		return;
	}

	// Update hold location
	PhysicsHandle->SetTargetLocation(GetHoldLocation());
}

bool UARPGGrabComponent::TryGrab()
{
	if (GrabbedComponent) return false;
	if (!PhysicsHandle) return false;

	AActor* Owner = GetOwner();
	if (!Owner) return false;

	UWorld* World = GetWorld();
	if (!World) return false;

	// Trace forward from owner to find a grabbable physics object
	const FVector Start = Owner->GetActorLocation();
	const FVector End = Start + Owner->GetActorForwardVector() * GrabReach;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(GrabTrace), false, Owner);
	FHitResult Hit;
	bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ARPGCollision::Interactable, Params);

	if (!bHit || !Hit.GetComponent()) return false;

	UPrimitiveComponent* HitComp = Hit.GetComponent();

	// Must be simulating physics
	if (!HitComp->IsSimulatingPhysics()) return false;

	// Check mass limit
	if (HitComp->GetMass() > MaxGrabMass) return false;

	// Grab it
	GrabbedComponent = HitComp;
	PhysicsHandle->GrabComponentAtLocationWithRotation(
		GrabbedComponent, NAME_None, Hit.ImpactPoint, GrabbedComponent->GetComponentRotation());

	SetComponentTickEnabled(true);

	OnObjectGrabbed.Broadcast(GrabbedComponent);
	return true;
}

void UARPGGrabComponent::Release()
{
	if (!GrabbedComponent) return;

	UPrimitiveComponent* Released = GrabbedComponent;
	GrabbedComponent = nullptr;

	if (PhysicsHandle)
	{
		PhysicsHandle->ReleaseComponent();
	}

	SetComponentTickEnabled(false);

	OnObjectReleased.Broadcast(Released);
}

void UARPGGrabComponent::Throw()
{
	if (!GrabbedComponent) return;

	UPrimitiveComponent* Thrown = GrabbedComponent;

	// Release the physics handle first
	GrabbedComponent = nullptr;
	if (PhysicsHandle)
	{
		PhysicsHandle->ReleaseComponent();
	}
	SetComponentTickEnabled(false);

	// Apply throw impulse in aim direction
	AActor* Owner = GetOwner();
	if (Owner && Thrown->IsSimulatingPhysics())
	{
		FVector ThrowDir = Owner->GetActorForwardVector();
		ThrowDir.Z += 0.15f; // Slight upward arc
		ThrowDir.Normalize();
		const FVector Impulse = ThrowDir * ThrowImpulse;
		Thrown->AddImpulse(Impulse, NAME_None, true);

		OnObjectThrown.Broadcast(Thrown, Impulse);
	}
	else
	{
		OnObjectReleased.Broadcast(Thrown);
	}
}

FVector UARPGGrabComponent::GetHoldLocation() const
{
	AActor* Owner = GetOwner();
	if (!Owner) return FVector::ZeroVector;

	return Owner->GetActorLocation()
		+ Owner->GetActorForwardVector() * HoldDistance
		+ FVector(0.f, 0.f, HoldHeightOffset);
}
