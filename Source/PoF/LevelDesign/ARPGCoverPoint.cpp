#include "LevelDesign/ARPGCoverPoint.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"

AARPGCoverPoint::AARPGCoverPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

#if WITH_EDITORONLY_DATA
	CoverDirectionArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("CoverDirection"));
	CoverDirectionArrow->SetupAttachment(Root);
	CoverDirectionArrow->ArrowColor = FColor::Yellow;
	CoverDirectionArrow->SetHiddenInGame(true);

	CoverVisualizer = CreateDefaultSubobject<UBoxComponent>(TEXT("CoverVisualizer"));
	CoverVisualizer->SetupAttachment(Root);
	CoverVisualizer->SetBoxExtent(FVector(10.f, 75.f, 50.f));
	CoverVisualizer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CoverVisualizer->SetHiddenInGame(true);
	CoverVisualizer->ShapeColor = FColor::Yellow;
#endif
}

bool AARPGCoverPoint::Claim(AActor* Claimant)
{
	if (!Claimant) return false;

	// Clean up stale claim
	if (OccupantActor.IsValid() && OccupantActor.Get() != Claimant)
	{
		return false;
	}

	OccupantActor = Claimant;
	return true;
}

void AARPGCoverPoint::Release()
{
	OccupantActor = nullptr;
}

FVector AARPGCoverPoint::GetCoverDirection() const
{
	return GetActorForwardVector();
}

void AARPGCoverPoint::GetPeekPositions(FVector& OutLeftPeek, FVector& OutRightPeek) const
{
	const FVector Right = GetActorRightVector();
	const FVector Forward = GetActorForwardVector();
	const FVector Loc = GetActorLocation();

	OutLeftPeek = Loc - Right * (CoverWidth * 0.5f + PeekDistance) + Forward * PeekDistance;
	OutRightPeek = Loc + Right * (CoverWidth * 0.5f + PeekDistance) + Forward * PeekDistance;
}

bool AARPGCoverPoint::ProvidesCoverFrom(const FVector& AttackerPosition) const
{
	const FVector ToAttacker = (AttackerPosition - GetActorLocation()).GetSafeNormal();
	const FVector CoverDir = GetCoverDirection();

	// Cover is effective when the attacker is on the other side of the cover surface
	// (facing away from the cover direction = attacker is behind relative to cover facing)
	const float Dot = FVector::DotProduct(ToAttacker, CoverDir);

	// If dot < 0, attacker is behind the cover — the cover protects against them
	// We actually want the attacker to be in FRONT of the cover normal for it to block
	// Cover normal points toward what it blocks: if attacker is in the cone in front of normal, cover works
	const float AngleRad = FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f));
	return FMath::RadiansToDegrees(AngleRad) <= CoverAngle;
}
