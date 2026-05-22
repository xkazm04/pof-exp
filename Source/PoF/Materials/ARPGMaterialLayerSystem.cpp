#include "Materials/ARPGMaterialLayerSystem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture2D.h"

static void ApplyLayerToMID(UMaterialInstanceDynamic* MID, const UARPGMaterialLayer* Layer, int32 Index)
{
	if (!MID || !Layer)
	{
		return;
	}

	const FString Prefix = FString::Printf(TEXT("Layer%d_"), Index);

	// Textures
	if (UTexture2D* Tex = Layer->BaseColor.LoadSynchronous())
	{
		MID->SetTextureParameterValue(FName(*(Prefix + TEXT("BaseColor"))), Tex);
	}
	if (UTexture2D* Tex = Layer->Normal.LoadSynchronous())
	{
		MID->SetTextureParameterValue(FName(*(Prefix + TEXT("Normal"))), Tex);
	}
	if (UTexture2D* Tex = Layer->ORM.LoadSynchronous())
	{
		MID->SetTextureParameterValue(FName(*(Prefix + TEXT("ORM"))), Tex);
	}
	if (UTexture2D* Tex = Layer->HeightMap.LoadSynchronous())
	{
		MID->SetTextureParameterValue(FName(*(Prefix + TEXT("HeightMap"))), Tex);
	}

	// Scalars
	MID->SetScalarParameterValue(FName(*(Prefix + TEXT("Roughness"))), Layer->Roughness);
	MID->SetScalarParameterValue(FName(*(Prefix + TEXT("Metallic"))), Layer->Metallic);
	MID->SetScalarParameterValue(FName(*(Prefix + TEXT("Specular"))), Layer->Specular);
	MID->SetScalarParameterValue(FName(*(Prefix + TEXT("UVScale"))), Layer->UVScale);
	MID->SetScalarParameterValue(FName(*(Prefix + TEXT("NormalIntensity"))), Layer->NormalIntensity);

	// Color tint
	MID->SetVectorParameterValue(FName(*(Prefix + TEXT("Tint"))), Layer->Tint);
}

UMaterialInstanceDynamic* UARPGMaterialLayerStack::CreateLayeredInstance(UObject* Outer) const
{
	UMaterialInterface* Mat = MasterMaterial.LoadSynchronous();
	if (!Mat)
	{
		return nullptr;
	}

	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Mat, Outer);

	// Total layer count (base + blends, capped)
	const int32 TotalLayers = GetActiveLayerCount();
	MID->SetScalarParameterValue(TEXT("LayerCount"), static_cast<float>(TotalLayers));

	// Apply base layer at index 0
	const UARPGMaterialLayer* Base = BaseLayer.LoadSynchronous();
	if (Base)
	{
		ApplyLayerToMID(MID, Base, 0);
		MID->SetScalarParameterValue(TEXT("Layer0_BlendWeight"), 1.f);
		MID->SetScalarParameterValue(TEXT("Layer0_BlendMode"), 0.f); // N/A for base
	}

	// Apply blend layers at index 1..N
	const int32 MaxBlend = FMath::Min(BlendLayers.Num(), MaxLayers - 1);
	for (int32 i = 0; i < MaxBlend; ++i)
	{
		const FARPGLayerBlendConfig& Blend = BlendLayers[i];
		const UARPGMaterialLayer* Layer = Blend.Layer.LoadSynchronous();
		const int32 LayerIdx = i + 1;

		if (Layer)
		{
			ApplyLayerToMID(MID, Layer, LayerIdx);
		}

		const FString Prefix = FString::Printf(TEXT("Layer%d_"), LayerIdx);

		MID->SetScalarParameterValue(FName(*(Prefix + TEXT("BlendWeight"))), Blend.BlendWeight);
		MID->SetScalarParameterValue(FName(*(Prefix + TEXT("BlendMode"))), static_cast<float>(Blend.BlendMode));
		MID->SetScalarParameterValue(FName(*(Prefix + TEXT("HeightContrast"))), Blend.HeightContrast);
		MID->SetScalarParameterValue(FName(*(Prefix + TEXT("AngleFalloff"))), Blend.AngleFalloff);
		MID->SetScalarParameterValue(FName(*(Prefix + TEXT("VertexChannel"))), static_cast<float>(Blend.VertexChannel));
	}

	return MID;
}

int32 UARPGMaterialLayerStack::GetActiveLayerCount() const
{
	// Base layer + blend layers, capped by MaxLayers
	return FMath::Min(1 + BlendLayers.Num(), MaxLayers);
}

void UARPGMaterialLayerStack::SetLayerBlendWeight(UMaterialInstanceDynamic* MID, int32 LayerIndex, float Weight)
{
	if (!MID || LayerIndex < 0)
	{
		return;
	}

	const FString ParamName = FString::Printf(TEXT("Layer%d_BlendWeight"), LayerIndex);
	MID->SetScalarParameterValue(FName(*ParamName), FMath::Clamp(Weight, 0.f, 1.f));
}
