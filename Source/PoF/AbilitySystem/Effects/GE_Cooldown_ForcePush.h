#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_Cooldown_ForcePush.generated.h"

/** Cooldown effect for the Force Push ability — grants Cooldown.ForcePush for its duration. */
UCLASS()
class POF_API UGE_Cooldown_ForcePush : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_Cooldown_ForcePush();
};
