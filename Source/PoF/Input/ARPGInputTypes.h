#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ARPGInputTypes.generated.h"

/** Active input device type for UI glyph display and cursor management. */
UENUM(BlueprintType)
enum class EARPGInputDevice : uint8
{
	KeyboardMouse,
	Gamepad
};

/** Broadcast when the player switches between keyboard/mouse and gamepad. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInputDeviceChanged, EARPGInputDevice, NewDevice);

/** Stores a single key rebinding for save/load persistence. */
USTRUCT()
struct FARPGKeyRebind
{
	GENERATED_BODY()

	UPROPERTY()
	FName ActionName;

	UPROPERTY()
	FKey OriginalKey;

	UPROPERTY()
	FKey NewKey;
};

/** SaveGame object that persists custom key bindings to disk. */
UCLASS()
class POF_API UARPGInputSaveData : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr const TCHAR* SlotName = TEXT("InputBindings");

	UPROPERTY()
	TArray<FARPGKeyRebind> Rebinds;
};
