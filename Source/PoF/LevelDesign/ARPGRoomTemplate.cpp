#include "LevelDesign/ARPGRoomTemplate.h"

ERoomConnectionDirection UARPGRoomTemplate::GetOppositeDirection(ERoomConnectionDirection Dir)
{
	switch (Dir)
	{
	case ERoomConnectionDirection::North: return ERoomConnectionDirection::South;
	case ERoomConnectionDirection::South: return ERoomConnectionDirection::North;
	case ERoomConnectionDirection::East:  return ERoomConnectionDirection::West;
	case ERoomConnectionDirection::West:  return ERoomConnectionDirection::East;
	default:                              return ERoomConnectionDirection::South;
	}
}

bool UARPGRoomTemplate::CanConnect(const FRoomConnectionSlot& A, const FRoomConnectionSlot& B)
{
	// Must face opposite directions
	if (GetOppositeDirection(A.Direction) != B.Direction) return false;

	// Width must be compatible (within 10% tolerance)
	const float WidthRatio = A.ConnectionWidth / FMath::Max(B.ConnectionWidth, 1.f);
	return FMath::IsNearlyEqual(WidthRatio, 1.0f, 0.1f);
}

FPrimaryAssetId UARPGRoomTemplate::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("RoomTemplate"), TemplateID);
}
