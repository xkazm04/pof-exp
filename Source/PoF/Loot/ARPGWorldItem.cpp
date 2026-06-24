#include "Loot/ARPGWorldItem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Inventory/ARPGItemInstance.h"
#include "Inventory/ARPGItemDefinition.h"
#include "Inventory/ARPGInventoryComponent.h"
#include "Performance/ARPGTickOptimizer.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "GameFramework/Character.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

// Static delegate instance
FOnItemPickedUpStatic AARPGWorldItem::OnAnyItemPickedUp;

AARPGWorldItem::AARPGWorldItem()
{
	PrimaryActorTick.bCanEverTick = true;

	// Pickup trigger as root
	PickupTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("PickupTrigger"));
	PickupTrigger->SetSphereRadius(PickupRadius);
	PickupTrigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	PickupTrigger->SetGenerateOverlapEvents(true);
	RootComponent = PickupTrigger;

	// Item mesh
	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(RootComponent);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Rarity light beam
	RarityLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("RarityLight"));
	RarityLight->SetupAttachment(RootComponent);
	RarityLight->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	RarityLight->SetIntensity(LightIntensity);
	RarityLight->SetAttenuationRadius(LightRadius);
	RarityLight->SetCastShadows(false);

	// Nameplate
	NameplateWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Nameplate"));
	NameplateWidget->SetupAttachment(RootComponent);
	NameplateWidget->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	NameplateWidget->SetWidgetSpace(EWidgetSpace::Screen);
	NameplateWidget->SetDrawAtDesiredSize(true);
	NameplateWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Tag for interaction system
	Tags.Add(FName("Interactable"));
	Tags.Add(FName("WorldItem"));
}

void AARPGWorldItem::BeginPlay()
{
	Super::BeginPlay();
	InitialLocation = GetActorLocation();

	// Bind overlap for auto-pickup
	PickupTrigger->OnComponentBeginOverlap.AddDynamic(this, &AARPGWorldItem::OnPickupTriggerOverlap);

	// Register with tick optimizer for distance-based tick reduction
	if (UWorld* World = GetWorld())
	{
		if (UARPGTickOptimizer* TickOpt = World->GetSubsystem<UARPGTickOptimizer>())
		{
			TickOpt->RegisterActor(this);
		}
	}
}

void AARPGWorldItem::InitFromItemInstance(UARPGItemInstance* InInstance, bool bRouteToInventory)
{
	if (!InInstance || !InInstance->Definition)
	{
		return;
	}

	ItemInstance = InInstance;

	// Loot routed from a chest goes into the looter's inventory; the vertical-slice
	// default (VFX-and-destroy) stays in effect for every other spawn path.
	bSliceMode = !bRouteToInventory;

	// Set mesh from definition
	if (InInstance->Definition->WorldMesh)
	{
		ItemMesh->SetStaticMesh(InInstance->Definition->WorldMesh);
	}

	ApplyRarityVisuals();
}

void AARPGWorldItem::ApplyRarityVisuals()
{
	if (!ItemInstance)
	{
		return;
	}

	const EARPGItemRarity Rarity = ItemInstance->GetEffectiveRarity();
	const FLinearColor RarityColor = UARPGItemDefinition::GetRarityColor(Rarity);
	const int32 RarityTier = static_cast<int32>(Rarity);

	// Light color and intensity scale with rarity
	RarityLight->SetLightColor(RarityColor);
	const float RarityScale = 1.f + RarityTier * 0.5f;
	RarityLight->SetIntensity(LightIntensity * RarityScale);
	RarityLight->SetAttenuationRadius(LightRadius * RarityScale);

	// Only show light beam for Uncommon+
	RarityLight->SetVisibility(Rarity > EARPGItemRarity::Common);

	// Scale mesh slightly for higher rarities
	const float MeshScale = 1.f + RarityTier * MeshScalePerRarity;
	ItemMesh->SetWorldScale3D(FVector(MeshScale));

	// Higher rarity = faster rotation to draw attention
	if (Rarity >= EARPGItemRarity::Epic)
	{
		RotationSpeed = 90.f;
		BobAmplitude = 15.f;
	}
	else if (Rarity >= EARPGItemRarity::Rare)
	{
		RotationSpeed = 60.f;
		BobAmplitude = 12.f;
	}
}

void AARPGWorldItem::LaunchItem(FVector LaunchVelocity)
{
	bIsLaunching = true;
	LaunchVel = LaunchVelocity;
	LaunchTimer = 0.f;
}

void AARPGWorldItem::PickupSlice(AActor* PickupActor)
{
	const FVector PickupLocation = GetActorLocation();

	// "+gold" burst at the pickup location.
	if (PickupVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			PickupVFX,
			PickupLocation,
			FRotator::ZeroRotator,
			FVector(1.f),
			true,  // bAutoDestroy
			true,  // bAutoActivate
			ENCPoolMethod::None);
	}

	// Pickup sound.
	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PickupSound, PickupLocation);
	}

	// Brief pickup notification — use the item name if one happens to be set,
	// otherwise the configured slice name (e.g. "Gold"). HUD listeners bind to
	// OnAnyItemPickedUp to show the on-screen toast.
	const FText PickupName = (ItemInstance && ItemInstance->Definition)
		? ItemInstance->GetDisplayName()
		: SlicePickupName;
	const EARPGItemRarity Rarity = ItemInstance
		? ItemInstance->GetEffectiveRarity()
		: EARPGItemRarity::Common;

	OnItemPickedUp.Broadcast(PickupName, Rarity, 1);
	OnAnyItemPickedUp.Broadcast(PickupName, Rarity, 1);

	UE_LOG(LogTemp, Display, TEXT("[WorldItem] %s picked up %s (slice mode)"),
		PickupActor ? *PickupActor->GetName() : TEXT("<unknown>"),
		*PickupName.ToString());

	// No inventory, no item storage — just remove the world actor.
	Destroy();
}

bool AARPGWorldItem::TryPickup(AActor* PickupActor)
{
	if (!PickupActor)
	{
		return false;
	}

	if (bSliceMode)
	{
		// Vertical slice: no inventory — overlap and interact both just trigger
		// the VFX-and-destroy pickup.
		PickupSlice(PickupActor);
		return true;
	}

	// LATER: full inventory pickup variant — used when bSliceMode is false.
	if (!ItemInstance || !ItemInstance->Definition)
	{
		return false;
	}

	UARPGInventoryComponent* Inventory = PickupActor->FindComponentByClass<UARPGInventoryComponent>();
	if (!Inventory)
	{
		return false;
	}

	// Check weight limit
	if (Inventory->WouldExceedWeight(ItemInstance->Definition, ItemInstance->StackCount))
	{
		UE_LOG(LogTemp, Display, TEXT("[WorldItem] Pickup failed — would exceed carry weight for %s"),
			*ItemInstance->Definition->DisplayName.ToString());
		return false;
	}

	// Try to add to inventory
	const int32 OriginalCount = ItemInstance->StackCount;
	const int32 Remainder = Inventory->AddItem(ItemInstance->Definition, ItemInstance->StackCount);
	if (Remainder < OriginalCount)
	{
		const int32 PickedUpCount = OriginalCount - Remainder;
		const FText ItemName = ItemInstance->GetDisplayName();
		const EARPGItemRarity Rarity = ItemInstance->GetEffectiveRarity();

		UE_LOG(LogTemp, Display, TEXT("[WorldItem] %s picked up %s (x%d) [%s]"),
			*PickupActor->GetName(),
			*ItemName.ToString(),
			PickedUpCount,
			*UARPGItemDefinition::GetRarityDisplayName(Rarity).ToString());

		// Broadcast pickup notifications
		OnItemPickedUp.Broadcast(ItemName, Rarity, PickedUpCount);
		OnAnyItemPickedUp.Broadcast(ItemName, Rarity, PickedUpCount);

		// Play pickup sound
		if (PickupSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
		}

		if (Remainder > 0)
		{
			// Partial pickup — update remaining stack
			ItemInstance->StackCount = Remainder;
		}
		else
		{
			// Full pickup — destroy world item
			Destroy();
		}
		return true;
	}

	UE_LOG(LogTemp, Display, TEXT("[WorldItem] Pickup failed — inventory full for %s"),
		*ItemInstance->Definition->DisplayName.ToString());
	return false;
}

void AARPGWorldItem::OnPickupTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	// Spawn-delay guard — prevents an instant grab the moment the item appears.
	if (!bAutoPickupReady)
	{
		return;
	}

	// Only pick up for characters (players).
	if (!Cast<ACharacter>(OtherActor))
	{
		return;
	}

	if (bSliceMode)
	{
		// Vertical slice: overlap-destroy with a "+gold" VFX burst — no inventory.
		PickupSlice(OtherActor);
		return;
	}

	// --- Full inventory variant (LATER follow-up) ---
	if (!bAutoPickupCommon || !ItemInstance)
	{
		return;
	}

	// Only auto-pickup items at or below the configured rarity.
	const EARPGItemRarity Rarity = ItemInstance->GetEffectiveRarity();
	if (Rarity > AutoPickupMaxRarity)
	{
		return;
	}

	TryPickup(OtherActor);
}

void AARPGWorldItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// --- Launch arc ---
	if (bIsLaunching)
	{
		LaunchTimer += DeltaTime;
		LaunchVel.Z -= LaunchGravity * DeltaTime;

		FVector NewPos = GetActorLocation() + LaunchVel * DeltaTime;

		// Ground check — stop launch when hitting ground level or below initial spawn height
		if (LaunchTimer > 0.1f && LaunchVel.Z < 0.f)
		{
			// Simple ground detection via line trace
			FHitResult Hit;
			FVector TraceStart = NewPos;
			FVector TraceEnd = NewPos - FVector(0.f, 0.f, 50.f);
			if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility))
			{
				NewPos = Hit.ImpactPoint + FVector(0.f, 0.f, 5.f);
				bIsLaunching = false;
				InitialLocation = NewPos; // Reset bob origin to landing position
			}
		}

		SetActorLocation(NewPos);
		return; // Skip bob/rotation during launch
	}

	// --- Auto-pickup delay ---
	if (!bAutoPickupReady)
	{
		AutoPickupTimer += DeltaTime;
		if (AutoPickupTimer >= AutoPickupDelay)
		{
			bAutoPickupReady = true;
		}
	}

	// --- Despawn timer ---
	if (DespawnTime > 0.f)
	{
		DespawnTimer += DeltaTime;

		const float TimeRemaining = DespawnTime - DespawnTimer;
		if (TimeRemaining <= 0.f)
		{
			Destroy();
			return;
		}

		// Blink warning before despawn
		if (TimeRemaining <= DespawnWarningTime)
		{
			if (!bDespawnWarning)
			{
				bDespawnWarning = true;
			}

			const float BlinkPhase = FMath::Fmod(DespawnTimer * DespawnBlinkRate, 1.f);
			const bool bVisible = BlinkPhase < 0.5f;
			ItemMesh->SetVisibility(bVisible);
			RarityLight->SetVisibility(bVisible && ItemInstance && ItemInstance->GetEffectiveRarity() > EARPGItemRarity::Common);
		}
	}

	// --- Bob & Rotation ---
	BobTimer += DeltaTime;

	FVector NewLocation = InitialLocation;
	NewLocation.Z += FMath::Sin(BobTimer * BobSpeed) * BobAmplitude;
	SetActorLocation(NewLocation);

	AddActorWorldRotation(FRotator(0.f, RotationSpeed * DeltaTime, 0.f));
}
