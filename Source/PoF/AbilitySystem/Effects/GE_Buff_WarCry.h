#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_Buff_WarCry.generated.h"

/**
 * Duration-based buff: +20 AttackPower, +15 Armor for 15 seconds.
 * Grants State.Buffed.WarCry while active for VFX/UI hooks.
 */
UCLASS()
class POF_API UGE_Buff_WarCry : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_Buff_WarCry();
};
