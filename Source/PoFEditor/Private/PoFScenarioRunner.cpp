// Copyright PoF. All Rights Reserved.

#include "PoFScenarioRunner.h"

#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "FileHelpers.h"
#include "GameFramework/PlayerController.h"
#include "HighResScreenshot.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "RenderingThread.h"
#include "UnrealClient.h"
#include "Engine/GameViewportClient.h"

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

bool UPoFScenarioRunner::RunScenario(const FString& MapPath, const TArray<FPoFTimedInput>& Inputs,
    float TotalSeconds, const FString& ScreenshotPath)
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

    // Capture a screenshot of the live PIE game view — the deterministic T4 moment
    // (character posed, PIE active). Requested on the viewport, then we tick + flush
    // so the rendering thread writes the PNG before we return.
    if (!ScreenshotPath.IsEmpty())
    {
        if (UGameViewportClient* VP = World->GetGameViewport())
        {
            GScreenshotResolutionX = 512;
            GScreenshotResolutionY = 512;
            FScreenshotRequest::RequestScreenshot(ScreenshotPath, /*bShowUI*/ false, /*bAddFilenameSuffix*/ false);
            VP->Viewport->TakeHighResScreenShot();
            for (int32 i = 0; i < 12; ++i)
            {
                World->Tick(LEVELTICK_All, 1.f / 60.f);
                FlushRenderingCommands();
            }
        }
    }

    // PIE is intentionally left running so EvaluatePose(mode=pie) can observe it too.
    return true;
}

void UPoFScenarioRunner::StopScenario()
{
    if (GEditor)
    {
        GEditor->RequestEndPlayMap();
    }
}
