#pragma once

#include "Modules/ModuleManager.h"

class UPofAssetManifest;
class UPofTestRunner;
class UPofSnapshotCapture;
class FPofHttpServer;
class UPofBlueprintIntrospector;
class UPofLiveCodingBridge;
class UPofWebSocketLiveState;
class UPofPythonRunner;

class FPillarsOfFortuneBridgeEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    UPofAssetManifest* GetManifest() const { return AssetManifest; }
    UPofTestRunner* GetTestRunner() const { return TestRunner; }
    UPofSnapshotCapture* GetSnapshotCapture() const { return SnapshotCapture; }
    UPofBlueprintIntrospector* GetIntrospector() const { return Introspector; }
    UPofLiveCodingBridge* GetLiveCoding() const { return LiveCoding; }
    UPofWebSocketLiveState* GetWebSocketServer() const { return WebSocketServer; }
    UPofPythonRunner* GetPythonRunner() const { return PythonRunner; }

    static FPillarsOfFortuneBridgeEditorModule& Get()
    {
        return FModuleManager::GetModuleChecked<FPillarsOfFortuneBridgeEditorModule>("PillarsOfFortuneBridgeEditor");
    }

private:
    TSharedPtr<FPofHttpServer> HttpServer;
    UPofAssetManifest* AssetManifest = nullptr;
    UPofTestRunner* TestRunner = nullptr;
    UPofSnapshotCapture* SnapshotCapture = nullptr;
    UPofBlueprintIntrospector* Introspector = nullptr;
    UPofLiveCodingBridge* LiveCoding = nullptr;
    UPofWebSocketLiveState* WebSocketServer = nullptr;
    UPofPythonRunner* PythonRunner = nullptr;
};
