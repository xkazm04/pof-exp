#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_Dazed.generated.h"

/**
 * Duration-based daze effect — Force Push's landing follow-up (catalog contract
 * status-effects::status-dazed). Grants State.Dazed for 1.6 s (refresh stacking is
 * GAS default for a fresh application). While dazed the character shambles at
 * DazedSpeedMultiplier (ARPGCharacterBase::UpdateSprintEffects) and abilities are
 * blocked via ActivationBlockedTags (ARPGGameplayAbility). Gated by State.Immune.Daze
 * at the apply site (AARPGCharacterBase::Landed).
 */
UCLASS()
class POF_API UGE_Dazed : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_Dazed();
};
