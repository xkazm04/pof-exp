#include "Economy/ARPGFactionRules.h"

EARPGFactionTier UARPGFactionRules::GetTierForReputation(int32 ReputationPoints)
{
	// Inclusive lower bounds, contiguous & non-overlapping (see factions.ts tiers table).
	if (ReputationPoints >= 12000) return EARPGFactionTier::Exalted;
	if (ReputationPoints >= 9000)  return EARPGFactionTier::Revered;
	if (ReputationPoints >= 6000)  return EARPGFactionTier::Honored;
	if (ReputationPoints >= 3000)  return EARPGFactionTier::Friendly;
	if (ReputationPoints >= 0)     return EARPGFactionTier::Neutral;
	if (ReputationPoints >= -3000) return EARPGFactionTier::Unfriendly;
	return EARPGFactionTier::Hated;
}

int32 UARPGFactionRules::GetDiscountPercent(EARPGFactionTier Tier)
{
	switch (Tier)
	{
	case EARPGFactionTier::Exalted:    return 20;
	case EARPGFactionTier::Revered:    return 15;
	case EARPGFactionTier::Honored:    return 10;
	case EARPGFactionTier::Friendly:   return 5;
	case EARPGFactionTier::Neutral:    return 0;
	case EARPGFactionTier::Unfriendly: return -5; // 5% surcharge
	case EARPGFactionTier::Hated:      return 0;  // trade closed; discount N/A
	default:                           return 0;
	}
}

int32 UARPGFactionRules::GetDecayPerDay(EARPGFactionTier Tier)
{
	// Only the top tiers decay; floored at Honored (handled by the caller against
	// HonoredFloorPoints). Honored and below never decay.
	switch (Tier)
	{
	case EARPGFactionTier::Exalted:
	case EARPGFactionTier::Revered:
		return 10;
	default:
		return 0;
	}
}
