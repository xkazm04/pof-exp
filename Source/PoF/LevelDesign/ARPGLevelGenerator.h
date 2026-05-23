#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelDesign/ARPGRoomTemplate.h"
#include "ARPGLevelGenerator.generated.h"
class AARPGBlockoutRoom;
class UBoxComponent;

/** A placed room instance in the generated layout. */
USTRUCT(BlueprintType)
struct FPlacedRoom
{
	GENERATED_BODY()

	/** The template used for this room. */
	UPROPERTY(BlueprintReadOnly, Category = "Generation")
	TObjectPtr<UARPGRoomTemplate> Template;

	/** World-space position of the room center. */
	UPROPERTY(BlueprintReadOnly, Category = "Generation")
	FVector WorldPosition = FVector::ZeroVector;

	/** Room index in the layout sequence (0 = start room). */
	UPROPERTY(BlueprintReadOnly, Category = "Generation")
	int32 RoomIndex = 0;

	/** Depth from the start room (for difficulty scaling). */
	UPROPERTY(BlueprintReadOnly, Category = "Generation")
	int32 Depth = 0;

	/** Indices of connected rooms. */
	UPROPERTY(BlueprintReadOnly, Category = "Generation")
	TArray<int32> ConnectedRoomIndices;

	/** Directions whose wall is an open archway to a connected room. */
	UPROPERTY(BlueprintReadOnly, Category = "Generation")
	TArray<ERoomConnectionDirection> OpenDirections;

	/** The spawned blockout room actor. */
	UPROPERTY(BlueprintReadOnly, Category = "Generation")
	TWeakObjectPtr<AARPGBlockoutRoom> BlockoutActor;
};

/** Broadcast when generation completes: (NumRooms) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLevelGenerated, int32, NumRooms);

/**
 * Procedural level generator that places rooms from a template pool.
 *
 * Uses a graph-based approach:
 * 1. Start with a seed room
 * 2. Pick open connection slots and attach new rooms
 * 3. Respect overlap/collision constraints
 * 4. Build corridors between connected rooms
 * 5. Apply difficulty scaling based on depth from start
 */
UCLASS()
class POF_API AARPGLevelGenerator : public AActor
{
	GENERATED_BODY()

public:
	AARPGLevelGenerator();

	/** Generate a new level layout. Destroys previous layout if any. */
	UFUNCTION(BlueprintCallable, Category = "LevelDesign|Generation")
	void GenerateLevel();

	/** Clear the current generated layout. */
	UFUNCTION(BlueprintCallable, Category = "LevelDesign|Generation")
	void ClearLevel();

	/** Get the generated room layout. */
	UFUNCTION(BlueprintPure, Category = "LevelDesign|Generation")
	const TArray<FPlacedRoom>& GetPlacedRooms() const { return PlacedRooms; }

	/** Get a room by index. */
	UFUNCTION(BlueprintPure, Category = "LevelDesign|Generation")
	bool GetPlacedRoom(int32 Index, FPlacedRoom& OutRoom) const;

	UPROPERTY(BlueprintAssignable, Category = "Events|LevelDesign")
	FOnLevelGenerated OnLevelGenerated;

protected:
	/** Pool of room templates to pick from. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Templates")
	TArray<TObjectPtr<UARPGRoomTemplate>> RoomTemplatePool;

	/** Template to use for the starting room (if null, picks from pool). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Templates")
	TObjectPtr<UARPGRoomTemplate> StartRoomTemplate;

	/** Template to use for the boss/end room (if null, picks from pool). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Templates")
	TObjectPtr<UARPGRoomTemplate> EndRoomTemplate;

	/** Total rooms to generate (including start/end). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "2"))
	int32 TargetRoomCount = 8;

	/** Random seed. 0 = random each time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	int32 RandomSeed = 0;

	/** Minimum corridor length between rooms. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Corridors", meta = (ClampMin = "100"))
	float MinCorridorLength = 200.f;

	/** Maximum corridor length between rooms. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Corridors", meta = (ClampMin = "100"))
	float MaxCorridorLength = 600.f;

	/** Gap between rooms to prevent overlap (UE units). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "0"))
	float RoomPadding = 50.f;

	/** Maximum attempts to place a room before giving up on a slot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "1"))
	int32 MaxPlacementAttempts = 10;

	/** If true, spawns BlockoutRoom actors for each placed room. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Visualization")
	bool bSpawnBlockoutActors = true;

	/** If true, generates on BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	bool bGenerateOnBeginPlay = false;

	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TArray<FPlacedRoom> PlacedRooms;

	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> SpawnedActors;

	FRandomStream RNG;

	/** Pick a weighted-random template from the pool. */
	UARPGRoomTemplate* PickRandomTemplate();

	/** Check if a room can be placed at a given position without overlapping existing rooms. */
	bool CanPlaceRoom(const UARPGRoomTemplate* Template, const FVector& Position) const;

	/** Compute world position for a new room connecting to an existing slot. */
	FVector ComputeConnectionPosition(const FPlacedRoom& ExistingRoom, int32 SlotIndex,
		const UARPGRoomTemplate* NewTemplate, int32 NewSlotIndex, float CorridorLength) const;

	/** Get the world-space direction vector for a connection direction. */
	static FVector GetDirectionVector(ERoomConnectionDirection Dir);

	/** Spawn a BlockoutRoom actor for a placed room. */
	AARPGBlockoutRoom* SpawnBlockoutForRoom(const FPlacedRoom& Room);

	/** Track open connection slots: (RoomIndex, SlotIndex) */
	TArray<TPair<int32, int32>> OpenSlots;
};
