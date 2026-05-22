#include "Physics/ARPGPhysicsDoor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "Kismet/GameplayStatics.h"

AARPGPhysicsDoor::AARPGPhysicsDoor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Door frame (static, immovable)
	DoorFrame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorFrame"));
	SetRootComponent(DoorFrame);
	DoorFrame->SetMobility(EComponentMobility::Static);
	DoorFrame->SetCollisionProfileName(TEXT("BlockAll"));

	// Door panel (physics-simulated)
	DoorPanel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorPanel"));
	DoorPanel->SetupAttachment(DoorFrame);
	DoorPanel->SetMobility(EComponentMobility::Movable);
	DoorPanel->SetSimulatePhysics(true);
	DoorPanel->SetCollisionProfileName(TEXT("PhysicsActor"));
	DoorPanel->SetAngularDamping(AngularDamping);

	// Hinge constraint
	HingeConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("HingeConstraint"));
	HingeConstraint->SetupAttachment(DoorFrame);
	HingeConstraint->ComponentName1.ComponentName = TEXT("DoorFrame");
	HingeConstraint->ComponentName2.ComponentName = TEXT("DoorPanel");

	// Configure as hinge (only twist is free)
	HingeConstraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0.f);
	HingeConstraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0.f);
	HingeConstraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Limited, HingeAngleLimit);

	// Interaction volume
	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(DoorFrame);
	InteractionVolume->SetBoxExtent(FVector(150.f, 100.f, 100.f));
	InteractionVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionVolume->SetGenerateOverlapEvents(true);
}

void AARPGPhysicsDoor::BeginPlay()
{
	Super::BeginPlay();

	// Apply runtime mass and damping
	if (DoorPanel)
	{
		DoorPanel->SetMassOverrideInKg(NAME_None, DoorMass);
		DoorPanel->SetAngularDamping(AngularDamping);
	}

	// Update hinge limit at runtime
	HingeConstraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Limited, HingeAngleLimit);
}

void AARPGPhysicsDoor::ToggleDoor(AActor* Interactor)
{
	if (!DoorPanel || !Interactor) return;

	// Determine push direction based on which side the interactor is on
	const FVector ToInteractor = Interactor->GetActorLocation() - DoorPanel->GetComponentLocation();
	const FVector DoorForward = DoorPanel->GetForwardVector();
	const float Dot = FVector::DotProduct(ToInteractor.GetSafeNormal(), DoorForward);

	// Push away from the interactor
	const FVector PushDir = (Dot > 0.f) ? -DoorForward : DoorForward;

	DoorPanel->AddImpulse(PushDir * OpenImpulse, NAME_None, true);

	bIsOpen = !bIsOpen;

	// Play appropriate sound
	USoundBase* Sound = bIsOpen ? OpenSound : CloseSound;
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
	}

	OnDoorStateChanged.Broadcast(bIsOpen);
}

void AARPGPhysicsDoor::KickOpen(const FVector& KickDirection, float KickStrength)
{
	if (!DoorPanel) return;

	DoorPanel->AddImpulse(KickDirection.GetSafeNormal() * KickStrength, NAME_None, true);

	if (!bIsOpen)
	{
		bIsOpen = true;
		if (OpenSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, OpenSound, GetActorLocation());
		}
		OnDoorStateChanged.Broadcast(true);
	}
}
