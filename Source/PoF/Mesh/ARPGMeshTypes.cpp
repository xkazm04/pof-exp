#include "Mesh/ARPGMeshTypes.h"

FARPGNaniteFoliageConfig UARPGNaniteFoliageSettings::GetPreset(FName PresetName) const
{
	if (const FARPGNaniteFoliageConfig* Found = FoliagePresets.Find(PresetName))
	{
		return *Found;
	}
	return FARPGNaniteFoliageConfig();
}

void UARPGNaniteFoliageSettings::InitializeFoliageDefaults()
{
	// Large trees — high detail, moderate wind
	{
		FARPGNaniteFoliageConfig Config;
		Config.bEnableNanite = true;
		Config.bEnableWPOWind = true;
		Config.WindIntensity = 0.8f;
		Config.FallbackTrianglePercent = 0.3f;
		Config.CullDistance = 80000.f;
		Config.DensityScale = 1.0f;
		FoliagePresets.Add(FName("Tree"), Config);
	}

	// Bushes/shrubs — moderate detail, strong wind
	{
		FARPGNaniteFoliageConfig Config;
		Config.bEnableNanite = true;
		Config.bEnableWPOWind = true;
		Config.WindIntensity = 1.5f;
		Config.FallbackTrianglePercent = 0.4f;
		Config.CullDistance = 30000.f;
		Config.DensityScale = 1.0f;
		FoliagePresets.Add(FName("Bush"), Config);
	}

	// Grass — aggressive culling, high wind
	{
		FARPGNaniteFoliageConfig Config;
		Config.bEnableNanite = true;
		Config.bEnableWPOWind = true;
		Config.WindIntensity = 2.0f;
		Config.FallbackTrianglePercent = 0.5f;
		Config.CullDistance = 15000.f;
		Config.DensityScale = 0.8f;
		Config.MinLOD = 1;
		FoliagePresets.Add(FName("Grass"), Config);
	}

	// Rocks/static foliage — no wind
	{
		FARPGNaniteFoliageConfig Config;
		Config.bEnableNanite = true;
		Config.bEnableWPOWind = false;
		Config.WindIntensity = 0.f;
		Config.FallbackTrianglePercent = 0.3f;
		Config.CullDistance = 60000.f;
		Config.DensityScale = 1.0f;
		FoliagePresets.Add(FName("Rock"), Config);
	}
}

FARPGMeshImportConfig UARPGMeshImportSettings::GetDefaultConfig(EARPGMeshCategory Category) const
{
	if (const FARPGMeshImportConfig* Found = CategoryDefaults.Find(Category))
	{
		return *Found;
	}

	// Return sensible default
	FARPGMeshImportConfig Default;
	Default.Category = Category;
	return Default;
}

FARPGSkeletalMeshImportConfig UARPGMeshImportSettings::GetDefaultSkeletalConfig(EARPGMeshCategory Category) const
{
	if (const FARPGSkeletalMeshImportConfig* Found = SkeletalDefaults.Find(Category))
	{
		return *Found;
	}
	return FARPGSkeletalMeshImportConfig();
}

void UARPGMeshImportSettings::InitializeDefaults()
{
	// Prop defaults: simple collision, 3 LODs, no Nanite
	{
		FARPGMeshImportConfig Config;
		Config.Category = EARPGMeshCategory::Prop;
		Config.CollisionPreset = EARPGCollisionPreset::ConvexDecomposition;
		Config.MaxConvexHulls = 4;
		Config.bEnableNanite = false;
		Config.LightmapResolution = 64;
		Config.LODTiers.Add({0.75f, 0.25f}); // LOD1
		Config.LODTiers.Add({0.4f, 0.5f});   // LOD2
		Config.LODTiers.Add({0.15f, 0.75f}); // LOD3
		CategoryDefaults.Add(EARPGMeshCategory::Prop, Config);
	}

	// Architecture defaults: complex collision (walkable), Nanite enabled, 2 LODs
	{
		FARPGMeshImportConfig Config;
		Config.Category = EARPGMeshCategory::Architecture;
		Config.CollisionPreset = EARPGCollisionPreset::ComplexAsSimple;
		Config.bEnableNanite = true;
		Config.LightmapResolution = 128;
		Config.LODTiers.Add({0.5f, 0.4f});
		Config.LODTiers.Add({0.2f, 0.7f});
		CategoryDefaults.Add(EARPGMeshCategory::Architecture, Config);
	}

	// Foliage defaults: no collision, Nanite enabled, 4 LODs for aggressive culling
	{
		FARPGMeshImportConfig Config;
		Config.Category = EARPGMeshCategory::Foliage;
		Config.CollisionPreset = EARPGCollisionPreset::None;
		Config.bEnableNanite = true;
		Config.bGenerateLightmapUVs = false; // Foliage typically uses dynamic lighting
		Config.LightmapResolution = 4;
		Config.LODTiers.Add({0.8f, 0.3f});
		Config.LODTiers.Add({0.5f, 0.5f});
		Config.LODTiers.Add({0.25f, 0.7f});
		Config.LODTiers.Add({0.1f, 0.9f});
		CategoryDefaults.Add(EARPGMeshCategory::Foliage, Config);
	}

	// Equipment defaults: simple box collision, 2 LODs, no Nanite
	{
		FARPGMeshImportConfig Config;
		Config.Category = EARPGMeshCategory::Equipment;
		Config.CollisionPreset = EARPGCollisionPreset::SimpleBox;
		Config.bEnableNanite = false;
		Config.LightmapResolution = 32;
		Config.LODTiers.Add({0.5f, 0.4f});
		Config.LODTiers.Add({0.2f, 0.7f});
		CategoryDefaults.Add(EARPGMeshCategory::Equipment, Config);
	}

	// VFX defaults: no collision, no LOD, no Nanite
	{
		FARPGMeshImportConfig Config;
		Config.Category = EARPGMeshCategory::VFX;
		Config.CollisionPreset = EARPGCollisionPreset::None;
		Config.bEnableNanite = false;
		Config.bGenerateLightmapUVs = false;
		Config.LightmapResolution = 4;
		CategoryDefaults.Add(EARPGMeshCategory::VFX, Config);
	}

	// Character defaults: no collision (handled by capsule), no Nanite, 3 LODs
	{
		FARPGMeshImportConfig Config;
		Config.Category = EARPGMeshCategory::Character;
		Config.CollisionPreset = EARPGCollisionPreset::None;
		Config.bEnableNanite = false;
		Config.bGenerateLightmapUVs = false;
		Config.LightmapResolution = 4;
		Config.LODTiers.Add({0.6f, 0.3f});
		Config.LODTiers.Add({0.35f, 0.5f});
		Config.LODTiers.Add({0.15f, 0.75f});
		CategoryDefaults.Add(EARPGMeshCategory::Character, Config);
	}

	// Skeletal mesh defaults — Character
	{
		FARPGSkeletalMeshImportConfig Config;
		Config.bImportMorphTargets = true;
		Config.bCreatePhysicsAsset = true;
		Config.MaxBonesPerVertex = 4;
		Config.LODTiers.Add({0.6f, 0.3f});
		Config.LODTiers.Add({0.35f, 0.5f});
		Config.LODTiers.Add({0.15f, 0.75f});
		SkeletalDefaults.Add(EARPGMeshCategory::Character, Config);
	}

	// Skeletal mesh defaults — Equipment (weapons with simple rigs)
	{
		FARPGSkeletalMeshImportConfig Config;
		Config.bImportMorphTargets = false;
		Config.bCreatePhysicsAsset = false;
		Config.MaxBonesPerVertex = 2;
		Config.LODTiers.Add({0.5f, 0.4f});
		Config.LODTiers.Add({0.2f, 0.7f});
		SkeletalDefaults.Add(EARPGMeshCategory::Equipment, Config);
	}
}
