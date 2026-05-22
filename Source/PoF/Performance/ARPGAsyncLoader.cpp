#include "Performance/ARPGAsyncLoader.h"

DEFINE_LOG_CATEGORY_STATIC(LogARPGAsync, Log, All);

void UARPGAsyncLoader::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogARPGAsync, Display, TEXT("AsyncLoader subsystem initialized"));
}

void UARPGAsyncLoader::Deinitialize()
{
	// Release all streamable handles
	ActiveHandles.Empty();
	Super::Deinitialize();
}

void UARPGAsyncLoader::RequestAsyncLoad(const FSoftObjectPath& AssetPath, FOnAssetLoaded OnLoaded, int32 Priority)
{
	if (!AssetPath.IsValid())
	{
		UE_LOG(LogARPGAsync, Warning, TEXT("RequestAsyncLoad: invalid asset path"));
		OnLoaded.ExecuteIfBound(nullptr);
		return;
	}

	// Already loaded?
	if (UObject* Existing = AssetPath.ResolveObject())
	{
		OnLoaded.ExecuteIfBound(Existing);
		return;
	}

	PendingLoads++;

	TSharedPtr<FStreamableHandle> Handle = StreamableManager.RequestAsyncLoad(
		AssetPath,
		FStreamableDelegate::CreateLambda([this, AssetPath, OnLoaded]()
		{
			PendingLoads = FMath::Max(0, PendingLoads - 1);

			UObject* Loaded = AssetPath.ResolveObject();
			if (Loaded)
			{
				UE_LOG(LogARPGAsync, Verbose, TEXT("Async loaded: %s"), *AssetPath.ToString());
			}
			else
			{
				UE_LOG(LogARPGAsync, Warning, TEXT("Async load failed: %s"), *AssetPath.ToString());
			}

			OnLoaded.ExecuteIfBound(Loaded);
		}),
		FStreamableManager::AsyncLoadHighPriority - Priority // Higher priority = lower internal value
	);

	if (Handle.IsValid())
	{
		ActiveHandles.Add(AssetPath, Handle);
	}
}

void UARPGAsyncLoader::RequestBatchLoad(const TArray<FSoftObjectPath>& AssetPaths, FOnBatchLoaded OnAllLoaded, int32 Priority)
{
	if (AssetPaths.IsEmpty())
	{
		TArray<UObject*> Empty;
		OnAllLoaded.ExecuteIfBound(Empty);
		return;
	}

	// Filter out invalid paths
	TArray<FSoftObjectPath> ValidPaths;
	for (const FSoftObjectPath& Path : AssetPaths)
	{
		if (Path.IsValid())
		{
			ValidPaths.Add(Path);
		}
	}

	if (ValidPaths.IsEmpty())
	{
		TArray<UObject*> Empty;
		OnAllLoaded.ExecuteIfBound(Empty);
		return;
	}

	PendingLoads += ValidPaths.Num();

	TSharedPtr<FStreamableHandle> Handle = StreamableManager.RequestAsyncLoad(
		ValidPaths,
		FStreamableDelegate::CreateLambda([this, ValidPaths, OnAllLoaded]()
		{
			PendingLoads = FMath::Max(0, PendingLoads - ValidPaths.Num());

			TArray<UObject*> LoadedAssets;
			LoadedAssets.Reserve(ValidPaths.Num());

			for (const FSoftObjectPath& Path : ValidPaths)
			{
				UObject* Obj = Path.ResolveObject();
				if (Obj)
				{
					LoadedAssets.Add(Obj);
				}
			}

			UE_LOG(LogARPGAsync, Display, TEXT("Batch loaded %d/%d assets"), LoadedAssets.Num(), ValidPaths.Num());
			OnAllLoaded.ExecuteIfBound(LoadedAssets);
		}),
		FStreamableManager::AsyncLoadHighPriority - Priority
	);

	if (Handle.IsValid())
	{
		// Store under the first path as batch key
		ActiveHandles.Add(ValidPaths[0], Handle);
	}
}

UObject* UARPGAsyncLoader::LoadAssetBlocking(const FSoftObjectPath& AssetPath)
{
	if (!AssetPath.IsValid())
	{
		return nullptr;
	}

	// Try to resolve without loading
	if (UObject* Existing = AssetPath.ResolveObject())
	{
		return Existing;
	}

	// Synchronous load (use sparingly — causes hitches)
	TSharedPtr<FStreamableHandle> Handle = StreamableManager.RequestSyncLoad(AssetPath);
	if (Handle.IsValid())
	{
		ActiveHandles.Add(AssetPath, Handle);
		return AssetPath.ResolveObject();
	}

	return nullptr;
}

bool UARPGAsyncLoader::IsAssetLoaded(const FSoftObjectPath& AssetPath) const
{
	if (!AssetPath.IsValid())
	{
		return false;
	}

	return AssetPath.ResolveObject() != nullptr;
}

void UARPGAsyncLoader::UnloadAsset(const FSoftObjectPath& AssetPath)
{
	ActiveHandles.Remove(AssetPath);
}
