// Copyright PoF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PoFAnimBPAuthoringLibrary.generated.h"

class UAnimBlueprint;
class USkeleton;
class UBlendSpace;

/**
 * Procedural AnimBP authoring — exposes graph-mutation primitives to Python.
 *
 * Every method is idempotent and returns success/failure. The public surface is
 * deliberately small: just enough to assemble a Locomotion-state-machine ABP
 * with a default slot and a blend-space-evaluator inner state. Future ABPs
 * (enemies, ability instances) compose by reusing these primitives.
 *
 * Python usage:
 *   lib = unreal.PoFAnimBPAuthoringLibrary
 *   abp = lib.create_anim_blueprint(skel, "/Game/.../Player", "ABP_VSPlayer")
 *   lib.add_state_machine(abp, "Locomotion")
 *   lib.add_blend_space_state(abp, "Locomotion", "Strafe", bs, "Speed", "Direction")
 *   lib.add_default_slot(abp, "DefaultSlot")
 *   lib.connect_state_machine_to_output_pose(abp, "Locomotion", "DefaultSlot")
 *   lib.compile_and_save(abp)
 */
UCLASS()
class POFEDITOR_API UPoFAnimBPAuthoringLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Create (or fetch existing) an AnimBlueprint. Idempotent: returns the same asset on re-call. */
    UFUNCTION(BlueprintCallable, Category = "PoF|AnimBP", meta = (ScriptMethod))
    static UAnimBlueprint* CreateAnimBlueprint(USkeleton* Skeleton, const FString& PackagePath, const FString& AssetName);

    /** Add a named state machine to the AnimGraph. Idempotent on the name. */
    UFUNCTION(BlueprintCallable, Category = "PoF|AnimBP", meta = (ScriptMethod))
    static bool AddStateMachine(UAnimBlueprint* AnimBP, const FString& StateMachineName);

    /**
     * Add a state to the named state machine whose body is a BlendSpace evaluator,
     * driven by two named float variables (which are auto-created on the AnimBP if
     * they don't exist). Idempotent on the state name.
     */
    UFUNCTION(BlueprintCallable, Category = "PoF|AnimBP", meta = (ScriptMethod))
    static bool AddBlendSpaceState(UAnimBlueprint* AnimBP, const FString& StateMachineName, const FString& StateName,
        UBlendSpace* BlendSpace, const FString& SpeedVarName, const FString& DirectionVarName);

    /** Add a default slot node to the AnimGraph (for montage playback). Idempotent on the slot name. */
    UFUNCTION(BlueprintCallable, Category = "PoF|AnimBP", meta = (ScriptMethod))
    static bool AddDefaultSlot(UAnimBlueprint* AnimBP, const FString& SlotName);

    /** Rewire the AnimGraph so StateMachine -> Slot -> OutputPose. Idempotent (rewires cleanly). */
    UFUNCTION(BlueprintCallable, Category = "PoF|AnimBP", meta = (ScriptMethod))
    static bool ConnectStateMachineToOutputPose(UAnimBlueprint* AnimBP, const FString& StateMachineName, const FString& SlotName);

    /** Compile + save the AnimBP. Returns false if compilation failed (check ABP->Status). */
    UFUNCTION(BlueprintCallable, Category = "PoF|AnimBP", meta = (ScriptMethod))
    static bool CompileAndSave(UAnimBlueprint* AnimBP);
};
