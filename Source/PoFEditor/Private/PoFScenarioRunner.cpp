// Copyright PoF. All Rights Reserved.

#include "PoFScenarioRunner.h"

#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "FileHelpers.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"

static UWorld* FindPieWorld()
{
    if (!GEngine) return nullptr;
    for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
    {
        if (Ctx.WorldType == EWorldType::PIE && Ctx.World())
        {
            return Ctx.World();
        }
    }
    return nullptr;
}

bool UPoFScenarioRunner::RunScenario(const FString& MapPath, const TArray<FPoFTimedInput>& Inputs, float TotalSeconds)
{
    if (!GEditor)
    {
        return false;
    }

    FEditorFileUtils::LoadMap(MapPath, /*bLoadAsTemplate*/ false, /*bShowProgress*/ false);

    FRequestPlaySessionParams Params;
    GEditor->RequestPlaySession(Params);
    GEditor->StartQueuedPlaySessionRequest();

    // Wait for the PIE world + a possessed pawn (tick the editor while we wait).
    UWorld* World = nullptr;
    APlayerController* PC = nullptr;
    const double Start = FPlatformTime::Seconds();
    while (FPlatformTime::Seconds() - Start < 5.0)
    {
        World = FindPieWorld();
        if (World)
        {
            PC = World->GetFirstPlayerController();
            if (PC && PC->GetPawn())
            {
                break;
            }
            World->Tick(LEVELTICK_All, 1.f / 60.f);
        }
        FPlatformProcess::Sleep(0.02f);
    }
    if (!World || !PC || !PC->GetPawn())
    {
        return false;
    }

    UEnhancedInputLocalPlayerSubsystem* Subsys =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());
    if (!Subsys)
    {
        return false;
    }

    const int32 TotalFrames = FMath::CeilToInt(TotalSeconds * 60.f);
    for (int32 Frame = 0; Frame < TotalFrames; ++Frame)
    {
        const float T = Frame / 60.f;
        for (const FPoFTimedInput& In : Inputs)
        {
            if (T >= In.StartSeconds && T < In.StartSeconds + In.DurationSeconds)
            {
                if (UInputAction* IA = LoadObject<UInputAction>(nullptr, *In.ActionPath))
                {
                    Subsys->InjectInputForAction(IA, FInputActionValue(In.Value), {}, {});
                }
            }
        }
        World->Tick(LEVELTICK_All, 1.f / 60.f);
    }

    // PIE is intentionally left running so EvaluatePose(mode=pie) + CaptureFrame can observe it.
    return true;
}

void UPoFScenarioRunner::StopScenario()
{
    if (GEditor)
    {
        GEditor->RequestEndPlayMap();
    }
}
