#include "Materials/ARPGMasterMaterialConfig.h"
#include "Materials/MaterialInstanceDynamic.h"

UMaterialInstanceDynamic* UARPGMasterMaterialConfig::CreateConfiguredInstance(UObject* Outer) const
{
	UMaterialInterface* Mat = MasterMaterial.LoadSynchronous();
	if (!Mat)
	{
		return nullptr;
	}

	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Mat, Outer);

	for (const FARPGMaterialScalarParam& Param : ScalarParameters)
	{
		MID->SetScalarParameterValue(Param.ParameterName, Param.DefaultValue);
	}
	for (const FARPGMaterialVectorParam& Param : VectorParameters)
	{
		MID->SetVectorParameterValue(Param.ParameterName, Param.DefaultValue);
	}
	for (const FARPGMaterialTextureParam& Param : TextureParameters)
	{
		if (Param.DefaultTexture)
		{
			MID->SetTextureParameterValue(Param.ParameterName, Param.DefaultTexture);
		}
	}

	return MID;
}
