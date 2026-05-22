#include "Animation/AnimNotifyState_HitDetection.h"
#include "Character/ARPGCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "DrawDebugHelpers.h"

FString UAnimNotifyState_HitDetection::GetNotifyName_Implementation() const
{
	return TEXT("Hit Detection");
}

void UAnimNotifyState_HitDetection::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp) return;

	AlreadyHitActors.Reset();
	bHasPreviousLocations = false;

	if (AARPGCharacterBase* Character = Cast<AARPGCharacterBase>(MeshComp->GetOwner()))
	{
		Character->EnableHitDetection();
	}
}

void UAnimNotifyState_HitDetection::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!MeshComp) return;

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor) return;

	UWorld* World = OwnerActor->GetWorld();
	if (!World) return;

	// Configure trace params (shared across all traces)
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerActor);
	QueryParams.bTraceComplex = false;
	QueryParams.bReturnPhysicalMaterial = false;

	// Build trace segments
	TArray<TPair<FVector, FVector>> TraceSegments;

	if (TipSocketName != NAME_None)
	{
		// Multi-point mode: trace along the weapon from base to tip
		const FVector BasePos = MeshComp->GetSocketLocation(WeaponSocketName);
		const FVector TipPos = MeshComp->GetSocketLocation(TipSocketName);

		if (bHasPreviousLocations)
		{
			// Frame-to-frame sweep: trace from previous positions to current positions
			// at each sample point along the weapon for full coverage during fast swings
			const int32 TotalPoints = IntermediateTracePoints + 2; // base + intermediates + tip
			for (int32 i = 0; i < TotalPoints; ++i)
			{
				const float Alpha = (TotalPoints > 1) ? static_cast<float>(i) / (TotalPoints - 1) : 0.f;
				const FVector PrevPos = FMath::Lerp(PrevBaseLocation, PrevTipLocation, Alpha);
				const FVector CurrPos = FMath::Lerp(BasePos, TipPos, Alpha);
				TraceSegments.Emplace(PrevPos, CurrPos);
			}
		}
		else
		{
			// First frame: single trace from base to tip
			TraceSegments.Emplace(BasePos, TipPos);
		}

		PrevBaseLocation = BasePos;
		PrevTipLocation = TipPos;
		bHasPreviousLocations = true;
	}
	else
	{
		// Legacy single-trace mode: forward from weapon socket
		const FTransform SocketTransform = MeshComp->GetSocketTransform(WeaponSocketName, RTS_World);
		const FVector TraceStart = SocketTransform.GetLocation();
		const FVector TraceEnd = TraceStart + SocketTransform.GetRotation().GetForwardVector() * TraceLength;
		TraceSegments.Emplace(TraceStart, TraceEnd);
	}

	// Execute all trace segments and collect hits
	TArray<FHitResult> AllHits;
	bool bAnyHit = false;

	for (const auto& Segment : TraceSegments)
	{
		TArray<FHitResult> HitResults;
		const bool bHit = World->SweepMultiByChannel(
			HitResults,
			Segment.Key,
			Segment.Value,
			FQuat::Identity,
			TraceChannel,
			FCollisionShape::MakeSphere(TraceRadius),
			QueryParams);

		if (bHit)
		{
			bAnyHit = true;
			AllHits.Append(HitResults);
		}

#if ENABLE_DRAW_DEBUG
		if (bDrawDebug)
		{
			const float DebugDuration = 0.5f;
			const FColor DebugColor = bHit ? FColor::Red : FColor::Green;
			DrawDebugSphere(World, Segment.Key, TraceRadius, 8, DebugColor, false, DebugDuration);
			DrawDebugSphere(World, Segment.Value, TraceRadius, 8, DebugColor, false, DebugDuration);
			DrawDebugLine(World, Segment.Key, Segment.Value, DebugColor, false, DebugDuration);
		}
#endif
	}

	if (!bAnyHit) return;

	// Get the owner's ASC for sending gameplay events
	AARPGCharacterBase* OwnerCharacter = Cast<AARPGCharacterBase>(OwnerActor);
	UAbilitySystemComponent* OwnerASC = OwnerCharacter ? OwnerCharacter->GetAbilitySystemComponent() : nullptr;

	for (const FHitResult& Hit : AllHits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor) continue;

		// Skip self
		if (HitActor == OwnerActor) continue;

		// Skip duplicates within this swing
		bool bAlreadyInSet = false;
		AlreadyHitActors.Add(HitActor, &bAlreadyInSet);
		if (bAlreadyInSet) continue;

		// Send gameplay event to the owner's ASC so the active ability handles damage
		if (OwnerASC)
		{
			FGameplayEventData Payload;
			Payload.EventTag = ARPGGameplayTags::Event_MeleeHit;
			Payload.Instigator = OwnerActor;
			Payload.Target = HitActor;
			// Store the hit location for VFX/SFX spawning
			Payload.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(Hit);
			OwnerASC->HandleGameplayEvent(Payload.EventTag, &Payload);
		}

		UE_LOG(LogTemp, Verbose, TEXT("[HitDetection] %s hit %s at %s"),
			*OwnerActor->GetName(), *HitActor->GetName(), *Hit.ImpactPoint.ToString());
	}
}

void UAnimNotifyState_HitDetection::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	AlreadyHitActors.Reset();
	bHasPreviousLocations = false;

	if (AARPGCharacterBase* Character = Cast<AARPGCharacterBase>(MeshComp->GetOwner()))
	{
		Character->DisableHitDetection();
	}
}
