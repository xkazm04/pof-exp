#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_Stun.generated.h"

/**
 * Duration-based stun effect. Grants State.Stunned for a short duration.
 * While stunned, abilities with ActivationBlockedTags containing State.Stunned
 * are blocked.
 */
UCLASS()
class POF_API UGE_Stun : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_Stun();
};
