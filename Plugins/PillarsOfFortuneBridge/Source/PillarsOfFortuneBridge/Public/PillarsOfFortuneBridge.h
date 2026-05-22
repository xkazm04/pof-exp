#pragma once

#include "Modules/ModuleManager.h"

PILLARSOFFORTUNEBRIDGE_API DECLARE_LOG_CATEGORY_EXTERN(LogPofBridge, Log, All);

class FPillarsOfFortuneBridgeModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
