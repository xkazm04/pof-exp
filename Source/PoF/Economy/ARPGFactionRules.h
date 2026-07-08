#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ARPGFactionRules.generated.h"

/**
 * Faction standing tiers for the reputation ladder (catalog pipeline: factions,
 * "Standing & Rep Tiers"). Seven contiguous tiers from Hated (deep negative) to
 * Exalted (max). Mirrors the app-side pipeline contract in
 * src/lib/catalog/pipelines/factions.ts (the tiers table + decayRules).
 */
UENUM(BlueprintType)
enum class EARPGFactionTier : uint8
{
	Hated,
	Unfriendly,
	Neutral,
	Friendly,
	Honored,
	Revered,
	Exalted
};

/**
 * Pure, world-free faction reputation rules — the single C++ source of truth the
 * game (UARPGFactionSubsystem / vendor pricing / dialogue greeting) and the L3
 * gate (VSFactionRepTest) both read. Blueprint-callable static library so it needs
 * no world/GameInstance to evaluate — matches the app pipeline's Standing & Rep
 * Tiers contract exactly (thresholds, discounts, top-tier decay floored at Honored).
 */
UCLASS()
class POF_API UARPGFactionRules : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Reputation floor for the passive-decay rule: decay cannot drop below Honored. */
	static constexpr int32 HonoredFloorPoints = 6000;

	/** Map a raw reputation-points value to its standing tier (inclusive lower bounds:
	 *  -10000 Hated / -3000 Unfriendly / 0 Neutral / 3000 Friendly / 6000 Honored /
	 *  9000 Revered / 12000 Exalted). */
	UFUNCTION(BlueprintPure, Category = "Faction")
	static EARPGFactionTier GetTierForReputation(int32 ReputationPoints);

	/** Vendor discount percent for a tier (negative = surcharge): Unfriendly -5,
	 *  Neutral 0, Friendly 5, Honored 10, Revered 15, Exalted 20. Hated = 0 (trade closed). */
	UFUNCTION(BlueprintPure, Category = "Faction")
	static int32 GetDiscountPercent(EARPGFactionTier Tier);

	/** Passive reputation decay per day. Only the top tiers decay (Revered & Exalted:
	 *  10 pts/day); Honored and below do not. Decay is floored at Honored (HonoredFloorPoints). */
	UFUNCTION(BlueprintPure, Category = "Faction")
	static int32 GetDecayPerDay(EARPGFactionTier Tier);

	/** True when the tier permits trading services at all (Hated closes all services). */
	UFUNCTION(BlueprintPure, Category = "Faction")
	static bool AreServicesOpen(EARPGFactionTier Tier) { return Tier != EARPGFactionTier::Hated; }
};
