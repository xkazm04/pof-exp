#include "World/ARPGBlockoutRoom.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

AARPGBlockoutRoom::AARPGBlockoutRoom()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	FloorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FloorMesh"));
	FloorMesh->SetupAttachment(Root);
	FloorMesh->SetCollisionProfileName(TEXT("BlockAll"));

	// Use engine cube as default floor (scaled flat)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		FloorMesh->SetStaticMesh(CubeFinder.Object);
	}

	BoundsIndicator = CreateDefaultSubobject<UBoxComponent>(TEXT("BoundsIndicator"));
	BoundsIndicator->SetupAttachment(Root);
	BoundsIndicator->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoundsIndicator->SetHiddenInGame(true);
	BoundsIndicator->ShapeColor = FColor::Cyan;
	BoundsIndicator->SetLineThickness(3.f);
}

void AARPGBlockoutRoom::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateFloorScale();
	UpdateWalls();

	// Color-code bounds indicator by room purpose
	BoundsIndicator->ShapeColor = GetPurposeColor();
}

void AARPGBlockoutRoom::UpdateFloorScale()
{
	// Floor is a cube scaled to room dimensions
	// Cube mesh is 100x100x100 by default
	const float FloorThickness = 10.f; // 10 UE units thick floor
	const FVector FloorScale(
		RoomDimensions.X / 100.f,
		RoomDimensions.Y / 100.f,
		FloorThickness / 100.f
	);
	FloorMesh->SetRelativeScale3D(FloorScale);
	FloorMesh->SetRelativeLocation(FVector(0.f, 0.f, -FloorThickness * 0.5f));

	// Update bounds indicator to show full room volume
	BoundsIndicator->SetBoxExtent(RoomDimensions * 0.5f);
	BoundsIndicator->SetRelativeLocation(FVector(0.f, 0.f, RoomDimensions.Z * 0.5f));
}

void AARPGBlockoutRoom::UpdateWalls()
{
	// Remove walls if not needed
	if (!bGenerateWalls)
	{
		for (UStaticMeshComponent* Wall : WallMeshes)
		{
			if (Wall) Wall->DestroyComponent();
		}
		WallMeshes.Empty();
		return;
	}

	UStaticMesh* CubeMesh = FloorMesh ? FloorMesh->GetStaticMesh() : nullptr;
	if (!CubeMesh) return;

	// Ensure we have 4 wall slots
	while (WallMeshes.Num() < 4)
	{
		WallMeshes.Add(nullptr);
	}

	const float HalfX = RoomDimensions.X * 0.5f;
	const float HalfY = RoomDimensions.Y * 0.5f;
	const float HalfZ = RoomDimensions.Z * 0.5f;
	const float HalfThick = WallThickness * 0.5f;

	// Wall scale: cube is 100x100x100 base
	// +X wall
	CreateOrUpdateWall(0,
		FVector(HalfX + HalfThick, 0.f, HalfZ),
		FVector(WallThickness / 100.f, RoomDimensions.Y / 100.f, RoomDimensions.Z / 100.f));
	// -X wall
	CreateOrUpdateWall(1,
		FVector(-HalfX - HalfThick, 0.f, HalfZ),
		FVector(WallThickness / 100.f, RoomDimensions.Y / 100.f, RoomDimensions.Z / 100.f));
	// +Y wall
	CreateOrUpdateWall(2,
		FVector(0.f, HalfY + HalfThick, HalfZ),
		FVector((RoomDimensions.X + WallThickness * 2.f) / 100.f, WallThickness / 100.f, RoomDimensions.Z / 100.f));
	// -Y wall
	CreateOrUpdateWall(3,
		FVector(0.f, -HalfY - HalfThick, HalfZ),
		FVector((RoomDimensions.X + WallThickness * 2.f) / 100.f, WallThickness / 100.f, RoomDimensions.Z / 100.f));
}

void AARPGBlockoutRoom::CreateOrUpdateWall(int32 WallIndex, const FVector& RelativeLocation, const FVector& Scale)
{
	UStaticMesh* CubeMesh = FloorMesh ? FloorMesh->GetStaticMesh() : nullptr;
	if (!CubeMesh) return;

	UStaticMeshComponent* Wall = WallMeshes[WallIndex];
	if (!Wall)
	{
		const FName WallName = *FString::Printf(TEXT("Wall_%d"), WallIndex);
		Wall = NewObject<UStaticMeshComponent>(this, WallName);
		Wall->SetStaticMesh(CubeMesh);
		Wall->SetCollisionProfileName(TEXT("BlockAll"));
		Wall->SetupAttachment(GetRootComponent());
		Wall->RegisterComponent();
		WallMeshes[WallIndex] = Wall;
	}

	Wall->SetRelativeLocation(RelativeLocation);
	Wall->SetRelativeScale3D(Scale);
}

FColor AARPGBlockoutRoom::GetPurposeColor() const
{
	switch (RoomPurpose)
	{
	case EBlockoutRoomPurpose::Corridor:   return FColor::White;
	case EBlockoutRoomPurpose::SmallRoom:  return FColor::Cyan;
	case EBlockoutRoomPurpose::LargeRoom:  return FColor::Blue;
	case EBlockoutRoomPurpose::Arena:      return FColor::Red;
	case EBlockoutRoomPurpose::Hub:        return FColor::Green;
	case EBlockoutRoomPurpose::Transition: return FColor::Yellow;
	default:                               return FColor::Cyan;
	}
}

FBox AARPGBlockoutRoom::GetRoomWorldBounds() const
{
	const FVector Origin = GetActorLocation();
	const FVector HalfSize = RoomDimensions * 0.5f;
	return FBox(Origin - HalfSize, Origin + FVector(HalfSize.X, HalfSize.Y, RoomDimensions.Z));
}
