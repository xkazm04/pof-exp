#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_InitAttributes.generated.h"

/**
 * Instant Gameplay Effect that initializes all attributes using SetByCaller magnitudes.
 *
 * Each attribute gets its own SetByCaller tag (e.g., "Data.Init.Health").
 * This allows the character to set per-level values at spawn time via
 * a Curve Table lookup before applying the effect.
 *
 * Usage:
 *   FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UGE_InitAttributes::StaticClass(), Level, Context);
 *   Spec.Data->SetSetByCallerMagnitude(ARPGGameplayTags::Data_Init_Health, CurveTable.Eval(Level));
 *   ... (repeat for each attribute)
 *   ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
 *
 * Alternatively, use AARPGCharacterBase::InitializeAttributes() which does
 * the Curve Table lookup and applies everything automatically.
 */
UCLASS()
class POF_API UGE_InitAttributes : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_InitAttributes();
};
