#include "ARPGPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputActionValue.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputKeyEventArgs.h"
#include "Character/ARPGCharacterBase.h"
#include "Player/ARPGPlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "Inventory/ARPGInventoryComponent.h"
#include "UI/DamageNumberManagerComponent.h"
#include "Combat/CombatFeedbackComponent.h"
#include "UI/PauseMenuWidget.h"
#include "UI/ARPGHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Debug/ARPGCheatManager.h"

// Helper — create a UInputAction with the given value type as a default sub-object.
static UInputAction* CreateIA(UObject* Outer, const TCHAR* Name, EInputActionValueType ValueType)
{
	UInputAction* Action = Outer->CreateDefaultSubobject<UInputAction>(Name);
	Action->ValueType = ValueType;
	return Action;
}

AARPGPlayerController::AARPGPlayerController()
{
	CheatClass = UARPGCheatManager::StaticClass();

	// --- Damage Numbers ---
	DamageNumberManager = CreateDefaultSubobject<UDamageNumberManagerComponent>(TEXT("DamageNumberManager"));

	// --- Combat Feedback ---
	CombatFeedback = CreateDefaultSubobject<UCombatFeedbackComponent>(TEXT("CombatFeedback"));

	// --- Build Input Actions as default sub-objects ---
	IA_Move          = CreateIA(this, TEXT("IA_Move"),          EInputActionValueType::Axis2D);
	IA_Look          = CreateIA(this, TEXT("IA_Look"),          EInputActionValueType::Axis2D);
	IA_Interact      = CreateIA(this, TEXT("IA_Interact"),      EInputActionValueType::Boolean);
	IA_PrimaryAttack = CreateIA(this, TEXT("IA_PrimaryAttack"), EInputActionValueType::Boolean);
	IA_Dodge         = CreateIA(this, TEXT("IA_Dodge"),          EInputActionValueType::Boolean);
	IA_Sprint        = CreateIA(this, TEXT("IA_Sprint"),        EInputActionValueType::Boolean);
	IA_Zoom          = CreateIA(this, TEXT("IA_Zoom"),          EInputActionValueType::Axis1D);
	IA_RotateCameraLeft  = CreateIA(this, TEXT("IA_RotateCameraLeft"),  EInputActionValueType::Boolean);
	IA_RotateCameraRight = CreateIA(this, TEXT("IA_RotateCameraRight"), EInputActionValueType::Boolean);
	IA_RotateCameraAnalog = CreateIA(this, TEXT("IA_RotateCameraAnalog"), EInputActionValueType::Axis1D);
	IA_UsePotion         = CreateIA(this, TEXT("IA_UsePotion"),         EInputActionValueType::Boolean);
	IA_Pause             = CreateIA(this, TEXT("IA_Pause"),             EInputActionValueType::Boolean);
	IA_AbilitySlot1      = CreateIA(this, TEXT("IA_AbilitySlot1"),      EInputActionValueType::Boolean);
	IA_AbilitySlot2      = CreateIA(this, TEXT("IA_AbilitySlot2"),      EInputActionValueType::Boolean);
	IA_AbilitySlot3      = CreateIA(this, TEXT("IA_AbilitySlot3"),      EInputActionValueType::Boolean);
	IA_AbilitySlot4      = CreateIA(this, TEXT("IA_AbilitySlot4"),      EInputActionValueType::Boolean);
	IA_Inventory         = CreateIA(this, TEXT("IA_Inventory"),         EInputActionValueType::Boolean);
	IA_CharacterStats    = CreateIA(this, TEXT("IA_CharacterStats"),    EInputActionValueType::Boolean);
	IA_QuestLog          = CreateIA(this, TEXT("IA_QuestLog"),          EInputActionValueType::Boolean);
	IA_SkillTree         = CreateIA(this, TEXT("IA_SkillTree"),         EInputActionValueType::Boolean);

	// --- Register all actions for lookup by name (used by rebinding system) ---
	RegisterAction(IA_Move);
	RegisterAction(IA_Look);
	RegisterAction(IA_Interact);
	RegisterAction(IA_PrimaryAttack);
	RegisterAction(IA_Dodge);
	RegisterAction(IA_Sprint);
	RegisterAction(IA_Zoom);
	RegisterAction(IA_RotateCameraLeft);
	RegisterAction(IA_RotateCameraRight);
	RegisterAction(IA_RotateCameraAnalog);
	RegisterAction(IA_UsePotion);
	RegisterAction(IA_Pause);
	RegisterAction(IA_AbilitySlot1);
	RegisterAction(IA_AbilitySlot2);
	RegisterAction(IA_AbilitySlot3);
	RegisterAction(IA_AbilitySlot4);
	RegisterAction(IA_Inventory);
	RegisterAction(IA_CharacterStats);
	RegisterAction(IA_QuestLog);
	RegisterAction(IA_SkillTree);

	// --- Build the default mapping context ---
	DefaultMappingContext = CreateDefaultSubobject<UInputMappingContext>(TEXT("IMC_Default"));

	// =====================================================================
	// Keyboard + Mouse Bindings
	// =====================================================================

	// WASD → IA_Move  (Swizzle YX so W/S map to Y, A/D map to X, then Negate for S/A)
	{
		FEnhancedActionKeyMapping& MapW = DefaultMappingContext->MapKey(IA_Move, EKeys::W);
		UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(DefaultMappingContext);
		Swizzle->Order = EInputAxisSwizzle::YXZ;
		MapW.Modifiers.Add(Swizzle);

		FEnhancedActionKeyMapping& MapS = DefaultMappingContext->MapKey(IA_Move, EKeys::S);
		UInputModifierSwizzleAxis* SwizzleS = NewObject<UInputModifierSwizzleAxis>(DefaultMappingContext);
		SwizzleS->Order = EInputAxisSwizzle::YXZ;
		UInputModifierNegate* NegS = NewObject<UInputModifierNegate>(DefaultMappingContext);
		MapS.Modifiers.Add(SwizzleS);
		MapS.Modifiers.Add(NegS);

		DefaultMappingContext->MapKey(IA_Move, EKeys::D);

		FEnhancedActionKeyMapping& MapA = DefaultMappingContext->MapKey(IA_Move, EKeys::A);
		UInputModifierNegate* NegA = NewObject<UInputModifierNegate>(DefaultMappingContext);
		MapA.Modifiers.Add(NegA);
	}

	// Mouse XY → IA_Look
	DefaultMappingContext->MapKey(IA_Look, EKeys::Mouse2D);

	// F → IA_Interact
	DefaultMappingContext->MapKey(IA_Interact, EKeys::F);

	// Left Click → IA_PrimaryAttack
	DefaultMappingContext->MapKey(IA_PrimaryAttack, EKeys::LeftMouseButton);

	// Space → IA_Dodge
	DefaultMappingContext->MapKey(IA_Dodge, EKeys::SpaceBar);

	// Left Shift → IA_Sprint
	DefaultMappingContext->MapKey(IA_Sprint, EKeys::LeftShift);

	// Mouse Wheel → IA_Zoom
	DefaultMappingContext->MapKey(IA_Zoom, EKeys::MouseWheelAxis);

	// Q/E → Camera Rotation
	DefaultMappingContext->MapKey(IA_RotateCameraLeft, EKeys::Q);
	DefaultMappingContext->MapKey(IA_RotateCameraRight, EKeys::E);

	// 1-4 → Ability Hotbar Slots
	DefaultMappingContext->MapKey(IA_AbilitySlot1, EKeys::One);
	DefaultMappingContext->MapKey(IA_AbilitySlot2, EKeys::Two);
	DefaultMappingContext->MapKey(IA_AbilitySlot3, EKeys::Three);
	DefaultMappingContext->MapKey(IA_AbilitySlot4, EKeys::Four);

	// 5 → Use Potion
	DefaultMappingContext->MapKey(IA_UsePotion, EKeys::Five);

	// Escape → Pause Menu
	DefaultMappingContext->MapKey(IA_Pause, EKeys::Escape);

	// I → Inventory Toggle
	DefaultMappingContext->MapKey(IA_Inventory, EKeys::I);

	// C → Character Stats Toggle
	DefaultMappingContext->MapKey(IA_CharacterStats, EKeys::C);

	// J → Quest Log Toggle
	DefaultMappingContext->MapKey(IA_QuestLog, EKeys::J);

	// K → Skill Tree Toggle
	DefaultMappingContext->MapKey(IA_SkillTree, EKeys::K);

	// =====================================================================
	// Gamepad Bindings
	// =====================================================================

	// Left Stick → IA_Move (with deadzone)
	{
		// Left Stick X → Move X axis
		FEnhancedActionKeyMapping& MapLX = DefaultMappingContext->MapKey(IA_Move, EKeys::Gamepad_LeftX);
		UInputModifierDeadZone* DZLX = NewObject<UInputModifierDeadZone>(DefaultMappingContext);
		DZLX->LowerThreshold = GamepadLeftStickDeadzone;
		DZLX->Type = EDeadZoneType::Axial;
		MapLX.Modifiers.Add(DZLX);

		// Left Stick Y → Move Y axis (swizzle so Y input maps to forward/backward)
		FEnhancedActionKeyMapping& MapLY = DefaultMappingContext->MapKey(IA_Move, EKeys::Gamepad_LeftY);
		UInputModifierDeadZone* DZLY = NewObject<UInputModifierDeadZone>(DefaultMappingContext);
		DZLY->LowerThreshold = GamepadLeftStickDeadzone;
		DZLY->Type = EDeadZoneType::Axial;
		UInputModifierSwizzleAxis* SwizzleLY = NewObject<UInputModifierSwizzleAxis>(DefaultMappingContext);
		SwizzleLY->Order = EInputAxisSwizzle::YXZ;
		MapLY.Modifiers.Add(DZLY);
		MapLY.Modifiers.Add(SwizzleLY);
	}

	// Right Stick X → IA_RotateCameraAnalog (smooth camera rotation)
	{
		FEnhancedActionKeyMapping& MapRX = DefaultMappingContext->MapKey(IA_RotateCameraAnalog, EKeys::Gamepad_RightX);
		UInputModifierDeadZone* DZRX = NewObject<UInputModifierDeadZone>(DefaultMappingContext);
		DZRX->LowerThreshold = GamepadRightStickDeadzone;
		DZRX->Type = EDeadZoneType::Axial;
		MapRX.Modifiers.Add(DZRX);
	}

	// Right Stick Y → IA_Zoom (negate so stick-up = zoom in)
	{
		FEnhancedActionKeyMapping& MapRY = DefaultMappingContext->MapKey(IA_Zoom, EKeys::Gamepad_RightY);
		UInputModifierDeadZone* DZRY = NewObject<UInputModifierDeadZone>(DefaultMappingContext);
		DZRY->LowerThreshold = GamepadRightStickDeadzone;
		DZRY->Type = EDeadZoneType::Axial;
		UInputModifierNegate* NegRY = NewObject<UInputModifierNegate>(DefaultMappingContext);
		MapRY.Modifiers.Add(DZRY);
		MapRY.Modifiers.Add(NegRY);
	}

	// Face Buttons
	DefaultMappingContext->MapKey(IA_PrimaryAttack, EKeys::Gamepad_FaceButton_Left);    // X / Square
	DefaultMappingContext->MapKey(IA_Dodge, EKeys::Gamepad_FaceButton_Bottom);           // A / Cross
	DefaultMappingContext->MapKey(IA_Interact, EKeys::Gamepad_FaceButton_Top);           // Y / Triangle
	DefaultMappingContext->MapKey(IA_UsePotion, EKeys::Gamepad_FaceButton_Right);        // B / Circle

	// Shoulders
	DefaultMappingContext->MapKey(IA_RotateCameraLeft, EKeys::Gamepad_LeftShoulder);     // LB
	DefaultMappingContext->MapKey(IA_RotateCameraRight, EKeys::Gamepad_RightShoulder);   // RB

	// Triggers
	DefaultMappingContext->MapKey(IA_Sprint, EKeys::Gamepad_LeftTriggerAxis);            // LT = Sprint
	DefaultMappingContext->MapKey(IA_PrimaryAttack, EKeys::Gamepad_RightTriggerAxis);    // RT = Attack (alt)

	// D-Pad → Ability Slots
	DefaultMappingContext->MapKey(IA_AbilitySlot1, EKeys::Gamepad_DPad_Up);
	DefaultMappingContext->MapKey(IA_AbilitySlot2, EKeys::Gamepad_DPad_Right);
	DefaultMappingContext->MapKey(IA_AbilitySlot3, EKeys::Gamepad_DPad_Down);
	DefaultMappingContext->MapKey(IA_AbilitySlot4, EKeys::Gamepad_DPad_Left);

	// Start/Menu → Pause
	DefaultMappingContext->MapKey(IA_Pause, EKeys::Gamepad_Special_Right);
}

void AARPGPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Add the mapping context to the local player's Enhanced Input subsystem.
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}

	// Load any saved key rebindings
	LoadInputBindings();
}

void AARPGPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC) return;

	EIC->BindAction(IA_Move,          ETriggerEvent::Triggered, this, &AARPGPlayerController::HandleMove);
	EIC->BindAction(IA_Look,          ETriggerEvent::Triggered, this, &AARPGPlayerController::HandleLook);
	EIC->BindAction(IA_Interact,      ETriggerEvent::Triggered, this, &AARPGPlayerController::HandleInteract);
	EIC->BindAction(IA_PrimaryAttack, ETriggerEvent::Triggered, this, &AARPGPlayerController::HandlePrimaryAttack);
	EIC->BindAction(IA_Dodge,         ETriggerEvent::Triggered, this, &AARPGPlayerController::HandleDodge);
	EIC->BindAction(IA_Sprint,        ETriggerEvent::Started,   this, &AARPGPlayerController::HandleSprintStart);
	EIC->BindAction(IA_Sprint,        ETriggerEvent::Completed, this, &AARPGPlayerController::HandleSprintStop);
	EIC->BindAction(IA_Zoom,          ETriggerEvent::Triggered, this, &AARPGPlayerController::HandleZoom);
	EIC->BindAction(IA_RotateCameraLeft,  ETriggerEvent::Triggered, this, &AARPGPlayerController::HandleRotateCameraLeft);
	EIC->BindAction(IA_RotateCameraRight, ETriggerEvent::Triggered, this, &AARPGPlayerController::HandleRotateCameraRight);
	EIC->BindAction(IA_RotateCameraAnalog, ETriggerEvent::Triggered, this, &AARPGPlayerController::HandleRotateCameraAnalog);
	EIC->BindAction(IA_UsePotion,         ETriggerEvent::Triggered, this, &AARPGPlayerController::HandleUsePotion);
	EIC->BindAction(IA_Pause,             ETriggerEvent::Triggered, this, &AARPGPlayerController::HandlePause);
	EIC->BindAction(IA_Inventory,         ETriggerEvent::Triggered, this, &AARPGPlayerController::HandleInventory);
	EIC->BindAction(IA_CharacterStats,    ETriggerEvent::Triggered, this, &AARPGPlayerController::HandleCharacterStats);
	EIC->BindAction(IA_QuestLog,          ETriggerEvent::Triggered, this, &AARPGPlayerController::HandleQuestLog);
	EIC->BindAction(IA_SkillTree,         ETriggerEvent::Triggered, this, &AARPGPlayerController::HandleSkillTree);

	// Ability hotbar slots 1-4
	EIC->BindAction(IA_AbilitySlot1, ETriggerEvent::Triggered, this, &AARPGPlayerController::HandleAbilitySlot1);
	EIC->BindAction(IA_AbilitySlot2, ETriggerEvent::Triggered, this, &AARPGPlayerController::HandleAbilitySlot2);
	EIC->BindAction(IA_AbilitySlot3, ETriggerEvent::Triggered, this, &AARPGPlayerController::HandleAbilitySlot3);
	EIC->BindAction(IA_AbilitySlot4, ETriggerEvent::Triggered, this, &AARPGPlayerController::HandleAbilitySlot4);
}

// ---------------------------------------------------------------------------
// Input Device Detection
// ---------------------------------------------------------------------------

bool AARPGPlayerController::InputKey(const FInputKeyEventArgs& Params)
{
	const EARPGInputDevice NewDevice = Params.IsGamepad()
		? EARPGInputDevice::Gamepad
		: EARPGInputDevice::KeyboardMouse;

	if (NewDevice != CurrentInputDevice)
	{
		CurrentInputDevice = NewDevice;
		OnInputDeviceChanged.Broadcast(CurrentInputDevice);

		// Auto-show/hide cursor based on device — but never hide it while in UI mode
		if (!bUIInputModeActive)
		{
			const bool bShowCursor = (CurrentInputDevice == EARPGInputDevice::KeyboardMouse);
			SetShowMouseCursor(bShowCursor);
		}
	}

	return Super::InputKey(Params);
}

// ---------------------------------------------------------------------------
// Input handlers
// ---------------------------------------------------------------------------

void AARPGPlayerController::HandleMove(const FInputActionValue& Value)
{
	const FVector2D RawAxis = Value.Get<FVector2D>();
	if (RawAxis.IsNearlyZero()) return;

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;

	// Check CanMove() gate — blocks input during dodges, attacks, stagger, stun
	const AARPGCharacterBase* ARPGChar = Cast<AARPGCharacterBase>(ControlledPawn);
	if (ARPGChar && !ARPGChar->CanMove()) return;

	// Normalize diagonal input so diagonal movement isn't faster than cardinal
	const FVector2D Axis = RawAxis.GetSafeNormal() * FMath::Min(RawAxis.Size(), 1.f);

	// Movement relative to camera yaw (accounts for Q/E camera rotation).
	float CameraYaw = GetControlRotation().Yaw;
	if (ARPGChar)
	{
		CameraYaw = ARPGChar->GetCurrentCameraYaw();
	}
	const FRotator CameraRot(0.f, CameraYaw, 0.f);
	const FVector Forward = FRotationMatrix(CameraRot).GetUnitAxis(EAxis::X);
	const FVector Right   = FRotationMatrix(CameraRot).GetUnitAxis(EAxis::Y);

	ControlledPawn->AddMovementInput(Forward, Axis.Y);
	ControlledPawn->AddMovementInput(Right,   Axis.X);
}

void AARPGPlayerController::HandleLook(const FInputActionValue& Value)
{
	// For an isometric camera we don't rotate the view with the mouse.
	// Stub kept for future cursor-aim / highlight logic.
}

void AARPGPlayerController::HandleInteract(const FInputActionValue& Value)
{
	if (AARPGPlayerCharacter* PlayerChar = Cast<AARPGPlayerCharacter>(GetPawn()))
	{
		PlayerChar->PerformInteraction();
	}
}

void AARPGPlayerController::HandlePrimaryAttack(const FInputActionValue& Value)
{
	AARPGCharacterBase* ARPGChar = Cast<AARPGCharacterBase>(GetPawn());
	if (!ARPGChar) return;

	// Cancel sprint when attacking
	if (ARPGChar->IsSprinting())
	{
		ARPGChar->StopSprinting();
	}

	UAbilitySystemComponent* ASC = ARPGChar->GetAbilitySystemComponent();
	if (!ASC) return;

	// If already attacking, send a combo input event so the active ability can chain
	if (ARPGChar->IsAttacking())
	{
		FGameplayEventData Payload;
		Payload.Instigator = ARPGChar;
		Payload.EventTag = ARPGGameplayTags::Event_Combo_Input;
		ASC->HandleGameplayEvent(Payload.EventTag, &Payload);
		return;
	}

	// Try to activate the melee attack ability by its tag
	FGameplayTagContainer AbilityTags;
	AbilityTags.AddTag(ARPGGameplayTags::Ability_Melee_LightAttack);
	ASC->TryActivateAbilitiesByTag(AbilityTags);
}

void AARPGPlayerController::HandleDodge(const FInputActionValue& Value)
{
	AARPGCharacterBase* ARPGChar = Cast<AARPGCharacterBase>(GetPawn());
	if (!ARPGChar) return;

	UAbilitySystemComponent* ASC = ARPGChar->GetAbilitySystemComponent();
	if (!ASC) return;

	// Activate the dodge ability via its tag — GA_Dodge handles direction,
	// stamina, invulnerability, montage, and cooldown internally.
	FGameplayTagContainer AbilityTags;
	AbilityTags.AddTag(ARPGGameplayTags::Ability_Dodge);
	ASC->TryActivateAbilitiesByTag(AbilityTags);
}

void AARPGPlayerController::HandleSprintStart(const FInputActionValue& Value)
{
	if (AARPGCharacterBase* ARPGChar = Cast<AARPGCharacterBase>(GetPawn()))
	{
		ARPGChar->StartSprinting();
	}
}

void AARPGPlayerController::HandleSprintStop(const FInputActionValue& Value)
{
	if (AARPGCharacterBase* ARPGChar = Cast<AARPGCharacterBase>(GetPawn()))
	{
		ARPGChar->StopSprinting();
	}
}

void AARPGPlayerController::HandleZoom(const FInputActionValue& Value)
{
	const float Axis = Value.Get<float>();
	if (AARPGCharacterBase* ARPGChar = Cast<AARPGCharacterBase>(GetPawn()))
	{
		ARPGChar->ZoomCamera(-Axis * ARPGChar->ZoomStep);
	}
}

void AARPGPlayerController::HandleRotateCameraLeft(const FInputActionValue& Value)
{
	if (AARPGCharacterBase* ARPGChar = Cast<AARPGCharacterBase>(GetPawn()))
	{
		ARPGChar->RotateCamera(-ARPGChar->CameraRotationStep);
	}
}

void AARPGPlayerController::HandleRotateCameraRight(const FInputActionValue& Value)
{
	if (AARPGCharacterBase* ARPGChar = Cast<AARPGCharacterBase>(GetPawn()))
	{
		ARPGChar->RotateCamera(ARPGChar->CameraRotationStep);
	}
}

void AARPGPlayerController::HandleRotateCameraAnalog(const FInputActionValue& Value)
{
	const float Axis = Value.Get<float>();
	if (FMath::IsNearlyZero(Axis)) return;

	if (AARPGCharacterBase* ARPGChar = Cast<AARPGCharacterBase>(GetPawn()))
	{
		const float DeltaTime = GetWorld()->GetDeltaSeconds();
		ARPGChar->RotateCamera(Axis * GamepadCameraRotationSpeed * DeltaTime);
	}
}

void AARPGPlayerController::HandleUsePotion(const FInputActionValue& Value)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;

	if (UARPGInventoryComponent* Inventory = ControlledPawn->FindComponentByClass<UARPGInventoryComponent>())
	{
		Inventory->UseFirstConsumable();
	}
}

void AARPGPlayerController::HandleAbilitySlot1(const FInputActionValue& Value) { HandleAbilitySlot(0); }
void AARPGPlayerController::HandleAbilitySlot2(const FInputActionValue& Value) { HandleAbilitySlot(1); }
void AARPGPlayerController::HandleAbilitySlot3(const FInputActionValue& Value) { HandleAbilitySlot(2); }
void AARPGPlayerController::HandleAbilitySlot4(const FInputActionValue& Value) { HandleAbilitySlot(3); }

void AARPGPlayerController::HandleAbilitySlot(int32 SlotIndex)
{
	if (AARPGPlayerCharacter* PlayerChar = Cast<AARPGPlayerCharacter>(GetPawn()))
	{
		PlayerChar->TryActivateAbilitySlot(SlotIndex);
	}
}

void AARPGPlayerController::HandlePause(const FInputActionValue& Value)
{
	TogglePauseMenu();
}

void AARPGPlayerController::HandleInventory(const FInputActionValue& Value)
{
	if (AARPGHUD* HUD = Cast<AARPGHUD>(GetHUD()))
	{
		HUD->ToggleInventory();
	}
}

void AARPGPlayerController::HandleCharacterStats(const FInputActionValue& Value)
{
	if (AARPGHUD* HUD = Cast<AARPGHUD>(GetHUD()))
	{
		HUD->ToggleCharacterStats();
	}
}

void AARPGPlayerController::HandleQuestLog(const FInputActionValue& Value)
{
	if (AARPGHUD* HUD = Cast<AARPGHUD>(GetHUD()))
	{
		HUD->ToggleQuestLog();
	}
}

void AARPGPlayerController::HandleSkillTree(const FInputActionValue& Value)
{
	if (AARPGHUD* HUD = Cast<AARPGHUD>(GetHUD()))
	{
		HUD->ToggleSkillTree();
	}
}

// ---------------------------------------------------------------------------
// Key Rebinding
// ---------------------------------------------------------------------------

UInputAction* AARPGPlayerController::FindConflictingAction(FKey Key, UInputAction* ExcludeAction) const
{
	if (!DefaultMappingContext || !Key.IsValid()) return nullptr;

	const TArray<FEnhancedActionKeyMapping>& Mappings = DefaultMappingContext->GetMappings();
	for (const FEnhancedActionKeyMapping& Mapping : Mappings)
	{
		if (Mapping.Key == Key && Mapping.Action != ExcludeAction)
		{
			return const_cast<UInputAction*>(Mapping.Action.Get());
		}
	}
	return nullptr;
}

bool AARPGPlayerController::RemapAction(UInputAction* Action, FKey OldKey, FKey NewKey)
{
	if (!Action || !OldKey.IsValid() || !NewKey.IsValid() || OldKey == NewKey) return false;

	// Reject if the new key conflicts with another action
	if (FindConflictingAction(NewKey, Action) != nullptr) return false;

	if (!RemapActionInternal(Action, OldKey, NewKey)) return false;

	// Track the rebind for persistence
	FARPGKeyRebind Rebind;
	Rebind.ActionName = Action->GetFName();
	Rebind.OriginalKey = OldKey;
	Rebind.NewKey = NewKey;

	// Check if we're re-remapping an already remapped key (update instead of add)
	for (int32 i = 0; i < AppliedRebinds.Num(); ++i)
	{
		if (AppliedRebinds[i].ActionName == Rebind.ActionName && AppliedRebinds[i].NewKey == OldKey)
		{
			// Chain: original -> old -> new, simplify to original -> new
			Rebind.OriginalKey = AppliedRebinds[i].OriginalKey;
			AppliedRebinds.RemoveAt(i);
			break;
		}
	}

	// If remapped back to original, just remove the entry
	if (Rebind.OriginalKey == NewKey)
	{
		return true;
	}

	AppliedRebinds.Add(Rebind);
	RefreshMappingContext();
	return true;
}

bool AARPGPlayerController::RemapActionInternal(UInputAction* Action, FKey OldKey, FKey NewKey)
{
	if (!DefaultMappingContext) return false;

	const TArray<FEnhancedActionKeyMapping>& Mappings = DefaultMappingContext->GetMappings();

	// Find the mapping with the matching action + key
	int32 FoundIndex = INDEX_NONE;
	for (int32 i = 0; i < Mappings.Num(); ++i)
	{
		if (Mappings[i].Action == Action && Mappings[i].Key == OldKey)
		{
			FoundIndex = i;
			break;
		}
	}

	if (FoundIndex == INDEX_NONE) return false;

	// Save modifiers and triggers before removing
	TArray<TObjectPtr<UInputModifier>> SavedModifiers = Mappings[FoundIndex].Modifiers;
	TArray<TObjectPtr<UInputTrigger>> SavedTriggers = Mappings[FoundIndex].Triggers;

	// Remove old mapping and add new one
	DefaultMappingContext->UnmapKey(Action, OldKey);
	FEnhancedActionKeyMapping& NewMapping = DefaultMappingContext->MapKey(Action, NewKey);
	NewMapping.Modifiers = SavedModifiers;
	NewMapping.Triggers = SavedTriggers;

	return true;
}

TArray<FKey> AARPGPlayerController::GetBoundKeysForAction(const UInputAction* Action) const
{
	TArray<FKey> Keys;
	if (!DefaultMappingContext || !Action) return Keys;

	const TArray<FEnhancedActionKeyMapping>& Mappings = DefaultMappingContext->GetMappings();
	for (const FEnhancedActionKeyMapping& Mapping : Mappings)
	{
		if (Mapping.Action == Action)
		{
			Keys.Add(Mapping.Key);
		}
	}
	return Keys;
}

void AARPGPlayerController::ResetBindingsToDefault()
{
	// Undo all applied rebinds in reverse order
	for (int32 i = AppliedRebinds.Num() - 1; i >= 0; --i)
	{
		const FARPGKeyRebind& R = AppliedRebinds[i];
		UInputAction* Action = FindInputActionByName(R.ActionName);
		if (Action)
		{
			RemapActionInternal(Action, R.NewKey, R.OriginalKey);
		}
	}
	AppliedRebinds.Empty();
	RefreshMappingContext();
}

void AARPGPlayerController::SaveInputBindings()
{
	UARPGInputSaveData* SaveData = NewObject<UARPGInputSaveData>();
	SaveData->Rebinds = AppliedRebinds;
	UGameplayStatics::SaveGameToSlot(SaveData, UARPGInputSaveData::SlotName, 0);
}

void AARPGPlayerController::LoadInputBindings()
{
	if (!UGameplayStatics::DoesSaveGameExist(UARPGInputSaveData::SlotName, 0)) return;

	UARPGInputSaveData* SaveData = Cast<UARPGInputSaveData>(
		UGameplayStatics::LoadGameFromSlot(UARPGInputSaveData::SlotName, 0));
	if (!SaveData) return;

	// Apply each saved rebind
	for (const FARPGKeyRebind& Rebind : SaveData->Rebinds)
	{
		UInputAction* Action = FindInputActionByName(Rebind.ActionName);
		if (Action)
		{
			if (RemapActionInternal(Action, Rebind.OriginalKey, Rebind.NewKey))
			{
				AppliedRebinds.Add(Rebind);
			}
		}
	}

	if (AppliedRebinds.Num() > 0)
	{
		RefreshMappingContext();
	}
}

void AARPGPlayerController::RefreshMappingContext()
{
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->RequestRebuildControlMappings();
	}
}

UInputAction* AARPGPlayerController::FindInputActionByName(FName ActionName) const
{
	const TObjectPtr<UInputAction>* Found = ActionLookup.Find(ActionName);
	return Found ? Found->Get() : nullptr;
}

void AARPGPlayerController::RegisterAction(UInputAction* Action)
{
	if (Action)
	{
		ActionLookup.Add(Action->GetFName(), Action);
	}
}

// ---------------------------------------------------------------------------
// Input Context Stacking
// ---------------------------------------------------------------------------

void AARPGPlayerController::PushInputContext(UInputMappingContext* Context, int32 Priority)
{
	if (!Context) return;

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(Context, Priority);
	}
}

void AARPGPlayerController::PopInputContext(UInputMappingContext* Context)
{
	if (!Context) return;

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->RemoveMappingContext(Context);
	}
}

// ---------------------------------------------------------------------------
// Input Mode Management
// ---------------------------------------------------------------------------

void AARPGPlayerController::SetInputModeGameOnly()
{
	bUIInputModeActive = false;
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(CurrentInputDevice == EARPGInputDevice::KeyboardMouse);
}

void AARPGPlayerController::SetInputModeUIOnly()
{
	bUIInputModeActive = true;
	SetInputMode(FInputModeUIOnly());
	SetShowMouseCursor(true);
}

void AARPGPlayerController::SetInputModeGameAndUI()
{
	bUIInputModeActive = true;
	SetInputMode(FInputModeGameAndUI());
	SetShowMouseCursor(true);
}

// ---------------------------------------------------------------------------
// Pause Menu
// ---------------------------------------------------------------------------

void AARPGPlayerController::TogglePauseMenu()
{
	// If menu is up, close it
	if (PauseMenuInstance && PauseMenuInstance->IsInViewport())
	{
		PauseMenuInstance->RemoveFromParent();
		PauseMenuInstance = nullptr;
		UGameplayStatics::SetGamePaused(this, false);
		SetInputModeGameOnly();
		return;
	}

	// Open the menu
	if (!PauseMenuWidgetClass) return;

	PauseMenuInstance = CreateWidget<UPauseMenuWidget>(this, PauseMenuWidgetClass);
	if (!PauseMenuInstance) return;

	PauseMenuInstance->OnResumeRequested.AddDynamic(this, &AARPGPlayerController::OnResumeFromPauseMenu);
	PauseMenuInstance->AddToViewport(50);

	UGameplayStatics::SetGamePaused(this, true);
	SetInputModeGameAndUI();
}

void AARPGPlayerController::OnResumeFromPauseMenu()
{
	TogglePauseMenu();
}
