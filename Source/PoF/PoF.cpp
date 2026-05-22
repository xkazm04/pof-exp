#include "PoF.h"
#include "Modules/ModuleManager.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebugger.h"
#include "AI/Debug/GameplayDebuggerCategory_ARPG.h"
#endif

class FPoFModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();

#if WITH_GAMEPLAY_DEBUGGER
		IGameplayDebugger& GDModule = IGameplayDebugger::Get();
		GDModule.RegisterCategory("ARPG_AI",
			IGameplayDebugger::FOnGetCategory::CreateStatic(&FGameplayDebuggerCategory_ARPG::MakeInstance),
			EGameplayDebuggerCategoryState::EnabledInGameAndSimulate);
		GDModule.NotifyCategoriesChanged();
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_GAMEPLAY_DEBUGGER
		if (IGameplayDebugger::IsAvailable())
		{
			IGameplayDebugger& GDModule = IGameplayDebugger::Get();
			GDModule.UnregisterCategory("ARPG_AI");
			GDModule.NotifyCategoriesChanged();
		}
#endif

		FDefaultGameModuleImpl::ShutdownModule();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FPoFModule, PoF, "PoF");
