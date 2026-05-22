#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PofBridgeSettings.generated.h"

UCLASS(config = EditorPerProjectUserSettings, defaultconfig, meta = (DisplayName = "Pillars of Fortune Bridge"))
class PILLARSOFFORTUNEBRIDGE_API UPofBridgeSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPofBridgeSettings();

    /** HTTP port the PoF Bridge plugin listens on. */
    UPROPERTY(config, EditAnywhere, Category = "Server", meta = (ClampMin = "1024", ClampMax = "65535"))
    int32 ServerPort;

    /** Optional shared secret for authentication. Must match the PoF web app setting. */
    UPROPERTY(config, EditAnywhere, Category = "Server")
    FString AuthToken;

    /** Comma-separated list of allowed CORS origins (e.g. http://localhost:3000). */
    UPROPERTY(config, EditAnywhere, Category = "Server")
    FString AllowedOrigins;

    /** Whether to automatically regenerate the asset manifest when assets change. */
    UPROPERTY(config, EditAnywhere, Category = "Manifest")
    bool bAutoRegenerateManifest;

    /** Seconds between automatic manifest regeneration checks. */
    UPROPERTY(config, EditAnywhere, Category = "Manifest", meta = (ClampMin = "5.0", ClampMax = "300.0", EditCondition = "bAutoRegenerateManifest"))
    float ManifestRegenerationInterval;

    /** Get the settings instance. */
    static const UPofBridgeSettings* Get()
    {
        return GetDefault<UPofBridgeSettings>();
    }

    virtual FName GetCategoryName() const override { return FName(TEXT("Plugins")); }
};
