#include "PofBridgeSettings.h"

UPofBridgeSettings::UPofBridgeSettings()
    : ServerPort(30040)
    , AuthToken(TEXT(""))
    , AllowedOrigins(TEXT("http://localhost:3000"))
    , bAutoRegenerateManifest(true)
    , ManifestRegenerationInterval(30.0f)
{
}
