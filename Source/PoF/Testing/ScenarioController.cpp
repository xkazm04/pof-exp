#include "ScenarioController.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimMontage.h"
#include "GameplayTagContainer.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Player/ARPGPlayerCharacter.h"
#include "HAL/PlatformMisc.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "RenderingThread.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogPoFScenario, Log, All);

void UScenarioController::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    FString ScenarioPath;
    if (!FParse::Value(FCommandLine::Get(), TEXT("PoFScenario="), ScenarioPath) || ScenarioPath.IsEmpty())
    {
        return; // dormant in normal play
    }
    bIsStandalone = (InWorld.WorldType == EWorldType::Game);
    if (!LoadScenario(ScenarioPath))
    {
        UE_LOG(LogPoFScenario, Error, TEXT("[scenario] failed to load %s"), *ScenarioPath);
        return;
    }
    for (int32 s = 0; s < NumSamples; ++s)
    {
        const float Frac = (NumSamples <= 1) ? 1.f : (float)(s + 1) / (float)NumSamples;
        SampleTimes.Add(Frac * TotalSeconds);
    }
    bArmed = true;
    UE_LOG(LogPoFScenario, Display,
        TEXT("[scenario] ARMED total=%.1fs samples=%d inputs=%d out=%s standalone=%d"),
        TotalSeconds, NumSamples, Inputs.Num(), *OutDir, bIsStandalone ? 1 : 0);
}

bool UScenarioController::LoadScenario(const FString& Path)
{
    FString Json;
    if (!FFileHelper::LoadFileToString(Json, *Path))
    {
        return false;
    }
    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        return false;
    }
    double D = 0.0;
    int32 I = 0;
    if (Root->TryGetNumberField(TEXT("total_seconds"), D)) TotalSeconds = (float)D;
    if (Root->TryGetNumberField(TEXT("num_samples"), I)) NumSamples = FMath::Clamp(I, 1, 60);
    if (Root->TryGetNumberField(TEXT("settle"), D)) SettleTime = (float)D;
    Root->TryGetStringField(TEXT("out_dir"), OutDir);
    Root->TryGetStringField(TEXT("play_anim"), PlayAnim);

    const TArray<TSharedPtr<FJsonValue>>* InArr = nullptr;
    if (Root->TryGetArrayField(TEXT("inputs"), InArr))
    {
        for (const TSharedPtr<FJsonValue>& V : *InArr)
        {
            const TSharedPtr<FJsonObject>* O = nullptr;
            if (!V->TryGetObject(O)) continue;
            FScnInput In;
            (*O)->TryGetStringField(TEXT("key"), In.Key);
            (*O)->TryGetStringField(TEXT("action"), In.ActionPath);
            (*O)->TryGetStringField(TEXT("event"), In.Event);
            (*O)->TryGetStringField(TEXT("event_arg"), In.EventArg);
            double S = 0.0;
            if ((*O)->TryGetNumberField(TEXT("start"), S)) In.Start = (float)S;
            if ((*O)->TryGetNumberField(TEXT("duration"), S)) In.Duration = (float)S;
            const TArray<TSharedPtr<FJsonValue>>* Val = nullptr;
            if ((*O)->TryGetArrayField(TEXT("value"), Val) && Val->Num() >= 2)
            {
                In.Value.X = (float)(*Val)[0]->AsNumber();
                In.Value.Y = (float)(*Val)[1]->AsNumber();
            }
            Inputs.Add(In);
        }
    }
    return !OutDir.IsEmpty();
}

APawn* UScenarioController::GetPawn() const
{
    if (UWorld* W = GetWorld())
    {
        if (APlayerController* PC = W->GetFirstPlayerController())
        {
            return PC->GetPawn();
        }
    }
    return nullptr;
}

USkeletalMeshComponent* UScenarioController::GetMesh() const
{
    if (APawn* P = GetPawn())
    {
        return P->FindComponentByClass<USkeletalMeshComponent>();
    }
    return nullptr;
}

void UScenarioController::Begin()
{
    bStarted = true;
    ScnTime = 0.f;
    WasActive.Init(false, Inputs.Num());
    if (!PlayAnim.IsEmpty())
    {
        if (USkeletalMeshComponent* Mesh = GetMesh())
        {
            if (UAnimationAsset* Anim = LoadObject<UAnimationAsset>(nullptr, *PlayAnim))
            {
                Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
                Mesh->PlayAnimation(Anim, true);
                UE_LOG(LogPoFScenario, Display, TEXT("[scenario] force-play anim %s"), *PlayAnim);
            }
        }
    }
    UE_LOG(LogPoFScenario, Display, TEXT("[scenario] BEGIN (pawn possessed, settled)"));
}

void UScenarioController::ApplyInputs()
{
    UWorld* W = GetWorld();
    APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
    if (!PC) return;
    UEnhancedInputLocalPlayerSubsystem* EI =
        PC->GetLocalPlayer() ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()) : nullptr;

    for (int32 i = 0; i < Inputs.Num(); ++i)
    {
        const FScnInput& In = Inputs[i];
        const bool bNow = (ScnTime >= In.Start && ScnTime < In.Start + In.Duration);
        const bool bWas = WasActive.IsValidIndex(i) && WasActive[i];
        if (!In.Key.IsEmpty())
        {
            // Held-key semantics: Pressed on rising edge, Released on falling edge. The key
            // stays DOWN between edges (PlayerInput tracks it), so movement sustains —
            // repeated per-frame Pressed events were read as taps and the player braked.
            const FKey K(FName(*In.Key));
            if (bNow && !bWas)
            {
                PC->InputKey(FInputKeyEventArgs::CreateSimulated(K, IE_Pressed, 1.0f));
            }
            else if (!bNow && bWas)
            {
                PC->InputKey(FInputKeyEventArgs::CreateSimulated(K, IE_Released, 0.0f));
            }
        }
        else if (EI && bNow && !In.ActionPath.IsEmpty())
        {
            if (UInputAction* IA = LoadObject<UInputAction>(nullptr, *In.ActionPath))
            {
                EI->InjectInputForAction(IA, FInputActionValue(In.Value), {}, {});
            }
        }
        // Non-input event on the rising edge (activate an ability directly — bypasses the
        // Boolean-key injection fidelity gap, so the ability's EFFECT is what we observe).
        if (In.Event == TEXT("activate_ability") && bNow && !bWas)
        {
            if (APawn* P = PC->GetPawn())
            {
                if (UAbilitySystemComponent* ASC = P->FindComponentByClass<UAbilitySystemComponent>())
                {
                    FGameplayTagContainer Tags;
                    Tags.AddTag(FGameplayTag::RequestGameplayTag(FName(*In.EventArg), /*ErrorIfNotFound*/ false));
                    ASC->TryActivateAbilitiesByTag(Tags);
                    UE_LOG(LogPoFScenario, Display, TEXT("[scenario] activate_ability %s"), *In.EventArg);
                }
            }
        }
        // Deterministic mouse-aim seam: place the player's cursor aim point at a fixed world
        // location so "does the body face the cursor / does it strafe" is harness-verifiable
        // without an OS mouse. Routed to AARPGPlayerCharacter::SetCursorWorldOverride.
        if (In.Event == TEXT("set_cursor") && bNow && !bWas)
        {
            TArray<FString> Parts;
            In.EventArg.ParseIntoArray(Parts, TEXT(","));
            if (Parts.Num() >= 3)
            {
                const FVector Loc(FCString::Atof(*Parts[0]), FCString::Atof(*Parts[1]), FCString::Atof(*Parts[2]));
                if (AARPGPlayerCharacter* Player = Cast<AARPGPlayerCharacter>(PC->GetPawn()))
                {
                    Player->SetCursorWorldOverride(Loc);
                    UE_LOG(LogPoFScenario, Display, TEXT("[scenario] set_cursor %s"), *In.EventArg);
                }
            }
        }
        // Screen-space mouse position -> exercises the player's REAL deproject cursor-aim path
        // (not the world-override proxy), so the contract suite catches real-mouse regressions.
        if (In.Event == TEXT("set_mouse") && bNow && !bWas)
        {
            TArray<FString> Parts;
            In.EventArg.ParseIntoArray(Parts, TEXT(","));
            if (Parts.Num() >= 2)
            {
                if (AARPGPlayerCharacter* Player = Cast<AARPGPlayerCharacter>(PC->GetPawn()))
                {
                    Player->SetCursorScreenOverride(FVector2D(FCString::Atof(*Parts[0]), FCString::Atof(*Parts[1])));
                    UE_LOG(LogPoFScenario, Display, TEXT("[scenario] set_mouse %s"), *In.EventArg);
                }
            }
        }
        if (WasActive.IsValidIndex(i)) WasActive[i] = bNow;
    }
}

void UScenarioController::DoSample(int32 Idx)
{
    APawn* P = GetPawn();
    TSharedPtr<FJsonObject> S = MakeShared<FJsonObject>();
    S->SetNumberField(TEXT("t"), ScnTime);
    if (P)
    {
        const FVector L = P->GetActorLocation();
        S->SetNumberField(TEXT("loc_x"), L.X);
        S->SetNumberField(TEXT("loc_y"), L.Y);
        S->SetNumberField(TEXT("loc_z"), L.Z);
        S->SetNumberField(TEXT("speed"), P->GetVelocity().Size2D());

        // GAS attribute readout (reflection — works for any ARPG character/attribute set).
        auto ReadAttr = [P](const TCHAR* N) -> double
        {
            if (FFloatProperty* Fp = FindFProperty<FFloatProperty>(P->GetClass(), N))
                return Fp->GetPropertyValue_InContainer(P);
            if (FDoubleProperty* Dp = FindFProperty<FDoubleProperty>(P->GetClass(), N))
                return Dp->GetPropertyValue_InContainer(P);
            return -1.0;
        };
        S->SetNumberField(TEXT("health"), ReadAttr(TEXT("Health")));
        S->SetNumberField(TEXT("stamina"), ReadAttr(TEXT("Stamina")));
        S->SetNumberField(TEXT("mana"), ReadAttr(TEXT("Mana")));
    }
    bool bPoseValid = false;
    if (USkeletalMeshComponent* Mesh = GetMesh())
    {
        Mesh->RefreshBoneTransforms();
        auto Droop = [Mesh](const TCHAR* Up, const TCHAR* Lo, bool& bOk) -> float
        {
            bOk = false;
            const FVector U = Mesh->GetSocketTransform(FName(Up), RTS_Component).GetTranslation();
            const FVector Lw = Mesh->GetSocketTransform(FName(Lo), RTS_Component).GetTranslation();
            const FVector V = Lw - U;
            const float Len = V.Size();
            if (Len < KINDA_SMALL_NUMBER) return 0.f;
            bOk = true;
            return FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(-V.Z / Len, -1.f, 1.f)));
        };
        bool bL = false, bR = false;
        const float DL = Droop(TEXT("upperarm_l"), TEXT("lowerarm_l"), bL);
        const float DR = Droop(TEXT("upperarm_r"), TEXT("lowerarm_r"), bR);
        S->SetNumberField(TEXT("droopL"), DL);
        S->SetNumberField(TEXT("droopR"), DR);
        bPoseValid = bL && bR;

        // Read the anim driver values the AnimBP actually sees (describe WHY it animates
        // or not): reflection over the live anim instance's Speed/Direction floats.
        if (UAnimInstance* AI = Mesh->GetAnimInstance())
        {
            S->SetStringField(TEXT("anim_class"), AI->GetClass()->GetName());
            auto ReadF = [AI](const TCHAR* N) -> double
            {
                if (FFloatProperty* P = FindFProperty<FFloatProperty>(AI->GetClass(), N))
                    return P->GetPropertyValue_InContainer(AI);
                if (FDoubleProperty* DP = FindFProperty<FDoubleProperty>(AI->GetClass(), N))
                    return DP->GetPropertyValue_InContainer(AI);
                return -999.0;
            };
            S->SetNumberField(TEXT("anim_speed"), ReadF(TEXT("Speed")));
            S->SetNumberField(TEXT("anim_direction"), ReadF(TEXT("Direction")));

            // Montage state — abilities (attacks, dodge, casts) play montages; this is the
            // primary "an ability activated" observable.
            const bool bMontage = AI->IsAnyMontagePlaying();
            S->SetBoolField(TEXT("montage_playing"), bMontage);
            if (UAnimMontage* Active = AI->GetCurrentActiveMontage())
            {
                S->SetStringField(TEXT("montage_name"), Active->GetName());
            }
        }
    }
    S->SetBoolField(TEXT("pose_valid"), bPoseValid);
    S->SetStringField(TEXT("frame"), CaptureFrame(Idx));
    SamplesJson.Add(MakeShared<FJsonValueObject>(S));
    UE_LOG(LogPoFScenario, Display, TEXT("[scenario] sample %d t=%.2f"), Idx, ScnTime);
}

FString UScenarioController::CaptureView(int32 Idx, const FString& Suffix, const FVector& CamLoc, const FVector& LookAt)
{
    UWorld* W = GetWorld();
    if (!W || OutDir.IsEmpty()) return FString();
    const FRotator CamRot = (LookAt - CamLoc).Rotation();

    UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(W);
    RT->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
    RT->InitAutoFormat(512, 512);
    RT->UpdateResourceImmediate(true);

    ASceneCapture2D* Cap = W->SpawnActor<ASceneCapture2D>(CamLoc, CamRot);
    if (!Cap) return FString();
    USceneCaptureComponent2D* C = Cap->GetCaptureComponent2D();
    C->TextureTarget = RT;
    C->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    C->FOVAngle = 75.f;
    C->bCaptureEveryFrame = false;
    C->bCaptureOnMovement = false;
    C->CaptureScene();
    FlushRenderingCommands();

    const FString Name = FString::Printf(TEXT("frame_%02d%s.png"), Idx, *Suffix);
    UKismetRenderingLibrary::ExportRenderTarget(W, RT, OutDir, Name);
    Cap->Destroy();
    return FPaths::Combine(OutDir, Name);
}

FString UScenarioController::CaptureFrame(int32 Idx)
{
    APawn* P = GetPawn();
    if (!P) return FString();
    const FVector PawnLoc = P->GetActorLocation();
    // Chase (behind + above) shows the pose; side (low, level with the floor) shows the floor
    // line so mesh-into-floor clipping is directly visible.
    const FVector ChaseLoc = PawnLoc + P->GetActorForwardVector() * -320.f + FVector(0, 0, 170.f);
    const FVector SideLoc = PawnLoc + P->GetActorRightVector() * 360.f + FVector(0, 0, 70.f);
    const FString Chase = CaptureView(Idx, TEXT(""), ChaseLoc, PawnLoc + FVector(0, 0, 90.f));
    CaptureView(Idx, TEXT("_side"), SideLoc, PawnLoc + FVector(0, 0, 50.f));
    return Chase;
}

void UScenarioController::RecordTrace(float DeltaTime)
{
    APawn* P = GetPawn();
    if (!P) return;

    const FVector Loc = P->GetActorLocation();
    const FVector Vel = P->GetVelocity();

    TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
    R->SetNumberField(TEXT("t"), ScnTime);
    R->SetNumberField(TEXT("dt"), DeltaTime);
    R->SetNumberField(TEXT("x"), Loc.X);
    R->SetNumberField(TEXT("y"), Loc.Y);
    R->SetNumberField(TEXT("z"), Loc.Z);
    R->SetNumberField(TEXT("vx"), Vel.X);
    R->SetNumberField(TEXT("vy"), Vel.Y);
    R->SetNumberField(TEXT("speed"), Vel.Size2D());
    R->SetNumberField(TEXT("yaw"), P->GetActorRotation().Yaw);

    // Capsule displacement this tick — what actually moved the character.
    const FVector CapsuleDelta = bHavePrevTrace ? (Loc - PrevTraceLoc) : FVector::ZeroVector;
    R->SetNumberField(TEXT("dCapX"), CapsuleDelta.X);
    R->SetNumberField(TEXT("dCapY"), CapsuleDelta.Y);

    // Animation root-motion this tick — what the animation INTENDED to move. Foot-slide is
    // capsule delta diverging from this; extracted from the active montage's track range.
    FVector RootDelta = FVector::ZeroVector;
    FString MontName;
    float MontPos = -1.f;
    if (USkeletalMeshComponent* Mesh = GetMesh())
    {
        // Lowest world-Z of the mesh bounds — floor-penetration = this dipping below the floor.
        R->SetNumberField(TEXT("meshMinZ"), Mesh->Bounds.GetBox().Min.Z);
        if (UAnimInstance* AI = Mesh->GetAnimInstance())
        {
            if (UAnimMontage* M = AI->GetCurrentActiveMontage())
            {
                MontName = M->GetName();
                MontPos = AI->Montage_GetPosition(M);
                if (M == PrevMontage && PrevMontagePos >= 0.f && MontPos >= PrevMontagePos)
                {
                    const FTransform RM = M->ExtractRootMotionFromTrackRange(PrevMontagePos, MontPos);
                    RootDelta = RM.GetTranslation();
                }
                PrevMontage = M;
                PrevMontagePos = MontPos;
            }
            else
            {
                PrevMontage = nullptr;
                PrevMontagePos = -1.f;
            }
        }
    }
    R->SetStringField(TEXT("montage"), MontName);
    R->SetNumberField(TEXT("montPos"), MontPos);
    R->SetNumberField(TEXT("dRootX"), RootDelta.X);
    R->SetNumberField(TEXT("dRootY"), RootDelta.Y);

    TraceJson.Add(MakeShared<FJsonValueObject>(R));
    PrevTraceLoc = Loc;
    bHavePrevTrace = true;
}

void UScenarioController::Finish()
{
    bDone = true;

    TSharedPtr<FJsonObject> Out = MakeShared<FJsonObject>();
    Out->SetBoolField(TEXT("started"), true);
    Out->SetNumberField(TEXT("total_seconds"), TotalSeconds);
    Out->SetArrayField(TEXT("samples"), SamplesJson);

    FString OutStr;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutStr);
    FJsonSerializer::Serialize(Out.ToSharedRef(), Writer);
    FFileHelper::SaveStringToFile(OutStr, *FPaths::Combine(OutDir, TEXT("observations.json")));

    // Dense per-tick motion trace (analysed for the motion profile + capsule-vs-anim slide).
    TSharedPtr<FJsonObject> TraceOut = MakeShared<FJsonObject>();
    TraceOut->SetArrayField(TEXT("trace"), TraceJson);
    FString TraceStr;
    TSharedRef<TJsonWriter<>> TWriter = TJsonWriterFactory<>::Create(&TraceStr);
    FJsonSerializer::Serialize(TraceOut.ToSharedRef(), TWriter);
    FFileHelper::SaveStringToFile(TraceStr, *FPaths::Combine(OutDir, TEXT("trace.json")));

    // DONE marker written last — the poller waits on it before reading the outputs.
    FFileHelper::SaveStringToFile(TEXT("done"), *FPaths::Combine(OutDir, TEXT("DONE")));

    UE_LOG(LogPoFScenario, Display, TEXT("[scenario] FINISH wrote %d samples to %s"),
        SamplesJson.Num(), *OutDir);

    if (bIsStandalone)
    {
        FPlatformMisc::RequestExit(false);
    }
}

void UScenarioController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (!bArmed || bDone) return;

    if (!bStarted)
    {
        PreElapsed += DeltaTime;
        if (GetPawn() && PreElapsed >= SettleTime)
        {
            Begin();
        }
        return;
    }

    ScnTime += DeltaTime;
    ApplyInputs();
    RecordTrace(DeltaTime);
    while (NextSample < SampleTimes.Num() && ScnTime >= SampleTimes[NextSample])
    {
        DoSample(NextSample);
        ++NextSample;
    }
    if (ScnTime >= TotalSeconds)
    {
        Finish();
    }
}
