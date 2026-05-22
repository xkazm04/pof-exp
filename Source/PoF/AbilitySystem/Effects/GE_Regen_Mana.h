#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_Regen_Mana.generated.h"

/**
 * Periodic mana regeneration -- restores +3 Mana every 2 seconds.
 * Runs indefinitely (Infinite duration). Remove the active effect handle to stop it.
 */
UCLASS()
class POF_API UGE_Regen_Mana : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_Regen_Mana();
};
