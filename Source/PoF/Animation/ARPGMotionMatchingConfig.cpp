#include "Animation/ARPGMotionMatchingConfig.h"

TArray<FMotionMatchEntry> UARPGMotionMatchingConfig::GetEntriesWithTag(FName Tag) const
{
	TArray<FMotionMatchEntry> Result;
	for (const FMotionMatchEntry& Entry : Entries)
	{
		if (Entry.Tags.Contains(Tag))
		{
			Result.Add(Entry);
		}
	}
	return Result;
}

TArray<FMotionMatchEntry> UARPGMotionMatchingConfig::GetEntriesWithAllTags(const TArray<FName>& Tags) const
{
	TArray<FMotionMatchEntry> Result;
	for (const FMotionMatchEntry& Entry : Entries)
	{
		bool bAllMatch = true;
		for (const FName& Tag : Tags)
		{
			if (!Entry.Tags.Contains(Tag))
			{
				bAllMatch = false;
				break;
			}
		}
		if (bAllMatch)
		{
			Result.Add(Entry);
		}
	}
	return Result;
}
