#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ARPGRoomTemplate.generated.h"

class AARPGEnemyCharacter;

/** Direction a connection slot faces. */
UENUM(BlueprintType)
enum class ERoomConnectionDirection : uint8
{
	North  UMETA(DisplayName = "North (+X)"),
	South  UMETA(DisplayName = "South (-X)"),
	East   UMETA(DisplayName = "East (+Y)"),
	West   UMETA(DisplayName = "West (-Y)")
};

/** A single connection slot on a room template. */
USTRUCT(BlueprintType)
struct FRoomConnectionSlot
{
	GENERATED_BODY()

	/** Local-space offset from room center to the connection point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection")
	FVector LocalOffset = FVector::ZeroVector;

	/** Which direction this slot faces (determines valid connections). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection")
	ERoomConnectionDirection Direction = ERoomConnectionDirection::North;

	/** Width of the doorway/corridor at this connection. Must match connecting room. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection", meta = (ClampMin = "100"))
	float ConnectionWidth = 300.f;
};

/** Prop placement rule within a room. */
USTRUCT(BlueprintType)
struct FRoomPropPlacement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Props")
	TSoftObjectPtr<UStaticMesh> PropMesh;

	/** Local-space offset from room center. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Props")
	FVector LocalOffset = FVector::ZeroVector;

	/** Local rotation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Props")
	FRotator LocalRotation = FRotator::ZeroRotator;

	/** Scale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Props")
	FVector Scale = FVector::OneVector;

	/** Probability of placing this prop (0-1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Props", meta = (ClampMin = "0", ClampMax = "1"))
	float Probability = 1.0f;
};

/** Cover point definition within a room template. */
USTRUCT(BlueprintType)
struct FRoomCoverPoint
{
	GENERATED_BODY()

	/** Local-space position of cover. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover")
	FVector LocalOffset = FVector::ZeroVector;

	/** Direction the cover faces (normal of the cover surface). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover")
	FVector CoverNormal = FVector(1.f, 0.f, 0.f);

	/** Whether this is full (standing) cover or half (crouching) cover. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover")
	bool bFullCover = false;
};

/**
 * Data asset defining a room template for procedural level generation.
 * Describes room geometry, connection slots, prop placement, and encounter setup.
 */
UCLASS(BlueprintType)
class POF_API UARPGRoomTemplate : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Unique template identifier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room")
	FName TemplateID;

	/** Room dimensions (X=length, Y=width, Z=height). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room", meta = (ClampMin = "100"))
	FVector RoomSize = FVector(800.f, 800.f, 300.f);

	/** Room purpose (maps to EBlockoutRoomPurpose). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room")
	FName RoomPurpose = TEXT("SmallRoom");

	/** Connection slots where corridors/doors can attach. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room|Connections")
	TArray<FRoomConnectionSlot> ConnectionSlots;

	/** Cover points for tactical AI. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room|Tactical")
	TArray<FRoomCoverPoint> CoverPoints;

	/** Static prop placements for environmental storytelling. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room|Props")
	TArray<FRoomPropPlacement> Props;

	/** Whether enemies can spawn in this room. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room|Encounters")
	bool bAllowEnemySpawns = true;

	/** Max enemies this room type supports simultaneously. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room|Encounters", meta = (ClampMin = "0", EditCondition = "bAllowEnemySpawns"))
	int32 MaxEnemies = 5;

	/** Weight for random selection during procedural generation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room|Generation", meta = (ClampMin = "0.01"))
	float SelectionWeight = 1.0f;

	/** Minimum times this room type can appear in a generated layout. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room|Generation", meta = (ClampMin = "0"))
	int32 MinCount = 0;

	/** Maximum times this room type can appear. 0 = unlimited. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room|Generation", meta = (ClampMin = "0"))
	int32 MaxCount = 0;

	/** Get the opposite direction for a connection. */
	static ERoomConnectionDirection GetOppositeDirection(ERoomConnectionDirection Dir);

	/** Check if two connection slots can link (opposite directions, matching width). */
	static bool CanConnect(const FRoomConnectionSlot& A, const FRoomConnectionSlot& B);

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
