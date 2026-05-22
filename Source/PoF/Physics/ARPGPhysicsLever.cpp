#include "Physics/ARPGPhysicsLever.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "Kismet/GameplayStatics.h"

AARPGPhysicsLever::AARPGPhysicsLever()
{
	PrimaryActorTick.bCanEverTick = false;

	// Base (static mount)
	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	SetRootComponent(BaseMesh);
	BaseMesh->SetMobility(EComponentMobility::Static);
	BaseMesh->SetCollisionProfileName(TEXT("BlockAll"));

	// Handle (physics-simulated)
	HandleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HandleMesh"));
	HandleMesh->SetupAttachment(BaseMesh);
	HandleMesh->SetMobility(EComponentMobility::Movable);
	HandleMesh->SetSimulatePhysics(true);
	HandleMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	HandleMesh->SetAngularDamping(10.f);

	// Hinge
	HingeConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("HingeConstraint"));
	HingeConstraint->SetupAttachment(BaseMesh);
	HingeConstraint->ComponentName1.ComponentName = TEXT("BaseMesh");
	HingeConstraint->ComponentName2.ComponentName = TEXT("HandleMesh");

	// Only allow rotation around one axis
	HingeConstraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0.f);
	HingeConstraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0.f);
	HingeConstraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Limited, LeverAngle);

	// Interaction volume
	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(BaseMesh);
	InteractionVolume->SetBoxExtent(FVector(80.f, 80.f, 80.f));
	InteractionVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionVolume->SetGenerateOverlapEvents(true);
}

void AARPGPhysicsLever::BeginPlay()
{
	Super::BeginPlay();

	HingeConstraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Limited, LeverAngle);
}

void AARPGPhysicsLever::Interact(AActor* Interactor)
{
	if (!HandleMesh) return;

	bIsActivated = !bIsActivated;

	// Apply impulse to flip the lever
	const FVector LeverUp = HandleMesh->GetUpVector();
	const float Direction = bIsActivated ? 1.f : -1.f;
	HandleMesh->AddImpulse(LeverUp * Direction * ToggleImpulse, NAME_None, true);

	if (ToggleSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ToggleSound, GetActorLocation());
	}

	OnLeverActivated.Broadcast(this, bIsActivated);
}
