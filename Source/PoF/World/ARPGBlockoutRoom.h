#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARPGBlockoutRoom.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UNavModifierComponent;

/** Room shape for blockout generation. */
UENUM(BlueprintType)
enum class EBlockoutRoomShape : uint8
{
	Rectangular  UMETA(DisplayName = "Rectangular"),
	Circular     UMETA(DisplayName = "Circular (Approximated)")
};

/** Room purpose — affects default sizing and NavMesh treatment. */
UENUM(BlueprintType)
enum class EBlockoutRoomPurpose : uint8
{
	Corridor    UMETA(DisplayName = "Corridor"),
	SmallRoom   UMETA(DisplayName = "Small Room"),
	LargeRoom   UMETA(DisplayName = "Large Room"),
	Arena       UMETA(DisplayName = "Arena/Boss Room"),
	Hub         UMETA(DisplayName = "Town Hub"),
	Transition  UMETA(DisplayName = "Transition Space")
};

/**
 * A blockout room actor for rapid greybox level design.
 * Place these in the level and configure dimensions to build zone layouts.
 *
 * Provides:
 * - Configurable floor plane with dimensions
 * - Optional walls (for enclosed rooms)
 * - NavMesh bounds indicator
 * - Scale reference (UE units → meters)
 * - Room purpose tagging for encounter design
 */
UCLASS()
class POF_API AARPGBlockoutRoom : public AActor
{
	GENERATED_BODY()

public:
	AARPGBlockoutRoom();

	UFUNCTION(BlueprintPure, Category = "Blockout")
	EBlockoutRoomPurpose GetRoomPurpose() const { return RoomPurpose; }

	UFUNCTION(BlueprintPure, Category = "Blockout")
	FVector GetRoomDimensions() const { return RoomDimensions; }

	/** Get the world-space bounds of this room (for NavMesh coverage checks). */
	UFUNCTION(BlueprintPure, Category = "Blockout")
	FBox GetRoomWorldBounds() const;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Room purpose — affects default sizing suggestions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blockout")
	EBlockoutRoomPurpose RoomPurpose = EBlockoutRoomPurpose::SmallRoom;

	/** Room shape. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blockout")
	EBlockoutRoomShape RoomShape = EBlockoutRoomShape::Rectangular;

	/**
	 * Room dimensions in UE units (X = length, Y = width, Z = height).
	 * Reference: 100 UE units ≈ 1 meter.
	 * Corridor: ~300x1500x300, Small Room: ~800x800x300,
	 * Large Room: ~1500x1500x400, Arena: ~3000x3000x500
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blockout", meta = (ClampMin = "100"))
	FVector RoomDimensions = FVector(800.f, 800.f, 300.f);

	/** If true, generates wall outlines (for enclosed rooms). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blockout")
	bool bGenerateWalls = true;

	/** Wall thickness in UE units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blockout", meta = (ClampMin = "10", EditCondition = "bGenerateWalls"))
	float WallThickness = 20.f;

	/** Display name for this room (shown in editor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blockout")
	FString RoomLabel;

	/** Floor mesh component. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Blockout")
	TObjectPtr<UStaticMeshComponent> FloorMesh;

	/** Box showing room bounds in editor. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Blockout")
	TObjectPtr<UBoxComponent> BoundsIndicator;

private:
	void UpdateFloorScale();
	void UpdateWalls();
	void CreateOrUpdateWall(int32 WallIndex, const FVector& RelativeLocation, const FVector& Scale);
	FColor GetPurposeColor() const;

	/** Dynamically created wall meshes. Order: +X, -X, +Y, -Y */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> WallMeshes;
};
