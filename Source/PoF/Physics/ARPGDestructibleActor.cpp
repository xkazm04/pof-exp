#include "Physics/ARPGDestructibleActor.h"
#include "Physics/ARPGCollisionChannels.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"

AARPGDestructibleActor::AARPGDestructibleActor()
{
	PrimaryActorTick.bCanEverTick = false;

	GeometryCollection = CreateDefaultSubobject<UGeometryCollectionComponent>(TEXT("GeometryCollection"));
	SetRootComponent(GeometryCollection);

	// Use destructible collision profile
	GeometryCollection->SetCollisionProfileName(ARPGCollision::Profiles::Destructible);
	GeometryCollection->SetSimulatePhysics(false); // Static until broken
	GeometryCollection->SetNotifyBreaks(true);
	GeometryCollection->SetGenerateOverlapEvents(false);
}

void AARPGDestructibleActor::BeginPlay()
{
	Super::BeginPlay();
}

void AARPGDestructibleActor::ApplyDestructionDamage(float DamageAmount, const FVector& ImpactPoint, const FVector& ImpactNormal, float ImpulseStrength)
{
	if (bIsBroken) return;

	AccumulatedDamage += DamageAmount;

	if (AccumulatedDamage >= DamageThreshold)
	{
		TriggerBreak(ImpactPoint, ImpactNormal, ImpulseStrength);
	}
}

void AARPGDestructibleActor::ForceDestruct(const FVector& ImpactPoint, float ImpulseStrength)
{
	if (bIsBroken) return;

	AccumulatedDamage = DamageThreshold;
	TriggerBreak(ImpactPoint, FVector::UpVector, ImpulseStrength);
}

void AARPGDestructibleActor::TriggerBreak(const FVector& ImpactPoint, const FVector& ImpactNormal, float ImpulseStrength)
{
	bIsBroken = true;

	// Enable physics simulation for fracture
	GeometryCollection->SetSimulatePhysics(true);

	// Apply radial impulse at impact point to scatter fragments
	if (ImpulseStrength > 0.f)
	{
		// Apply impulse from impact direction
		const FVector ImpulseDir = -ImpactNormal;
		GeometryCollection->AddImpulseAtLocation(ImpulseDir * ImpulseStrength, ImpactPoint);
	}

	// Spawn break VFX
	if (BreakVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), BreakVFX, ImpactPoint, ImpactNormal.Rotation(),
			FVector(1.f), true, true, ENCPoolMethod::None);
	}

	// Play break sound
	if (BreakSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BreakSound, ImpactPoint, BreakSoundVolume);
	}

	// Schedule debris cleanup
	if (DebrisLifetime > 0.f)
	{
		SetLifeSpan(DebrisLifetime);
	}

	// Spawn loot
	SpawnLoot();

	// Broadcast event
	OnBroken.Broadcast(this, AccumulatedDamage);

	UE_LOG(LogTemp, Log, TEXT("[Destructible] %s broken with %.1f total damage"), *GetName(), AccumulatedDamage);
}

void AARPGDestructibleActor::SpawnLoot()
{
	UWorld* World = GetWorld();
	if (!World || LootClasses.Num() == 0) return;

	const FVector Origin = GetActorLocation();

	for (const TSubclassOf<AActor>& LootClass : LootClasses)
	{
		if (!LootClass) continue;

		// Random offset within spawn radius
		const FVector2D RandOffset = FMath::RandPointInCircle(LootSpawnRadius);
		const FVector SpawnLoc = Origin + FVector(RandOffset.X, RandOffset.Y, 30.f);

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AActor* Loot = World->SpawnActor<AActor>(LootClass, SpawnLoc, FRotator::ZeroRotator, SpawnParams);
		if (Loot && LootEjectImpulse > 0.f)
		{
			// Apply upward + random horizontal impulse for pop-out effect
			if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Loot->GetRootComponent()))
			{
				if (PrimComp->IsSimulatingPhysics())
				{
					const FVector EjectDir = FVector(FMath::FRandRange(-0.3f, 0.3f), FMath::FRandRange(-0.3f, 0.3f), 1.f).GetSafeNormal();
					PrimComp->AddImpulse(EjectDir * LootEjectImpulse, NAME_None, true);
				}
			}
		}
	}
}
