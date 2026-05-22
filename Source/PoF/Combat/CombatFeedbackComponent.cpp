#include "Combat/CombatFeedbackComponent.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

UCombatFeedbackComponent::UCombatFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Only tick when hitstops are active — no need to tick every frame when idle
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UCombatFeedbackComponent::BeginPlay()
{
	Super::BeginPlay();

	GlobalDelegateHandle = UARPGAttributeSet::OnDamageNumberRequestedGlobal.AddUObject(
		this, &UCombatFeedbackComponent::HandleDamageEvent);
}

void UCombatFeedbackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UARPGAttributeSet::OnDamageNumberRequestedGlobal.Remove(GlobalDelegateHandle);

	// Restore time dilation on any actors still in hitstop
	for (FHitstopEntry& Entry : ActiveHitstops)
	{
		if (AActor* Actor = Entry.Actor.Get())
		{
			Actor->CustomTimeDilation = Entry.OriginalTimeDilation;
		}
	}
	ActiveHitstops.Empty();

	Super::EndPlay(EndPlayReason);
}

void UCombatFeedbackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateHitstops(DeltaTime);
}

void UCombatFeedbackComponent::HandleDamageEvent(
	AActor* TargetActor, float DamageAmount, bool bIsCrit,
	bool bIsHeal, FGameplayTag DamageType, FVector HitLocation)
{
	// Skip heals — no combat feedback for healing
	if (bIsHeal) return;
	if (DamageAmount <= 0.f) return;

	// Camera shake on the player controller that owns this component
	PlayCameraShake(bIsCrit);

	// Hitstop on target
	if (bEnableHitstop && TargetActor)
	{
		ApplyHitstop(TargetActor);
	}

	// Hitstop on attacker (the player character) — only if we're the player's controller
	if (bEnableHitstop)
	{
		APlayerController* PC = Cast<APlayerController>(GetOwner());
		if (PC)
		{
			APawn* PlayerPawn = PC->GetPawn();
			if (PlayerPawn && PlayerPawn != TargetActor)
			{
				ApplyHitstop(PlayerPawn);
			}
		}
	}

	// Hit VFX at impact location
	// Use a default normal (upward) since the delegate doesn't carry the full hit result
	SpawnHitVFX(HitLocation, FVector::UpVector);

	// Hit SFX at impact location
	PlayHitSFX(HitLocation, DamageType, bIsCrit);

	// Controller vibration
	PlayControllerVibration(bIsCrit);
}

// =====================================================================
// Camera Shake
// =====================================================================

void UCombatFeedbackComponent::PlayCameraShake(bool bIsCrit)
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC) return;

	APlayerCameraManager* CamMgr = PC->PlayerCameraManager;
	if (!CamMgr) return;

	TSubclassOf<UCameraShakeBase> ShakeClass = bIsCrit ? HeavyHitShake : LightHitShake;
	if (!ShakeClass) return;

	CamMgr->StartCameraShake(ShakeClass, CameraShakeScale);
}

// =====================================================================
// Hitstop
// =====================================================================

void UCombatFeedbackComponent::ApplyHitstop(AActor* Actor)
{
	if (!Actor) return;

	// Check if this actor already has an active hitstop — just reset the timer
	for (FHitstopEntry& Entry : ActiveHitstops)
	{
		if (Entry.Actor.Get() == Actor)
		{
			Entry.RemainingTime = HitstopDuration;
			return;
		}
	}

	FHitstopEntry NewEntry;
	NewEntry.Actor = Actor;
	NewEntry.RemainingTime = HitstopDuration;
	NewEntry.OriginalTimeDilation = Actor->CustomTimeDilation;
	ActiveHitstops.Add(NewEntry);

	Actor->CustomTimeDilation = HitstopTimeDilation;

	// Enable tick to process hitstop countdowns
	SetComponentTickEnabled(true);
}

void UCombatFeedbackComponent::UpdateHitstops(float DeltaTime)
{
	// Use real (unscaled) delta. TickComponent's DeltaTime is already unaffected
	// by per-actor CustomTimeDilation (it's the controller's delta), so it's safe.
	const float RealDelta = DeltaTime;

	for (int32 i = ActiveHitstops.Num() - 1; i >= 0; --i)
	{
		FHitstopEntry& Entry = ActiveHitstops[i];
		Entry.RemainingTime -= RealDelta;

		if (Entry.RemainingTime <= 0.f || !Entry.Actor.IsValid())
		{
			// Restore original time dilation
			if (AActor* Actor = Entry.Actor.Get())
			{
				Actor->CustomTimeDilation = Entry.OriginalTimeDilation;
			}
			ActiveHitstops.RemoveAtSwap(i);
		}
	}

	// Disable tick when no hitstops are active
	if (ActiveHitstops.IsEmpty())
	{
		SetComponentTickEnabled(false);
	}
}

// =====================================================================
// Hit VFX
// =====================================================================

void UCombatFeedbackComponent::SpawnHitVFX(const FVector& Location, const FVector& ImpactNormal)
{
	if (!HitImpactVFX) return;

	UWorld* World = GetWorld();
	if (!World) return;

	const FRotator SpawnRotation = ImpactNormal.Rotation();

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World,
		HitImpactVFX,
		Location,
		SpawnRotation,
		HitVFXScale,
		true,  // bAutoDestroy
		true,  // bAutoActivate
		ENCPoolMethod::None
	);
}

// =====================================================================
// Hit SFX
// =====================================================================

void UCombatFeedbackComponent::PlayHitSFX(const FVector& Location, FGameplayTag DamageType, bool bIsCrit)
{
	// Select sound based on damage type
	USoundBase* Sound = HitSound_Physical;

	if (DamageType == ARPGGameplayTags::Damage_Fire && HitSound_Fire)
	{
		Sound = HitSound_Fire;
	}
	else if (DamageType == ARPGGameplayTags::Damage_Ice && HitSound_Ice)
	{
		Sound = HitSound_Ice;
	}
	else if (DamageType == ARPGGameplayTags::Damage_Lightning && HitSound_Lightning)
	{
		Sound = HitSound_Lightning;
	}

	// Play element/type sound
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this, Sound, Location, HitSoundVolume,
			FMath::FRandRange(0.9f, 1.1f)); // Slight pitch variation
	}

	// Play additional crit sound
	if (bIsCrit && HitSound_Crit)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this, HitSound_Crit, Location, HitSoundVolume,
			FMath::FRandRange(0.95f, 1.05f));
	}
}

// =====================================================================
// Controller Vibration
// =====================================================================

void UCombatFeedbackComponent::PlayControllerVibration(bool bIsCrit)
{
	if (!bEnableControllerVibration) return;

	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC) return;

	const float Intensity = bIsCrit ? CritHitVibrationIntensity : NormalHitVibrationIntensity;

	PC->PlayDynamicForceFeedback(
		Intensity,  // Intensity
		VibrationDuration,  // Duration
		true,   // bAffectsLeftLarge
		true,   // bAffectsLeftSmall
		true,   // bAffectsRightLarge
		true);  // bAffectsRightSmall
}
