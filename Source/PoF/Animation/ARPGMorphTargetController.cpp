#include "Animation/ARPGMorphTargetController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Character/ARPGCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ARPGAttributeSet.h"

UARPGMorphTargetController::UARPGMorphTargetController()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UARPGMorphTargetController::BeginPlay()
{
	Super::BeginPlay();

	// Find the skeletal mesh on the owning actor
	if (AActor* Owner = GetOwner())
	{
		TargetMesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
	}
}

void UARPGMorphTargetController::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!TargetMesh)
	{
		return;
	}

	bool bAnyActive = false;

	// Update all active blends
	for (int32 i = ActiveBlends.Num() - 1; i >= 0; --i)
	{
		FActiveMorphBlend& Blend = ActiveBlends[i];
		Blend.CurrentAlpha = FMath::FInterpConstantTo(Blend.CurrentAlpha, Blend.TargetAlpha, DeltaTime, Blend.BlendSpeed);

		if (Blend.bPendingRemoval && FMath::IsNearlyEqual(Blend.CurrentAlpha, 0.f, 0.001f))
		{
			ActiveBlends.RemoveAt(i);
			continue;
		}

		bAnyActive = true;
	}

	// Auto-drive damage morph from health
	if (bAutoDriveDamageMorph && DamageMorphTarget != NAME_None)
	{
		float DamageWeight = 0.f;
		if (const AARPGCharacterBase* Character = Cast<AARPGCharacterBase>(GetOwner()))
		{
			float HealthRatio = 1.f;
			if (UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent())
			{
				if (const UARPGAttributeSet* Attrs = ASC->GetSet<UARPGAttributeSet>())
				{
					const float MaxHP = Attrs->GetMaxHealth();
					HealthRatio = MaxHP > 0.f ? FMath::Clamp(Attrs->GetHealth() / MaxHP, 0.f, 1.f) : 1.f;
				}
			}
			DamageWeight = 1.f - HealthRatio;
		}
		TargetMesh->SetMorphTarget(DamageMorphTarget, DamageWeight);
		bAnyActive = true;
	}

	ApplyMorphWeights();

	// Disable tick when nothing is active
	if (!bAnyActive && ActiveBlends.Num() == 0)
	{
		SetComponentTickEnabled(false);
	}
}

void UARPGMorphTargetController::BlendToProfile(FName ProfileName, float Alpha, float BlendSpeed)
{
	const FMorphTargetProfile* Profile = FindProfile(ProfileName);
	if (!Profile)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MorphTarget] Profile '%s' not found"), *ProfileName.ToString());
		return;
	}

	// Check if already active — update target
	for (FActiveMorphBlend& Blend : ActiveBlends)
	{
		if (Blend.TargetProfile.ProfileName == ProfileName)
		{
			Blend.TargetAlpha = Alpha;
			Blend.BlendSpeed = BlendSpeed;
			Blend.bPendingRemoval = false;
			SetComponentTickEnabled(true);
			return;
		}
	}

	// New blend
	FActiveMorphBlend NewBlend;
	NewBlend.TargetProfile = *Profile;
	NewBlend.CurrentAlpha = 0.f;
	NewBlend.TargetAlpha = Alpha;
	NewBlend.BlendSpeed = BlendSpeed;
	NewBlend.bPendingRemoval = false;
	ActiveBlends.Add(MoveTemp(NewBlend));

	SetComponentTickEnabled(true);
}

void UARPGMorphTargetController::BlendOutProfile(FName ProfileName, float BlendSpeed)
{
	for (FActiveMorphBlend& Blend : ActiveBlends)
	{
		if (Blend.TargetProfile.ProfileName == ProfileName)
		{
			Blend.TargetAlpha = 0.f;
			Blend.BlendSpeed = BlendSpeed;
			Blend.bPendingRemoval = true;
			SetComponentTickEnabled(true);
			return;
		}
	}
}

void UARPGMorphTargetController::SetMorphTargetDirect(FName MorphTargetName, float Weight)
{
	if (TargetMesh)
	{
		TargetMesh->SetMorphTarget(MorphTargetName, FMath::Clamp(Weight, 0.f, 1.f));
	}
}

void UARPGMorphTargetController::ResetAllMorphTargets()
{
	ActiveBlends.Empty();

	if (TargetMesh)
	{
		TargetMesh->ClearMorphTargets();
	}

	SetComponentTickEnabled(false);
}

const FMorphTargetProfile* UARPGMorphTargetController::FindProfile(FName ProfileName) const
{
	for (const FMorphTargetProfile& Profile : Profiles)
	{
		if (Profile.ProfileName == ProfileName)
		{
			return &Profile;
		}
	}
	return nullptr;
}

void UARPGMorphTargetController::ApplyMorphWeights()
{
	if (!TargetMesh)
	{
		return;
	}

	// Composite all active blends additively
	CompositeWeights.Reset();

	for (const FActiveMorphBlend& Blend : ActiveBlends)
	{
		for (const auto& Pair : Blend.TargetProfile.MorphWeights)
		{
			float& Weight = CompositeWeights.FindOrAdd(Pair.Key, 0.f);
			Weight += Pair.Value * Blend.CurrentAlpha;
		}
	}

	// Apply composited weights to the mesh (clamped 0-1)
	for (const auto& Pair : CompositeWeights)
	{
		TargetMesh->SetMorphTarget(Pair.Key, FMath::Clamp(Pair.Value, 0.f, 1.f));
	}
}
