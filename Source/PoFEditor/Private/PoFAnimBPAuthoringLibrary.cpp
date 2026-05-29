// Copyright PoF. All Rights Reserved.

#include "PoFAnimBPAuthoringLibrary.h"

#include "Animation/AnimBlueprint.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/BlendSpace.h"
#include "Animation/Skeleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "EdGraphSchema_K2.h"
#include "EdGraphUtilities.h"
#include "Factories/AnimBlueprintFactory.h"
#include "IAssetTools.h"
#include "K2Node_VariableGet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

// AnimGraph node types
#include "AnimGraphNode_BlendSpaceEvaluator.h"
#include "AnimGraphNode_BlendSpacePlayer.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_Slot.h"
#include "AnimGraphNode_StateMachine.h"
#include "AnimStateNode.h"
#include "AnimStateEntryNode.h"
#include "AnimationGraph.h"
#include "AnimationStateGraph.h"
#include "AnimationStateMachineGraph.h"
#include "AnimationStateMachineSchema.h"

namespace
{
    /** Find the AnimBP's main AnimGraph by canonical name. */
    UEdGraph* FindAnimGraph(UAnimBlueprint* AnimBP)
    {
        if (!AnimBP) return nullptr;
        for (UEdGraph* G : AnimBP->FunctionGraphs)
        {
            if (G && G->GetFName() == UEdGraphSchema_K2::GN_AnimGraph) return G;
        }
        return nullptr;
    }

    /** Find a pin by name (case-insensitive on PinName.ToString). */
    UEdGraphPin* FindPin(UEdGraphNode* Node, const TCHAR* Name)
    {
        if (!Node) return nullptr;
        for (UEdGraphPin* P : Node->Pins)
        {
            if (P && P->PinName.ToString().Equals(Name, ESearchCase::IgnoreCase)) return P;
        }
        return nullptr;
    }

    UAnimGraphNode_StateMachine* FindStateMachineByName(UEdGraph* AnimGraph, const FString& Name)
    {
        if (!AnimGraph) return nullptr;
        for (UEdGraphNode* N : AnimGraph->Nodes)
        {
            if (auto* SM = Cast<UAnimGraphNode_StateMachine>(N))
            {
                if (SM->EditorStateMachineGraph && SM->EditorStateMachineGraph->GetName() == Name) return SM;
            }
        }
        return nullptr;
    }

    UAnimGraphNode_Slot* FindSlotByName(UEdGraph* AnimGraph, const FString& Name)
    {
        if (!AnimGraph) return nullptr;
        for (UEdGraphNode* N : AnimGraph->Nodes)
        {
            if (auto* S = Cast<UAnimGraphNode_Slot>(N))
            {
                if (S->Node.SlotName == FName(*Name)) return S;
            }
        }
        return nullptr;
    }

    UAnimGraphNode_Root* FindResultRoot(UEdGraph* AnimGraph)
    {
        if (!AnimGraph) return nullptr;
        for (UEdGraphNode* N : AnimGraph->Nodes)
        {
            if (auto* R = Cast<UAnimGraphNode_Root>(N)) return R;
        }
        return nullptr;
    }

    template <typename TNode>
    TNode* SpawnAnimGraphNode(UEdGraph* InGraph, int32 X = 0, int32 Y = 0)
    {
        TNode* Node = NewObject<TNode>(InGraph);
        Node->CreateNewGuid();
        Node->NodePosX = X;
        Node->NodePosY = Y;
        InGraph->AddNode(Node, /*bUserAction*/ false, /*bSelectNewNode*/ false);
        Node->PostPlacedNewNode();
        Node->AllocateDefaultPins();
        return Node;
    }
}

// =====================================================================
// CreateAnimBlueprint
// =====================================================================

UAnimBlueprint* UPoFAnimBPAuthoringLibrary::CreateAnimBlueprint(USkeleton* Skeleton,
    const FString& PackagePath, const FString& AssetName, const FString& ParentClassPath)
{
    if (!Skeleton) return nullptr;

    const FString ObjectPath = PackagePath / AssetName + TEXT(".") + AssetName;
    if (UAnimBlueprint* Existing = LoadObject<UAnimBlueprint>(nullptr, *ObjectPath))
    {
        return Existing;
    }

    // Resolve the parent AnimInstance class. A custom parent (e.g.
    // /Script/PoF.ARPGAnimInstance) lets the AnimBP inherit C++-computed locomotion
    // variables (Speed, Direction, ...) so the blend space is driven by movement
    // without any EventGraph logic.
    UClass* ParentClass = UAnimInstance::StaticClass();
    if (!ParentClassPath.IsEmpty())
    {
        if (UClass* Loaded = LoadObject<UClass>(nullptr, *ParentClassPath))
        {
            if (Loaded->IsChildOf(UAnimInstance::StaticClass()))
            {
                ParentClass = Loaded;
            }
        }
    }

    FAssetToolsModule& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));

    UAnimBlueprintFactory* Factory = NewObject<UAnimBlueprintFactory>();
    Factory->TargetSkeleton = Skeleton;
    Factory->ParentClass = ParentClass;

    UObject* New = AssetTools.Get().CreateAsset(AssetName, PackagePath, UAnimBlueprint::StaticClass(), Factory);
    return Cast<UAnimBlueprint>(New);
}

// =====================================================================
// AddStateMachine
// =====================================================================

bool UPoFAnimBPAuthoringLibrary::AddStateMachine(UAnimBlueprint* AnimBP, const FString& StateMachineName)
{
    UEdGraph* AnimGraph = FindAnimGraph(AnimBP);
    if (!AnimGraph) return false;

    if (FindStateMachineByName(AnimGraph, StateMachineName))
    {
        return true; // idempotent
    }

    UAnimGraphNode_StateMachine* SMNode = SpawnAnimGraphNode<UAnimGraphNode_StateMachine>(AnimGraph, 0, 0);

    // Inner graph for the state machine
    UEdGraph* SMGraph = FBlueprintEditorUtils::CreateNewGraph(AnimBP, FName(*StateMachineName),
        UAnimationStateMachineGraph::StaticClass(), UAnimationStateMachineSchema::StaticClass());
    if (!SMGraph) return false;

    SMNode->EditorStateMachineGraph = Cast<UAnimationStateMachineGraph>(SMGraph);
    AnimBP->FunctionGraphs.Add(SMGraph);

    FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBP);
    return true;
}

// =====================================================================
// AddBlendSpaceState
// =====================================================================

bool UPoFAnimBPAuthoringLibrary::AddBlendSpaceState(UAnimBlueprint* AnimBP,
    const FString& StateMachineName, const FString& StateName, UBlendSpace* BlendSpace,
    const FString& SpeedVarName, const FString& DirectionVarName)
{
    if (!AnimBP || !BlendSpace) return false;
    UEdGraph* AnimGraph = FindAnimGraph(AnimBP);
    UAnimGraphNode_StateMachine* SMNode = FindStateMachineByName(AnimGraph, StateMachineName);
    if (!SMNode || !SMNode->EditorStateMachineGraph) return false;
    UEdGraph* SMGraph = SMNode->EditorStateMachineGraph;

    // Idempotency: state of that name already?
    for (UEdGraphNode* N : SMGraph->Nodes)
    {
        if (auto* S = Cast<UAnimStateNode>(N))
        {
            if (S->GetStateName() == StateName) return true;
        }
    }

    // Ensure the two driver variables exist on the AnimBP.
    auto EnsureFloatVar = [AnimBP](const FString& VarName)
    {
        FEdGraphPinType FloatPin;
        FloatPin.PinCategory = UEdGraphSchema_K2::PC_Real;
        FloatPin.PinSubCategory = UEdGraphSchema_K2::PC_Float;
        FBlueprintEditorUtils::AddMemberVariable(AnimBP, FName(*VarName), FloatPin);
    };
    EnsureFloatVar(SpeedVarName);
    EnsureFloatVar(DirectionVarName);

    // Create the state node. UAnimStateNode has no settable StateName member —
    // the state's name is its BoundGraph's name, so we rename the bound graph
    // (created during PostPlacedNewNode) to set it. GetStateName() then matches.
    UAnimStateNode* State = NewObject<UAnimStateNode>(SMGraph);
    State->CreateNewGuid();
    State->NodePosX = 100;
    State->NodePosY = 100;
    SMGraph->AddNode(State, false, false);
    State->PostPlacedNewNode();
    State->AllocateDefaultPins();

    if (!State->BoundGraph) return false;
    UEdGraph* StateGraph = State->BoundGraph;
    FBlueprintEditorUtils::RenameGraph(StateGraph, *StateName);

    // Connect the state machine's Entry node to this state (the initial/default state).
    // Without this the compiler warns "no entry state connection" and the SM produces
    // no pose at runtime. Only wire it if the entry isn't already connected.
    if (UAnimationStateMachineGraph* SMG = Cast<UAnimationStateMachineGraph>(SMGraph))
    {
        if (SMG->EntryNode)
        {
            UEdGraphPin* EntryOut = SMG->EntryNode->GetOutputPin();
            UEdGraphPin* StateIn = State->GetInputPin();
            if (EntryOut && StateIn && EntryOut->LinkedTo.Num() == 0)
            {
                EntryOut->MakeLinkTo(StateIn);
            }
        }
    }

    // Spawn the BlendSpace evaluator
    UAnimGraphNode_BlendSpaceEvaluator* BSEval = SpawnAnimGraphNode<UAnimGraphNode_BlendSpaceEvaluator>(StateGraph, 0, 0);
    BSEval->Node.SetBlendSpace(BlendSpace);

    // Variable getters
    auto SpawnGetter = [StateGraph](FName VarName) -> UK2Node_VariableGet*
    {
        UK2Node_VariableGet* Getter = NewObject<UK2Node_VariableGet>(StateGraph);
        Getter->VariableReference.SetSelfMember(VarName);
        Getter->CreateNewGuid();
        StateGraph->AddNode(Getter, false, false);
        Getter->PostPlacedNewNode();
        Getter->AllocateDefaultPins();
        return Getter;
    };
    UK2Node_VariableGet* SpeedGet = SpawnGetter(FName(*SpeedVarName));
    UK2Node_VariableGet* DirGet   = SpawnGetter(FName(*DirectionVarName));

    UEdGraphSchema_K2 const* Sch = GetDefault<UEdGraphSchema_K2>();
    if (UEdGraphPin* XPin = FindPin(BSEval, TEXT("X")))
    {
        UEdGraphPin* DirOut = (DirGet && DirGet->Pins.Num() > 0) ? DirGet->Pins[0] : nullptr;
        if (DirOut) Sch->TryCreateConnection(XPin, DirOut);
    }
    if (UEdGraphPin* YPin = FindPin(BSEval, TEXT("Y")))
    {
        UEdGraphPin* SpeedOut = (SpeedGet && SpeedGet->Pins.Num() > 0) ? SpeedGet->Pins[0] : nullptr;
        if (SpeedOut) Sch->TryCreateConnection(YPin, SpeedOut);
    }

    // Wire BSEval output -> state graph's Result/Root.
    if (UAnimGraphNode_Root* ResultRoot = FindResultRoot(StateGraph))
    {
        UEdGraphPin* OutPose = FindPin(BSEval, TEXT("Pose"));
        UEdGraphPin* ResultIn = FindPin(ResultRoot, TEXT("Result"));
        if (OutPose && ResultIn) Sch->TryCreateConnection(OutPose, ResultIn);
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBP);
    return true;
}

// =====================================================================
// AddDefaultSlot
// =====================================================================

bool UPoFAnimBPAuthoringLibrary::AddDefaultSlot(UAnimBlueprint* AnimBP, const FString& SlotName)
{
    UEdGraph* AnimGraph = FindAnimGraph(AnimBP);
    if (!AnimGraph) return false;

    if (FindSlotByName(AnimGraph, SlotName)) return true; // idempotent

    UAnimGraphNode_Slot* Slot = SpawnAnimGraphNode<UAnimGraphNode_Slot>(AnimGraph, 200, 0);
    Slot->Node.SlotName = FName(*SlotName);

    FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBP);
    return true;
}

// =====================================================================
// AddBlendSpacePlayerToOutput  (no state machine — bulletproof for locomotion)
// =====================================================================

bool UPoFAnimBPAuthoringLibrary::AddBlendSpacePlayerToOutput(UAnimBlueprint* AnimBP,
    UBlendSpace* BlendSpace, const FString& SpeedVarName, const FString& DirectionVarName,
    const FString& SlotName)
{
    if (!AnimBP || !BlendSpace) return false;
    UEdGraph* AnimGraph = FindAnimGraph(AnimBP);
    if (!AnimGraph) return false;
    UAnimGraphNode_Root* Root = FindResultRoot(AnimGraph);
    if (!Root) return false;

    // Ensure the two driver float variables exist on the AnimBP — UNLESS the parent
    // class already provides them (e.g. UARPGAnimInstance::Speed/Direction). Creating a
    // member var that shadows an inherited property breaks the getter resolution.
    auto EnsureFloatVar = [AnimBP](const FString& VarName)
    {
        if (AnimBP->ParentClass && FindFProperty<FProperty>(AnimBP->ParentClass, FName(*VarName)))
        {
            return; // inherited from the parent AnimInstance — the getter resolves to it
        }
        FEdGraphPinType FloatPin;
        FloatPin.PinCategory = UEdGraphSchema_K2::PC_Real;
        FloatPin.PinSubCategory = UEdGraphSchema_K2::PC_Float;
        FBlueprintEditorUtils::AddMemberVariable(AnimBP, FName(*VarName), FloatPin);
    };
    EnsureFloatVar(SpeedVarName);
    EnsureFloatVar(DirectionVarName);

    // BlendSpace player node, pins regenerated after the blend space is set so the
    // axis pins are named after the BS axes (e.g. "Direction"/"Speed").
    UAnimGraphNode_BlendSpacePlayer* BSP = SpawnAnimGraphNode<UAnimGraphNode_BlendSpacePlayer>(AnimGraph, -400, 0);
    BSP->Node.SetBlendSpace(BlendSpace);
    BSP->ReconstructNode();

    auto SpawnGetter = [AnimGraph](FName VarName) -> UK2Node_VariableGet*
    {
        UK2Node_VariableGet* Getter = NewObject<UK2Node_VariableGet>(AnimGraph);
        Getter->VariableReference.SetSelfMember(VarName);
        Getter->CreateNewGuid();
        AnimGraph->AddNode(Getter, false, false);
        Getter->PostPlacedNewNode();
        Getter->AllocateDefaultPins();
        return Getter;
    };
    UK2Node_VariableGet* SpeedGet = SpawnGetter(FName(*SpeedVarName));
    UK2Node_VariableGet* DirGet   = SpawnGetter(FName(*DirectionVarName));

    UEdGraphSchema_K2 const* Sch = GetDefault<UEdGraphSchema_K2>();
    auto ConnectVar = [&](UK2Node_VariableGet* Getter, const FString& PinName, const TCHAR* Fallback)
    {
        UEdGraphPin* Pin = FindPin(BSP, *PinName);
        if (!Pin) Pin = FindPin(BSP, Fallback);
        if (Pin && Getter && Getter->Pins.Num() > 0) Sch->TryCreateConnection(Pin, Getter->Pins[0]);
    };
    ConnectVar(DirGet, DirectionVarName, TEXT("X"));
    ConnectVar(SpeedGet, SpeedVarName, TEXT("Y"));

    // Slot node (find or create) for montage layering.
    UAnimGraphNode_Slot* Slot = FindSlotByName(AnimGraph, SlotName);
    if (!Slot)
    {
        Slot = SpawnAnimGraphNode<UAnimGraphNode_Slot>(AnimGraph, -150, 0);
        Slot->Node.SlotName = FName(*SlotName);
    }

    // Wire BlendSpacePlayer.Pose -> Slot.Source -> Root, using ROBUST pose-pin lookup:
    // anim-node pose pins are struct-typed (the X/Y data pins are float), and the pose
    // OUTPUT is the node's GetOutputPin(). FindPin-by-name silently failed before,
    // leaving Root unconnected -> ref pose -> the T-pose. MakeLinkTo is the direct,
    // non-rejecting link for pose pins.
    auto PosePin = [](UEdGraphNode* N, EEdGraphPinDirection Dir) -> UEdGraphPin*
    {
        for (UEdGraphPin* P : N->Pins)
        {
            if (P->Direction == Dir && P->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
            {
                return P;
            }
        }
        return nullptr;
    };
    UEdGraphPin* BspOut  = PosePin(BSP, EGPD_Output);
    UEdGraphPin* SlotIn  = PosePin(Slot, EGPD_Input);
    UEdGraphPin* SlotOut = PosePin(Slot, EGPD_Output);
    UEdGraphPin* RootIn  = PosePin(Root, EGPD_Input);
    if (SlotIn)  SlotIn->BreakAllPinLinks();
    if (RootIn)  RootIn->BreakAllPinLinks();
    if (BspOut && SlotIn)  BspOut->MakeLinkTo(SlotIn);
    if (SlotOut && RootIn) SlotOut->MakeLinkTo(RootIn);

    FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBP);
    return true;
}

// =====================================================================
// ConnectStateMachineToOutputPose
// =====================================================================

bool UPoFAnimBPAuthoringLibrary::ConnectStateMachineToOutputPose(UAnimBlueprint* AnimBP,
    const FString& StateMachineName, const FString& SlotName)
{
    UEdGraph* AnimGraph = FindAnimGraph(AnimBP);
    if (!AnimGraph) return false;

    UAnimGraphNode_StateMachine* SMNode = FindStateMachineByName(AnimGraph, StateMachineName);
    UAnimGraphNode_Slot*         SlotNode = FindSlotByName(AnimGraph, SlotName);
    UAnimGraphNode_Root*         ResultNode = FindResultRoot(AnimGraph);
    if (!SMNode || !SlotNode || !ResultNode) return false;

    UEdGraphSchema_K2 const* Sch = GetDefault<UEdGraphSchema_K2>();

    // The state machine node exposes its output via the "Pose" / "Result" output pin.
    UEdGraphPin* SMOut   = FindPin(SMNode,    TEXT("Pose"));
    if (!SMOut) SMOut    = FindPin(SMNode,    TEXT("Result"));
    UEdGraphPin* SlotIn  = FindPin(SlotNode,  TEXT("Source"));
    UEdGraphPin* SlotOut = FindPin(SlotNode,  TEXT("Pose"));
    if (!SlotOut) SlotOut = FindPin(SlotNode, TEXT("Result"));
    UEdGraphPin* ResIn   = FindPin(ResultNode,TEXT("Result"));
    if (!ResIn) ResIn    = FindPin(ResultNode,TEXT("Pose"));
    if (!SMOut || !SlotIn || !SlotOut || !ResIn) return false;

    // Idempotent re-wiring: break any prior links on the inputs first.
    SlotIn->BreakAllPinLinks();
    ResIn->BreakAllPinLinks();
    Sch->TryCreateConnection(SMOut, SlotIn);
    Sch->TryCreateConnection(SlotOut, ResIn);

    FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBP);
    return true;
}

// =====================================================================
// CompileAndSave
// =====================================================================

bool UPoFAnimBPAuthoringLibrary::CompileAndSave(UAnimBlueprint* AnimBP)
{
    if (!AnimBP) return false;

    FKismetEditorUtilities::CompileBlueprint(AnimBP, EBlueprintCompileOptions::None);
    if (AnimBP->Status != EBlueprintStatus::BS_UpToDate &&
        AnimBP->Status != EBlueprintStatus::BS_UpToDateWithWarnings)
    {
        return false;
    }

    UPackage* Pkg = AnimBP->GetOutermost();
    if (!Pkg) return false;

    const FString PkgFile = FPackageName::LongPackageNameToFilename(
        Pkg->GetName(), FPackageName::GetAssetPackageExtension());

    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Standalone | RF_Public;
    Args.SaveFlags = SAVE_NoError;
    return UPackage::SavePackage(Pkg, AnimBP, *PkgFile, Args);
}
