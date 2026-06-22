#include "AbilitySystem/GA_MeleeAttack.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "Character/ARPGCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Engine/OverlapResult.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraShakeBase.h"
#include "Engine/World.h"
#include "TimerManager.h"

UGA_MeleeAttack::UGA_MeleeAttack()
{
	// This ability is activated by input, not auto-ending
	bAutoEndAbility = false;

	// Tag this ability for identification
	SetAssetTags(FGameplayTagContainer(ARPGGameplayTags::Ability_Melee_LightAttack));

	// Add State.Attacking while active so other systems know we're attacking
	ActivationOwnedTags.AddTag(ARPGGameplayTags::State_Attacking);

	// Block re-activation while already attacking (prevent double-trigger)
	ActivationBlockedTags.AddTag(ARPGGameplayTags::State_Attacking);

	// A single default combo section so the ability can activate even when a Blueprint
	// authored none; StartMontageAndListenForCombo also swaps in a real attack montage when
	// the assigned one is the empty AM_MeleeCombo placeholder, so a bare setup still swings.
	ComboSectionNames.Add(FName("Default"));
}

bool UGA_MeleeAttack::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// A null montage is allowed — the runtime fallback (timer-driven attack window)
	// handles characters without a skeletal mesh / anim instance. Empty combo sections no
	// longer block activation: StartMontageAndListenForCombo guarantees a default section
	// + a real attack montage at runtime, so the attack still fires + swings.
	return true;
}

void UGA_MeleeAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// CommitAbility is called by Super — checks cost (mana) and applies cooldown.
	// If it fails, Super will EndAbility with bWasCancelled=true.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Set attacking state on the character
	if (AARPGCharacterBase* Character = GetARPGCharacter())
	{
		Character->SetAttacking(true);
	}

	// Reset combo state
	CurrentComboIndex = 0;
	bComboInputBuffered = false;
	bComboWindowOpen = false;

	// Acquire a warp target for attack magnetism before starting the montage
	AcquireWarpTarget();

	// Start the montage
	StartMontageAndListenForCombo();
}

void UGA_MeleeAttack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// Clear warp target and attacking state on the character
	ClearWarpTarget();

	// Reset transient combo / fallback state so a future activation starts clean.
	bUsingFallbackWindow = false;

	if (AARPGCharacterBase* Character = GetARPGCharacter())
	{
		Character->SetAttacking(false);
		Character->CloseComboWindow();
	}

	// Safety: if the ability ends mid hit-pause, restore time dilation so the world
	// never gets stuck in slow-mo (the restore timer is bound to this instance).
	if (UWorld* World = GetWorld())
	{
		if (HitStopTimerHandle.IsValid())
		{
			World->GetTimerManager().ClearTimer(HitStopTimerHandle);
			UGameplayStatics::SetGlobalTimeDilation(World, 1.0f);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_MeleeAttack::StartMontageAndListenForCombo()
{
	// Set up the hit / combo gameplay-event listeners FIRST, before the montage.
	// These listeners (especially Event.MeleeHit) must outlive a montage that
	// fails to play — otherwise EndAbility tears them down before a hit lands.
	ListenForComboWindow();

	// Self-heal a broken/empty attack montage: the project ships an empty AM_MeleeCombo
	// placeholder (0s, no animation), so the basic attack played nothing. Fall back to the
	// real sword slash (AM_SwordSlash) so the attack actually SWINGS, and guarantee a section.
	// ALWAYS use the custom code-authored slash (AM_SwordSlashC) for the basic attack so it
	// shows in normal play (PIE), not only when the assigned montage is empty. Fall back to
	// the original / any assigned montage only if the custom one is missing.
	if (UAnimMontage* Slash = LoadObject<UAnimMontage>(nullptr, TEXT("/Game/Weapons/AM_SwordSlashC.AM_SwordSlashC")))
	{
		AttackMontage = Slash;
	}
	else if (!AttackMontage || AttackMontage->GetPlayLength() <= 0.f)
	{
		if (UAnimMontage* Fallback = LoadObject<UAnimMontage>(nullptr, TEXT("/Game/Weapons/AM_SwordSlash.AM_SwordSlash")))
		{
			AttackMontage = Fallback;
		}
	}
	if (ComboSectionNames.Num() == 0)
	{
		ComboSectionNames.Add(FName("Default"));
	}

	// Gray-box (false) mode: the ability applies its own damage at a deterministic
	// mid-swing offset, resolving a forward melee target itself — no hit-detection
	// AnimNotify required. Scheduled here (before the montage branch, which may
	// return early) so it fires whether or not a montage actually plays.
	if (!bUseAnimationDrivenDamage)
	{
		UAbilityTask_WaitDelay* HitTask = UAbilityTask_WaitDelay::WaitDelay(this, GrayBoxHitDelay);
		HitTask->OnFinish.AddDynamic(this, &UGA_MeleeAttack::OnGrayBoxHitWindow);
		HitTask->ReadyForActivation();
	}

	// Determine up-front whether the avatar can actually play an anim montage.
	// On a gray-box character (no SkeletalMesh / no AnimInstance) PlayMontageAndWait
	// fails internally and the engine logs "PlayMontage failed!". We pre-check so
	// we can skip the montage task entirely in that case — avoiding the engine
	// warning at its source — and run a pure timer-driven attack window instead.
	bool bCanPlayMontage = false;
	if (AttackMontage)
	{
		if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
		{
			if (USkeletalMeshComponent* MeshComp = Character->GetMesh())
			{
				bCanPlayMontage =
					MeshComp->GetSkeletalMeshAsset() != nullptr && MeshComp->GetAnimInstance() != nullptr;
			}
		}
	}

	if (bCanPlayMontage)
	{
		// Normal path: drive the attack with the montage; its callbacks end the ability.
		const FName StartSection = ComboSectionNames.IsValidIndex(CurrentComboIndex)
			? ComboSectionNames[CurrentComboIndex]
			: NAME_None;

		// Pre-arm the fallback flag BEFORE ReadyForActivation() so that if the
		// montage task's OnInterrupted / OnCancelled fires synchronously inside
		// ReadyForActivation() (empty / zero-duration montage with a live AnimInstance),
		// those callbacks see bUsingFallbackWindow=true and do NOT call EndAbility —
		// which would destroy the Event.MeleeHit listener before the hit arrives.
		// If the montage actually starts (confirmed below), we reset the flag.
		bUsingFallbackWindow = true;

		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			TEXT("PlayMeleeAttack"),
			AttackMontage,
			MontagePlayRate,
			StartSection,
			/*bStopWhenAbilityEnds=*/ true);

		MontageTask->OnCompleted.AddDynamic(this, &UGA_MeleeAttack::OnMontageCompleted);
		MontageTask->OnBlendOut.AddDynamic(this, &UGA_MeleeAttack::OnMontageBlendOut);
		MontageTask->OnInterrupted.AddDynamic(this, &UGA_MeleeAttack::OnMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &UGA_MeleeAttack::OnMontageCancelled);
		MontageTask->ReadyForActivation();

		// Confirm the montage actually started; if PlayMontageAndWait still failed
		// (e.g. an empty montage with no sequence length) fall through to the timer.
		UAbilitySystemComponent* ASC = GetARPGAbilitySystemComponent();
		if (ASC && ASC->GetCurrentMontage() == AttackMontage)
		{
			// Montage is playing — normal path, not using fallback.
			bUsingFallbackWindow = false;
			return;
		}
		// Montage did not start — bUsingFallbackWindow remains true; fall through.
	}

	// Fallback path: no playable montage on this avatar (gray-box / empty montage).
	// Keep the ability — and crucially its Event.MeleeHit listener — alive for a
	// fixed attack window so a hit can still resolve, then end the ability cleanly.
	// bUsingFallbackWindow is already true if we came from the failed-montage branch.
	bUsingFallbackWindow = true;
	UE_LOG(LogTemp, Log,
		TEXT("[GA_MeleeAttack] No playable attack montage on this avatar; "
			 "using %.2fs timer-driven attack window."),
		FallbackAttackWindow);

	UAbilityTask_WaitDelay* FallbackTask = UAbilityTask_WaitDelay::WaitDelay(this, FallbackAttackWindow);
	FallbackTask->OnFinish.AddDynamic(this, &UGA_MeleeAttack::OnFallbackWindowElapsed);
	FallbackTask->ReadyForActivation();
}

void UGA_MeleeAttack::OnFallbackWindowElapsed()
{
	// The fallback attack window expired — end the ability normally (not cancelled),
	// so any combo/hit state is cleaned up the same way a finished montage would.
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_MeleeAttack::OnGrayBoxHitWindow()
{
	// Gray-box (bUseAnimationDrivenDamage=false): no Event.MeleeHit / notify fires,
	// so resolve the melee target ourselves with a forward-cone search and apply
	// the damage GE. Reuses the shared search + apply paths.
	if (AActor* Target = FindForwardTarget(MeleeHitRange, WarpTargetSearchAngle))
	{
		ApplyMeleeDamageTo(Target, nullptr);
	}
}

void UGA_MeleeAttack::ListenForComboWindow()
{
	// Listen for combo open
	UAbilityTask_WaitGameplayEvent* WaitOpen = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		ARPGGameplayTags::Event_Combo_Open,
		nullptr,
		/*OnlyTriggerOnce=*/ false,
		/*OnlyMatchExact=*/ true);

	WaitOpen->EventReceived.AddDynamic(this, &UGA_MeleeAttack::OnComboWindowOpened);
	WaitOpen->ReadyForActivation();

	// Listen for combo close
	UAbilityTask_WaitGameplayEvent* WaitClose = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		ARPGGameplayTags::Event_Combo_Close,
		nullptr,
		/*OnlyTriggerOnce=*/ false,
		/*OnlyMatchExact=*/ true);

	WaitClose->EventReceived.AddDynamic(this, &UGA_MeleeAttack::OnComboWindowClosed);
	WaitClose->ReadyForActivation();

	// Listen for combo input from the controller
	UAbilityTask_WaitGameplayEvent* WaitInput = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		ARPGGameplayTags::Event_Combo_Input,
		nullptr,
		/*OnlyTriggerOnce=*/ false,
		/*OnlyMatchExact=*/ true);

	WaitInput->EventReceived.AddDynamic(this, &UGA_MeleeAttack::OnComboInputReceived);
	WaitInput->ReadyForActivation();

	// Listen for melee hit events from the hit detection notify state
	UAbilityTask_WaitGameplayEvent* WaitHit = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		ARPGGameplayTags::Event_MeleeHit,
		nullptr,
		/*OnlyTriggerOnce=*/ false,
		/*OnlyMatchExact=*/ true);

	WaitHit->EventReceived.AddDynamic(this, &UGA_MeleeAttack::OnMeleeHit);
	WaitHit->ReadyForActivation();
}

void UGA_MeleeAttack::OnComboWindowOpened(FGameplayEventData Payload)
{
	bComboWindowOpen = true;

	// If the player already buffered an input, advance immediately
	if (bComboInputBuffered)
	{
		AdvanceCombo();
	}
}

void UGA_MeleeAttack::OnComboWindowClosed(FGameplayEventData Payload)
{
	bComboWindowOpen = false;
	bComboInputBuffered = false;
}

void UGA_MeleeAttack::OnMontageCompleted()
{
	// Ignore a stale completed callback from a montage that failed to play —
	// the fallback timer keeps the ability (and its hit listener) alive instead.
	if (bUsingFallbackWindow)
	{
		return;
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_MeleeAttack::OnMontageBlendOut()
{
	// Ignore a stale blend-out from a montage that never started —
	// same guard as OnMontageCompleted / OnMontageInterrupted / OnMontageCancelled.
	if (bUsingFallbackWindow)
	{
		return;
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_MeleeAttack::OnMontageInterrupted()
{
	// Ignore an interrupt that is really "the montage never played" — the
	// fallback timer keeps the ability (and its hit listener) alive instead.
	if (bUsingFallbackWindow)
	{
		return;
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_MeleeAttack::OnMontageCancelled()
{
	// Ignore a cancel that is really "the montage failed to play" — the
	// fallback timer keeps the ability (and its hit listener) alive instead.
	if (bUsingFallbackWindow)
	{
		return;
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_MeleeAttack::OnComboInputReceived(FGameplayEventData Payload)
{
	if (bComboWindowOpen)
	{
		// Window is open — advance immediately
		AdvanceCombo();
	}
	else
	{
		// Buffer the input for when the window opens
		bComboInputBuffered = true;
	}
}

void UGA_MeleeAttack::ApplyMeleeDamageTo(AActor* Target, const FHitResult* OptionalHit)
{
	if (!DamageEffect || !Target) return;

	// Get the target's ASC via the AbilitySystemInterface
	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Target);
	UAbilitySystemComponent* TargetASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
	if (!TargetASC) return;

	UAbilitySystemComponent* SourceASC = GetARPGAbilitySystemComponent();
	if (!SourceASC) return;

	// Build the GE spec
	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(this);
	if (OptionalHit)
	{
		Context.AddHitResult(*OptionalHit);
	}

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(
		DamageEffect, GetAbilityLevel(), Context);

	if (!SpecHandle.IsValid()) return;

	// Compute damage for this combo section
	const float ComboMultiplier = ComboDamageMultipliers.IsValidIndex(CurrentComboIndex)
		? ComboDamageMultipliers[CurrentComboIndex]
		: 1.f;

	FGameplayEffectSpec* Spec = SpecHandle.Data.Get();
	Spec->SetSetByCallerMagnitude(ARPGGameplayTags::Data_Damage_Base, BaseDamage * ComboMultiplier);
	Spec->AddDynamicAssetTag(ARPGGameplayTags::Damage_Physical);

	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec, TargetASC);

	// Confirmed hit — fire the combat-feel polish (hit pause + camera shake).
	ApplyHitFeel();

	UE_LOG(LogTemp, Log, TEXT("[GA_MeleeAttack] Applied damage to %s: Base=%.1f x Combo=%.2f"),
		*Target->GetName(), BaseDamage, ComboMultiplier);
}

void UGA_MeleeAttack::OnMeleeHit(FGameplayEventData Payload)
{
	AActor* HitActor = const_cast<AActor*>(Payload.Target.Get());
	const FHitResult* HitResult = (Payload.TargetData.Num() > 0)
		? Payload.TargetData.Get(0)->GetHitResult()
		: nullptr;
	ApplyMeleeDamageTo(HitActor, HitResult);
}

void UGA_MeleeAttack::ApplyHitFeel()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar) return;
	UWorld* World = Avatar->GetWorld();
	if (!World) return;

	// Hit pause: briefly dilate global time. The restore timer runs on dilated
	// game time, so scale the rate by the dilation to restore after ~HitStopDuration
	// of REAL time (otherwise the freeze would last HitStopDuration / dilation).
	if (HitStopDuration > 0.f && HitStopTimeDilation < 1.f)
	{
		UGameplayStatics::SetGlobalTimeDilation(World, HitStopTimeDilation);
		const float DilatedRate = FMath::Max(HitStopDuration * HitStopTimeDilation, KINDA_SMALL_NUMBER);
		World->GetTimerManager().SetTimer(
			HitStopTimerHandle, this, &UGA_MeleeAttack::RestoreTimeDilation, DilatedRate, /*bLoop=*/false);
	}

	// Camera shake on the instigating player's camera (guarded for headless / no PC).
	if (HitCameraShake)
	{
		if (APawn* Pawn = Cast<APawn>(Avatar))
		{
			if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
			{
				PC->ClientStartCameraShake(HitCameraShake, HitCameraShakeScale);
			}
		}
	}
}

void UGA_MeleeAttack::RestoreTimeDilation()
{
	if (AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		if (UWorld* World = Avatar->GetWorld())
		{
			UGameplayStatics::SetGlobalTimeDilation(World, 1.0f);
		}
	}
}

void UGA_MeleeAttack::AdvanceCombo()
{
	bComboInputBuffered = false;
	bComboWindowOpen = false;

	const int32 NextIndex = CurrentComboIndex + 1;
	if (!ComboSectionNames.IsValidIndex(NextIndex))
	{
		// No more sections — let the montage finish naturally
		return;
	}

	CurrentComboIndex = NextIndex;

	// Re-acquire warp target for the next combo section (target may have moved)
	AcquireWarpTarget();

	// Jump to the next section in the currently playing montage
	UAbilitySystemComponent* ASC = GetARPGAbilitySystemComponent();
	if (ASC && AttackMontage)
	{
		const FName NextSection = ComboSectionNames[CurrentComboIndex];
		ASC->CurrentMontageJumpToSection(NextSection);
	}
}

AActor* UGA_MeleeAttack::FindForwardTarget(float Range, float HalfAngleDeg)
{
	AARPGCharacterBase* Character = GetARPGCharacter();
	if (!Character) return nullptr;

	UWorld* World = Character->GetWorld();
	if (!World) return nullptr;

	const FVector Origin = Character->GetActorLocation();
	const FVector Forward = Character->GetActorForwardVector();
	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(HalfAngleDeg));

	// Overlap sphere to find candidates
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByChannel(
		Overlaps,
		Origin,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(Range),
		QueryParams);

	AActor* BestTarget = nullptr;
	float BestDistSq = MAX_FLT;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (!Candidate) continue;
		if (Candidate == Character) continue;

		// Must implement AbilitySystemInterface (i.e., be a combatant)
		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Candidate);
		if (!ASI) continue;

		// Must be alive — check for State.Dead tag
		if (UAbilitySystemComponent* TargetASC = ASI->GetAbilitySystemComponent())
		{
			if (TargetASC->HasMatchingGameplayTag(ARPGGameplayTags::State_Dead))
			{
				continue;
			}
		}

		const FVector ToTarget = Candidate->GetActorLocation() - Origin;
		const FVector ToTargetDir = ToTarget.GetSafeNormal2D();

		// Check forward cone
		if (FVector::DotProduct(Forward, ToTargetDir) < CosHalfAngle)
		{
			continue;
		}

		const float DistSq = ToTarget.SizeSquared2D();
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

void UGA_MeleeAttack::AcquireWarpTarget()
{
	if (!bEnableAttackMagnetism) return;

	AARPGCharacterBase* Character = GetARPGCharacter();
	if (!Character) return;

	AActor* BestTarget = FindForwardTarget(WarpTargetSearchRadius, WarpTargetSearchAngle);
	if (!BestTarget) return;

	const FVector Origin = Character->GetActorLocation();
	const FVector ToTarget = BestTarget->GetActorLocation() - Origin;
	const float Dist = ToTarget.Size2D();

	if (Dist > WarpStopDistance)
	{
		const FVector WarpLocation = BestTarget->GetActorLocation() - ToTarget.GetSafeNormal2D() * WarpStopDistance;
		Character->SetAttackWarpTarget(
			FVector(WarpLocation.X, WarpLocation.Y, Origin.Z),
			WarpTargetName);
	}
	else
	{
		// Already close enough — just face the target
		Character->SetAttackWarpTarget(Origin, WarpTargetName);
	}
}

void UGA_MeleeAttack::ClearWarpTarget()
{
	if (AARPGCharacterBase* Character = GetARPGCharacter())
	{
		Character->ClearWarpTarget(WarpTargetName);
	}
}
