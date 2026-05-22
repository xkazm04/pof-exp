#include "LevelDesign/ARPGVegetationScatter.h"
#include "LevelDesign/ARPGBiomeDefinition.h"
#include "Components/BoxComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"

AARPGVegetationScatter::AARPGVegetationScatter()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	ScatterBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("ScatterBounds"));
	ScatterBounds->SetupAttachment(Root);
	ScatterBounds->SetBoxExtent(FVector(2000.f, 2000.f, 500.f));
	ScatterBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ScatterBounds->SetHiddenInGame(true);
	ScatterBounds->ShapeColor = FColor::Green;
}

void AARPGVegetationScatter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bGenerateInEditor)
	{
		GenerateVegetation();
	}
}

void AARPGVegetationScatter::BeginPlay()
{
	Super::BeginPlay();

	if (bGenerateOnBeginPlay)
	{
		GenerateVegetation();
	}
}

void AARPGVegetationScatter::GenerateVegetation()
{
	ClearVegetation();

	if (!BiomeDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("[VegetationScatter] %s: No BiomeDefinition set"), *GetName());
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	RNG = (RandomSeed != 0) ? FRandomStream(RandomSeed) : FRandomStream(FMath::Rand());

	const FVector BoundsOrigin = ScatterBounds->GetComponentLocation();
	const FVector BoundsExtent = ScatterBounds->GetScaledBoxExtent();
	const FVector BoundsMin = BoundsOrigin - BoundsExtent;
	const FVector BoundsMax = BoundsOrigin + BoundsExtent;

	const float AreaX = BoundsExtent.X * 2.f;
	const float AreaY = BoundsExtent.Y * 2.f;
	const float EffectiveDensity = BiomeDefinition->GlobalDensityMultiplier * LocalDensityMultiplier;

	int32 TotalPlaced = 0;

	// Process vegetation entries
	for (const FBiomeVegetationEntry& Entry : BiomeDefinition->Vegetation)
	{
		UStaticMesh* Mesh = Entry.Mesh.LoadSynchronous();
		if (!Mesh) continue;

		UHierarchicalInstancedStaticMeshComponent* HISM = GetOrCreateHISM(Mesh, Entry.bBlocksNavigation);
		if (!HISM) continue;

		// Calculate number of candidate points from density
		const float Area100Sq = (AreaX / 100.f) * (AreaY / 100.f);
		const int32 TargetCount = FMath::RoundToInt(Entry.DensityPer100Sq * Area100Sq * EffectiveDensity);

		for (int32 i = 0; i < TargetCount; ++i)
		{
			// Random XY within bounds
			const float X = RNG.FRandRange(BoundsMin.X, BoundsMax.X);
			const float Y = RNG.FRandRange(BoundsMin.Y, BoundsMax.Y);
			const FVector TraceStart(X, Y, BoundsMax.Z + 100.f);
			const FVector TraceEnd(X, Y, BoundsMin.Z - 100.f);

			FHitResult Hit;
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(this);

			if (!World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, TraceChannel, QueryParams))
			{
				continue;
			}

			const FVector& HitLocation = Hit.ImpactPoint;
			const FVector& HitNormal = Hit.ImpactNormal;

			// Check altitude
			if (HitLocation.Z < Entry.MinAltitude || HitLocation.Z > Entry.MaxAltitude)
			{
				continue;
			}

			// Check slope
			const float SlopeAngle = GetSlopeAngleFromNormal(HitNormal);
			if (SlopeAngle < Entry.MinSlopeAngle || SlopeAngle > Entry.MaxSlopeAngle)
			{
				continue;
			}

			// Check exclusion zones
			if (IsInExclusionZone(HitLocation))
			{
				continue;
			}

			// Check minimum spacing
			if (Entry.MinSpacing > 0.f && !CheckMinSpacing(HISM, HitLocation, Entry.MinSpacing))
			{
				continue;
			}

			// Compute transform
			const float Scale = RNG.FRandRange(Entry.MinScale, Entry.MaxScale);
			const float Yaw = RNG.FRandRange(0.f, Entry.RandomYawRange);

			FRotator Rotation(0.f, Yaw, 0.f);
			if (Entry.bAlignToSurface)
			{
				Rotation = FRotationMatrix::MakeFromZ(HitNormal).Rotator();
				Rotation.Yaw += Yaw;
			}

			FTransform InstanceTransform(Rotation, HitLocation, FVector(Scale));
			// Convert to component-local space
			InstanceTransform = InstanceTransform.GetRelativeTransform(HISM->GetComponentTransform());

			HISM->AddInstance(InstanceTransform, /*bWorldSpace=*/false);
			++TotalPlaced;
		}
	}

	// Process scatter layers
	for (const FBiomeScatterLayer& Layer : BiomeDefinition->ScatterLayers)
	{
		if (Layer.Meshes.Num() == 0) continue;

		const float Area100Sq = (AreaX / 100.f) * (AreaY / 100.f);
		const int32 TargetCount = FMath::RoundToInt(Layer.DensityPer100Sq * Area100Sq * EffectiveDensity);

		for (int32 i = 0; i < TargetCount; ++i)
		{
			// Pick random mesh from layer
			const int32 MeshIdx = RNG.RandRange(0, Layer.Meshes.Num() - 1);
			UStaticMesh* Mesh = Layer.Meshes[MeshIdx].LoadSynchronous();
			if (!Mesh) continue;

			UHierarchicalInstancedStaticMeshComponent* HISM = GetOrCreateHISM(Mesh, false);
			if (!HISM) continue;

			const float X = RNG.FRandRange(BoundsMin.X, BoundsMax.X);
			const float Y = RNG.FRandRange(BoundsMin.Y, BoundsMax.Y);
			const FVector TraceStart(X, Y, BoundsMax.Z + 100.f);
			const FVector TraceEnd(X, Y, BoundsMin.Z - 100.f);

			FHitResult Hit;
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(this);

			if (!World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, TraceChannel, QueryParams))
			{
				continue;
			}

			const float SlopeAngle = GetSlopeAngleFromNormal(Hit.ImpactNormal);
			if (SlopeAngle < Layer.MinSlopeAngle || SlopeAngle > Layer.MaxSlopeAngle)
			{
				continue;
			}

			if (IsInExclusionZone(Hit.ImpactPoint))
			{
				continue;
			}

			if (Layer.MinSpacing > 0.f && !CheckMinSpacing(HISM, Hit.ImpactPoint, Layer.MinSpacing))
			{
				continue;
			}

			const float Scale = RNG.FRandRange(Layer.MinScale, Layer.MaxScale);
			const float Yaw = RNG.FRandRange(0.f, 360.f);

			FRotator Rotation(0.f, Yaw, 0.f);
			if (Layer.bAlignToSurface)
			{
				Rotation = FRotationMatrix::MakeFromZ(Hit.ImpactNormal).Rotator();
				Rotation.Yaw += Yaw;
			}

			FTransform InstanceTransform(Rotation, Hit.ImpactPoint, FVector(Scale));
			InstanceTransform = InstanceTransform.GetRelativeTransform(HISM->GetComponentTransform());
			HISM->AddInstance(InstanceTransform, /*bWorldSpace=*/false);
			++TotalPlaced;
		}
	}

	UE_LOG(LogTemp, Display, TEXT("[VegetationScatter] %s: Placed %d instances (biome=%s)"),
		*GetName(), TotalPlaced, *BiomeDefinition->BiomeID.ToString());
}

void AARPGVegetationScatter::ClearVegetation()
{
	for (UHierarchicalInstancedStaticMeshComponent* HISM : HISMComponents)
	{
		if (HISM)
		{
			HISM->ClearInstances();
			HISM->DestroyComponent();
		}
	}
	HISMComponents.Empty();
}

void AARPGVegetationScatter::RegenerateWithSeed(int32 NewSeed)
{
	RandomSeed = NewSeed;
	GenerateVegetation();
}

int32 AARPGVegetationScatter::GetTotalInstanceCount() const
{
	int32 Total = 0;
	for (const UHierarchicalInstancedStaticMeshComponent* HISM : HISMComponents)
	{
		if (HISM)
		{
			Total += HISM->GetInstanceCount();
		}
	}
	return Total;
}

UHierarchicalInstancedStaticMeshComponent* AARPGVegetationScatter::GetOrCreateHISM(UStaticMesh* Mesh, bool bBlocksNav)
{
	// Check existing
	for (UHierarchicalInstancedStaticMeshComponent* Existing : HISMComponents)
	{
		if (Existing && Existing->GetStaticMesh() == Mesh)
		{
			return Existing;
		}
	}

	// Create new
	UHierarchicalInstancedStaticMeshComponent* HISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
	HISM->SetStaticMesh(Mesh);
	HISM->SetupAttachment(GetRootComponent());
	HISM->SetCollisionEnabled(bBlocksNav ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	HISM->SetCastShadow(true);
	HISM->RegisterComponent();

	HISMComponents.Add(HISM);
	return HISM;
}

bool AARPGVegetationScatter::IsInExclusionZone(const FVector& Point) const
{
	for (const AActor* Exclusion : ExclusionVolumes)
	{
		if (!Exclusion) continue;

		// Use actor bounds for simple check
		FVector Origin, BoxExtent;
		Exclusion->GetActorBounds(false, Origin, BoxExtent);

		const FVector Delta = (Point - Origin).GetAbs();
		if (Delta.X <= BoxExtent.X && Delta.Y <= BoxExtent.Y && Delta.Z <= BoxExtent.Z)
		{
			return true;
		}
	}
	return false;
}

float AARPGVegetationScatter::GetSlopeAngleFromNormal(const FVector& Normal)
{
	// Angle between surface normal and up vector
	const float Dot = FVector::DotProduct(Normal, FVector::UpVector);
	return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));
}

bool AARPGVegetationScatter::CheckMinSpacing(
	const UHierarchicalInstancedStaticMeshComponent* HISM, const FVector& Point, float MinSpacing) const
{
	if (!HISM) return true;

	const float MinSpacingSq = MinSpacing * MinSpacing;
	const int32 InstanceCount = HISM->GetInstanceCount();

	// For performance, only check the last N instances (spatial locality)
	const int32 CheckCount = FMath::Min(InstanceCount, 50);
	const int32 StartIdx = FMath::Max(0, InstanceCount - CheckCount);

	for (int32 i = StartIdx; i < InstanceCount; ++i)
	{
		FTransform InstanceTransform;
		if (HISM->GetInstanceTransform(i, InstanceTransform, /*bWorldSpace=*/true))
		{
			if (FVector::DistSquared(Point, InstanceTransform.GetLocation()) < MinSpacingSq)
			{
				return false;
			}
		}
	}
	return true;
}
