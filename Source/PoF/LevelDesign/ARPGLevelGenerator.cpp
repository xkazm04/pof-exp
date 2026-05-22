#include "LevelDesign/ARPGLevelGenerator.h"
#include "LevelDesign/ARPGRoomTemplate.h"
#include "World/ARPGBlockoutRoom.h"

AARPGLevelGenerator::AARPGLevelGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AARPGLevelGenerator::BeginPlay()
{
	Super::BeginPlay();

	if (bGenerateOnBeginPlay)
	{
		GenerateLevel();
	}
}

void AARPGLevelGenerator::GenerateLevel()
{
	ClearLevel();

	if (RoomTemplatePool.Num() == 0 && !StartRoomTemplate)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LevelGenerator] %s: No room templates configured"), *GetName());
		return;
	}

	// Initialize RNG
	RNG = (RandomSeed != 0) ? FRandomStream(RandomSeed) : FRandomStream(FMath::Rand());

	// Place starting room
	UARPGRoomTemplate* StartTemplate = StartRoomTemplate ? StartRoomTemplate.Get() : PickRandomTemplate();
	if (!StartTemplate) return;

	FPlacedRoom StartRoom;
	StartRoom.Template = StartTemplate;
	StartRoom.WorldPosition = GetActorLocation();
	StartRoom.RoomIndex = 0;
	StartRoom.Depth = 0;
	PlacedRooms.Add(StartRoom);

	// Register open slots from start room
	for (int32 i = 0; i < StartTemplate->ConnectionSlots.Num(); ++i)
	{
		OpenSlots.Add(TPair<int32, int32>(0, i));
	}

	// Iteratively place rooms by connecting to open slots
	int32 SafetyCounter = TargetRoomCount * MaxPlacementAttempts;
	while (PlacedRooms.Num() < TargetRoomCount && OpenSlots.Num() > 0 && SafetyCounter-- > 0)
	{
		// Pick a random open slot
		const int32 SlotPickIdx = RNG.RandRange(0, OpenSlots.Num() - 1);
		const int32 ExistingRoomIdx = OpenSlots[SlotPickIdx].Key;
		const int32 ExistingSlotIdx = OpenSlots[SlotPickIdx].Value;

		const FPlacedRoom& ExistingRoom = PlacedRooms[ExistingRoomIdx];
		const FRoomConnectionSlot& ExistingSlot = ExistingRoom.Template->ConnectionSlots[ExistingSlotIdx];

		// Determine if this should be the end room
		const bool bIsLastRoom = (PlacedRooms.Num() == TargetRoomCount - 1);
		UARPGRoomTemplate* NewTemplate = nullptr;
		if (bIsLastRoom && EndRoomTemplate)
		{
			NewTemplate = EndRoomTemplate;
		}
		else
		{
			NewTemplate = PickRandomTemplate();
		}

		if (!NewTemplate || NewTemplate->ConnectionSlots.Num() == 0)
		{
			OpenSlots.RemoveAt(SlotPickIdx);
			continue;
		}

		// Find a compatible slot on the new template
		const ERoomConnectionDirection NeededDir = UARPGRoomTemplate::GetOppositeDirection(ExistingSlot.Direction);
		int32 CompatibleSlotIdx = INDEX_NONE;
		for (int32 i = 0; i < NewTemplate->ConnectionSlots.Num(); ++i)
		{
			if (NewTemplate->ConnectionSlots[i].Direction == NeededDir &&
				UARPGRoomTemplate::CanConnect(ExistingSlot, NewTemplate->ConnectionSlots[i]))
			{
				CompatibleSlotIdx = i;
				break;
			}
		}

		if (CompatibleSlotIdx == INDEX_NONE)
		{
			OpenSlots.RemoveAt(SlotPickIdx);
			continue;
		}

		// Compute placement position
		const float CorridorLen = RNG.FRandRange(MinCorridorLength, MaxCorridorLength);
		const FVector NewPos = ComputeConnectionPosition(
			ExistingRoom, ExistingSlotIdx, NewTemplate, CompatibleSlotIdx, CorridorLen);

		// Check for overlaps
		if (!CanPlaceRoom(NewTemplate, NewPos))
		{
			// Slot might work with a different template later, but decrement attempts
			OpenSlots.RemoveAt(SlotPickIdx);
			continue;
		}

		// Place the room
		FPlacedRoom NewRoom;
		NewRoom.Template = NewTemplate;
		NewRoom.WorldPosition = NewPos;
		NewRoom.RoomIndex = PlacedRooms.Num();
		NewRoom.Depth = ExistingRoom.Depth + 1;
		NewRoom.ConnectedRoomIndices.Add(ExistingRoomIdx);

		const int32 NewRoomIdx = PlacedRooms.Num();

		// Update existing room connections
		PlacedRooms[ExistingRoomIdx].ConnectedRoomIndices.Add(NewRoomIdx);

		PlacedRooms.Add(NewRoom);

		// Remove the used slot
		OpenSlots.RemoveAt(SlotPickIdx);

		// Add new room's open slots (except the one used for connection)
		for (int32 i = 0; i < NewTemplate->ConnectionSlots.Num(); ++i)
		{
			if (i != CompatibleSlotIdx)
			{
				OpenSlots.Add(TPair<int32, int32>(NewRoomIdx, i));
			}
		}
	}

	// Spawn blockout actors
	if (bSpawnBlockoutActors)
	{
		for (FPlacedRoom& Room : PlacedRooms)
		{
			AARPGBlockoutRoom* Blockout = SpawnBlockoutForRoom(Room);
			if (Blockout)
			{
				Room.BlockoutActor = Blockout;
				SpawnedActors.Add(Blockout);
			}
		}
	}

	UE_LOG(LogTemp, Display, TEXT("[LevelGenerator] %s: Generated %d rooms (target=%d, seed=%d)"),
		*GetName(), PlacedRooms.Num(), TargetRoomCount, RNG.GetCurrentSeed());

	OnLevelGenerated.Broadcast(PlacedRooms.Num());
}

void AARPGLevelGenerator::ClearLevel()
{
	for (const TWeakObjectPtr<AActor>& Actor : SpawnedActors)
	{
		if (Actor.IsValid())
		{
			Actor->Destroy();
		}
	}
	SpawnedActors.Empty();
	PlacedRooms.Empty();
	OpenSlots.Empty();
}

bool AARPGLevelGenerator::GetPlacedRoom(int32 Index, FPlacedRoom& OutRoom) const
{
	if (!PlacedRooms.IsValidIndex(Index)) return false;
	OutRoom = PlacedRooms[Index];
	return true;
}

UARPGRoomTemplate* AARPGLevelGenerator::PickRandomTemplate()
{
	if (RoomTemplatePool.Num() == 0) return nullptr;

	// Weighted random selection
	float TotalWeight = 0.f;
	for (const UARPGRoomTemplate* Template : RoomTemplatePool)
	{
		if (Template)
		{
			// Respect max count
			if (Template->MaxCount > 0)
			{
				int32 UsedCount = 0;
				for (const FPlacedRoom& Placed : PlacedRooms)
				{
					if (Placed.Template == Template) ++UsedCount;
				}
				if (UsedCount >= Template->MaxCount) continue;
			}
			TotalWeight += Template->SelectionWeight;
		}
	}

	if (TotalWeight <= 0.f) return RoomTemplatePool[0];

	float Roll = RNG.FRandRange(0.f, TotalWeight);
	for (UARPGRoomTemplate* Template : RoomTemplatePool)
	{
		if (!Template) continue;

		// Skip templates at max count
		if (Template->MaxCount > 0)
		{
			int32 UsedCount = 0;
			for (const FPlacedRoom& Placed : PlacedRooms)
			{
				if (Placed.Template == Template) ++UsedCount;
			}
			if (UsedCount >= Template->MaxCount) continue;
		}

		Roll -= Template->SelectionWeight;
		if (Roll <= 0.f) return Template;
	}

	return RoomTemplatePool[0];
}

bool AARPGLevelGenerator::CanPlaceRoom(const UARPGRoomTemplate* Template, const FVector& Position) const
{
	if (!Template) return false;

	const FVector HalfSize = Template->RoomSize * 0.5f + FVector(RoomPadding);

	for (const FPlacedRoom& Existing : PlacedRooms)
	{
		if (!Existing.Template) continue;

		const FVector ExistingHalf = Existing.Template->RoomSize * 0.5f + FVector(RoomPadding);

		// AABB overlap test
		const FVector Delta = (Position - Existing.WorldPosition).GetAbs();
		const FVector Combined = HalfSize + ExistingHalf;

		if (Delta.X < Combined.X && Delta.Y < Combined.Y)
		{
			return false;
		}
	}

	return true;
}

FVector AARPGLevelGenerator::ComputeConnectionPosition(
	const FPlacedRoom& ExistingRoom, int32 SlotIndex,
	const UARPGRoomTemplate* NewTemplate, int32 NewSlotIndex, float CorridorLength) const
{
	const FRoomConnectionSlot& ExistingSlot = ExistingRoom.Template->ConnectionSlots[SlotIndex];
	const FRoomConnectionSlot& NewSlot = NewTemplate->ConnectionSlots[NewSlotIndex];

	// Direction the existing slot faces
	const FVector Dir = GetDirectionVector(ExistingSlot.Direction);

	// World position of the existing slot
	const FVector ExistingSlotWorld = ExistingRoom.WorldPosition + ExistingSlot.LocalOffset;

	// The new room's slot needs to align with the existing slot
	// New room center = existing slot world pos + corridor + direction - new slot local offset
	const FVector NewRoomCenter = ExistingSlotWorld
		+ Dir * (CorridorLength + ExistingRoom.Template->RoomSize.GetComponentForAxis(
			FMath::Abs(Dir.X) > 0.5f ? EAxis::X : EAxis::Y) * 0.5f
			+ NewTemplate->RoomSize.GetComponentForAxis(
			FMath::Abs(Dir.X) > 0.5f ? EAxis::X : EAxis::Y) * 0.5f)
		- NewSlot.LocalOffset;

	return NewRoomCenter;
}

FVector AARPGLevelGenerator::GetDirectionVector(ERoomConnectionDirection Dir)
{
	switch (Dir)
	{
	case ERoomConnectionDirection::North: return FVector(1.f, 0.f, 0.f);
	case ERoomConnectionDirection::South: return FVector(-1.f, 0.f, 0.f);
	case ERoomConnectionDirection::East:  return FVector(0.f, 1.f, 0.f);
	case ERoomConnectionDirection::West:  return FVector(0.f, -1.f, 0.f);
	default:                              return FVector(1.f, 0.f, 0.f);
	}
}

AARPGBlockoutRoom* AARPGLevelGenerator::SpawnBlockoutForRoom(const FPlacedRoom& Room)
{
	if (!Room.Template) return nullptr;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AARPGBlockoutRoom* Blockout = GetWorld()->SpawnActor<AARPGBlockoutRoom>(
		AARPGBlockoutRoom::StaticClass(), Room.WorldPosition, FRotator::ZeroRotator, SpawnParams);

	if (!Blockout) return nullptr;

	// Configure blockout from template
	// Note: we can't set protected properties directly, but OnConstruction will use defaults
	// The blockout dimensions are configured via editor — here we set label
#if WITH_EDITOR
	Blockout->SetActorLabel(FString::Printf(TEXT("Room_%d_%s"), Room.RoomIndex, *Room.Template->TemplateID.ToString()));
#endif

	return Blockout;
}
