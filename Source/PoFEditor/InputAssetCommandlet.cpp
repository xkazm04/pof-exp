#include "InputAssetCommandlet.h"
#include "InputAction.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"

UInputAssetCommandlet::UInputAssetCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UInputAssetCommandlet::Main(const FString& /*Params*/)
{
	UE_LOG(LogTemp, Display, TEXT("=== InputAsset Commandlet: Starting Input Action asset creation ==="));

	const FString PackagePath = TEXT("/Game/Input/Actions");

	int32 SuccessCount = 0;
	int32 FailCount = 0;

	// --- IA_Move (Axis2D, WASD) ---
	UE_LOG(LogTemp, Display, TEXT("--- Creating IA_Move (Axis2D) ---"));
	if (CreateInputAction(PackagePath, TEXT("IA_Move"), EInputActionValueType::Axis2D))
	{
		SuccessCount++;
		UE_LOG(LogTemp, Display, TEXT("[OK] IA_Move created successfully."));
	}
	else
	{
		FailCount++;
		UE_LOG(LogTemp, Warning, TEXT("[FAIL] IA_Move creation failed."));
	}

	// --- IA_Attack (Digital/bool, LMB) ---
	UE_LOG(LogTemp, Display, TEXT("--- Creating IA_Attack (Boolean) ---"));
	if (CreateInputAction(PackagePath, TEXT("IA_Attack"), EInputActionValueType::Boolean))
	{
		SuccessCount++;
		UE_LOG(LogTemp, Display, TEXT("[OK] IA_Attack created successfully."));
	}
	else
	{
		FailCount++;
		UE_LOG(LogTemp, Warning, TEXT("[FAIL] IA_Attack creation failed."));
	}

	UE_LOG(LogTemp, Display, TEXT("=== InputAsset Commandlet: Done. Success=%d, Failed=%d ==="), SuccessCount, FailCount);

	return FailCount > 0 ? 1 : 0;
}

bool UInputAssetCommandlet::CreateInputAction(const FString& PackagePath, const FString& AssetName, EInputActionValueType ValueType)
{
	const FString FullPath = PackagePath / AssetName;

	UPackage* Package = CreatePackage(*FullPath);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create package: %s"), *FullPath);
		return false;
	}

	Package->FullyLoad();

	UInputAction* InputAction = NewObject<UInputAction>(
		Package, UInputAction::StaticClass(),
		*AssetName, RF_Public | RF_Standalone);

	if (!InputAction)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create UInputAction object"));
		return false;
	}

	InputAction->ValueType = ValueType;

	const TCHAR* TypeName =
		ValueType == EInputActionValueType::Boolean ? TEXT("Boolean")
		: ValueType == EInputActionValueType::Axis1D ? TEXT("Axis1D")
		: ValueType == EInputActionValueType::Axis2D ? TEXT("Axis2D")
		: TEXT("Axis3D");
	UE_LOG(LogTemp, Display, TEXT("  ValueType: %s"), TypeName);

	return SaveAsset(InputAction, Package, FullPath);
}

bool UInputAssetCommandlet::SaveAsset(UObject* Asset, UPackage* Package, const FString& PackagePath)
{
	FAssetRegistryModule::AssetCreated(Asset);
	Asset->MarkPackageDirty();

	const FString FilePath = FPackageName::LongPackageNameToFilename(
		PackagePath, FPackageName::GetAssetPackageExtension());

	const FString Directory = FPaths::GetPath(FilePath);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*Directory))
	{
		PlatformFile.CreateDirectoryTree(*Directory);
	}

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	const bool bSaved = UPackage::SavePackage(Package, Asset, *FilePath, SaveArgs);

	if (bSaved)
	{
		UE_LOG(LogTemp, Display, TEXT("  Saved: %s"), *FilePath);
		return true;
	}

	UE_LOG(LogTemp, Error, TEXT("  Failed to save: %s"), *FilePath);
	return false;
}
