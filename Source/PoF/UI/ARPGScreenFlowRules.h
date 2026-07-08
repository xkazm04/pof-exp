#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ARPGScreenFlowRules.generated.h"

/**
 * Screen-flow z-ordering + overlay rules (catalog pipeline: screen-flow). The InGame
 * HUD sits at z-depth 1; modal overlays (Inventory / CharStats) push to z-depth 3 with
 * the HUD dimmed behind them; the Pause menu sits above at z-depth 4. Mirrors
 * src/lib/catalog/pipelines/screen-flow.ts (z-depth contract). The full PIE reachability
 * / back-stack behaviour is the runtime concern; this encodes the deterministic ordering.
 */
UCLASS()
class POF_API UARPGScreenFlowRules : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static constexpr int32 HudZDepth = 1;
	static constexpr int32 OverlayZDepth = 3;
	static constexpr int32 PauseZDepth = 4;
	static constexpr float HudOpacityBehindOverlay = 0.5f;

	/** Modal overlays must render above the HUD. */
	UFUNCTION(BlueprintPure, Category = "UI|ScreenFlow")
	static bool OverlaysRenderAboveHud() { return OverlayZDepth > HudZDepth; }

	/** The Pause menu renders above the item/stat overlays. */
	UFUNCTION(BlueprintPure, Category = "UI|ScreenFlow")
	static bool PauseRendersAboveOverlays() { return PauseZDepth > OverlayZDepth; }
};
