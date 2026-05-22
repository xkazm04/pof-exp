#include "Materials/ARPGEnvironmentMaterialSet.h"
#include "Materials/MaterialInstanceDynamic.h"

UMaterialInstanceDynamic* UARPGEnvironmentMaterialSet::CreateSurfaceInstance(int32 SurfaceIndex, UObject* Outer) const
{
	if (!Surfaces.IsValidIndex(SurfaceIndex))
	{
		return nullptr;
	}

	UMaterialInterface* BaseMat = BaseMasterMaterial.LoadSynchronous();
	if (!BaseMat)
	{
		return nullptr;
	}

	const FARPGSurfaceMaterialDef& Def = Surfaces[SurfaceIndex];
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, Outer);

	// Textures
	if (UTexture2D* Tex = Def.BaseColorMap.LoadSynchronous())
	{
		MID->SetTextureParameterValue(TEXT("BaseColorMap"), Tex);
	}
	if (UTexture2D* Tex = Def.NormalMap.LoadSynchronous())
	{
		MID->SetTextureParameterValue(TEXT("NormalMap"), Tex);
	}
	if (UTexture2D* Tex = Def.ORMMap.LoadSynchronous())
	{
		MID->SetTextureParameterValue(TEXT("ORMMap"), Tex);
	}

	// Scalar parameters
	MID->SetScalarParameterValue(TEXT("Roughness"), Def.Roughness);
	MID->SetScalarParameterValue(TEXT("Metallic"), Def.Metallic);
	MID->SetScalarParameterValue(TEXT("Specular"), Def.Specular);
	MID->SetScalarParameterValue(TEXT("UVScale"), Def.UVScale);
	MID->SetScalarParameterValue(TEXT("NormalIntensity"), Def.NormalIntensity);

	// Color tint
	MID->SetVectorParameterValue(TEXT("ColorTint"), Def.ColorTint);

	// Detail texture
	MID->SetScalarParameterValue(TEXT("UseDetailTexture"), Def.bUseDetailTexture ? 1.f : 0.f);
	if (Def.bUseDetailTexture)
	{
		if (UTexture2D* Tex = Def.DetailNormalMap.LoadSynchronous())
		{
			MID->SetTextureParameterValue(TEXT("DetailNormalMap"), Tex);
		}
		MID->SetScalarParameterValue(TEXT("DetailUVScale"), Def.DetailUVScale);
		MID->SetScalarParameterValue(TEXT("DetailNormalIntensity"), Def.DetailNormalIntensity);
	}

	// Macro variation
	MID->SetScalarParameterValue(TEXT("UseMacroVariation"), Def.bUseMacroVariation ? 1.f : 0.f);
	MID->SetScalarParameterValue(TEXT("MacroVariationAmount"), Def.MacroVariationAmount);

	return MID;
}

int32 UARPGEnvironmentMaterialSet::FindSurfaceByName(const FString& SurfaceName) const
{
	for (int32 i = 0; i < Surfaces.Num(); ++i)
	{
		if (Surfaces[i].Name.Equals(SurfaceName, ESearchCase::IgnoreCase))
		{
			return i;
		}
	}
	return INDEX_NONE;
}
