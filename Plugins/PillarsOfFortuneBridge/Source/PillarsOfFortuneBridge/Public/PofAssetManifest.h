#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PofAssetManifest.generated.h"

USTRUCT()
struct PILLARSOFFORTUNEBRIDGE_API FPofComponentEntry
{
    GENERATED_BODY()

    UPROPERTY() FString ComponentName;
    UPROPERTY() FString ComponentClass;
    UPROPERTY() bool IsSceneComponent = false;
    UPROPERTY() FString AttachParent;
    UPROPERTY() TMap<FString, FString> DefaultValues;
};

USTRUCT()
struct PILLARSOFFORTUNEBRIDGE_API FPofFunctionOverride
{
    GENERATED_BODY()

    UPROPERTY() FString FunctionName;
    UPROPERTY() FString DeclaringClass;
    UPROPERTY() bool IsEvent = false;
    UPROPERTY() bool IsBlueprintCallable = false;
};

USTRUCT()
struct PILLARSOFFORTUNEBRIDGE_API FPofVariableEntry
{
    GENERATED_BODY()

    UPROPERTY() FString Name;
    UPROPERTY() FString Type;
    UPROPERTY() FString SubType;
    UPROPERTY() FString DefaultValue;
    UPROPERTY() bool IsReplicated = false;
    UPROPERTY() FString Category;
};

USTRUCT()
struct PILLARSOFFORTUNEBRIDGE_API FPofMaterialParameter
{
    GENERATED_BODY()

    UPROPERTY() FString Name;
    UPROPERTY() FString Type;
    UPROPERTY() FString DefaultTexture;
    UPROPERTY() float DefaultScalar = 0.0f;
    UPROPERTY() FLinearColor DefaultVector = FLinearColor::White;
    UPROPERTY() float Min = 0.0f;
    UPROPERTY() float Max = 1.0f;
};

USTRUCT()
struct PILLARSOFFORTUNEBRIDGE_API FPofAnimNotify
{
    GENERATED_BODY()

    UPROPERTY() FString Name;
    UPROPERTY() float Time = 0.0f;
    UPROPERTY() FString NotifyClass;
};

USTRUCT()
struct PILLARSOFFORTUNEBRIDGE_API FPofStateMachineTransition
{
    GENERATED_BODY()

    UPROPERTY() FString From;
    UPROPERTY() FString To;
    UPROPERTY() FString Condition;
};

USTRUCT()
struct PILLARSOFFORTUNEBRIDGE_API FPofStateMachine
{
    GENERATED_BODY()

    UPROPERTY() FString Name;
    UPROPERTY() TArray<FString> States;
    UPROPERTY() TArray<FPofStateMachineTransition> Transitions;
};

USTRUCT()
struct PILLARSOFFORTUNEBRIDGE_API FPofAssetEntry
{
    GENERATED_BODY()

    UPROPERTY() FString AssetPath;
    UPROPERTY() FString AssetClass;
    UPROPERTY() FString ParentClass;
    UPROPERTY() FString SkeletonPath;
    UPROPERTY() int32 ContentHash = 0;
    UPROPERTY() TArray<FString> Tags;
    UPROPERTY() TArray<FString> CrossRefs;
};

USTRUCT()
struct PILLARSOFFORTUNEBRIDGE_API FPofBlueprintEntry
{
    GENERATED_BODY()

    UPROPERTY() FString Path;
    UPROPERTY() FString ParentCppClass;
    UPROPERTY() FString ParentCppModule;
    UPROPERTY() FString ParentBlueprintClass;
    UPROPERTY() TArray<FPofFunctionOverride> OverriddenFunctions;
    UPROPERTY() TArray<FPofComponentEntry> AddedComponents;
    UPROPERTY() TArray<FPofVariableEntry> Variables;
    UPROPERTY() TArray<FString> EventGraphEntryPoints;
    UPROPERTY() TArray<FString> Interfaces;
    UPROPERTY() TArray<FString> CrossReferences;
    UPROPERTY() FString ContentHash;
};

USTRUCT()
struct PILLARSOFFORTUNEBRIDGE_API FPofMaterialEntry
{
    GENERATED_BODY()

    UPROPERTY() FString Path;
    UPROPERTY() FString ParentMaterial;
    UPROPERTY() FString Domain;
    UPROPERTY() FString BlendMode;
    UPROPERTY() FString ShadingModel;
    UPROPERTY() TArray<FPofMaterialParameter> Parameters;
    UPROPERTY() TArray<FString> MaterialInstances;
    UPROPERTY() TArray<FString> TextureReferences;
    UPROPERTY() TArray<FString> CrossReferences;
    UPROPERTY() FString ContentHash;
};

USTRUCT()
struct PILLARSOFFORTUNEBRIDGE_API FPofAnimAssetEntry
{
    GENERATED_BODY()

    UPROPERTY() FString Path;
    UPROPERTY() FString AssetType;
    UPROPERTY() FString SkeletonPath;
    UPROPERTY() float Duration = 0.0f;
    UPROPERTY() TArray<FPofAnimNotify> Notifies;
    UPROPERTY() TArray<FString> Sections;
    UPROPERTY() TArray<FPofStateMachine> StateMachines;
    UPROPERTY() TArray<FString> CrossReferences;
    UPROPERTY() FString ContentHash;
};

USTRUCT()
struct PILLARSOFFORTUNEBRIDGE_API FPofDataTableEntry
{
    GENERATED_BODY()

    UPROPERTY() FString Path;
    UPROPERTY() FString RowStruct;
    UPROPERTY() FString RowStructModule;
    UPROPERTY() int32 RowCount = 0;
    UPROPERTY() TArray<FString> ColumnNames;
    UPROPERTY() TArray<FString> CrossReferences;
    UPROPERTY() FString ContentHash;
};

USTRUCT()
struct PILLARSOFFORTUNEBRIDGE_API FPofOtherAssetEntry
{
    GENERATED_BODY()

    UPROPERTY() FString Path;
    UPROPERTY() FString AssetClass;
    UPROPERTY() TArray<FString> CrossReferences;
    UPROPERTY() FString ContentHash;
};

USTRUCT()
struct PILLARSOFFORTUNEBRIDGE_API FPofManifest
{
    GENERATED_BODY()

    UPROPERTY() int32 Version = 2;
    UPROPERTY() FString GeneratedAt;
    UPROPERTY() FString ProjectName;
    UPROPERTY() FString EngineVersion;
    UPROPERTY() int32 AssetCount = 0;
    UPROPERTY() FString ChecksumSha256;
    UPROPERTY() TArray<FPofBlueprintEntry> Blueprints;
    UPROPERTY() TArray<FPofMaterialEntry> Materials;
    UPROPERTY() TArray<FPofAnimAssetEntry> AnimAssets;
    UPROPERTY() TArray<FPofDataTableEntry> DataTables;
    UPROPERTY() TArray<FPofOtherAssetEntry> OtherAssets;
};

UCLASS()
class PILLARSOFFORTUNEBRIDGE_API UPofAssetManifest : public UObject
{
    GENERATED_BODY()

public:
    void Initialize();
    void Shutdown();

    const FPofManifest& GetManifest() const { return CachedManifest; }
    FString GetManifestJson(bool bChecksumOnly = false) const;

    bool IsReady() const { return bIsReady; }

private:
    void OnInitialScanComplete();
    void OnAssetAdded(const FAssetData& AssetData);
    void OnAssetRemoved(const FAssetData& AssetData);
    void OnAssetRenamed(const FAssetData& AssetData, const FString& OldPath);
    void OnAssetUpdated(const FAssetData& AssetData);

    void RebuildManifest();
    void FlushManifestToDisk();
    uint32 ComputeAssetHash(const FAssetData& AssetData) const;
    FString ComputeManifestChecksum() const;
    FString GetPofDirectory() const;

    FPofManifest CachedManifest;
    TMap<FSoftObjectPath, FPofAssetEntry> ManifestEntries;
    bool bManifestDirty = false;
    bool bIsReady = false;
    FTimerHandle FlushTimerHandle;
};
