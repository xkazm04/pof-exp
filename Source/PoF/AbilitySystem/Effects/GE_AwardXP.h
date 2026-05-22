#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_AwardXP.generated.h"

/**
 * Instant effect that adds XP to the target's CurrentXP attribute.
 *
 * Uses SetByCaller magnitude with tag "Data.XP.Amount" so the caller
 * controls exactly how much XP to award (based on enemy type, level, etc.).
 *
 * Usage:
 *   FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UGE_AwardXP::StaticClass(), 1, Context);
 *   Spec.Data->SetSetByCallerMagnitude(ARPGGameplayTags::Data_XP_Amount, XPAmount);
 *   ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
 */
UCLASS()
class POF_API UGE_AwardXP : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_AwardXP();
};
