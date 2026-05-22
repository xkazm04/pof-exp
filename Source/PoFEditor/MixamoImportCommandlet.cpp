#include "MixamoImportCommandlet.h"
#include "IPythonScriptPlugin.h"

UMixamoImportCommandlet::UMixamoImportCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UMixamoImportCommandlet::Main(const FString& Params)
{
	UE_LOG(LogTemp, Display, TEXT("=== MixamoImport Commandlet ==="));

	// Parse parameters
	FString FbxFolder;
	FString TargetSkeleton;
	FString OutputBase = TEXT("/Game/Characters/Player/Animations/Mixamo");
	FString HipBone = TEXT("Hips");
	bool bSkipRetarget = Params.Contains(TEXT("-SkipRetarget"));
	bool bSkipRootMotion = Params.Contains(TEXT("-SkipRootMotion"));
	bool bDryRun = Params.Contains(TEXT("-DryRun"));

	ParseParam(Params, TEXT("FbxFolder"), FbxFolder);
	ParseParam(Params, TEXT("TargetSkeleton"), TargetSkeleton);
	ParseParam(Params, TEXT("OutputBase"), OutputBase);
	ParseParam(Params, TEXT("HipBone"), HipBone);

	if (FbxFolder.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Missing required parameter: -FbxFolder=\"path/to/mixamo/fbx/folder\""));
		UE_LOG(LogTemp, Display, TEXT("Usage: -run=MixamoImport -FbxFolder=\"C:\\Downloads\\Mixamo\" -TargetSkeleton=\"/Game/SK_Mannequin\""));
		return 1;
	}

	if (TargetSkeleton.IsEmpty() && !bSkipRetarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("No -TargetSkeleton specified. Retargeting will be skipped."));
		bSkipRetarget = true;
	}

	UE_LOG(LogTemp, Display, TEXT("  FBX Folder:      %s"), *FbxFolder);
	UE_LOG(LogTemp, Display, TEXT("  Target Skeleton:  %s"), *TargetSkeleton);
	UE_LOG(LogTemp, Display, TEXT("  Output Base:      %s"), *OutputBase);
	UE_LOG(LogTemp, Display, TEXT("  Hip Bone:         %s"), *HipBone);
	UE_LOG(LogTemp, Display, TEXT("  Skip Retarget:    %s"), bSkipRetarget ? TEXT("Yes") : TEXT("No"));
	UE_LOG(LogTemp, Display, TEXT("  Skip Root Motion: %s"), bSkipRootMotion ? TEXT("Yes") : TEXT("No"));
	UE_LOG(LogTemp, Display, TEXT("  Dry Run:          %s"), bDryRun ? TEXT("Yes") : TEXT("No"));

	const bool bSuccess = ExecutePythonPipeline(FbxFolder, TargetSkeleton, OutputBase, HipBone,
		bSkipRetarget, bSkipRootMotion, bDryRun);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Display, TEXT("=== MixamoImport Commandlet: SUCCESS ==="));
		return 0;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("=== MixamoImport Commandlet: FAILED ==="));
		return 1;
	}
}

bool UMixamoImportCommandlet::ExecutePythonPipeline(const FString& FbxFolder, const FString& TargetSkeleton,
	const FString& OutputBase, const FString& HipBone,
	bool bSkipRetarget, bool bSkipRootMotion, bool bDryRun)
{
	// Build Python script to execute the pipeline
	// Escape backslashes for Python string literals
	FString EscapedFbxFolder = FbxFolder.Replace(TEXT("\\"), TEXT("\\\\"));
	FString EscapedTargetSkeleton = TargetSkeleton.Replace(TEXT("\\"), TEXT("\\\\"));
	FString EscapedOutputBase = OutputBase.Replace(TEXT("\\"), TEXT("\\\\"));
	FString EscapedHipBone = HipBone.Replace(TEXT("\\"), TEXT("\\\\"));

	FString PythonScript = FString::Printf(TEXT(
		"import mixamo_pipeline\n"
		"config = mixamo_pipeline.PipelineConfig()\n"
		"config.fbx_folder = r'%s'\n"
		"config.target_skeleton = '%s'\n"
		"config.import_destination = '%s/Source'\n"
		"config.retarget_output = '%s/Retargeted'\n"
		"config.root_motion_hip_bone = '%s'\n"
		"config.skip_retarget = %s\n"
		"config.skip_root_motion = %s\n"
		"config.dry_run = %s\n"
		"pipeline = mixamo_pipeline.MixamoPipeline(config)\n"
		"result = pipeline.run()\n"
		"print(result.summary())\n"
	),
		*EscapedFbxFolder,
		*EscapedTargetSkeleton,
		*EscapedOutputBase,
		*EscapedOutputBase,
		*EscapedHipBone,
		bSkipRetarget ? TEXT("True") : TEXT("False"),
		bSkipRootMotion ? TEXT("True") : TEXT("False"),
		bDryRun ? TEXT("True") : TEXT("False")
	);

	UE_LOG(LogTemp, Display, TEXT("Executing Python pipeline..."));

	// Execute via Python scripting plugin
	IPythonScriptPlugin* PythonPlugin = IPythonScriptPlugin::Get();
	if (!PythonPlugin)
	{
		UE_LOG(LogTemp, Error, TEXT("Python Script Plugin is not available. Ensure PythonScriptPlugin is enabled."));
		return false;
	}

	const bool bResult = PythonPlugin->ExecPythonCommand(*PythonScript);

	return bResult;
}

bool UMixamoImportCommandlet::ParseParam(const FString& Params, const FString& ParamName, FString& OutValue)
{
	// Look for -ParamName="value" or -ParamName=value
	FString SearchKey = FString::Printf(TEXT("-%s="), *ParamName);

	int32 KeyIdx = Params.Find(SearchKey, ESearchCase::IgnoreCase);
	if (KeyIdx == INDEX_NONE)
	{
		return false;
	}

	int32 ValueStart = KeyIdx + SearchKey.Len();
	if (ValueStart >= Params.Len())
	{
		return false;
	}

	// Check if value is quoted
	if (Params[ValueStart] == TEXT('"'))
	{
		ValueStart++;
		int32 ValueEnd = Params.Find(TEXT("\""), ESearchCase::IgnoreCase, ESearchDir::FromStart, ValueStart);
		if (ValueEnd != INDEX_NONE)
		{
			OutValue = Params.Mid(ValueStart, ValueEnd - ValueStart);
			return true;
		}
	}
	else
	{
		// Unquoted: read until space
		int32 ValueEnd = Params.Find(TEXT(" "), ESearchCase::IgnoreCase, ESearchDir::FromStart, ValueStart);
		if (ValueEnd == INDEX_NONE)
		{
			ValueEnd = Params.Len();
		}
		OutValue = Params.Mid(ValueStart, ValueEnd - ValueStart);
		return true;
	}

	return false;
}
