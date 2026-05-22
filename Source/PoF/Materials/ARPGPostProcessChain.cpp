#include "Materials/ARPGPostProcessChain.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Camera/CameraComponent.h"
#include "Components/PostProcessComponent.h"
#include "Algo/Sort.h"

UARPGPostProcessChain::UARPGPostProcessChain()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UARPGPostProcessChain::BeginPlay()
{
	Super::BeginPlay();
	InitializeChain();
}

void UARPGPostProcessChain::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveBlendables();
	Super::EndPlay(EndPlayReason);
}

void UARPGPostProcessChain::InitializeChain()
{
	for (FARPGPostProcessEntry& Entry : EffectChain)
	{
		UMaterialInterface* Mat = Entry.Material.LoadSynchronous();
		if (!Mat)
		{
			continue;
		}

		Entry.DynamicInstance = UMaterialInstanceDynamic::Create(Mat, this);
	}

	ApplyBlendables();
}

void UARPGPostProcessChain::ApplyBlendables()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Find a post-process component or camera component to attach blendables to
	UPostProcessComponent* PPComp = Owner->FindComponentByClass<UPostProcessComponent>();
	UCameraComponent* CamComp = PPComp ? nullptr : Owner->FindComponentByClass<UCameraComponent>();

	if (!PPComp && !CamComp)
	{
		return;
	}

	// Sort by priority — use Algo::Sort to avoid TDereferenceWrapper issues with TArray<T*>
	TArray<FARPGPostProcessEntry*> Sorted;
	for (FARPGPostProcessEntry& Entry : EffectChain)
	{
		Sorted.Add(&Entry);
	}
	Algo::Sort(Sorted, [](const FARPGPostProcessEntry* A, const FARPGPostProcessEntry* B)
	{
		return A->Priority < B->Priority;
	});

	// Clear existing blendables and re-add
	FPostProcessSettings* Settings = nullptr;
	if (PPComp)
	{
		Settings = &PPComp->Settings;
	}
	else if (CamComp)
	{
		Settings = &CamComp->PostProcessSettings;
	}

	if (!Settings)
	{
		return;
	}

	// Remove only our managed blendables (by checking against our MIDs)
	for (int32 i = Settings->WeightedBlendables.Array.Num() - 1; i >= 0; --i)
	{
		UObject* Obj = Settings->WeightedBlendables.Array[i].Object;
		for (const FARPGPostProcessEntry& Entry : EffectChain)
		{
			if (Entry.DynamicInstance && Obj == Entry.DynamicInstance)
			{
				Settings->WeightedBlendables.Array.RemoveAt(i);
				break;
			}
		}
	}

	// Add enabled effects
	for (const FARPGPostProcessEntry* Entry : Sorted)
	{
		if (!Entry->bEnabled || !Entry->DynamicInstance)
		{
			continue;
		}

		FWeightedBlendable Blendable;
		Blendable.Weight = Entry->BlendWeight;
		Blendable.Object = Entry->DynamicInstance;
		Settings->WeightedBlendables.Array.Add(Blendable);
	}
}

void UARPGPostProcessChain::RemoveBlendables()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	UPostProcessComponent* PPComp = Owner->FindComponentByClass<UPostProcessComponent>();
	UCameraComponent* CamComp = PPComp ? nullptr : Owner->FindComponentByClass<UCameraComponent>();

	FPostProcessSettings* Settings = nullptr;
	if (PPComp)
	{
		Settings = &PPComp->Settings;
	}
	else if (CamComp)
	{
		Settings = &CamComp->PostProcessSettings;
	}

	if (!Settings)
	{
		return;
	}

	for (int32 i = Settings->WeightedBlendables.Array.Num() - 1; i >= 0; --i)
	{
		UObject* Obj = Settings->WeightedBlendables.Array[i].Object;
		for (const FARPGPostProcessEntry& Entry : EffectChain)
		{
			if (Entry.DynamicInstance && Obj == Entry.DynamicInstance)
			{
				Settings->WeightedBlendables.Array.RemoveAt(i);
				break;
			}
		}
	}
}

void UARPGPostProcessChain::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	bool bAnyAnimating = false;

	for (int32 i = BlendAnimations.Num() - 1; i >= 0; --i)
	{
		FBlendAnimation& Anim = BlendAnimations[i];
		Anim.Elapsed += DeltaTime;

		const float Alpha = FMath::Clamp(Anim.Elapsed / Anim.Duration, 0.f, 1.f);
		const float Weight = FMath::Lerp(Anim.StartWeight, Anim.TargetWeight, FMath::SmoothStep(0.f, 1.f, Alpha));

		FARPGPostProcessEntry* Entry = FindEntry(Anim.EffectName);
		if (Entry)
		{
			Entry->BlendWeight = Weight;
		}

		if (Alpha >= 1.f)
		{
			BlendAnimations.RemoveAt(i);
		}
		else
		{
			bAnyAnimating = true;
		}
	}

	if (bAnyAnimating)
	{
		ApplyBlendables();
	}

	if (BlendAnimations.Num() == 0)
	{
		SetComponentTickEnabled(false);
	}
}

// ----- Chain Management -----

int32 UARPGPostProcessChain::AddEffect(FName EffectName, UMaterialInterface* PPMaterial, float BlendWeight, int32 Priority)
{
	if (!PPMaterial)
	{
		return INDEX_NONE;
	}

	// Remove existing effect with same name
	RemoveEffect(EffectName);

	FARPGPostProcessEntry Entry;
	Entry.EffectName = EffectName;
	Entry.bEnabled = true;
	Entry.BlendWeight = BlendWeight;
	Entry.Priority = Priority;
	Entry.DynamicInstance = UMaterialInstanceDynamic::Create(PPMaterial, this);

	const int32 Index = EffectChain.Add(Entry);
	ApplyBlendables();
	return Index;
}

void UARPGPostProcessChain::RemoveEffect(FName EffectName)
{
	for (int32 i = EffectChain.Num() - 1; i >= 0; --i)
	{
		if (EffectChain[i].EffectName == EffectName)
		{
			// Remove from blendables before removing from chain
			if (EffectChain[i].DynamicInstance)
			{
				RemoveBlendables();
			}
			EffectChain.RemoveAt(i);
			ApplyBlendables();
			return;
		}
	}
}

// ----- Per-Effect Control -----

void UARPGPostProcessChain::SetEffectEnabled(FName EffectName, bool bEnabled)
{
	FARPGPostProcessEntry* Entry = FindEntry(EffectName);
	if (Entry && Entry->bEnabled != bEnabled)
	{
		Entry->bEnabled = bEnabled;
		ApplyBlendables();
	}
}

void UARPGPostProcessChain::SetEffectBlendWeight(FName EffectName, float Weight)
{
	FARPGPostProcessEntry* Entry = FindEntry(EffectName);
	if (Entry)
	{
		Entry->BlendWeight = FMath::Clamp(Weight, 0.f, 1.f);
		ApplyBlendables();
	}
}

void UARPGPostProcessChain::AnimateBlendWeight(FName EffectName, float TargetWeight, float Duration)
{
	FARPGPostProcessEntry* Entry = FindEntry(EffectName);
	if (!Entry)
	{
		return;
	}

	TargetWeight = FMath::Clamp(TargetWeight, 0.f, 1.f);

	if (Duration <= 0.f)
	{
		Entry->BlendWeight = TargetWeight;
		ApplyBlendables();
		return;
	}

	// Remove existing animation for this effect
	BlendAnimations.RemoveAll([EffectName](const FBlendAnimation& A) { return A.EffectName == EffectName; });

	FBlendAnimation Anim;
	Anim.EffectName = EffectName;
	Anim.StartWeight = Entry->BlendWeight;
	Anim.TargetWeight = TargetWeight;
	Anim.Duration = Duration;
	Anim.Elapsed = 0.f;
	BlendAnimations.Add(Anim);

	SetComponentTickEnabled(true);
}

void UARPGPostProcessChain::SetEffectScalar(FName EffectName, FName ParamName, float Value)
{
	FARPGPostProcessEntry* Entry = FindEntry(EffectName);
	if (Entry && Entry->DynamicInstance)
	{
		Entry->DynamicInstance->SetScalarParameterValue(ParamName, Value);
	}
}

void UARPGPostProcessChain::SetEffectVector(FName EffectName, FName ParamName, FLinearColor Value)
{
	FARPGPostProcessEntry* Entry = FindEntry(EffectName);
	if (Entry && Entry->DynamicInstance)
	{
		Entry->DynamicInstance->SetVectorParameterValue(ParamName, Value);
	}
}

UMaterialInstanceDynamic* UARPGPostProcessChain::GetEffectMID(FName EffectName) const
{
	const FARPGPostProcessEntry* Entry = FindEntry(EffectName);
	return Entry ? Entry->DynamicInstance : nullptr;
}

bool UARPGPostProcessChain::HasActiveEffects() const
{
	for (const FARPGPostProcessEntry& Entry : EffectChain)
	{
		if (Entry.bEnabled && Entry.DynamicInstance)
		{
			return true;
		}
	}
	return false;
}

// ----- Internal -----

FARPGPostProcessEntry* UARPGPostProcessChain::FindEntry(FName EffectName)
{
	for (FARPGPostProcessEntry& Entry : EffectChain)
	{
		if (Entry.EffectName == EffectName)
		{
			return &Entry;
		}
	}
	return nullptr;
}

const FARPGPostProcessEntry* UARPGPostProcessChain::FindEntry(FName EffectName) const
{
	for (const FARPGPostProcessEntry& Entry : EffectChain)
	{
		if (Entry.EffectName == EffectName)
		{
			return &Entry;
		}
	}
	return nullptr;
}
