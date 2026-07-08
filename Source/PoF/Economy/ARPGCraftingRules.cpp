#include "Economy/ARPGCraftingRules.h"

bool UARPGCraftingRules::CanCraft(int32 PlayerGold, int32 CraftingSkill, bool bAtAlchemistBench, bool bHasAllReagents)
{
	return PlayerGold >= GoldCost
		&& CraftingSkill >= RequiredSkill
		&& bAtAlchemistBench
		&& bHasAllReagents;
}
