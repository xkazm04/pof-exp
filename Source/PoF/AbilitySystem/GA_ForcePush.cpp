#include "AbilitySystem/GA_ForcePush.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystem/Effects/GE_Cooldown_ForcePush.h"
#include "AbilitySystem/Effects/GE_Damage.h"
#include "Character/ARPGCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "GameFramework/Character.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"

UGA_ForcePush::UGA_ForcePush()
{
	bAutoEndAbility = false;

	SetAssetTags(FGameplayTagContainer(ARPGGameplayTags::Ability_ForcePush));
	ActivationBlockedTags.AddTag(ARPGGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(ARPGGameplayTags::State_Stunned);

	AbilityManaCost = 20.f;
	CooldownGameplayEffectClass = UGE_Cooldown_ForcePush::StaticClass();
	AbilityCooldownTag = ARPGGameplayTags::Cooldown_ForcePush;

	// Default damage effect — UGE_Damage runs the shared UARPGDamageExecution.
	DamageEffect = UGE_Damage::StaticClass();
}

void UGA_ForcePush::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ApplyForcePush();

	// Play the believable force-push body animation (mocap, retargeted onto Manny).
	// Fire-and-forget on the mesh AnimInstance so it survives this ability's immediate
	// EndAbility (the push effect itself is instant; the montage is the visual).
	if (UAnimMontage* PushMontage = LoadObject<UAnimMontage>(nullptr, TEXT("/Game/Anims/Jedi/AM_JediForcePush.AM_JediForcePush")))
	{
		if (ACharacter* AvatarChar = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
		{
			if (USkeletalMeshComponent* Mesh = AvatarChar->GetMesh())
			{
				if (UAnimInstance* AnimInst = Mesh->GetAnimInstance())
				{
					AnimInst->Montage_Play(PushMontage, 1.0f);
				}
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_ForcePush::ApplyForcePush()
{
	AARPGCharacterBase* Character = GetARPGCharacter();
	if (!Character) return;

	UAbilitySystemComponent* SourceASC = GetARPGAbilitySystemComponent();
	if (!SourceASC) return;

	UWorld* World = Character->GetWorld();
	if (!World) return;

	const FVector Origin = Character->GetActorLocation();
	const FVector Forward = Character->GetActorForwardVector().GetSafeNormal2D();
	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(ConeHalfAngleDeg));

	// Caster impact VFX (optional).
	if (PushVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, PushVFX, Origin, Character->GetActorRotation());
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);

	World->OverlapMultiByChannel(
		Overlaps, Origin, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeSphere(PushRange), QueryParams);

	int32 NumPushed = 0;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* HitActor = Overlap.GetActor();
		if (!HitActor || HitActor == Character) continue;

		// Cone gate: only affect targets in front of the caster.
		const FVector ToTarget = (HitActor->GetActorLocation() - Origin).GetSafeNormal2D();
		if (FVector::DotProduct(Forward, ToTarget) < CosHalfAngle) continue;

		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(HitActor);
		UAbilitySystemComponent* TargetASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
		if (!TargetASC) continue;

		// 1. Physics knockback — away from the caster, with an upward arc.
		if (ACharacter* TargetChar = Cast<ACharacter>(HitActor))
		{
			const FVector LaunchVel = ToTarget * HorizontalKnockback + FVector(0.f, 0.f, VerticalKnockback);
			TargetChar->LaunchCharacter(LaunchVel, true, true);

			// Landing follow-up (status-effects::status-dazed): flag the target so
			// AARPGCharacterBase::Landed applies UGE_Dazed at ground re-contact.
			if (AARPGCharacterBase* ARPGChar = Cast<AARPGCharacterBase>(TargetChar))
			{
				ARPGChar->bPendingDazeOnLanding = true;
			}
		}

		// 2. Light physical damage via the shared damage execution.
		if (DamageEffect)
		{
			FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
			Context.AddSourceObject(this);

			FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffect, GetAbilityLevel(), Context);
			if (SpecHandle.IsValid())
			{
				SpecHandle.Data->SetSetByCallerMagnitude(ARPGGameplayTags::Data_Damage_Base, BaseDamage);
				SpecHandle.Data->AddDynamicAssetTag(ARPGGameplayTags::Damage_Physical);
				SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
			}
		}

		++NumPushed;
	}

	UE_LOG(LogTemp, Log, TEXT("[GA_ForcePush] pushed %d target(s) inside a %.0f-deg forward cone"), NumPushed, ConeHalfAngleDeg);
}
