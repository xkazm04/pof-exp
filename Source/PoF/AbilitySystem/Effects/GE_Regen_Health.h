#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_Regen_Health.generated.h"

/**
 * Periodic health regeneration — restores +5 Health every 2 seconds.
 * Runs indefinitely (Infinite duration). Remove the active effect handle to stop it.
 * Uses ExecutePeriodicEffect so each tick triggers PostGameplayEffectExecute
 * in the AttributeSet (ensuring proper clamping and death checks).
 */
UCLASS()
class POF_API UGE_Regen_Health : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_Regen_Health();
};
