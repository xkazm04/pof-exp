#include "World/ARPGInteractableBase.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"

AARPGInteractableBase::AARPGInteractableBase()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetSphereRadius(InteractionRadius);
	InteractionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionSphere->SetGenerateOverlapEvents(true);
	SetRootComponent(InteractionSphere);

	Tags.Add(FName("Interactable"));
}

void AARPGInteractableBase::BeginPlay()
{
	Super::BeginPlay();

	InteractionSphere->SetSphereRadius(InteractionRadius);
	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &AARPGInteractableBase::OnSphereBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &AARPGInteractableBase::OnSphereEndOverlap);
}

bool AARPGInteractableBase::Interact(AActor* Interactor)
{
	if (!bCanInteract || !Interactor) return false;

	OnInteracted.Broadcast(this, Interactor);
	return true;
}

void AARPGInteractableBase::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (OtherActor == PlayerPawn)
	{
		bPlayerInRange = true;
	}
}

void AARPGInteractableBase::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (OtherActor == PlayerPawn)
	{
		bPlayerInRange = false;
	}
}
