#include "LevelDesign/ARPGEncounterArena.h"
#include "LevelDesign/ARPGCoverPoint.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

AARPGEncounterArena::AARPGEncounterArena()
{
	PrimaryActorTick.bCanEverTick = false;

	ArenaBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("ArenaBounds"));
	ArenaBounds->SetBoxExtent(FVector(1500.f, 1500.f, 500.f));
	ArenaBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ArenaBounds->SetHiddenInGame(true);
	ArenaBounds->ShapeColor = FColor::Red;
	SetRootComponent(ArenaBounds);
}

void AARPGEncounterArena::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoScan)
	{
		ScanArena();
	}
}

void AARPGEncounterArena::ScanArena()
{
	DiscoveredCoverPoints.Empty();

	const FVector ArenaCenter = GetActorLocation();
	const FVector ArenaExtent = ArenaBounds->GetScaledBoxExtent();
	const FBox ArenaBox(ArenaCenter - ArenaExtent, ArenaCenter + ArenaExtent);

	UWorld* World = GetWorld();
	if (!World) return;

	// Find all cover points within arena bounds
	for (TActorIterator<AARPGCoverPoint> It(World); It; ++It)
	{
		AARPGCoverPoint* CoverPoint = *It;
		if (ArenaBox.IsInsideOrOn(CoverPoint->GetActorLocation()))
		{
			DiscoveredCoverPoints.Add(CoverPoint);
		}
	}

	// Auto-generate tactical point data from cover points
	for (const TWeakObjectPtr<AARPGCoverPoint>& CoverPtr : DiscoveredCoverPoints)
	{
		if (!CoverPtr.IsValid()) continue;

		FArenaTacticalPoint TacPoint;
		TacPoint.Location = CoverPtr->GetActorLocation();
		TacPoint.bHasNearbyCover = true;

		// Determine elevation tier
		const float HeightAboveCenter = TacPoint.Location.Z - ArenaCenter.Z;
		if (HeightAboveCenter >= HighGroundThreshold)
		{
			TacPoint.Tier = EArenaTier::High;
			TacPoint.bGoodForRanged = true;
		}
		else if (HeightAboveCenter >= ElevatedThreshold)
		{
			TacPoint.Tier = EArenaTier::Elevated;
			TacPoint.bGoodForRanged = true;
		}
		else
		{
			TacPoint.Tier = EArenaTier::Ground;
		}

		// Check if it's a flanking position (far from center axis)
		const FVector ToPoint = (TacPoint.Location - ArenaCenter);
		const float LateralDist = FVector(ToPoint.X, ToPoint.Y, 0.f).Size();
		TacPoint.bFlankingPosition = LateralDist > ArenaExtent.GetMin() * 0.5f;

		TacticalPoints.Add(TacPoint);
	}

	UE_LOG(LogTemp, Display, TEXT("[EncounterArena] %s: Found %d cover points, %d tactical points"),
		*GetName(), DiscoveredCoverPoints.Num(), TacticalPoints.Num());
}

TArray<AARPGCoverPoint*> AARPGEncounterArena::GetCoverPoints() const
{
	TArray<AARPGCoverPoint*> Result;
	for (const TWeakObjectPtr<AARPGCoverPoint>& Ptr : DiscoveredCoverPoints)
	{
		if (Ptr.IsValid())
		{
			Result.Add(Ptr.Get());
		}
	}
	return Result;
}

TArray<AARPGCoverPoint*> AARPGEncounterArena::GetAvailableCoverPoints() const
{
	TArray<AARPGCoverPoint*> Result;
	for (const TWeakObjectPtr<AARPGCoverPoint>& Ptr : DiscoveredCoverPoints)
	{
		if (Ptr.IsValid() && Ptr->IsAvailable())
		{
			Result.Add(Ptr.Get());
		}
	}
	return Result;
}

AARPGCoverPoint* AARPGEncounterArena::FindBestCoverFrom(
	const FVector& AttackerPosition, const FVector& DefenderPosition) const
{
	AARPGCoverPoint* BestCover = nullptr;
	float BestScore = -1.f;

	for (const TWeakObjectPtr<AARPGCoverPoint>& Ptr : DiscoveredCoverPoints)
	{
		if (!Ptr.IsValid() || !Ptr->IsAvailable()) continue;

		AARPGCoverPoint* Cover = Ptr.Get();

		// Must actually provide cover from the attacker
		if (!Cover->ProvidesCoverFrom(AttackerPosition)) continue;

		// Score: closer to defender is better, but not too close to attacker
		const float DistToDefender = FVector::Dist(Cover->GetActorLocation(), DefenderPosition);
		const float DistToAttacker = FVector::Dist(Cover->GetActorLocation(), AttackerPosition);

		// Prefer cover that is close to the defender but maintains distance from attacker
		float Score = DistToAttacker / FMath::Max(DistToDefender, 1.f);

		if (Score > BestScore)
		{
			BestScore = Score;
			BestCover = Cover;
		}
	}

	return BestCover;
}

TArray<FArenaTacticalPoint> AARPGEncounterArena::GetPointsByTier(EArenaTier Tier) const
{
	TArray<FArenaTacticalPoint> Result;
	for (const FArenaTacticalPoint& Point : TacticalPoints)
	{
		if (Point.Tier == Tier)
		{
			Result.Add(Point);
		}
	}
	return Result;
}

bool AARPGEncounterArena::FindFlankingPosition(
	const FVector& TargetPos, const FVector& AttackerPos, FVector& OutFlankPos) const
{
	// Find a tactical point that is roughly 90 degrees from the attacker-target line
	const FVector AttackDir = (TargetPos - AttackerPos).GetSafeNormal2D();
	const FVector FlankDir = FVector(-AttackDir.Y, AttackDir.X, 0.f); // Perpendicular

	float BestScore = -1.f;
	bool bFound = false;

	for (const FArenaTacticalPoint& Point : TacticalPoints)
	{
		if (!Point.bFlankingPosition) continue;

		const FVector ToPoint = (Point.Location - TargetPos).GetSafeNormal2D();
		const float FlankDot = FMath::Abs(FVector::DotProduct(ToPoint, FlankDir));

		// Higher dot = more perpendicular to attack direction = better flank
		if (FlankDot > BestScore)
		{
			BestScore = FlankDot;
			OutFlankPos = Point.Location;
			bFound = true;
		}
	}

	return bFound;
}

float AARPGEncounterArena::GetArenaRadius() const
{
	const FVector Extent = ArenaBounds->GetScaledBoxExtent();
	return FMath::Max(Extent.X, Extent.Y);
}
