#include "PillarsOfFortuneBridgeEditor.h"
#include "PillarsOfFortuneBridge.h"
#include "PofBridgeSettings.h"
#include "PofAssetManifest.h"
#include "PofTestRunner.h"
#include "PofSnapshotCapture.h"
#include "PofHttpServer.h"
#include "PofBlueprintIntrospector.h"
#include "PofLiveCodingBridge.h"
#include "PofWebSocketServer.h"
#include "PofPythonRunner.h"

#define LOCTEXT_NAMESPACE "FPillarsOfFortuneBridgeEditorModule"

void FPillarsOfFortuneBridgeEditorModule::StartupModule()
{
    UE_LOG(LogPofBridge, Log, TEXT("PillarsOfFortuneBridgeEditor starting up..."));

    const UPofBridgeSettings* Settings = UPofBridgeSettings::Get();

    // Create subsystem UObjects (prevent GC with AddToRoot)
    AssetManifest = NewObject<UPofAssetManifest>();
    AssetManifest->AddToRoot();
    AssetManifest->Initialize();

    TestRunner = NewObject<UPofTestRunner>();
    TestRunner->AddToRoot();

    SnapshotCapture = NewObject<UPofSnapshotCapture>();
    SnapshotCapture->AddToRoot();

    Introspector = NewObject<UPofBlueprintIntrospector>();
    Introspector->AddToRoot();

    LiveCoding = NewObject<UPofLiveCodingBridge>();
    LiveCoding->AddToRoot();

    PythonRunner = NewObject<UPofPythonRunner>();
    PythonRunner->AddToRoot();

    // Start HTTP server
    HttpServer = MakeShared<FPofHttpServer>();
    HttpServer->Start(Settings->ServerPort, Settings->AuthToken, Settings->AllowedOrigins);

    // Start WebSocket live state server (HTTP port + 1)
    WebSocketServer = NewObject<UPofWebSocketLiveState>();
    WebSocketServer->AddToRoot();
    WebSocketServer->Start(Settings->ServerPort + 1, Settings->AllowedOrigins);

    UE_LOG(LogPofBridge, Log, TEXT("PillarsOfFortuneBridgeEditor ready (HTTP %d, WS %d)"), Settings->ServerPort, Settings->ServerPort + 1);
}

void FPillarsOfFortuneBridgeEditorModule::ShutdownModule()
{
    UE_LOG(LogPofBridge, Log, TEXT("PillarsOfFortuneBridgeEditor shutting down..."));

    if (HttpServer.IsValid())
    {
        HttpServer->Stop();
        HttpServer.Reset();
    }

    if (AssetManifest)
    {
        AssetManifest->Shutdown();
        AssetManifest->RemoveFromRoot();
        AssetManifest = nullptr;
    }

    if (TestRunner)
    {
        TestRunner->RemoveFromRoot();
        TestRunner = nullptr;
    }

    if (SnapshotCapture)
    {
        SnapshotCapture->RemoveFromRoot();
        SnapshotCapture = nullptr;
    }

    if (Introspector)
    {
        Introspector->RemoveFromRoot();
        Introspector = nullptr;
    }

    if (LiveCoding)
    {
        LiveCoding->RemoveFromRoot();
        LiveCoding = nullptr;
    }

    if (PythonRunner)
    {
        PythonRunner->RemoveFromRoot();
        PythonRunner = nullptr;
    }

    if (WebSocketServer)
    {
        WebSocketServer->Stop();
        WebSocketServer->RemoveFromRoot();
        WebSocketServer = nullptr;
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPillarsOfFortuneBridgeEditorModule, PillarsOfFortuneBridgeEditor)
