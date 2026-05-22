#include "MeshValidationCommandlet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "StaticMeshResources.h"
#include "Rendering/SkeletalMeshRenderData.h"

int32 UMeshValidationCommandlet::Main(const FString& Params)
{
	UE_LOG(LogTemp, Display, TEXT("=== Mesh Validation Commandlet ==="));

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	AssetRegistry.SearchAllAssets(true);

	TArray<FValidationResult> Results;

	// Validate all static meshes in /Game/
	{
		TArray<FAssetData> StaticMeshAssets;
		AssetRegistry.GetAssetsByClass(UStaticMesh::StaticClass()->GetClassPathName(), StaticMeshAssets, true);

		for (const FAssetData& AssetData : StaticMeshAssets)
		{
			if (!AssetData.PackageName.ToString().StartsWith(TEXT("/Game/")))
			{
				continue;
			}

			UStaticMesh* Mesh = Cast<UStaticMesh>(AssetData.GetAsset());
			if (!Mesh)
			{
				continue;
			}

			FValidationResult Result;
			Result.AssetPath = AssetData.GetObjectPathString();
			ValidateStaticMesh(Mesh, Result);
			Results.Add(MoveTemp(Result));
		}
	}

	// Validate all skeletal meshes in /Game/
	{
		TArray<FAssetData> SkeletalMeshAssets;
		AssetRegistry.GetAssetsByClass(USkeletalMesh::StaticClass()->GetClassPathName(), SkeletalMeshAssets, true);

		for (const FAssetData& AssetData : SkeletalMeshAssets)
		{
			if (!AssetData.PackageName.ToString().StartsWith(TEXT("/Game/")))
			{
				continue;
			}

			USkeletalMesh* Mesh = Cast<USkeletalMesh>(AssetData.GetAsset());
			if (!Mesh)
			{
				continue;
			}

			FValidationResult Result;
			Result.AssetPath = AssetData.GetObjectPathString();
			ValidateSkeletalMesh(Mesh, Result);
			Results.Add(MoveTemp(Result));
		}
	}

	PrintResults(Results);

	return 0;
}

void UMeshValidationCommandlet::ValidateStaticMesh(UStaticMesh* Mesh, FValidationResult& Result)
{
	const FString Name = Mesh->GetName();

	// Naming convention
	if (!Name.StartsWith(TEXT("SM_")))
	{
		Result.Warnings.Add(FString::Printf(TEXT("Static mesh '%s' does not follow SM_ naming convention"), *Name));
	}

	// Check render data
	if (!Mesh->GetRenderData() || Mesh->GetRenderData()->LODResources.Num() == 0)
	{
		Result.Errors.Add(TEXT("No render data / LOD resources"));
		Result.bPassed = false;
		return;
	}

	const FStaticMeshLODResources& LOD0 = Mesh->GetRenderData()->LODResources[0];
	const int32 NumTris = LOD0.IndexBuffer.GetNumIndices() / 3;
	const int32 NumVerts = LOD0.VertexBuffers.StaticMeshVertexBuffer.GetNumVertices();

	// Triangle budget
	const int32 Budget = GetTriBudgetForName(Name);
	if (NumTris > Budget)
	{
		Result.Warnings.Add(FString::Printf(TEXT("Triangle count %d exceeds budget %d"), NumTris, Budget));
	}

	// LOD check — meshes above 1000 tris should have at least 2 LODs (unless Nanite)
	const int32 NumLODs = Mesh->GetRenderData()->LODResources.Num();
	const bool bHasNanite = Mesh->IsNaniteEnabled();

	if (NumTris > 1000 && NumLODs < 2 && !bHasNanite)
	{
		Result.Warnings.Add(FString::Printf(TEXT("Only %d LOD(s) for %d tris mesh (no Nanite) — consider adding LODs"), NumLODs, NumTris));
	}

	// UV channel check
	const int32 NumUVChannels = LOD0.VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords();
	if (NumUVChannels == 0)
	{
		Result.Errors.Add(TEXT("No UV channels"));
		Result.bPassed = false;
	}

	// Material slot count
	const int32 NumMaterials = Mesh->GetStaticMaterials().Num();
	if (NumMaterials > MaxMaterialSlots)
	{
		Result.Warnings.Add(FString::Printf(TEXT("%d material slots (max recommended: %d)"), NumMaterials, MaxMaterialSlots));
	}

	// Null material check
	for (int32 i = 0; i < NumMaterials; ++i)
	{
		if (!Mesh->GetMaterial(i))
		{
			Result.Warnings.Add(FString::Printf(TEXT("Material slot %d is null"), i));
		}
	}

	// Collision check for non-VFX meshes
	if (!Name.Contains(TEXT("VFX")) && !Name.Contains(TEXT("Decal")))
	{
		if (!Mesh->GetBodySetup())
		{
			Result.Warnings.Add(TEXT("No collision body setup"));
		}
	}

	// LOD screen size validation
	if (NumLODs > 1)
	{
		for (int32 LODIdx = 1; LODIdx < NumLODs; ++LODIdx)
		{
			const float ScreenSize = Mesh->GetRenderData()->ScreenSize[LODIdx].Default;
			if (ScreenSize <= 0.f)
			{
				Result.Warnings.Add(FString::Printf(TEXT("LOD%d has invalid screen size threshold (%.4f)"), LODIdx, ScreenSize));
			}
		}
	}
}

void UMeshValidationCommandlet::ValidateSkeletalMesh(USkeletalMesh* Mesh, FValidationResult& Result)
{
	const FString Name = Mesh->GetName();

	// Naming convention
	if (!Name.StartsWith(TEXT("SK_")))
	{
		Result.Warnings.Add(FString::Printf(TEXT("Skeletal mesh '%s' does not follow SK_ naming convention"), *Name));
	}

	// Check render data
	FSkeletalMeshRenderData* RenderData = Mesh->GetResourceForRendering();
	if (!RenderData || RenderData->LODRenderData.Num() == 0)
	{
		Result.Errors.Add(TEXT("No render data / LOD resources"));
		Result.bPassed = false;
		return;
	}

	const FSkeletalMeshLODRenderData& LOD0 = RenderData->LODRenderData[0];
	const int32 NumVerts = LOD0.GetNumVertices();

	// Bone count check
	const int32 NumBones = Mesh->GetRefSkeleton().GetNum();
	if (NumBones > 256)
	{
		Result.Warnings.Add(FString::Printf(TEXT("High bone count: %d (consider optimizing for GPU skinning)"), NumBones));
	}

	// LOD check
	const int32 NumLODs = RenderData->LODRenderData.Num();
	if (NumVerts > 5000 && NumLODs < 2)
	{
		Result.Warnings.Add(FString::Printf(TEXT("Only %d LOD(s) for %d vert skeletal mesh — consider adding LODs"), NumLODs, NumVerts));
	}

	// Material slot count
	const int32 NumMaterials = Mesh->GetMaterials().Num();
	if (NumMaterials > MaxMaterialSlots)
	{
		Result.Warnings.Add(FString::Printf(TEXT("%d material slots (max recommended: %d)"), NumMaterials, MaxMaterialSlots));
	}

	// Physics asset check
	if (!Mesh->GetPhysicsAsset())
	{
		Result.Warnings.Add(TEXT("No physics asset assigned"));
	}
}

void UMeshValidationCommandlet::PrintResults(const TArray<FValidationResult>& Results)
{
	int32 TotalPassed = 0;
	int32 TotalWarnings = 0;
	int32 TotalErrors = 0;

	for (const FValidationResult& R : Results)
	{
		if (R.bPassed)
		{
			TotalPassed++;
		}
		TotalWarnings += R.Warnings.Num();
		TotalErrors += R.Errors.Num();

		if (R.Errors.Num() > 0 || R.Warnings.Num() > 0)
		{
			UE_LOG(LogTemp, Display, TEXT("--- %s ---"), *R.AssetPath);
			for (const FString& Err : R.Errors)
			{
				UE_LOG(LogTemp, Error, TEXT("  ERROR: %s"), *Err);
			}
			for (const FString& Warn : R.Warnings)
			{
				UE_LOG(LogTemp, Warning, TEXT("  WARN:  %s"), *Warn);
			}
		}
	}

	UE_LOG(LogTemp, Display, TEXT(""));
	UE_LOG(LogTemp, Display, TEXT("=== Mesh Validation Summary ==="));
	UE_LOG(LogTemp, Display, TEXT("Total assets: %d | Passed: %d | Warnings: %d | Errors: %d"),
		Results.Num(), TotalPassed, TotalWarnings, TotalErrors);
}

int32 UMeshValidationCommandlet::GetTriBudgetForName(const FString& AssetName)
{
	if (AssetName.Contains(TEXT("Arch")) || AssetName.Contains(TEXT("Wall")) || AssetName.Contains(TEXT("Floor")) || AssetName.Contains(TEXT("Building")))
	{
		return MaxTris_Architecture;
	}
	if (AssetName.Contains(TEXT("Foliage")) || AssetName.Contains(TEXT("Tree")) || AssetName.Contains(TEXT("Bush")) || AssetName.Contains(TEXT("Grass")))
	{
		return MaxTris_Foliage;
	}
	if (AssetName.Contains(TEXT("Weapon")) || AssetName.Contains(TEXT("Armor")) || AssetName.Contains(TEXT("Shield")) || AssetName.Contains(TEXT("Equip")))
	{
		return MaxTris_Equipment;
	}
	if (AssetName.Contains(TEXT("VFX")) || AssetName.Contains(TEXT("Particle")) || AssetName.Contains(TEXT("Decal")))
	{
		return MaxTris_VFX;
	}
	return MaxTris_Prop;
}
