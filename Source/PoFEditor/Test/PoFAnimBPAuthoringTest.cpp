// Copyright PoF. All Rights Reserved.

#include "PoFAnimBPAuthoringLibrary.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/BlendSpace.h"
#include "Animation/Skeleton.h"
#include "EditorAssetLibrary.h"
#include "Misc/AutomationTest.h"
#include "UObject/SoftObjectPath.h"

namespace PoFAnimBPAuth
{
    /** Load the Manny skeleton expected to be present in any UE5 project with the mannequin content pack.
     *  Returns nullptr if not present — caller's test should bail with TestNotNull. */
    USkeleton* LoadMannySkeleton()
    {
        // Try the modern (UE 5.7) Manny path first, then fall back to legacy UE5 mannequin folder.
        const TCHAR* CandidatePaths[] = {
            TEXT("/Game/Characters/Mannequins/Meshes/SK_Mannequin"),
            TEXT("/Game/Characters/Manny/Meshes/SK_Mannequin"),
            TEXT("/Engine/Mannequin/Mesh/SK_Mannequin_Skeleton"),
        };
        for (const TCHAR* Path : CandidatePaths)
        {
            if (USkeleton* S = LoadObject<USkeleton>(nullptr, Path)) return S;
        }
        return nullptr;
    }

    UBlendSpace* LoadLocomotionBlendSpace()
    {
        return LoadObject<UBlendSpace>(nullptr, TEXT("/Game/Characters/Player/Animations/BS_Locomotion"));
    }

    /** Test asset cleanup. AnimBPs created during tests live at /Game/Characters/Player/Test/ and are
     *  removed on test completion so re-runs are idempotent. */
    void CleanupTestAsset(const FString& ObjectPath)
    {
        if (UEditorAssetLibrary::DoesAssetExist(ObjectPath))
        {
            UEditorAssetLibrary::DeleteAsset(ObjectPath);
        }
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPoFAnimBPAuthoringCreateTest,
    "PoFEditor.AnimBPAuthoring.Create",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoFAnimBPAuthoringCreateTest::RunTest(const FString& Parameters)
{
    USkeleton* Skel = PoFAnimBPAuth::LoadMannySkeleton();
    if (!TestNotNull(TEXT("Manny skeleton must exist"), Skel)) return false;

    const FString PackagePath = TEXT("/Game/Characters/Player/Test");
    const FString AssetName   = TEXT("ABP_PoFAuthCreateTest");
    PoFAnimBPAuth::CleanupTestAsset(PackagePath / AssetName + TEXT(".") + AssetName);

    UAnimBlueprint* ABP = UPoFAnimBPAuthoringLibrary::CreateAnimBlueprint(Skel, PackagePath, AssetName);
    TestNotNull(TEXT("ABP created"), ABP);
    if (ABP)
    {
        TestEqual(TEXT("target skeleton"), ABP->TargetSkeleton.Get(), Skel);

        // Idempotency: re-call returns same asset
        UAnimBlueprint* ABP2 = UPoFAnimBPAuthoringLibrary::CreateAnimBlueprint(Skel, PackagePath, AssetName);
        TestEqual(TEXT("idempotent: same asset on re-create"), ABP, ABP2);
    }

    PoFAnimBPAuth::CleanupTestAsset(PackagePath / AssetName + TEXT(".") + AssetName);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPoFAnimBPAuthoringStateMachineTest,
    "PoFEditor.AnimBPAuthoring.StateMachine",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoFAnimBPAuthoringStateMachineTest::RunTest(const FString& Parameters)
{
    USkeleton* Skel = PoFAnimBPAuth::LoadMannySkeleton();
    if (!TestNotNull(TEXT("skeleton"), Skel)) return false;

    const FString PackagePath = TEXT("/Game/Characters/Player/Test");
    const FString AssetName   = TEXT("ABP_PoFAuthSMTest");
    PoFAnimBPAuth::CleanupTestAsset(PackagePath / AssetName + TEXT(".") + AssetName);

    UAnimBlueprint* ABP = UPoFAnimBPAuthoringLibrary::CreateAnimBlueprint(Skel, PackagePath, AssetName);
    if (!TestNotNull(TEXT("ABP created"), ABP)) return false;

    TestTrue(TEXT("AddStateMachine ok"),
        UPoFAnimBPAuthoringLibrary::AddStateMachine(ABP, TEXT("Locomotion")));
    TestTrue(TEXT("AddStateMachine idempotent"),
        UPoFAnimBPAuthoringLibrary::AddStateMachine(ABP, TEXT("Locomotion")));

    PoFAnimBPAuth::CleanupTestAsset(PackagePath / AssetName + TEXT(".") + AssetName);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPoFAnimBPAuthoringFullRoundTripTest,
    "PoFEditor.AnimBPAuthoring.FullRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoFAnimBPAuthoringFullRoundTripTest::RunTest(const FString& Parameters)
{
    USkeleton* Skel = PoFAnimBPAuth::LoadMannySkeleton();
    UBlendSpace* BS = PoFAnimBPAuth::LoadLocomotionBlendSpace();
    if (!TestNotNull(TEXT("skeleton"), Skel)) return false;
    if (!TestNotNull(TEXT("blend space (run step 06 first or commit BS_Locomotion)"), BS)) return false;

    const FString PackagePath = TEXT("/Game/Characters/Player/Test");
    const FString AssetName   = TEXT("ABP_PoFAuthRoundTrip");
    PoFAnimBPAuth::CleanupTestAsset(PackagePath / AssetName + TEXT(".") + AssetName);

    UAnimBlueprint* ABP = UPoFAnimBPAuthoringLibrary::CreateAnimBlueprint(Skel, PackagePath, AssetName);
    if (!TestNotNull(TEXT("ABP created"), ABP)) return false;

    TestTrue(TEXT("state machine"), UPoFAnimBPAuthoringLibrary::AddStateMachine(ABP, TEXT("Loco")));
    TestTrue(TEXT("blend space state"),
        UPoFAnimBPAuthoringLibrary::AddBlendSpaceState(ABP, TEXT("Loco"), TEXT("Strafe"),
            BS, TEXT("Speed"), TEXT("Direction")));
    TestTrue(TEXT("default slot"),
        UPoFAnimBPAuthoringLibrary::AddDefaultSlot(ABP, TEXT("DefaultSlot")));
    TestTrue(TEXT("connect SM -> Slot -> Output"),
        UPoFAnimBPAuthoringLibrary::ConnectStateMachineToOutputPose(ABP, TEXT("Loco"), TEXT("DefaultSlot")));
    TestTrue(TEXT("compile + save"), UPoFAnimBPAuthoringLibrary::CompileAndSave(ABP));
    TestEqual(TEXT("compile status up to date"),
        static_cast<int32>(ABP->Status), static_cast<int32>(EBlueprintStatus::BS_UpToDate));

    PoFAnimBPAuth::CleanupTestAsset(PackagePath / AssetName + TEXT(".") + AssetName);
    return true;
}
