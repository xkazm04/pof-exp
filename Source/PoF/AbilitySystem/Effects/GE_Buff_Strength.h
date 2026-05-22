#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_Buff_Strength.generated.h"

/**
 * Duration-based buff — adds +15 Strength for 10 seconds.
 * The modifier is automatically removed when the effect expires.
 */
UCLASS()
class POF_API UGE_Buff_Strength : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_Buff_Strength();
};
