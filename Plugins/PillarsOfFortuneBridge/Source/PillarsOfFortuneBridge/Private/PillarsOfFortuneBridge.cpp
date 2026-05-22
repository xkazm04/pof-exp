#include "PillarsOfFortuneBridge.h"

DEFINE_LOG_CATEGORY(LogPofBridge);

#define LOCTEXT_NAMESPACE "FPillarsOfFortuneBridgeModule"

void FPillarsOfFortuneBridgeModule::StartupModule()
{
    UE_LOG(LogPofBridge, Log, TEXT("PillarsOfFortuneBridge runtime module loaded"));
}

void FPillarsOfFortuneBridgeModule::ShutdownModule()
{
    UE_LOG(LogPofBridge, Log, TEXT("PillarsOfFortuneBridge runtime module unloaded"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPillarsOfFortuneBridgeModule, PillarsOfFortuneBridge)
