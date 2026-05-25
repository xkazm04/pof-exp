#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_Equip_IronLongsword.generated.h"

/**
 * Equip bonus for the Iron Longsword (catalog entity item-1, Core/Existing → items).
 *
 * Infinite duration — applied while the weapon is equipped and removed on unequip
 * via the inventory component's stored active handle. Grants +15 AttackPower, the
 * canonical average of the seeded item's 12-18 damage (item-1 numericValue 15).
 *
 * This class IS DA_IronLongsword.OnEquipEffect; the items test gate
 * (AVSItemsDefinitionsTest) asserts the modifier value matches the catalog entity,
 * so the weapon's offense stays in lockstep with the design data.
 */
UCLASS()
class POF_API UGE_Equip_IronLongsword : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_Equip_IronLongsword();
};
