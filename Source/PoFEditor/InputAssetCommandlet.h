#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "InputAction.h"
#include "InputAssetCommandlet.generated.h"

/**
 * Commandlet that programmatically creates Enhanced Input UInputAction assets
 * for the vertical slice:
 *  - IA_Move    (Axis2D, WASD movement)
 *  - IA_Attack  (Digital/bool, LMB primary attack)
 *
 * Run via:
 *   UnrealEditor-Cmd.exe PoF.uproject -run=InputAsset -nopause -unattended -nosplash
 */
UCLASS()
class UInputAssetCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UInputAssetCommandlet();
	virtual int32 Main(const FString& Params) override;

private:
	bool CreateInputAction(const FString& PackagePath, const FString& AssetName, EInputActionValueType ValueType);
	bool SaveAsset(UObject* Asset, UPackage* Package, const FString& PackagePath);
};
