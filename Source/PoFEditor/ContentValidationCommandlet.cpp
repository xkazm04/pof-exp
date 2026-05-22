#include "ContentValidationCommandlet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture2D.h"
#include "UObject/SoftObjectPath.h"

int32 UContentValidationCommandlet::Main(const FString& Params)
{
	UE_LOG(LogTemp, Display, TEXT("=== PoF Content Validation ==="));

	ValidateAssetReferences();
	CheckTexturesizes();
	ReportSummary();

	return ErrorCount > 0 ? 1 : 0;
}

void UContentValidationCommandlet::ValidateAssetReferences()
{
	UE_LOG(LogTemp, Display, TEXT("--- Checking Asset References ---"));

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	AssetRegistry.SearchAllAssets(true);

	TArray<FAssetData> AllAssets;
	AssetRegistry.GetAllAssets(AllAssets, true);

	int32 ScannedCount = 0;
	for (const FAssetData& Asset : AllAssets)
	{
		// Only check game content
		if (!Asset.PackageName.ToString().StartsWith(TEXT("/Game/")))
		{
			continue;
		}

		ScannedCount++;

		TArray<FName> Dependencies;
		AssetRegistry.GetDependencies(Asset.PackageName, Dependencies);

		for (const FName& Dep : Dependencies)
		{
			const FString DepStr = Dep.ToString();
			// Skip engine/plugin content
			if (DepStr.StartsWith(TEXT("/Engine/")) || DepStr.StartsWith(TEXT("/Script/")))
			{
				continue;
			}

			TArray<FAssetData> DepAssets;
			AssetRegistry.GetAssetsByPackageName(Dep, DepAssets, true);
			if (DepAssets.Num() == 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("  MISSING REF: %s -> %s"), *Asset.PackageName.ToString(), *DepStr);
				WarningCount++;
			}
		}
	}

	UE_LOG(LogTemp, Display, TEXT("  Scanned %d game assets"), ScannedCount);
}

void UContentValidationCommandlet::CheckTexturesizes()
{
	UE_LOG(LogTemp, Display, TEXT("--- Checking Texture Sizes ---"));

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	TArray<FAssetData> TextureAssets;
	AssetRegistry.GetAssetsByClass(UTexture2D::StaticClass()->GetClassPathName(), TextureAssets, true);

	constexpr int32 MaxRecommendedSize = 4096;
	int32 OversizedCount = 0;

	for (const FAssetData& TexAsset : TextureAssets)
	{
		if (!TexAsset.PackageName.ToString().StartsWith(TEXT("/Game/")))
		{
			continue;
		}

		// Load to check dimensions
		if (UTexture2D* Tex = Cast<UTexture2D>(TexAsset.GetAsset()))
		{
			const int32 MaxDim = FMath::Max(Tex->GetSizeX(), Tex->GetSizeY());
			if (MaxDim > MaxRecommendedSize)
			{
				UE_LOG(LogTemp, Warning, TEXT("  OVERSIZED: %s (%dx%d)"),
					*TexAsset.PackageName.ToString(), Tex->GetSizeX(), Tex->GetSizeY());
				OversizedCount++;
				WarningCount++;
			}
		}
	}

	UE_LOG(LogTemp, Display, TEXT("  Found %d oversized textures (>%d)"), OversizedCount, MaxRecommendedSize);
}

void UContentValidationCommandlet::ReportSummary()
{
	UE_LOG(LogTemp, Display, TEXT("=== Validation Complete ==="));
	UE_LOG(LogTemp, Display, TEXT("  Errors:   %d"), ErrorCount);
	UE_LOG(LogTemp, Display, TEXT("  Warnings: %d"), WarningCount);

	if (ErrorCount == 0 && WarningCount == 0)
	{
		UE_LOG(LogTemp, Display, TEXT("  Content is CLEAN — ready for packaging."));
	}
}
