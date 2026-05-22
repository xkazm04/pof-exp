#include "LevelDesign/ARPGBiomeDefinition.h"

FPrimaryAssetId UARPGBiomeDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("BiomeDefinition"), BiomeID);
}
