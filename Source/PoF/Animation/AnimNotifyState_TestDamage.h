#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_TestDamage.generated.h"

/**
 * Test-only notify state that fires a damage event on begin.
 *
 * The real `AM_MeleeCombo` is an empty gray-box montage with no hit-detection
 * notify, so PS-1's functional test had to send `Event.MeleeHit` by hand.
 * Place this notify inline on a one-frame test montage and the event fires
 * reliably regardless of the real montage's contents — combat tests no longer
 * depend on `AM_MeleeCombo` at all.
 *
 * Targets the first `AARPGEnemyCharacter` in the world (the slice scenario has
 * exactly one). `bSendMeleeHitEvent` drives the real ability path; the optional
 * `bApplyDirectDamage` bypasses the ability entirely for pure-damage tests.
 */
UCLASS(DisplayName = "ARPG Test Damage (test only)")
class POF_API UAnimNotifyState_TestDamage : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	/** Send Event.MeleeHit to the owner's ASC (drives GA_MeleeAttack). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TestDamage")
	bool bSendMeleeHitEvent = true;

	/** Also apply GE_Damage directly to the target, bypassing the ability. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TestDamage")
	bool bApplyDirectDamage = false;

	/** Damage magnitude (event EventMagnitude and/or direct GE_Damage base). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TestDamage")
	float DamageAmount = 9999.f;
};
