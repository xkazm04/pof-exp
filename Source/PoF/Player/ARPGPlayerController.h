#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Input/ARPGInputTypes.h"
#include "ARPGPlayerController.generated.h"

struct FInputActionValue;
struct FInputKeyEventArgs;
class UInputAction;
class UInputMappingContext;
class UDamageNumberManagerComponent;
class UCombatFeedbackComponent;
class UPauseMenuWidget;

UCLASS()
class POF_API AARPGPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AARPGPlayerController();

	/** Damage number manager — spawns floating combat numbers. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UDamageNumberManagerComponent> DamageNumberManager;

	/** Combat feedback — camera shake, hitstop, hit VFX, hit SFX. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UCombatFeedbackComponent> CombatFeedback;

	/** Toggle the pause menu on/off. */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void TogglePauseMenu();

	/** Widget class to use for the pause menu. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UPauseMenuWidget> PauseMenuWidgetClass;

	// =====================================================================
	// Input Device Detection
	// =====================================================================

	/** Broadcast when the active input device changes (KB/M <-> Gamepad). */
	UPROPERTY(BlueprintAssignable, Category = "Input|Device")
	FOnInputDeviceChanged OnInputDeviceChanged;

	/** Get the most recently used input device type. */
	UFUNCTION(BlueprintPure, Category = "Input|Device")
	EARPGInputDevice GetCurrentInputDevice() const { return CurrentInputDevice; }

	/** True if the player is currently using a gamepad. */
	UFUNCTION(BlueprintPure, Category = "Input|Device")
	bool IsUsingGamepad() const { return CurrentInputDevice == EARPGInputDevice::Gamepad; }

	// =====================================================================
	// Key Rebinding
	// =====================================================================

	/**
	 * Remap an action's key binding. Preserves modifiers and triggers.
	 * Fails if NewKey is already bound to a different action (conflict).
	 * @return True if the old key was found and remapped.
	 */
	UFUNCTION(BlueprintCallable, Category = "Input|Rebinding")
	bool RemapAction(UInputAction* Action, FKey OldKey, FKey NewKey);

	/**
	 * Check if a key is already bound to any action in the default mapping context.
	 * @param ExcludeAction If provided, bindings on this action are not considered conflicts.
	 * @return The conflicting action, or nullptr if the key is free.
	 */
	UFUNCTION(BlueprintPure, Category = "Input|Rebinding")
	UInputAction* FindConflictingAction(FKey Key, UInputAction* ExcludeAction = nullptr) const;

	/** Get all keys currently bound to an action in the default mapping context. */
	UFUNCTION(BlueprintCallable, Category = "Input|Rebinding")
	TArray<FKey> GetBoundKeysForAction(const UInputAction* Action) const;

	/** Reset all bindings to their original defaults. */
	UFUNCTION(BlueprintCallable, Category = "Input|Rebinding")
	void ResetBindingsToDefault();

	/** Persist current key bindings to disk. */
	UFUNCTION(BlueprintCallable, Category = "Input|Rebinding")
	void SaveInputBindings();

	/** Load and apply saved key bindings from disk. */
	UFUNCTION(BlueprintCallable, Category = "Input|Rebinding")
	void LoadInputBindings();

	// =====================================================================
	// Input Context Stacking
	// =====================================================================

	/** Push an additional input mapping context (e.g., vehicle, swimming, menu). */
	UFUNCTION(BlueprintCallable, Category = "Input|Context")
	void PushInputContext(UInputMappingContext* Context, int32 Priority = 1);

	/** Remove a previously pushed input mapping context. */
	UFUNCTION(BlueprintCallable, Category = "Input|Context")
	void PopInputContext(UInputMappingContext* Context);

	// =====================================================================
	// Input Mode Management
	// =====================================================================

	/** Switch to game-only input mode (hides cursor). */
	UFUNCTION(BlueprintCallable, Category = "Input|Mode")
	void SetInputModeGameOnly();

	/** Switch to UI-only input mode (shows cursor, blocks game input). */
	UFUNCTION(BlueprintCallable, Category = "Input|Mode")
	void SetInputModeUIOnly();

	/** Switch to game + UI input mode (shows cursor, both receive input). */
	UFUNCTION(BlueprintCallable, Category = "Input|Mode")
	void SetInputModeGameAndUI();

	// =====================================================================
	// Input Action Access (for rebinding UI)
	// =====================================================================

	/** Find an input action by name. Returns nullptr if not found. */
	UFUNCTION(BlueprintPure, Category = "Input")
	UInputAction* FindInputActionByName(FName ActionName) const;

	/** Get the default mapping context. */
	UFUNCTION(BlueprintPure, Category = "Input")
	UInputMappingContext* GetDefaultMappingContext() const { return DefaultMappingContext; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual bool InputKey(const FInputKeyEventArgs& Params) override;

	// --- Input Actions (assign in BP or constructor) ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Move;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Look;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Interact;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_PrimaryAttack;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Dodge;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Sprint;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Zoom;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_RotateCameraLeft;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_RotateCameraRight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_RotateCameraAnalog;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_UsePotion;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Pause;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Inventory;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_CharacterStats;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_QuestLog;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_SkillTree;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_AbilitySlot1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_AbilitySlot2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_AbilitySlot3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_AbilitySlot4;

	// --- Gamepad Tuning ---

	/** Deadzone for the left analog stick (movement). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Gamepad")
	float GamepadLeftStickDeadzone = 0.2f;

	/** Deadzone for the right analog stick (camera). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Gamepad")
	float GamepadRightStickDeadzone = 0.15f;

	/** Camera rotation speed (deg/sec) when using the right analog stick. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Gamepad")
	float GamepadCameraRotationSpeed = 180.f;

private:
	// --- Input handlers ---
	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleInteract(const FInputActionValue& Value);
	void HandlePrimaryAttack(const FInputActionValue& Value);
	void HandleDodge(const FInputActionValue& Value);
	void HandleSprintStart(const FInputActionValue& Value);
	void HandleSprintStop(const FInputActionValue& Value);
	void HandleZoom(const FInputActionValue& Value);
	void HandleRotateCameraLeft(const FInputActionValue& Value);
	void HandleRotateCameraRight(const FInputActionValue& Value);
	void HandleRotateCameraAnalog(const FInputActionValue& Value);
	void HandleUsePotion(const FInputActionValue& Value);
	void HandlePause(const FInputActionValue& Value);
	void HandleInventory(const FInputActionValue& Value);
	void HandleCharacterStats(const FInputActionValue& Value);
	void HandleQuestLog(const FInputActionValue& Value);
	void HandleSkillTree(const FInputActionValue& Value);
	void HandleAbilitySlot(int32 SlotIndex);
	void HandleAbilitySlot1(const FInputActionValue& Value);
	void HandleAbilitySlot2(const FInputActionValue& Value);
	void HandleAbilitySlot3(const FInputActionValue& Value);
	void HandleAbilitySlot4(const FInputActionValue& Value);

	// --- Pause menu ---
	UPROPERTY()
	TObjectPtr<UPauseMenuWidget> PauseMenuInstance;

	UFUNCTION()
	void OnResumeFromPauseMenu();

	// --- Input device detection ---
	EARPGInputDevice CurrentInputDevice = EARPGInputDevice::KeyboardMouse;
	bool bUIInputModeActive = false;

	// --- Key rebinding ---
	TArray<FARPGKeyRebind> AppliedRebinds;
	TMap<FName, TObjectPtr<UInputAction>> ActionLookup;

	/** Internal remap without tracking for save (used by load/reset). */
	bool RemapActionInternal(UInputAction* Action, FKey OldKey, FKey NewKey);

	/** Refresh the Enhanced Input subsystem after mapping changes. */
	void RefreshMappingContext();

	/** Register an input action in the action lookup map. */
	void RegisterAction(UInputAction* Action);
};
