#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "MeshValidationCommandlet.generated.h"

/**
 * Commandlet that validates mesh assets for quality, naming, and performance budgets.
 *
 * Run: UnrealEditor-Cmd.exe PoF.uproject -run=MeshValidation -nopause -unattended -nosplash
 *
 * Checks:
 * - Naming conventions (SM_ for static, SK_ for skeletal)
 * - Triangle count budgets per category
 * - LOD presence and screen size thresholds
 * - UV channel existence
 * - Material slot count limits
 * - Collision setup
 * - Nanite configuration
 */
UCLASS()
class UMeshValidationCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;

private:
	struct FValidationResult
	{
		FString AssetPath;
		TArray<FString> Warnings;
		TArray<FString> Errors;
		bool bPassed = true;
	};

	/** Triangle count budgets per mesh category heuristic. */
	static constexpr int32 MaxTris_Prop = 10000;
	static constexpr int32 MaxTris_Architecture = 50000;
	static constexpr int32 MaxTris_Foliage = 5000;
	static constexpr int32 MaxTris_Equipment = 8000;
	static constexpr int32 MaxTris_VFX = 2000;
	static constexpr int32 MaxMaterialSlots = 4;

	void ValidateStaticMesh(class UStaticMesh* Mesh, FValidationResult& Result);
	void ValidateSkeletalMesh(class USkeletalMesh* Mesh, FValidationResult& Result);
	void PrintResults(const TArray<FValidationResult>& Results);
	int32 GetTriBudgetForName(const FString& AssetName);
};
