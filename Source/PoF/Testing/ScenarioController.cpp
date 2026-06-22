#include "ScenarioController.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSingleNodeInstance.h"
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
#include "EngineUtils.h"
#include "UnrealClient.h"
#include "AIController.h"
#include "AI/ARPGSimpleAIController.h"
#include "Character/ARPGEnemyCharacter.h"
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
    Root->TryGetBoolField(TEXT("disable_ai"), bDisableAI);
    Root->TryGetBoolField(TEXT("simple_enemy_ai"), bSimpleEnemyAI);
    Root->TryGetBoolField(TEXT("melee_engage"), bMeleeEngage);

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

    // Deterministic-capture config (all optional; absent => behavior unchanged).
    const TArray<TSharedPtr<FJsonValue>>* StrArr = nullptr;
    if (Root->TryGetArrayField(TEXT("remove_actor_classes"), StrArr))
    {
        for (const TSharedPtr<FJsonValue>& V : *StrArr)
        {
            RemoveActorClasses.Add(V->AsString());
        }
    }
    const TArray<TSharedPtr<FJsonValue>>* PosArr = nullptr;
    if (Root->TryGetArrayField(TEXT("det_capture_positions"), PosArr))
    {
        for (const TSharedPtr<FJsonValue>& V : *PosArr)
        {
            DetCapturePositions.Add((float)V->AsNumber());
        }
        DetCapturePositions.Sort();
    }
    auto ParseVec = [](const FString& S, FVector& Out) -> bool
    {
        TArray<FString> Parts;
        S.ParseIntoArray(Parts, TEXT(","));
        if (Parts.Num() < 3) return false;
        Out = FVector(FCString::Atof(*Parts[0]), FCString::Atof(*Parts[1]), FCString::Atof(*Parts[2]));
        return true;
    };
    FString CamS, LookS;
    if (Root->TryGetStringField(TEXT("det_cam"), CamS) && Root->TryGetStringField(TEXT("det_lookat"), LookS))
    {
        bHaveDetCam = ParseVec(CamS, DetCamLoc) && ParseVec(LookS, DetLookAt);
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

void UScenarioController::RemoveNoiseActors()
{
    // Destroy configured actor classes in the PIE world only — transient, the saved map is
    // untouched (level-edit duplication crashed the headless editor; this avoids it entirely).
    UWorld* W = GetWorld();
    if (!W || RemoveActorClasses.IsEmpty()) return;
    TArray<AActor*> ToKill;
    for (TActorIterator<AActor> It(W); It; ++It)
    {
        if (RemoveActorClasses.Contains(It->GetClass()->GetName()))
        {
            ToKill.Add(*It);
        }
    }
    for (AActor* A : ToKill)
    {
        UE_LOG(LogPoFScenario, Display, TEXT("[scenario] removing noise actor %s (%s)"),
            *A->GetActorLabel(), *A->GetClass()->GetName());
        A->Destroy();
    }
}

void UScenarioController::DisableCombatants()
{
    bCombatantsDisabled = true;
    UWorld* W = GetWorld();
    if (!W) return;
    const APawn* Player = GetPawn();
    TArray<APawn*> ToKill;
    for (TActorIterator<APawn> It(W); It; ++It)
    {
        APawn* P = *It;
        if (P && P != Player && Cast<AAIController>(P->GetController()))
        {
            ToKill.Add(P);
        }
    }
    for (APawn* P : ToKill)
    {
        P->Destroy();
    }
    UE_LOG(LogPoFScenario, Display, TEXT("[scenario] disable_ai: removed %d AI-possessed pawn(s)"), ToKill.Num());
}

void UScenarioController::SwapEnemiesToSimpleAI()
{
    UWorld* W = GetWorld();
    if (!W) return;
    int32 Swapped = 0;
    for (TActorIterator<AARPGEnemyCharacter> It(W); It; ++It)
    {
        AARPGEnemyCharacter* Enemy = *It;
        if (!Enemy || Enemy->IsDead()) continue;
        // Already nav-independent? leave it.
        if (Cast<AARPGSimpleAIController>(Enemy->GetController())) continue;
        if (AController* Old = Enemy->GetController())
        {
            Old->UnPossess();
            Old->Destroy();
        }
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        if (AARPGSimpleAIController* SC = W->SpawnActor<AARPGSimpleAIController>(AARPGSimpleAIController::StaticClass(), Params))
        {
            SC->Possess(Enemy);
            ++Swapped;
        }
    }
    UE_LOG(LogPoFScenario, Display, TEXT("[scenario] simple_enemy_ai: re-possessed %d enemy(ies) with the nav-independent controller"), Swapped);
}

void UScenarioController::EngageNearestEnemy()
{
    UWorld* W = GetWorld();
    if (!W) return;
    APawn* Player = GetPawn();
    if (!Player) return;

    const FVector PLoc = Player->GetActorLocation();
    AARPGEnemyCharacter* Best = nullptr;
    float BestDist = TNumericLimits<float>::Max();
    for (TActorIterator<AARPGEnemyCharacter> It(W); It; ++It)
    {
        AARPGEnemyCharacter* E = *It;
        if (!E || E->IsDead()) continue;
        const float D = FVector::Dist(E->GetActorLocation(), PLoc);
        if (D < BestDist) { BestDist = D; Best = E; }
    }
    if (!Best)
    {
        UE_LOG(LogPoFScenario, Warning, TEXT("[scenario] melee_engage: no enemy found"));
        return;
    }

    // Place the enemy just inside its own AttackRange, directly ahead of the player, facing
    // back at the player so its very next controller tick faces + swings (front-arc damage lands).
    const float Range = FMath::Max(Best->GetAttackRange(), 100.f);
    const FVector Fwd = Player->GetActorForwardVector().GetSafeNormal2D();
    const FVector Target = PLoc + Fwd * (Range * 0.6f);
    Best->SetActorLocation(Target, false, nullptr, ETeleportType::TeleportPhysics);
    const FVector ToPlayer = (PLoc - Target).GetSafeNormal2D();
    Best->SetActorRotation(FRotator(0.f, ToPlayer.Rotation().Yaw, 0.f));
    UE_LOG(LogPoFScenario, Display, TEXT("[scenario] melee_engage: placed %s %.0fcm in front of player (range %.0f)"),
        *Best->GetName(), Range * 0.6f, Range);
}

void UScenarioController::Begin()
{
    bStarted = true;
    ScnTime = 0.f;
    WasActive.Init(false, Inputs.Num());
    RemoveNoiseActors();
    if (bSimpleEnemyAI && !bDisableAI)
    {
        SwapEnemiesToSimpleAI();
    }
    if (bMeleeEngage && !bDisableAI)
    {
        EngageNearestEnemy();
    }
    if (!PlayAnim.IsEmpty())
    {
        if (USkeletalMeshComponent* Mesh = GetMesh())
        {
            if (UAnimationAsset* Anim = LoadObject<UAnimationAsset>(nullptr, *PlayAnim))
            {
                // Headless: a skeletal mesh only advances its pose when "rendered". Off-screen
                // (-RenderOffScreen) a single-node anim otherwise freezes on an early frame, so
                // every captured sample looks identical. Force it to always tick + disable URO
                // so play_anim actually animates and we can sample distinct frames.
                Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
                Mesh->bEnableUpdateRateOptimizations = false;
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
    // Scrub a force-played single-node anim to scenario time so each sample captures a KNOWN
    // frame. Off-screen these anims don't auto-advance, so we set the position explicitly and
    // force a bone refresh before the scene capture below.
    if (!PlayAnim.IsEmpty())
    {
        if (USkeletalMeshComponent* Mesh = GetMesh())
        {
            if (UAnimSingleNodeInstance* SN = Mesh->GetSingleNodeInstance())
            {
                const float Len = SN->GetLength();
                SN->SetPosition((Len > 0.f) ? FMath::Fmod(ScnTime, Len) : ScnTime, false);
                Mesh->TickAnimation(0.f, false);
                Mesh->RefreshBoneTransforms();
            }
        }
    }

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
    CaptureViewport(Idx);
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

void UScenarioController::CaptureViewport(int32 Idx)
{
    if (OutDir.IsEmpty()) return;
    // The REAL game viewport: the player camera + full lit post-process pipeline — the floor and
    // world render exactly as they do in play (SceneCapture2D rendered them black/edge-on). The
    // request is serviced at end-of-frame, so the PNG appears ~1 tick later (well before DONE).
    const FString Path = FPaths::Combine(OutDir, FString::Printf(TEXT("shot_%02d.png"), Idx));
    FScreenshotRequest::RequestScreenshot(Path, /*bShowUI=*/false, /*bAddFilenameSuffix=*/false);
    UE_LOG(LogPoFScenario, Display, TEXT("[scenario] viewport screenshot requested -> %s"), *Path);
}

void UScenarioController::SampleMotionQuality(float DeltaTime)
{
    // The Motion Quality Probe: turns "the animation looks broken" into measurable, named
    // failures. VLMs judge fine-grained motion poorly (foot-slide, pops), but these are exactly
    // computable from the skeleton — so we measure, not guess. Signals:
    //   - foot_slide: a PLANTED foot should be world-stationary; its horizontal travel while
    //     grounded is the classic foot-skating artifact (locomotion speed != animation speed).
    //   - joint accel: a blend/state-machine pop is a velocity discontinuity = an accel spike.
    USkeletalMeshComponent* Mesh = GetMesh();
    LastFootSlideL = -1.0;
    LastFootSlideR = -1.0;
    LastJointAccel = 0.0;
    if (!Mesh || DeltaTime < KINDA_SMALL_NUMBER)
    {
        return;
    }
    Mesh->RefreshBoneTransforms();

    auto BoneWorld = [Mesh](const TCHAR* Name, bool& bOk) -> FVector
    {
        const FName BN(Name);
        if (Mesh->GetBoneIndex(BN) == INDEX_NONE) { bOk = false; return FVector::ZeroVector; }
        bOk = true;
        return Mesh->GetSocketTransform(BN, RTS_World).GetTranslation();
    };

    bool bFL = false, bFR = false, bPV = false;
    const FVector FL = BoneWorld(TEXT("foot_l"), bFL);
    const FVector FR = BoneWorld(TEXT("foot_r"), bFR);
    const FVector PV = BoneWorld(TEXT("pelvis"), bPV);

    if (bHavePrevKin && bFL && bFR)
    {
        // The in-contact foot is the lower one (+ the other if within a small band of it —
        // double stance). A grounded foot should not travel horizontally in WORLD space.
        const float GroundRef = FMath::Min(FL.Z, FR.Z);
        const float ContactBand = 6.f; // cm above the lower foot still counts as grounded
        auto SlideOf = [&](const FVector& Cur, const FVector& Prev, float FootZ) -> double
        {
            if ((FootZ - GroundRef) >= ContactBand) return -1.0; // airborne (swing phase)
            return FVector(Cur.X - Prev.X, Cur.Y - Prev.Y, 0.f).Size();
        };
        LastFootSlideL = SlideOf(FL, PrevFootL, FL.Z);
        LastFootSlideR = SlideOf(FR, PrevFootR, FR.Z);
        if (LastFootSlideL >= 0.0) { FootSlideAccum += LastFootSlideL; FootContactTime += DeltaTime; }
        if (LastFootSlideR >= 0.0) { FootSlideAccum += LastFootSlideR; FootContactTime += DeltaTime; }

        const FVector VL = (FL - PrevFootL) / DeltaTime;
        const FVector VR = (FR - PrevFootR) / DeltaTime;
        const FVector VP = (PV - PrevPelvis) / DeltaTime;
        if (bHavePrevKinVel)
        {
            const double AL = ((VL - PrevFootLVel) / DeltaTime).Size();
            const double AR = ((VR - PrevFootRVel) / DeltaTime).Size();
            const double AP = ((VP - PrevPelvisVel) / DeltaTime).Size();
            LastJointAccel = FMath::Max3(AL, AR, AP);
            JointJerkAccum += (AL + AR + AP);
            if (LastJointAccel > MaxJointAccel)
            {
                MaxJointAccel = LastJointAccel;
                MaxJointAccelTime = ScnTime;
            }
        }
        PrevFootLVel = VL;
        PrevFootRVel = VR;
        PrevPelvisVel = VP;
        bHavePrevKinVel = true;
    }

    PrevFootL = FL;
    PrevFootR = FR;
    PrevPelvis = PV;
    bHavePrevKin = bFL && bFR && bPV;
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
        // Mesh-lift ground truth: the live relative-Z (did the visual lift APPLY) and the pawn's
        // actual configured lift (did the CDO tune REACH the spawned instance) — separates
        // config-propagation failures from logic failures without guessing.
        R->SetNumberField(TEXT("meshRelZ"), Mesh->GetRelativeLocation().Z);
        if (FFloatProperty* LiftProp = FindFProperty<FFloatProperty>(P->GetClass(), TEXT("DodgeMeshLift")))
        {
            R->SetNumberField(TEXT("liftCfg"), LiftProp->GetPropertyValue_InContainer(P));
        }
        if (UAnimInstance* AI = Mesh->GetAnimInstance())
        {
            if (UAnimMontage* M = AI->GetCurrentActiveMontage())
            {
                MontName = M->GetName();
                MontPos = AI->Montage_GetPosition(M);
                if (M == PrevMontage && PrevMontagePos >= 0.f && MontPos >= PrevMontagePos)
                {
                    // UE 5.8: ExtractRootMotionFromTrackRange gained a required FAnimExtractContext
                    // param. A default context preserves the prior 2-arg behavior (matches how the
                    // engine migrated its own internal callers in AnimMontage.cpp).
                    const FTransform RM = M->ExtractRootMotionFromTrackRange(PrevMontagePos, MontPos, FAnimExtractContext());
                    RootDelta = RM.GetTranslation();
                }
                PrevMontage = M;
                PrevMontagePos = MontPos;

                // Deterministic captures: fire when the MONTAGE POSITION crosses each mark.
                // Pose + root-motion location are functions of montage position (not wall-clock),
                // and the camera is a fixed world pose -> frames are reproducible run-to-run.
                while (bHaveDetCam && NextDetCapture < DetCapturePositions.Num()
                    && MontPos >= DetCapturePositions[NextDetCapture])
                {
                    Mesh->RefreshBoneTransforms();
                    const FString Png = CaptureView(NextDetCapture, TEXT("_det"), DetCamLoc, DetLookAt);
                    UE_LOG(LogPoFScenario, Display, TEXT("[scenario] det capture %d at montPos=%.3f -> %s"),
                        NextDetCapture, MontPos, *Png);
                    ++NextDetCapture;
                }
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

    // --- Motion Quality Probe: per-tick foot-slide + joint-acceleration (pop) signals ---
    SampleMotionQuality(DeltaTime);
    R->SetNumberField(TEXT("footSlideL"), LastFootSlideL);
    R->SetNumberField(TEXT("footSlideR"), LastFootSlideR);
    R->SetNumberField(TEXT("jointAccel"), LastJointAccel);
    // Coarse layer-overlap signal: a montage driving the body while locomotion is also moving it.
    if (!MontName.IsEmpty() && Vel.Size2D() > 50.f)
    {
        MontageMoveOverlap += DeltaTime;
    }

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

    // Motion Quality Probe summary — the headline animation-quality signals an agent reads to
    // judge "does it move right", not just "did it move". foot_slide_rate_cps ~0 = clean stance;
    // a high value = foot-skating. max_joint_accel spikes on blend/state pops.
    TSharedPtr<FJsonObject> MQ = MakeShared<FJsonObject>();
    MQ->SetNumberField(TEXT("foot_slide_total_cm"), FootSlideAccum);
    MQ->SetNumberField(TEXT("foot_contact_time_s"), FootContactTime);
    MQ->SetNumberField(TEXT("foot_slide_rate_cps"),
        FootContactTime > KINDA_SMALL_NUMBER ? FootSlideAccum / FootContactTime : 0.0);
    MQ->SetNumberField(TEXT("joint_jerk_total"), JointJerkAccum);
    MQ->SetNumberField(TEXT("max_joint_accel"), MaxJointAccel);
    MQ->SetNumberField(TEXT("max_joint_accel_time"), MaxJointAccelTime);
    MQ->SetNumberField(TEXT("montage_move_overlap_s"), MontageMoveOverlap);
    Out->SetObjectField(TEXT("motion_quality"), MQ);

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
        if (GetPawn())
        {
            // Remove hostiles the instant the pawn exists (before settle) so the player can't be
            // damaged into a CanMove()-false state during the run.
            if (bDisableAI && !bCombatantsDisabled)
            {
                DisableCombatants();
            }
            if (PreElapsed >= SettleTime)
            {
                Begin();
            }
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
