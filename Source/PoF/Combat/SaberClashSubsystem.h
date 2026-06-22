#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SaberClashSubsystem.generated.h"

/**
 * Detects when two characters' saber blades cross and produces the clash feel: a bright
 * flash + spark at the contact point, plus a brief global hit-stop. Pure code, auto-created
 * per game/PIE world — no per-actor setup. Foundation for the parry system (Phase 3).
 */
UCLASS()
class POF_API USaberClashSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	/** Most recent clash location this tick (for the parry system / debugging). */
	bool bHadClashThisFrame = false;
	FVector LastClashLocation = FVector::ZeroVector;

private:
	float ClashCooldown = 0.f;
	float DiagAccum = 0.f;
	double HitStopUntilRealTime = 0.0;
	bool bHitStopActive = false;

	// Post-parry windows: the deflected attacker is briefly stunned, and the defender gets a
	// riposte window in which their next strike deals bonus damage. Tracked in real time so
	// the hit-stop dilation doesn't distort them.
	TWeakObjectPtr<class AARPGCharacterBase> StunnedChar;
	double StunUntilRealTime = 0.0;
	TWeakObjectPtr<class AARPGCharacterBase> RiposteChar;
	double RiposteUntilRealTime = 0.0;

	void SpawnClashFX(UWorld* World, const FVector& Loc);
};
