#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ARPGCustomMaterialExpression.generated.h"

/**
 * Predefined HLSL code snippets for use in Custom material expression nodes.
 * Each function returns the HLSL code string that can be pasted into a
 * Custom node in the material editor.
 *
 * These cover common ARPG visual effects that require custom shader math
 * beyond what the standard material nodes provide.
 */
UCLASS()
class POF_API UARPGCustomMaterialExpression : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// =====================================================================
	// Dissolve / Threshold Effects
	// =====================================================================

	/**
	 * HLSL for an edge-detection dissolve effect.
	 * Inputs: NoiseValue (float), DissolveAmount (float), EdgeWidth (float)
	 * Output: float3 (RGB with glowing edge band)
	 *
	 * The edge band emits based on proximity to the dissolve threshold,
	 * creating the classic burning-edge dissolve look.
	 */
	UFUNCTION(BlueprintPure, Category = "Materials|HLSL|Dissolve")
	static FString GetDissolveEdgeHLSL();

	/**
	 * HLSL for directional dissolve (dissolves from a world-space direction).
	 * Inputs: WorldPosition (float3), DissolveOrigin (float3),
	 *         DissolveDirection (float3), DissolveDistance (float)
	 * Output: float (clip mask — 0 = dissolved, 1 = visible)
	 */
	UFUNCTION(BlueprintPure, Category = "Materials|HLSL|Dissolve")
	static FString GetDirectionalDissolveHLSL();

	// =====================================================================
	// Stylized Shading
	// =====================================================================

	/**
	 * HLSL for cel/toon shading with configurable band count.
	 * Inputs: NdotL (float), BandCount (float), ShadowSoftness (float)
	 * Output: float (quantized light intensity)
	 */
	UFUNCTION(BlueprintPure, Category = "Materials|HLSL|Stylized")
	static FString GetCelShadingHLSL();

	/**
	 * HLSL for a rim/fresnel outline effect.
	 * Inputs: WorldNormal (float3), CameraDir (float3),
	 *         RimPower (float), RimIntensity (float)
	 * Output: float (rim light intensity)
	 */
	UFUNCTION(BlueprintPure, Category = "Materials|HLSL|Stylized")
	static FString GetRimOutlineHLSL();

	/**
	 * HLSL for a hatching/crosshatch shading effect.
	 * Inputs: UV (float2), Intensity (float — 0=dark, 1=light), Scale (float)
	 * Output: float (hatching pattern alpha)
	 */
	UFUNCTION(BlueprintPure, Category = "Materials|HLSL|Stylized")
	static FString GetHatchingHLSL();

	// =====================================================================
	// Procedural Patterns
	// =====================================================================

	/**
	 * HLSL for animated caustics (underwater light patterns).
	 * Inputs: UV (float2), Time (float), Scale (float), Speed (float)
	 * Output: float (caustic brightness)
	 */
	UFUNCTION(BlueprintPure, Category = "Materials|HLSL|Procedural")
	static FString GetCausticsHLSL();

	/**
	 * HLSL for hex grid pattern with smooth edges.
	 * Inputs: UV (float2), Scale (float), EdgeWidth (float)
	 * Output: float (0 = cell interior, 1 = edge)
	 */
	UFUNCTION(BlueprintPure, Category = "Materials|HLSL|Procedural")
	static FString GetHexGridHLSL();

	/**
	 * HLSL for animated energy shield hit effect.
	 * Inputs: UV (float2), HitUV (float2), Time (float),
	 *         RippleSpeed (float), RippleFrequency (float)
	 * Output: float (shield ripple intensity)
	 */
	UFUNCTION(BlueprintPure, Category = "Materials|HLSL|Procedural")
	static FString GetShieldRippleHLSL();

	// =====================================================================
	// Post-Process
	// =====================================================================

	/**
	 * HLSL for screen-space radial blur (e.g., speed lines, impact).
	 * Inputs: UV (float2), Center (float2), BlurStrength (float), Samples (float)
	 * Output: float (blur factor to multiply with scene color sampling offset)
	 */
	UFUNCTION(BlueprintPure, Category = "Materials|HLSL|PostProcess")
	static FString GetRadialBlurHLSL();

	/**
	 * HLSL for chromatic aberration.
	 * Inputs: UV (float2), Center (float2), Strength (float)
	 * Output: float2 (R channel offset, B channel offset — G stays at original UV)
	 */
	UFUNCTION(BlueprintPure, Category = "Materials|HLSL|PostProcess")
	static FString GetChromaticAberrationHLSL();

	/**
	 * HLSL for a vignette effect.
	 * Inputs: UV (float2), Intensity (float), Smoothness (float), Color (float3)
	 * Output: float3 (vignette color multiplier)
	 */
	UFUNCTION(BlueprintPure, Category = "Materials|HLSL|PostProcess")
	static FString GetVignetteHLSL();

	// =====================================================================
	// Utility
	// =====================================================================

	/**
	 * HLSL for improved 3D noise (Simplex-like gradient noise).
	 * Inputs: Position (float3)
	 * Output: float (noise value in [-1..1])
	 */
	UFUNCTION(BlueprintPure, Category = "Materials|HLSL|Utility")
	static FString GetGradientNoise3DHLSL();

	/**
	 * HLSL for animated flow mapping (lava, water surface).
	 * Inputs: UV (float2), FlowDir (float2), Time (float), Phase (float)
	 * Output: float2 (distorted UV for texture sampling)
	 */
	UFUNCTION(BlueprintPure, Category = "Materials|HLSL|Utility")
	static FString GetFlowMapHLSL();
};
