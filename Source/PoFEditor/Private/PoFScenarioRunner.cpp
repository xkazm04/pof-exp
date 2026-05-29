// Copyright PoF. All Rights Reserved.

#include "PoFScenarioRunner.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
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
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
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

// Capture the live PIE scene to a PNG via a SceneCapture2D -> render target, framed on
// the pawn (behind + above). Renders through the pipeline independent of any viewport.
// Returns the written path, or empty on failure.
static FString CaptureToPng(UWorld* World, APawn* Pawn, const FString& FullPath)
{
    if (FullPath.IsEmpty() || !World || !Pawn)
    {
        return FString();
    }
    const FVector PawnLoc = Pawn->GetActorLocation();
    const FVector LookAt = PawnLoc + FVector(0, 0, 90.f);
    const FVector CamLoc = PawnLoc + Pawn->GetActorForwardVector() * -320.f + FVector(0, 0, 170.f);
    const FRotator CamRot = (LookAt - CamLoc).Rotation();

    UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(World);
    RT->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
    RT->InitAutoFormat(512, 512);
    RT->UpdateResourceImmediate(true);

    ASceneCapture2D* Cap = World->SpawnActor<ASceneCapture2D>(CamLoc, CamRot);
    if (!Cap)
    {
        return FString();
    }
    USceneCaptureComponent2D* CapComp = Cap->GetCaptureComponent2D();
    CapComp->TextureTarget = RT;
    CapComp->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    CapComp->FOVAngle = 75.f;
    CapComp->bCaptureEveryFrame = false;
    CapComp->bCaptureOnMovement = false;
    CapComp->CaptureScene();
    FlushRenderingCommands();

    const FString Dir = FPaths::GetPath(FullPath);
    const FString Name = FPaths::GetBaseFilename(FullPath) + TEXT(".png");
    UKismetRenderingLibrary::ExportRenderTarget(World, RT, Dir, Name);
    Cap->Destroy();
    return FPaths::Combine(Dir, Name);
}

// Wait (ticking the editor) for PIE to start and possess a pawn. Returns the PC.
static APlayerController* WaitForPossessedPawn(UWorld*& OutWorld, double TimeoutSeconds)
{
    OutWorld = nullptr;
    const double Start = FPlatformTime::Seconds();
    while (FPlatformTime::Seconds() - Start < TimeoutSeconds)
    {
        OutWorld = FindPieWorld();
        if (OutWorld)
        {
            APlayerController* PC = OutWorld->GetFirstPlayerController();
            if (PC && PC->GetPawn())
            {
                return PC;
            }
            OutWorld->Tick(LEVELTICK_All, 1.f / 60.f);
        }
        FPlatformProcess::Sleep(0.02f);
    }
    return nullptr;
}

// Apply one timed input on the current frame at time T (real key when In.Key is set).
static void ApplyTimedInput(const FPoFTimedInput& In, float T, APlayerController* PC,
    UEnhancedInputLocalPlayerSubsystem* Subsys)
{
    if (T < In.StartSeconds || T >= In.StartSeconds + In.DurationSeconds)
    {
        return;
    }
    if (!In.Key.IsEmpty())
    {
        const FKey K(FName(*In.Key));
        const FInputKeyEventArgs Args = FInputKeyEventArgs::CreateSimulated(K, IE_Pressed, 1.0f);
        PC->InputKey(Args);
    }
    else if (Subsys && !In.ActionPath.IsEmpty())
    {
        if (UInputAction* IA = LoadObject<UInputAction>(nullptr, *In.ActionPath))
        {
            Subsys->InjectInputForAction(IA, FInputActionValue(FVector2D(In.Value)), {}, {});
        }
    }
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

    UWorld* World = nullptr;
    APlayerController* PC = WaitForPossessedPawn(World, 5.0);
    if (!World || !PC || !PC->GetPawn())
    {
        return false;
    }

    UEnhancedInputLocalPlayerSubsystem* Subsys =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());

    const int32 TotalFrames = FMath::Max(1, FMath::CeilToInt(TotalSeconds * 60.f));
    for (int32 Frame = 0; Frame < TotalFrames; ++Frame)
    {
        const float T = Frame / 60.f;
        for (const FPoFTimedInput& In : Inputs)
        {
            ApplyTimedInput(In, T, PC, Subsys);
        }
        World->Tick(LEVELTICK_All, 1.f / 60.f);
    }

    if (!ScreenshotPath.IsEmpty())
    {
        CaptureToPng(World, PC->GetPawn(), ScreenshotPath);
    }
    // PIE is left running so legacy observers can read it.
    return true;
}

FPoFScenarioResult UPoFScenarioRunner::RunScenarioEx(const FString& MapPath,
    const TArray<FPoFTimedInput>& Inputs, float TotalSeconds, const FString& FrameDir, int32 NumSamples)
{
    FPoFScenarioResult Result;
    if (!GEditor)
    {
        Result.Error = TEXT("no GEditor");
        return Result;
    }

    FEditorFileUtils::LoadMap(MapPath, /*bLoadAsTemplate*/ false, /*bShowProgress*/ false);
    FRequestPlaySessionParams Params;
    GEditor->RequestPlaySession(Params);
    GEditor->StartQueuedPlaySessionRequest();

    UWorld* World = nullptr;
    APlayerController* PC = WaitForPossessedPawn(World, 8.0);
    if (!World || !PC || !PC->GetPawn())
    {
        Result.Error = TEXT("no possessed pawn within timeout");
        return Result;
    }
    Result.Started = true;

    APawn* Pawn = PC->GetPawn();
    USkeletalMeshComponent* Mesh = Pawn->FindComponentByClass<USkeletalMeshComponent>();
    UEnhancedInputLocalPlayerSubsystem* Subsys =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());

    const int32 TotalFrames = FMath::Max(1, FMath::CeilToInt(TotalSeconds * 60.f));
    NumSamples = FMath::Clamp(NumSamples, 1, 60);

    // Evenly-spaced sample frame indices across the run.
    TArray<int32> SampleFrames;
    for (int32 s = 0; s < NumSamples; ++s)
    {
        const float Frac = (NumSamples == 1) ? 1.f : (float)(s + 1) / (float)NumSamples;
        SampleFrames.Add(FMath::Clamp(FMath::CeilToInt(Frac * TotalFrames) - 1, 0, TotalFrames - 1));
    }

    auto BoneDroop = [Mesh](const TCHAR* Up, const TCHAR* Lo, bool& bOk) -> float
    {
        bOk = false;
        if (!Mesh) return 0.f;
        const int32 Ui = Mesh->GetBoneIndex(FName(Up));
        const int32 Li = Mesh->GetBoneIndex(FName(Lo));
        if (Ui == INDEX_NONE || Li == INDEX_NONE) return 0.f;
        const FVector U = Mesh->GetBoneTransform(Ui, FTransform::Identity).GetTranslation();
        const FVector L = Mesh->GetBoneTransform(Li, FTransform::Identity).GetTranslation();
        const FVector V = L - U;
        const float Len = V.Size();
        if (Len < KINDA_SMALL_NUMBER) return 0.f;
        bOk = true;
        return FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(-V.Z / Len, -1.f, 1.f)));
    };

    int32 SampleIdx = 0;
    for (int32 Frame = 0; Frame < TotalFrames; ++Frame)
    {
        const float T = Frame / 60.f;
        for (const FPoFTimedInput& In : Inputs)
        {
            ApplyTimedInput(In, T, PC, Subsys);
        }
        World->Tick(LEVELTICK_All, 1.f / 60.f);

        if (SampleIdx < SampleFrames.Num() && Frame == SampleFrames[SampleIdx])
        {
            FPoFPoseSample S;
            S.Time = T;
            S.Location = Pawn->GetActorLocation();
            S.Speed = Pawn->GetVelocity().Size2D();
            if (Mesh)
            {
                Mesh->RefreshBoneTransforms();
                bool bOkL = false, bOkR = false;
                S.ArmDroopLeftDeg = BoneDroop(TEXT("upperarm_l"), TEXT("lowerarm_l"), bOkL);
                S.ArmDroopRightDeg = BoneDroop(TEXT("upperarm_r"), TEXT("lowerarm_r"), bOkR);
                S.PoseValid = bOkL && bOkR;
            }
            if (!FrameDir.IsEmpty())
            {
                const FString P = FPaths::Combine(FrameDir, FString::Printf(TEXT("sample_%02d.png"), SampleIdx));
                S.Frame = CaptureToPng(World, Pawn, P);
            }
            Result.Samples.Add(S);
            ++SampleIdx;
        }
    }
    // PIE left running for any follow-up observation; caller should StopScenario.
    return Result;
}

void UPoFScenarioRunner::StopScenario()
{
    if (GEditor)
    {
        GEditor->RequestEndPlayMap();
    }
}
