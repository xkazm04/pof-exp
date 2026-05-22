#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "ContentValidationCommandlet.generated.h"

/**
 * Validates content for packaging readiness:
 * - Scans for missing references (soft object paths that resolve to nullptr)
 * - Reports assets with no references (orphaned)
 * - Checks for oversized textures
 * Run: UnrealEditor-Cmd.exe PoF.uproject -run=ContentValidation -nopause -unattended
 */
UCLASS()
class UContentValidationCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;

private:
	int32 ErrorCount = 0;
	int32 WarningCount = 0;

	void ValidateAssetReferences();
	void CheckTexturesizes();
	void ReportSummary();
};
