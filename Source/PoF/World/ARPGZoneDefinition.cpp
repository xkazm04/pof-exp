#include "World/ARPGZoneDefinition.h"

const FZoneEncounterGroup& UARPGZoneDefinition::GetRandomEncounterGroup() const
{
	check(EncounterGroups.Num() > 0);

	float TotalWeight = 0.f;
	for (const FZoneEncounterGroup& Group : EncounterGroups)
	{
		TotalWeight += Group.SelectionWeight;
	}

	float Roll = FMath::FRandRange(0.f, TotalWeight);
	for (const FZoneEncounterGroup& Group : EncounterGroups)
	{
		Roll -= Group.SelectionWeight;
		if (Roll <= 0.f)
		{
			return Group;
		}
	}

	return EncounterGroups.Last();
}

FPrimaryAssetId UARPGZoneDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ZoneDefinition"), ZoneID);
}
