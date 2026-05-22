#include "Materials/ARPGMaterialPerformanceManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "Components/MeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

void UARPGMaterialPerformanceManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Default LOD levels if none configured
	if (LODLevels.Num() == 0)
	{
		FARPGMaterialLODConfig Near;
		Near.SwitchDistance = 0.f;
		Near.Quality = EARPGMaterialQuality::High;
		LODLevels.Add(Near);

		FARPGMaterialLODConfig Mid;
		Mid.SwitchDistance = 3000.f;
		Mid.Quality = EARPGMaterialQuality::Medium;
		Mid.bDisableDetailTextures = true;
		LODLevels.Add(Mid);

		FARPGMaterialLODConfig Far;
		Far.SwitchDistance = 8000.f;
		Far.Quality = EARPGMaterialQuality::Low;
		Far.bDisableDetailTextures = true;
		Far.bDisableMacroVariation = true;
		Far.bDisableNormalMap = true;
		Far.UVScaleMultiplier = 0.5f;
		LODLevels.Add(Far);
	}
}

void UARPGMaterialPerformanceManager::Tick(float DeltaTime)
{
	TimeSinceLastLODUpdate += DeltaTime;

	if (TimeSinceLastLODUpdate >= LODUpdateInterval)
	{
		TimeSinceLastLODUpdate = 0.f;
		CleanupStaleEntries();
		UpdateLODLevels();
	}
}

TStatId UARPGMaterialPerformanceManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UARPGMaterialPerformanceManager, STATGROUP_Tickables);
}

// ----- Quality Tier -----

void UARPGMaterialPerformanceManager::SetQualityTier(EARPGMaterialQuality Quality)
{
	if (CurrentQuality == Quality)
	{
		return;
	}

	CurrentQuality = Quality;

	// Force all meshes to re-evaluate LOD
	for (FARPGManagedMesh& Managed : ManagedMeshes)
	{
		Managed.CurrentLODLevel = INDEX_NONE;
	}
}

// ----- Mesh Registration -----

void UARPGMaterialPerformanceManager::RegisterMesh(UMeshComponent* MeshComp)
{
	if (!MeshComp)
	{
		return;
	}

	// Check if already registered
	for (const FARPGManagedMesh& Existing : ManagedMeshes)
	{
		if (Existing.MeshComp.Get() == MeshComp)
		{
			return;
		}
	}

	FARPGManagedMesh Managed;
	Managed.MeshComp = MeshComp;

	const int32 NumMats = MeshComp->GetNumMaterials();
	Managed.SharedMIDs.Reserve(NumMats);

	for (int32 i = 0; i < NumMats; ++i)
	{
		UMaterialInterface* BaseMat = MeshComp->GetMaterial(i);
		if (BaseMat)
		{
			UMaterialInstanceDynamic* MID = GetOrCreateSharedMID(BaseMat);
			Managed.SharedMIDs.Add(MID);
			MeshComp->SetMaterial(i, MID);
		}
		else
		{
			Managed.SharedMIDs.Add(nullptr);
		}
	}

	ManagedMeshes.Add(MoveTemp(Managed));
}

void UARPGMaterialPerformanceManager::UnregisterMesh(UMeshComponent* MeshComp)
{
	for (int32 i = ManagedMeshes.Num() - 1; i >= 0; --i)
	{
		if (ManagedMeshes[i].MeshComp.Get() == MeshComp)
		{
			ManagedMeshes.RemoveAt(i);
			return;
		}
	}
}

// ----- Profiling -----

FARPGMaterialComplexity UARPGMaterialPerformanceManager::ProfileMaterial(UMaterialInterface* Material) const
{
	FARPGMaterialComplexity Result;
	if (!Material)
	{
		return Result;
	}

	Result.MaterialName = Material->GetFName();

	UMaterial* BaseMat = Material->GetMaterial();
	if (!BaseMat)
	{
		return Result;
	}

	// Read material properties for complexity estimation
	Result.bIsTranslucent = BaseMat->GetBlendMode() != BLEND_Opaque;

	// Check for WPO usage
	Result.bUsesWPO = BaseMat->HasVertexPositionOffsetConnected();

	// Estimate texture samples from the material's referenced textures
	TArray<UTexture*> Textures;
	Material->GetUsedTextures(Textures);
	Result.TextureSamples = Textures.Num();

	// Instruction count estimate (heuristic based on material properties)
	Result.InstructionCount = 50; // Base cost
	Result.InstructionCount += Result.TextureSamples * 8; // Per texture sample
	if (Result.bIsTranslucent)
	{
		Result.InstructionCount += 30;
	}
	if (Result.bUsesWPO)
	{
		Result.InstructionCount += 25;
	}
	if (Result.bUsesDisplacement)
	{
		Result.InstructionCount += 40;
	}

	// Complexity score: normalized 0-100
	Result.ComplexityScore = FMath::Clamp(
		static_cast<float>(Result.InstructionCount) / 3.f + Result.TextureSamples * 2.f,
		0.f, 100.f
	);

	return Result;
}

// ----- Internal -----

void UARPGMaterialPerformanceManager::UpdateLODLevels()
{
	UWorld* World = GetWorld();
	if (!World || ManagedMeshes.Num() == 0 || LODLevels.Num() == 0)
	{
		return;
	}

	// Get camera location
	FVector CameraLocation = FVector::ZeroVector;
	APlayerController* PC = World->GetFirstPlayerController();
	if (PC)
	{
		FRotator CameraRotation;
		PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
	}
	else
	{
		return;
	}

	for (FARPGManagedMesh& Managed : ManagedMeshes)
	{
		UMeshComponent* Mesh = Managed.MeshComp.Get();
		if (!Mesh)
		{
			continue;
		}

		const float Distance = FVector::Dist(CameraLocation, Mesh->GetComponentLocation());
		const int32 NewLOD = DetermineLODLevel(Distance);

		if (NewLOD != Managed.CurrentLODLevel)
		{
			Managed.CurrentLODLevel = NewLOD;
			ApplyLODToMesh(Managed, NewLOD);
		}
	}
}

int32 UARPGMaterialPerformanceManager::DetermineLODLevel(float DistanceToCamera) const
{
	int32 BestLevel = 0;
	for (int32 i = LODLevels.Num() - 1; i >= 0; --i)
	{
		if (DistanceToCamera >= LODLevels[i].SwitchDistance)
		{
			BestLevel = i;
			break;
		}
	}

	// Clamp down based on quality tier
	const int32 QualityOffset = static_cast<int32>(EARPGMaterialQuality::Ultra) - static_cast<int32>(CurrentQuality);
	return FMath::Min(BestLevel + QualityOffset, LODLevels.Num() - 1);
}

void UARPGMaterialPerformanceManager::ApplyLODToMesh(FARPGManagedMesh& Managed, int32 LODLevel)
{
	if (!LODLevels.IsValidIndex(LODLevel))
	{
		return;
	}

	const FARPGMaterialLODConfig& Config = LODLevels[LODLevel];

	for (UMaterialInstanceDynamic* MID : Managed.SharedMIDs)
	{
		if (!MID)
		{
			continue;
		}

		MID->SetScalarParameterValue(TEXT("UseDetailTexture"), Config.bDisableDetailTextures ? 0.f : 1.f);
		MID->SetScalarParameterValue(TEXT("UseMacroVariation"), Config.bDisableMacroVariation ? 0.f : 1.f);
		MID->SetScalarParameterValue(TEXT("UseNormalMap"), Config.bDisableNormalMap ? 0.f : 1.f);
		MID->SetScalarParameterValue(TEXT("UVScaleMultiplier"), Config.UVScaleMultiplier);
	}
}

UMaterialInstanceDynamic* UARPGMaterialPerformanceManager::GetOrCreateSharedMID(UMaterialInterface* SourceMat)
{
	if (!SourceMat)
	{
		return nullptr;
	}

	const FName Key = SourceMat->GetFName();

	if (TObjectPtr<UMaterialInstanceDynamic>* Found = SharedMIDPool.Find(Key))
	{
		if (*Found)
		{
			return *Found;
		}
	}

	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(SourceMat, this);
	SharedMIDPool.Add(Key, MID);
	return MID;
}

void UARPGMaterialPerformanceManager::CleanupStaleEntries()
{
	// Remove managed meshes whose components have been destroyed
	for (int32 i = ManagedMeshes.Num() - 1; i >= 0; --i)
	{
		if (!ManagedMeshes[i].MeshComp.IsValid())
		{
			ManagedMeshes.RemoveAt(i);
		}
	}
}
