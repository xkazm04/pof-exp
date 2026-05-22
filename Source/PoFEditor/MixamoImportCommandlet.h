#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "MixamoImportCommandlet.generated.h"

/**
 * Commandlet for headless Mixamo import & retarget pipeline execution.
 *
 * Runs the full Python-based pipeline:
 *   1. Import Mixamo FBX files (with bone prefix stripping)
 *   2. Create IK Rigs and Retargeter (fuzzy chain mapping)
 *   3. Batch retarget animations to target skeleton
 *   4. Extract root motion from in-place animations
 *
 * Run via:
 *   UnrealEditor-Cmd.exe PoF.uproject -run=MixamoImport
 *     -FbxFolder="C:\Downloads\Mixamo"
 *     -TargetSkeleton="/Game/Characters/Player/Meshes/SK_Mannequin"
 *     [-OutputBase="/Game/Characters/Player/Animations/Mixamo"]
 *     [-HipBone="Hips"]
 *     [-SkipRetarget]
 *     [-SkipRootMotion]
 *     [-DryRun]
 */
UCLASS()
class UMixamoImportCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UMixamoImportCommandlet();
	virtual int32 Main(const FString& Params) override;

private:
	/** Execute the Python pipeline script with the given arguments. */
	bool ExecutePythonPipeline(const FString& FbxFolder, const FString& TargetSkeleton,
		const FString& OutputBase, const FString& HipBone,
		bool bSkipRetarget, bool bSkipRootMotion, bool bDryRun);

	/** Parse a command line parameter value. */
	static bool ParseParam(const FString& Params, const FString& ParamName, FString& OutValue);
};
