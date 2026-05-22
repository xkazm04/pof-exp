#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/StreamableManager.h"
#include "ARPGAsyncLoader.generated.h"

/**
 * Async asset loading subsystem using FStreamableManager.
 *
 * Provides a central interface for loading soft references (TSoftObjectPtr)
 * without hitching the game thread. Supports priority loading, batch requests,
 * and callback notification.
 *
 * Usage:
 *   auto* Loader = GetGameInstance()->GetSubsystem<UARPGAsyncLoader>();
 *   TSoftObjectPtr<UStaticMesh> MeshRef = ...;
 *   Loader->RequestAsyncLoad(MeshRef.ToSoftObjectPath(), FOnAssetLoaded::CreateLambda([](UObject* Loaded){ ... }));
 */

DECLARE_DELEGATE_OneParam(FOnAssetLoaded, UObject* /*LoadedAsset*/);
DECLARE_DELEGATE_OneParam(FOnBatchLoaded, const TArray<UObject*>& /*LoadedAssets*/);

UCLASS()
class POF_API UARPGAsyncLoader : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Request a single asset to be loaded asynchronously.
	 * @param AssetPath  Soft object path to load.
	 * @param OnLoaded   Callback fired when the asset is loaded (game thread).
	 * @param Priority   Loading priority (higher = loaded sooner). Default 0.
	 */
	void RequestAsyncLoad(const FSoftObjectPath& AssetPath, FOnAssetLoaded OnLoaded, int32 Priority = 0);

	/**
	 * Request a batch of assets to be loaded asynchronously.
	 * @param AssetPaths  Array of soft object paths.
	 * @param OnAllLoaded Callback fired when ALL assets in the batch are loaded.
	 * @param Priority    Loading priority.
	 */
	void RequestBatchLoad(const TArray<FSoftObjectPath>& AssetPaths, FOnBatchLoaded OnAllLoaded, int32 Priority = 0);

	/**
	 * Blueprint-friendly async load of a single asset by soft path.
	 * @param AssetPath  Soft object path string.
	 * @return The loaded asset (null if not yet loaded — use with latent actions).
	 */
	UFUNCTION(BlueprintCallable, Category = "Performance|AsyncLoad")
	UObject* LoadAssetBlocking(const FSoftObjectPath& AssetPath);

	/**
	 * Check if an asset is already loaded (in memory).
	 */
	UFUNCTION(BlueprintPure, Category = "Performance|AsyncLoad")
	bool IsAssetLoaded(const FSoftObjectPath& AssetPath) const;

	/**
	 * Unload a previously loaded asset handle (releases the streamable handle).
	 */
	UFUNCTION(BlueprintCallable, Category = "Performance|AsyncLoad")
	void UnloadAsset(const FSoftObjectPath& AssetPath);

	/** Get the number of assets currently being loaded. */
	UFUNCTION(BlueprintPure, Category = "Performance|AsyncLoad")
	int32 GetPendingLoadCount() const { return PendingLoads; }

private:
	FStreamableManager StreamableManager;
	int32 PendingLoads = 0;

	/** Active handles to keep loaded assets in memory. */
	TMap<FSoftObjectPath, TSharedPtr<FStreamableHandle>> ActiveHandles;
};
