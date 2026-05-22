#include "PillarsOfFortuneBridge.h"
#include "PofBridgeSettings.h"

/**
 * Console commands for the PoF Bridge plugin.
 *
 * Available commands:
 *   PofBridge.Status    - Print current bridge status
 *   PofBridge.Manifest  - Force manifest regeneration
 */

static FAutoConsoleCommand PofBridgeStatusCmd(
    TEXT("PofBridge.Status"),
    TEXT("Print the current PoF Bridge plugin status"),
    FConsoleCommandDelegate::CreateLambda([]()
    {
        const UPofBridgeSettings* Settings = UPofBridgeSettings::Get();
        UE_LOG(LogPofBridge, Log, TEXT("=== PoF Bridge Status ==="));
        UE_LOG(LogPofBridge, Log, TEXT("  Server Port: %d"), Settings->ServerPort);
        UE_LOG(LogPofBridge, Log, TEXT("  Auth Token: %s"), Settings->AuthToken.IsEmpty() ? TEXT("(none)") : TEXT("(set)"));
        UE_LOG(LogPofBridge, Log, TEXT("  Allowed Origins: %s"), *Settings->AllowedOrigins);
        UE_LOG(LogPofBridge, Log, TEXT("  Auto-Regen Manifest: %s"), Settings->bAutoRegenerateManifest ? TEXT("Yes") : TEXT("No"));
        UE_LOG(LogPofBridge, Log, TEXT("  Regen Interval: %.1fs"), Settings->ManifestRegenerationInterval);
    })
);

static FAutoConsoleCommand PofBridgeManifestCmd(
    TEXT("PofBridge.Manifest"),
    TEXT("Force regeneration of the asset manifest"),
    FConsoleCommandDelegate::CreateLambda([]()
    {
        UE_LOG(LogPofBridge, Log, TEXT("Manual manifest regeneration requested (use module API for full rebuild)"));
    })
);
