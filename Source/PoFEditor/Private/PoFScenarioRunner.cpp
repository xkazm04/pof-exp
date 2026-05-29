// Copyright PoF. All Rights Reserved.

#include "PoFScenarioRunner.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Engine/Engine.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "FileHelpers.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Misc/Paths.h"
#include "RenderingThread.h"

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

    // Capture via a SceneCapture2D -> render target -> PNG. This renders through the
    // pipeline independent of any editor/PIE viewport or swapchain (the high-res
    // viewport screenshot captured the empty editor perspective, not the game view).
    // The camera is placed behind + above the pawn, framing it.
    if (!ScreenshotPath.IsEmpty())
    {
        APawn* Pawn = PC->GetPawn();
        const FVector PawnLoc = Pawn->GetActorLocation();
        const FVector LookAt = PawnLoc + FVector(0, 0, 90.f);          // chest height
        const FVector CamLoc = PawnLoc + Pawn->GetActorForwardVector() * -320.f + FVector(0, 0, 170.f);
        const FRotator CamRot = (LookAt - CamLoc).Rotation();

        UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(World);
        RT->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
        RT->InitAutoFormat(512, 512);
        RT->UpdateResourceImmediate(true);

        ASceneCapture2D* Cap = World->SpawnActor<ASceneCapture2D>(CamLoc, CamRot);
        if (Cap)
        {
            USceneCaptureComponent2D* CapComp = Cap->GetCaptureComponent2D();
            CapComp->TextureTarget = RT;
            CapComp->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
            CapComp->FOVAngle = 75.f;
            CapComp->bCaptureEveryFrame = false;
            CapComp->bCaptureOnMovement = false;
            // Tick a few frames so the world is fully rendered, then capture + flush.
            for (int32 i = 0; i < 3; ++i) { World->Tick(LEVELTICK_All, 1.f / 60.f); }
            CapComp->CaptureScene();
            FlushRenderingCommands();

            const FString Dir = FPaths::GetPath(ScreenshotPath);
            const FString Name = FPaths::GetBaseFilename(ScreenshotPath) + TEXT(".png");
            UKismetRenderingLibrary::ExportRenderTarget(World, RT, Dir, Name);
            Cap->Destroy();
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
