#include "PofAssetManifest.h"
#include "PillarsOfFortuneBridge.h"
#include "PofBridgeSettings.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimationAsset.h"
#include "Engine/DataTable.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

void UPofAssetManifest::Initialize()
{
    FAssetRegistryModule& RegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry& AssetRegistry = RegistryModule.Get();

    if (AssetRegistry.IsLoadingAssets())
    {
        AssetRegistry.OnFilesLoaded().AddUObject(this, &UPofAssetManifest::OnInitialScanComplete);
    }
    else
    {
        OnInitialScanComplete();
    }

    AssetRegistry.OnAssetAdded().AddUObject(this, &UPofAssetManifest::OnAssetAdded);
    AssetRegistry.OnAssetRemoved().AddUObject(this, &UPofAssetManifest::OnAssetRemoved);
    AssetRegistry.OnAssetRenamed().AddUObject(this, &UPofAssetManifest::OnAssetRenamed);
    AssetRegistry.OnAssetUpdatedOnDisk().AddUObject(this, &UPofAssetManifest::OnAssetUpdated);

    UE_LOG(LogPofBridge, Log, TEXT("PofAssetManifest initialized, waiting for asset registry..."));
}

void UPofAssetManifest::Shutdown()
{
    FAssetRegistryModule* RegistryModule = FModuleManager::GetModulePtr<FAssetRegistryModule>(TEXT("AssetRegistry"));
    if (RegistryModule)
    {
        IAssetRegistry& AssetRegistry = RegistryModule->Get();
        AssetRegistry.OnAssetAdded().RemoveAll(this);
        AssetRegistry.OnAssetRemoved().RemoveAll(this);
        AssetRegistry.OnAssetRenamed().RemoveAll(this);
        AssetRegistry.OnAssetUpdatedOnDisk().RemoveAll(this);
    }

    if (bManifestDirty)
    {
        FlushManifestToDisk();
    }
}

void UPofAssetManifest::OnInitialScanComplete()
{
    UE_LOG(LogPofBridge, Log, TEXT("Asset registry scan complete, building initial manifest..."));
    RebuildManifest();
    bIsReady = true;
}

void UPofAssetManifest::OnAssetAdded(const FAssetData& AssetData)
{
    if (!AssetData.PackagePath.ToString().StartsWith(TEXT("/Game")))
    {
        return;
    }
    bManifestDirty = true;
}

void UPofAssetManifest::OnAssetRemoved(const FAssetData& AssetData)
{
    bManifestDirty = true;
}

void UPofAssetManifest::OnAssetRenamed(const FAssetData& AssetData, const FString& OldPath)
{
    bManifestDirty = true;
}

void UPofAssetManifest::OnAssetUpdated(const FAssetData& AssetData)
{
    if (!AssetData.PackagePath.ToString().StartsWith(TEXT("/Game")))
    {
        return;
    }
    bManifestDirty = true;
}

void UPofAssetManifest::RebuildManifest()
{
    FAssetRegistryModule& RegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry& AssetRegistry = RegistryModule.Get();

    TArray<FAssetData> AllAssets;
    AssetRegistry.GetAssetsByPath(TEXT("/Game"), AllAssets, true);

    CachedManifest = FPofManifest();
    CachedManifest.Version = 2;
    CachedManifest.GeneratedAt = FDateTime::UtcNow().ToIso8601();
    CachedManifest.ProjectName = FApp::GetProjectName();
    CachedManifest.EngineVersion = FEngineVersion::Current().ToString();
    CachedManifest.AssetCount = AllAssets.Num();

    for (const FAssetData& Asset : AllAssets)
    {
        FString AssetClassName = Asset.AssetClassPath.GetAssetName().ToString();
        FString AssetPath = Asset.GetSoftObjectPath().ToString();

        if (AssetClassName == TEXT("Blueprint") || AssetClassName == TEXT("WidgetBlueprint"))
        {
            FPofBlueprintEntry Entry;
            Entry.Path = AssetPath;
            Entry.ContentHash = FString::Printf(TEXT("%08x"), ComputeAssetHash(Asset));
            CachedManifest.Blueprints.Add(Entry);
        }
        else if (AssetClassName == TEXT("Material") || AssetClassName == TEXT("MaterialInstanceConstant"))
        {
            FPofMaterialEntry Entry;
            Entry.Path = AssetPath;
            Entry.ContentHash = FString::Printf(TEXT("%08x"), ComputeAssetHash(Asset));
            CachedManifest.Materials.Add(Entry);
        }
        else if (AssetClassName == TEXT("AnimMontage") || AssetClassName == TEXT("AnimBlueprint")
            || AssetClassName == TEXT("AnimSequence") || AssetClassName == TEXT("BlendSpace"))
        {
            FPofAnimAssetEntry Entry;
            Entry.Path = AssetPath;
            Entry.AssetType = AssetClassName;
            Entry.ContentHash = FString::Printf(TEXT("%08x"), ComputeAssetHash(Asset));
            CachedManifest.AnimAssets.Add(Entry);
        }
        else if (AssetClassName == TEXT("DataTable"))
        {
            FPofDataTableEntry Entry;
            Entry.Path = AssetPath;
            Entry.ContentHash = FString::Printf(TEXT("%08x"), ComputeAssetHash(Asset));
            CachedManifest.DataTables.Add(Entry);
        }
        else
        {
            FPofOtherAssetEntry Entry;
            Entry.Path = AssetPath;
            Entry.AssetClass = AssetClassName;
            Entry.ContentHash = FString::Printf(TEXT("%08x"), ComputeAssetHash(Asset));
            CachedManifest.OtherAssets.Add(Entry);
        }
    }

    CachedManifest.ChecksumSha256 = ComputeManifestChecksum();
    bManifestDirty = false;

    UE_LOG(LogPofBridge, Log, TEXT("Manifest built: %d assets (%d blueprints, %d materials, %d anim, %d data tables, %d other)"),
        CachedManifest.AssetCount,
        CachedManifest.Blueprints.Num(),
        CachedManifest.Materials.Num(),
        CachedManifest.AnimAssets.Num(),
        CachedManifest.DataTables.Num(),
        CachedManifest.OtherAssets.Num());

    FlushManifestToDisk();
}

FString UPofAssetManifest::GetManifestJson(bool bChecksumOnly) const
{
    TSharedRef<FJsonObject> Root = MakeShareable(new FJsonObject());
    Root->SetNumberField(TEXT("version"), CachedManifest.Version);
    Root->SetStringField(TEXT("generatedAt"), CachedManifest.GeneratedAt);
    Root->SetStringField(TEXT("projectName"), CachedManifest.ProjectName);
    Root->SetStringField(TEXT("engineVersion"), CachedManifest.EngineVersion);
    Root->SetNumberField(TEXT("assetCount"), CachedManifest.AssetCount);
    Root->SetStringField(TEXT("checksumSha256"), CachedManifest.ChecksumSha256);

    if (!bChecksumOnly)
    {
        // Blueprints
        TArray<TSharedPtr<FJsonValue>> BlueprintArray;
        for (const FPofBlueprintEntry& BP : CachedManifest.Blueprints)
        {
            TSharedRef<FJsonObject> Obj = MakeShareable(new FJsonObject());
            Obj->SetStringField(TEXT("path"), BP.Path);
            Obj->SetStringField(TEXT("parentCppClass"), BP.ParentCppClass);
            Obj->SetStringField(TEXT("parentCppModule"), BP.ParentCppModule);
            Obj->SetStringField(TEXT("contentHash"), BP.ContentHash);

            TArray<TSharedPtr<FJsonValue>> CrossRefArr;
            for (const FString& Ref : BP.CrossReferences)
            {
                CrossRefArr.Add(MakeShareable(new FJsonValueString(Ref)));
            }
            Obj->SetArrayField(TEXT("crossReferences"), CrossRefArr);

            BlueprintArray.Add(MakeShareable(new FJsonValueObject(Obj)));
        }
        Root->SetArrayField(TEXT("blueprints"), BlueprintArray);

        // Materials
        TArray<TSharedPtr<FJsonValue>> MaterialArray;
        for (const FPofMaterialEntry& Mat : CachedManifest.Materials)
        {
            TSharedRef<FJsonObject> Obj = MakeShareable(new FJsonObject());
            Obj->SetStringField(TEXT("path"), Mat.Path);
            Obj->SetStringField(TEXT("domain"), Mat.Domain);
            Obj->SetStringField(TEXT("blendMode"), Mat.BlendMode);
            Obj->SetStringField(TEXT("shadingModel"), Mat.ShadingModel);
            Obj->SetStringField(TEXT("contentHash"), Mat.ContentHash);

            TArray<TSharedPtr<FJsonValue>> ParamArr;
            for (const FPofMaterialParameter& Param : Mat.Parameters)
            {
                TSharedRef<FJsonObject> PObj = MakeShareable(new FJsonObject());
                PObj->SetStringField(TEXT("name"), Param.Name);
                PObj->SetStringField(TEXT("type"), Param.Type);
                ParamArr.Add(MakeShareable(new FJsonValueObject(PObj)));
            }
            Obj->SetArrayField(TEXT("parameters"), ParamArr);

            TArray<TSharedPtr<FJsonValue>> CrossRefArr;
            for (const FString& Ref : Mat.CrossReferences)
            {
                CrossRefArr.Add(MakeShareable(new FJsonValueString(Ref)));
            }
            Obj->SetArrayField(TEXT("crossReferences"), CrossRefArr);

            MaterialArray.Add(MakeShareable(new FJsonValueObject(Obj)));
        }
        Root->SetArrayField(TEXT("materials"), MaterialArray);

        // Anim Assets
        TArray<TSharedPtr<FJsonValue>> AnimArray;
        for (const FPofAnimAssetEntry& Anim : CachedManifest.AnimAssets)
        {
            TSharedRef<FJsonObject> Obj = MakeShareable(new FJsonObject());
            Obj->SetStringField(TEXT("path"), Anim.Path);
            Obj->SetStringField(TEXT("assetType"), Anim.AssetType);
            Obj->SetStringField(TEXT("skeletonPath"), Anim.SkeletonPath);
            Obj->SetNumberField(TEXT("duration"), Anim.Duration);
            Obj->SetStringField(TEXT("contentHash"), Anim.ContentHash);

            TArray<TSharedPtr<FJsonValue>> SmArray;
            for (const FPofStateMachine& SM : Anim.StateMachines)
            {
                TSharedRef<FJsonObject> SmObj = MakeShareable(new FJsonObject());
                SmObj->SetStringField(TEXT("name"), SM.Name);

                TArray<TSharedPtr<FJsonValue>> StateArr;
                for (const FString& S : SM.States)
                {
                    StateArr.Add(MakeShareable(new FJsonValueString(S)));
                }
                SmObj->SetArrayField(TEXT("states"), StateArr);

                TArray<TSharedPtr<FJsonValue>> TransArr;
                for (const FPofStateMachineTransition& T : SM.Transitions)
                {
                    TSharedRef<FJsonObject> TObj = MakeShareable(new FJsonObject());
                    TObj->SetStringField(TEXT("from"), T.From);
                    TObj->SetStringField(TEXT("to"), T.To);
                    TObj->SetStringField(TEXT("condition"), T.Condition);
                    TransArr.Add(MakeShareable(new FJsonValueObject(TObj)));
                }
                SmObj->SetArrayField(TEXT("transitions"), TransArr);

                SmArray.Add(MakeShareable(new FJsonValueObject(SmObj)));
            }
            Obj->SetArrayField(TEXT("stateMachines"), SmArray);

            TArray<TSharedPtr<FJsonValue>> CrossRefArr;
            for (const FString& Ref : Anim.CrossReferences)
            {
                CrossRefArr.Add(MakeShareable(new FJsonValueString(Ref)));
            }
            Obj->SetArrayField(TEXT("crossReferences"), CrossRefArr);

            AnimArray.Add(MakeShareable(new FJsonValueObject(Obj)));
        }
        Root->SetArrayField(TEXT("animAssets"), AnimArray);

        // Data Tables
        TArray<TSharedPtr<FJsonValue>> DtArray;
        for (const FPofDataTableEntry& DT : CachedManifest.DataTables)
        {
            TSharedRef<FJsonObject> Obj = MakeShareable(new FJsonObject());
            Obj->SetStringField(TEXT("path"), DT.Path);
            Obj->SetStringField(TEXT("rowStruct"), DT.RowStruct);
            Obj->SetNumberField(TEXT("rowCount"), DT.RowCount);
            Obj->SetStringField(TEXT("contentHash"), DT.ContentHash);

            TArray<TSharedPtr<FJsonValue>> ColArr;
            for (const FString& Col : DT.ColumnNames)
            {
                ColArr.Add(MakeShareable(new FJsonValueString(Col)));
            }
            Obj->SetArrayField(TEXT("columnNames"), ColArr);

            DtArray.Add(MakeShareable(new FJsonValueObject(Obj)));
        }
        Root->SetArrayField(TEXT("dataTables"), DtArray);

        // Other
        TArray<TSharedPtr<FJsonValue>> OtherArray;
        for (const FPofOtherAssetEntry& Other : CachedManifest.OtherAssets)
        {
            TSharedRef<FJsonObject> Obj = MakeShareable(new FJsonObject());
            Obj->SetStringField(TEXT("path"), Other.Path);
            Obj->SetStringField(TEXT("assetClass"), Other.AssetClass);
            Obj->SetStringField(TEXT("contentHash"), Other.ContentHash);
            OtherArray.Add(MakeShareable(new FJsonValueObject(Obj)));
        }
        Root->SetArrayField(TEXT("otherAssets"), OtherArray);
    }

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Root, Writer);
    return Output;
}

void UPofAssetManifest::FlushManifestToDisk()
{
    FString PofDir = GetPofDirectory();
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.DirectoryExists(*PofDir))
    {
        PlatformFile.CreateDirectoryTree(*PofDir);
    }

    FString ManifestPath = FPaths::Combine(PofDir, TEXT("manifest.json"));
    FString Json = GetManifestJson(false);
    FFileHelper::SaveStringToFile(Json, *ManifestPath);

    UE_LOG(LogPofBridge, Verbose, TEXT("Manifest flushed to %s"), *ManifestPath);
}

uint32 UPofAssetManifest::ComputeAssetHash(const FAssetData& AssetData) const
{
    FString PackagePath = AssetData.PackageName.ToString();
    FString FilePath = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());

    FFileStatData StatData = FPlatformFileManager::Get().GetPlatformFile().GetStatData(*FilePath);
    if (StatData.bIsValid)
    {
        FString HashInput = FString::Printf(TEXT("%s_%lld_%lld"),
            *FilePath, StatData.FileSize, StatData.ModificationTime.GetTicks());
        return GetTypeHash(HashInput);
    }

    return GetTypeHash(PackagePath);
}

FString UPofAssetManifest::ComputeManifestChecksum() const
{
    FString AllHashes;
    for (const FPofBlueprintEntry& BP : CachedManifest.Blueprints)
    {
        AllHashes += BP.ContentHash;
    }
    for (const FPofMaterialEntry& Mat : CachedManifest.Materials)
    {
        AllHashes += Mat.ContentHash;
    }
    for (const FPofAnimAssetEntry& Anim : CachedManifest.AnimAssets)
    {
        AllHashes += Anim.ContentHash;
    }
    for (const FPofDataTableEntry& DT : CachedManifest.DataTables)
    {
        AllHashes += DT.ContentHash;
    }

    // Use simple hash — FMD5::HashAnsiString is reliable across all UE5 versions
    FMD5 Md5;
    Md5.Update(reinterpret_cast<const uint8*>(TCHAR_TO_ANSI(*AllHashes)), AllHashes.Len());
    uint8 Digest[16];
    Md5.Final(Digest);

    FString Result;
    for (int32 i = 0; i < 16; i++)
    {
        Result += FString::Printf(TEXT("%02x"), Digest[i]);
    }
    return Result;
}

FString UPofAssetManifest::GetPofDirectory() const
{
    return FPaths::Combine(FPaths::ProjectDir(), TEXT(".pof"));
}
