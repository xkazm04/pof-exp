#include "World/ARPGLockedGate.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Quest/ARPGQuestSubsystem.h"
#include "Inventory/ARPGInventoryComponent.h"
#include "Kismet/GameplayStatics.h"

AARPGLockedGate::AARPGLockedGate()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	GateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GateMesh"));
	GateMesh->SetupAttachment(InteractionSphere.Get());

	InteractionPrompt = FText::FromString(TEXT("Press E to open gate"));
}

void AARPGLockedGate::BeginPlay()
{
	Super::BeginPlay();

	ClosedLocation = GateMesh->GetRelativeLocation();
	ClosedRotation = GateMesh->GetRelativeRotation();

	UpdateInteractionPrompt();
}

void AARPGLockedGate::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsAnimating) return;

	const float TargetProgress = bIsOpen ? 1.f : 0.f;
	const float Direction = FMath::Sign(TargetProgress - CurrentOpenProgress);
	const float Speed = OpenSpeed / FMath::Max(OpenDistance, 1.f);
	CurrentOpenProgress += Direction * Speed * DeltaTime;
	CurrentOpenProgress = FMath::Clamp(CurrentOpenProgress, 0.f, 1.f);

	if (bSlideOpen)
	{
		FVector NewLoc = ClosedLocation;
		NewLoc.Z += OpenDistance * CurrentOpenProgress;
		GateMesh->SetRelativeLocation(NewLoc);
	}
	else
	{
		FRotator NewRot = ClosedRotation;
		NewRot.Yaw += OpenDistance * CurrentOpenProgress;
		GateMesh->SetRelativeRotation(NewRot);
	}

	if (FMath::IsNearlyEqual(CurrentOpenProgress, TargetProgress, 0.001f))
	{
		bIsAnimating = false;
		SetActorTickEnabled(false);
	}
}

bool AARPGLockedGate::Interact(AActor* Interactor)
{
	if (!bCanInteract || !Interactor) return false;

	// Toggle close if open and closeable
	if (bIsOpen && bCanClose)
	{
		Close();
		return Super::Interact(Interactor);
	}

	if (bIsOpen) return false;

	if (bIsLocked)
	{
		if (!CheckRequirements(Interactor))
		{
			if (LockedSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, LockedSound, GetActorLocation());
			}
			UE_LOG(LogTemp, Log, TEXT("[Gate] %s is locked"), *GetName());
			return false;
		}

		// Consume key item if configured
		if (bConsumeKeyItem && !RequiredItemName.IsNone())
		{
			UARPGInventoryComponent* Inventory = Interactor->FindComponentByClass<UARPGInventoryComponent>();
			if (!Inventory)
			{
				if (APlayerController* PC = Cast<APlayerController>(Interactor))
				{
					if (APawn* Pawn = PC->GetPawn())
					{
						Inventory = Pawn->FindComponentByClass<UARPGInventoryComponent>();
					}
				}
			}
			if (Inventory)
			{
				for (UARPGItemInstance* Item : Inventory->GetAllItems())
				{
					if (Item && Item->Definition && Item->Definition->DisplayName.ToString() == RequiredItemName.ToString())
					{
						Inventory->RemoveItemByDefinition(Item->Definition, 1);
						break;
					}
				}
			}
		}

		Unlock();
	}

	Open();
	return Super::Interact(Interactor);
}

void AARPGLockedGate::Unlock()
{
	bIsLocked = false;
	UpdateInteractionPrompt();
	UE_LOG(LogTemp, Log, TEXT("[Gate] %s unlocked"), *GetName());
}

void AARPGLockedGate::Open()
{
	if (bIsOpen) return;

	bIsOpen = true;
	bIsAnimating = true;
	SetActorTickEnabled(true);

	if (OpenSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, OpenSound, GetActorLocation());
	}

	OnGateOpened.Broadcast(this);
	UpdateInteractionPrompt();
	UE_LOG(LogTemp, Log, TEXT("[Gate] %s opened"), *GetName());
}

void AARPGLockedGate::Close()
{
	if (!bIsOpen || !bCanClose) return;

	bIsOpen = false;
	bIsAnimating = true;
	SetActorTickEnabled(true);

	UpdateInteractionPrompt();
	UE_LOG(LogTemp, Log, TEXT("[Gate] %s closed"), *GetName());
}

void AARPGLockedGate::UpdateInteractionPrompt()
{
	if (!bIsLocked)
	{
		InteractionPrompt = bIsOpen && bCanClose
			? FText::FromString(TEXT("Press E to close gate"))
			: FText::FromString(TEXT("Press E to open gate"));
		return;
	}

	if (!RequiredItemName.IsNone())
	{
		InteractionPrompt = FText::FromString(FString::Printf(TEXT("Locked — requires %s"), *RequiredItemName.ToString()));
	}
	else if (!RequiredQuestID.IsNone())
	{
		InteractionPrompt = FText::FromString(FString::Printf(TEXT("Locked — complete quest: %s"), *RequiredQuestID.ToString()));
	}
	else
	{
		InteractionPrompt = FText::FromString(TEXT("Locked"));
	}
}

bool AARPGLockedGate::CheckRequirements(AActor* Interactor) const
{
	if (!RequiredQuestID.IsNone())
	{
		UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
		if (GI)
		{
			UARPGQuestSubsystem* QuestSys = GI->GetSubsystem<UARPGQuestSubsystem>();
			if (!QuestSys || !QuestSys->GetCompletedQuestIDs().Contains(RequiredQuestID))
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	if (!RequiredItemName.IsNone())
	{
		if (Interactor)
		{
			UARPGInventoryComponent* Inventory = Interactor->FindComponentByClass<UARPGInventoryComponent>();
			if (!Inventory)
			{
				// Try the controller's pawn
				if (APlayerController* PC = Cast<APlayerController>(Interactor))
				{
					if (APawn* Pawn = PC->GetPawn())
					{
						Inventory = Pawn->FindComponentByClass<UARPGInventoryComponent>();
					}
				}
			}
			if (!Inventory)
			{
				return false;
			}

			// Search inventory for item with matching display name
			bool bFound = false;
			for (UARPGItemInstance* Item : Inventory->GetAllItems())
			{
				if (Item && Item->Definition && Item->Definition->DisplayName.ToString() == RequiredItemName.ToString())
				{
					bFound = true;
					break;
				}
			}
			if (!bFound)
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	// All requirements met
	return true;
}
