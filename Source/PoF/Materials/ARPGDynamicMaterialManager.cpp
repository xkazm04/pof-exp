#include "Materials/ARPGDynamicMaterialManager.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

UARPGDynamicMaterialManager::UARPGDynamicMaterialManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UARPGDynamicMaterialManager::BeginPlay()
{
	Super::BeginPlay();
	CreateDynamicMaterials();
}

void UARPGDynamicMaterialManager::CreateDynamicMaterials()
{
	if (!TargetMesh)
	{
		AActor* Owner = GetOwner();
		if (Owner)
		{
			TargetMesh = Owner->FindComponentByClass<UMeshComponent>();
		}
	}

	if (!TargetMesh)
	{
		return;
	}

	const int32 NumMaterials = TargetMesh->GetNumMaterials();
	DynamicMaterials.Reserve(NumMaterials);

	for (int32 i = 0; i < NumMaterials; ++i)
	{
		UMaterialInterface* BaseMat = TargetMesh->GetMaterial(i);
		if (BaseMat)
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this);
			DynamicMaterials.Add(MID);
			TargetMesh->SetMaterial(i, MID);
		}
	}
}

void UARPGDynamicMaterialManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateDamageFlash(DeltaTime);
	UpdateDissolve(DeltaTime);
	UpdateStatusEffects(DeltaTime);

	// Disable tick when no active effects remain
	if (ActiveEffects.Num() == 0 && !bHighlightActive)
	{
		SetComponentTickEnabled(false);
	}
}

// ----- Effect API -----

void UARPGDynamicMaterialManager::PlayDamageFlash(FLinearColor FlashColor, float Duration, float Intensity)
{
	RemoveEffect(EARPGMaterialEffect::DamageFlash);

	FARPGActiveEffect Effect;
	Effect.Type = EARPGMaterialEffect::DamageFlash;
	Effect.Duration = Duration;
	Effect.Elapsed = 0.f;
	Effect.Intensity = Intensity;
	ActiveEffects.Add(Effect);

	SetVectorOnAll(DamageFlashColorParam, FlashColor);
	SetScalarOnAll(DamageFlashIntensityParam, Intensity);
	SetComponentTickEnabled(true);
}

void UARPGDynamicMaterialManager::StartDissolve(float Duration, bool bReverse, FLinearColor EdgeColor)
{
	RemoveEffect(EARPGMaterialEffect::Dissolve);

	bDissolveReverse = bReverse;

	FARPGActiveEffect Effect;
	Effect.Type = EARPGMaterialEffect::Dissolve;
	Effect.Duration = Duration;
	Effect.Elapsed = 0.f;
	Effect.Intensity = 1.f;
	ActiveEffects.Add(Effect);

	SetVectorOnAll(DissolveEdgeColorParam, EdgeColor);
	SetScalarOnAll(DissolveAmountParam, bReverse ? 1.f : 0.f);
	SetComponentTickEnabled(true);
}

void UARPGDynamicMaterialManager::SetHighlight(bool bEnable, FLinearColor Color, float Intensity)
{
	bHighlightActive = bEnable;

	if (bEnable)
	{
		SetVectorOnAll(HighlightColorParam, Color);
		SetScalarOnAll(HighlightIntensityParam, Intensity);
		SetComponentTickEnabled(true);
	}
	else
	{
		SetScalarOnAll(HighlightIntensityParam, 0.f);
	}
}

void UARPGDynamicMaterialManager::ApplyStatusEffect(EARPGMaterialEffect EffectType, float Duration, float Intensity)
{
	if (EffectType != EARPGMaterialEffect::Freeze && EffectType != EARPGMaterialEffect::Burn)
	{
		return;
	}

	RemoveEffect(EffectType);

	FARPGActiveEffect Effect;
	Effect.Type = EffectType;
	Effect.Duration = Duration;
	Effect.Elapsed = 0.f;
	Effect.Intensity = Intensity;
	ActiveEffects.Add(Effect);

	if (EffectType == EARPGMaterialEffect::Freeze)
	{
		SetVectorOnAll(FreezeTintParam, FLinearColor(0.3f, 0.5f, 1.f) * Intensity);
	}
	else
	{
		SetScalarOnAll(BurnIntensityParam, Intensity);
	}

	SetComponentTickEnabled(true);
}

void UARPGDynamicMaterialManager::ClearAllEffects()
{
	ActiveEffects.Empty();
	bHighlightActive = false;
	bDissolveReverse = false;

	SetScalarOnAll(DamageFlashIntensityParam, 0.f);
	SetScalarOnAll(DissolveAmountParam, 0.f);
	SetScalarOnAll(HighlightIntensityParam, 0.f);
	SetVectorOnAll(FreezeTintParam, FLinearColor::Black);
	SetScalarOnAll(BurnIntensityParam, 0.f);

	SetComponentTickEnabled(false);
}

void UARPGDynamicMaterialManager::ClearEffect(EARPGMaterialEffect EffectType)
{
	RemoveEffect(EffectType);

	switch (EffectType)
	{
	case EARPGMaterialEffect::DamageFlash:
		SetScalarOnAll(DamageFlashIntensityParam, 0.f);
		break;
	case EARPGMaterialEffect::Dissolve:
		SetScalarOnAll(DissolveAmountParam, 0.f);
		bDissolveReverse = false;
		break;
	case EARPGMaterialEffect::Highlight:
		SetScalarOnAll(HighlightIntensityParam, 0.f);
		bHighlightActive = false;
		break;
	case EARPGMaterialEffect::Freeze:
		SetVectorOnAll(FreezeTintParam, FLinearColor::Black);
		break;
	case EARPGMaterialEffect::Burn:
		SetScalarOnAll(BurnIntensityParam, 0.f);
		break;
	default:
		break;
	}
}

// ----- Update loops -----

void UARPGDynamicMaterialManager::UpdateDamageFlash(float DeltaTime)
{
	FARPGActiveEffect* Effect = FindEffect(EARPGMaterialEffect::DamageFlash);
	if (!Effect)
	{
		return;
	}

	Effect->Elapsed += DeltaTime;

	if (Effect->IsExpired())
	{
		SetScalarOnAll(DamageFlashIntensityParam, 0.f);
		RemoveEffect(EARPGMaterialEffect::DamageFlash);
		return;
	}

	// Quick flash: sharp attack then decay
	const float Alpha = Effect->GetAlpha();
	const float FlashCurve = FMath::Pow(1.f - Alpha, 3.f);
	SetScalarOnAll(DamageFlashIntensityParam, Effect->Intensity * FlashCurve);
}

void UARPGDynamicMaterialManager::UpdateDissolve(float DeltaTime)
{
	FARPGActiveEffect* Effect = FindEffect(EARPGMaterialEffect::Dissolve);
	if (!Effect)
	{
		return;
	}

	Effect->Elapsed += DeltaTime;

	const float Alpha = Effect->GetAlpha();
	const float DissolveValue = bDissolveReverse ? (1.f - Alpha) : Alpha;
	SetScalarOnAll(DissolveAmountParam, DissolveValue);

	if (Effect->IsExpired())
	{
		RemoveEffect(EARPGMaterialEffect::Dissolve);
	}
}

void UARPGDynamicMaterialManager::UpdateStatusEffects(float DeltaTime)
{
	for (int32 i = ActiveEffects.Num() - 1; i >= 0; --i)
	{
		FARPGActiveEffect& Effect = ActiveEffects[i];

		if (Effect.Type != EARPGMaterialEffect::Freeze && Effect.Type != EARPGMaterialEffect::Burn)
		{
			continue;
		}

		// Duration 0 = indefinite
		if (Effect.Duration <= 0.f)
		{
			continue;
		}

		Effect.Elapsed += DeltaTime;

		if (Effect.IsExpired())
		{
			if (Effect.Type == EARPGMaterialEffect::Freeze)
			{
				SetVectorOnAll(FreezeTintParam, FLinearColor::Black);
			}
			else
			{
				SetScalarOnAll(BurnIntensityParam, 0.f);
			}
			ActiveEffects.RemoveAt(i);
		}
		else
		{
			// Fade out in the last 25% of duration
			const float FadeStart = 0.75f;
			const float Alpha = Effect.GetAlpha();
			if (Alpha > FadeStart)
			{
				const float FadeAlpha = 1.f - ((Alpha - FadeStart) / (1.f - FadeStart));
				if (Effect.Type == EARPGMaterialEffect::Freeze)
				{
					SetVectorOnAll(FreezeTintParam, FLinearColor(0.3f, 0.5f, 1.f) * Effect.Intensity * FadeAlpha);
				}
				else
				{
					SetScalarOnAll(BurnIntensityParam, Effect.Intensity * FadeAlpha);
				}
			}
		}
	}
}

// ----- Internal helpers -----

void UARPGDynamicMaterialManager::SetScalarOnAll(FName ParamName, float Value)
{
	for (UMaterialInstanceDynamic* MID : DynamicMaterials)
	{
		if (MID)
		{
			MID->SetScalarParameterValue(ParamName, Value);
		}
	}
}

void UARPGDynamicMaterialManager::SetVectorOnAll(FName ParamName, const FLinearColor& Value)
{
	for (UMaterialInstanceDynamic* MID : DynamicMaterials)
	{
		if (MID)
		{
			MID->SetVectorParameterValue(ParamName, Value);
		}
	}
}

FARPGActiveEffect* UARPGDynamicMaterialManager::FindEffect(EARPGMaterialEffect Type)
{
	for (FARPGActiveEffect& Effect : ActiveEffects)
	{
		if (Effect.Type == Type)
		{
			return &Effect;
		}
	}
	return nullptr;
}

void UARPGDynamicMaterialManager::RemoveEffect(EARPGMaterialEffect Type)
{
	ActiveEffects.RemoveAll([Type](const FARPGActiveEffect& E) { return E.Type == Type; });
}
