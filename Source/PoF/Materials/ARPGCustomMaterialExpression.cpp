#include "Materials/ARPGCustomMaterialExpression.h"

// =====================================================================
// Dissolve / Threshold Effects
// =====================================================================

FString UARPGCustomMaterialExpression::GetDissolveEdgeHLSL()
{
	return TEXT(
		"// Dissolve with glowing edge\n"
		"// Inputs: NoiseValue, DissolveAmount, EdgeWidth\n"
		"float mask = step(DissolveAmount, NoiseValue);\n"
		"float edgeDist = abs(NoiseValue - DissolveAmount);\n"
		"float edge = 1.0 - saturate(edgeDist / max(EdgeWidth, 0.001));\n"
		"edge *= mask;\n"
		"// Output: x = clip mask, y = edge glow intensity\n"
		"return float2(mask, edge * edge);\n"
	);
}

FString UARPGCustomMaterialExpression::GetDirectionalDissolveHLSL()
{
	return TEXT(
		"// Directional dissolve from world-space origin along direction\n"
		"// Inputs: WorldPosition, DissolveOrigin, DissolveDirection, DissolveDistance\n"
		"float3 offset = WorldPosition - DissolveOrigin;\n"
		"float projDist = dot(offset, normalize(DissolveDirection));\n"
		"float normalizedDist = saturate(projDist / max(DissolveDistance, 0.001));\n"
		"return normalizedDist;\n"
	);
}

// =====================================================================
// Stylized Shading
// =====================================================================

FString UARPGCustomMaterialExpression::GetCelShadingHLSL()
{
	return TEXT(
		"// Cel shading with configurable bands\n"
		"// Inputs: NdotL, BandCount, ShadowSoftness\n"
		"float bands = max(BandCount, 1.0);\n"
		"float scaled = NdotL * bands;\n"
		"float stepped = floor(scaled) + smoothstep(0.0, ShadowSoftness, frac(scaled));\n"
		"return saturate(stepped / bands);\n"
	);
}

FString UARPGCustomMaterialExpression::GetRimOutlineHLSL()
{
	return TEXT(
		"// Rim/fresnel outline\n"
		"// Inputs: WorldNormal, CameraDir, RimPower, RimIntensity\n"
		"float NdotV = saturate(dot(normalize(WorldNormal), normalize(CameraDir)));\n"
		"float rim = 1.0 - NdotV;\n"
		"rim = pow(rim, max(RimPower, 0.01));\n"
		"return rim * RimIntensity;\n"
	);
}

FString UARPGCustomMaterialExpression::GetHatchingHLSL()
{
	return TEXT(
		"// Crosshatch shading\n"
		"// Inputs: UV, Intensity, Scale\n"
		"float2 uv = UV * Scale;\n"
		"// Diagonal lines in two directions\n"
		"float line1 = frac(uv.x + uv.y);\n"
		"float line2 = frac(uv.x - uv.y);\n"
		"// Vary line density with light intensity\n"
		"float t1 = step(0.5, line1) * step(Intensity, 0.66);\n"
		"float t2 = step(0.5, line2) * step(Intensity, 0.33);\n"
		"float hatch = max(t1, t2);\n"
		"return lerp(hatch, 1.0, saturate(Intensity * 1.5 - 0.5));\n"
	);
}

// =====================================================================
// Procedural Patterns
// =====================================================================

FString UARPGCustomMaterialExpression::GetCausticsHLSL()
{
	return TEXT(
		"// Animated caustics\n"
		"// Inputs: UV, Time, Scale, Speed\n"
		"float2 p = UV * Scale;\n"
		"float t = Time * Speed;\n"
		"// Two offset voronoi-like layers\n"
		"float2 p1 = p + float2(sin(t * 0.7), cos(t * 0.5)) * 0.5;\n"
		"float2 p2 = p + float2(cos(t * 0.6), sin(t * 0.8)) * 0.5;\n"
		"// Cell distance approximation\n"
		"float2 f1 = frac(p1) - 0.5;\n"
		"float2 f2 = frac(p2) - 0.5;\n"
		"float d1 = dot(f1, f1);\n"
		"float d2 = dot(f2, f2);\n"
		"float caustic = min(d1, d2);\n"
		"caustic = 1.0 - saturate(caustic * 4.0);\n"
		"return caustic * caustic;\n"
	);
}

FString UARPGCustomMaterialExpression::GetHexGridHLSL()
{
	return TEXT(
		"// Hex grid pattern\n"
		"// Inputs: UV, Scale, EdgeWidth\n"
		"float2 p = UV * Scale;\n"
		"// Hex grid math\n"
		"float2 h = float2(1.0, sqrt(3.0));\n"
		"float2 a = fmod(p, h) - h * 0.5;\n"
		"float2 b = fmod(p - h * 0.5, h) - h * 0.5;\n"
		"float2 g = dot(a, a) < dot(b, b) ? a : b;\n"
		"float dist = max(abs(g.x), abs(g.y * 0.577 + g.x * 0.5));\n"
		"float edge = smoothstep(0.5 - EdgeWidth, 0.5, dist);\n"
		"return edge;\n"
	);
}

FString UARPGCustomMaterialExpression::GetShieldRippleHLSL()
{
	return TEXT(
		"// Shield hit ripple\n"
		"// Inputs: UV, HitUV, Time, RippleSpeed, RippleFrequency\n"
		"float dist = length(UV - HitUV);\n"
		"float wave = sin((dist - Time * RippleSpeed) * RippleFrequency);\n"
		"wave = wave * 0.5 + 0.5;\n"
		"// Attenuate with distance from hit point\n"
		"float atten = 1.0 - saturate(dist * 2.0);\n"
		"// Fade over time\n"
		"float timeFade = saturate(1.0 - Time * 0.5);\n"
		"return wave * atten * atten * timeFade;\n"
	);
}

// =====================================================================
// Post-Process
// =====================================================================

FString UARPGCustomMaterialExpression::GetRadialBlurHLSL()
{
	return TEXT(
		"// Radial blur offset factor\n"
		"// Inputs: UV, Center, BlurStrength, Samples\n"
		"float2 dir = UV - Center;\n"
		"float dist = length(dir);\n"
		"dir = normalize(dir);\n"
		"// Per-sample offset scaled by distance from center\n"
		"float offset = (BlurStrength * dist) / max(Samples, 1.0);\n"
		"return float2(dir * offset);\n"
	);
}

FString UARPGCustomMaterialExpression::GetChromaticAberrationHLSL()
{
	return TEXT(
		"// Chromatic aberration UV offsets\n"
		"// Inputs: UV, Center, Strength\n"
		"float2 dir = UV - Center;\n"
		"float dist = length(dir);\n"
		"float2 offset = dir * dist * Strength;\n"
		"// Red channel shifts outward, Blue shifts inward\n"
		"// Green stays at original UV\n"
		"return float2(offset.x, -offset.x);\n"
	);
}

FString UARPGCustomMaterialExpression::GetVignetteHLSL()
{
	return TEXT(
		"// Vignette effect\n"
		"// Inputs: UV, Intensity, Smoothness, Color\n"
		"float2 centered = UV - 0.5;\n"
		"float dist = length(centered) * 2.0;\n"
		"float vignette = smoothstep(1.0 - Smoothness, 1.0, dist * Intensity);\n"
		"return lerp(float3(1, 1, 1), Color, vignette);\n"
	);
}

// =====================================================================
// Utility
// =====================================================================

FString UARPGCustomMaterialExpression::GetGradientNoise3DHLSL()
{
	return TEXT(
		"// 3D Gradient noise (Simplex-like)\n"
		"// Input: Position (float3)\n"
		"float3 p = Position;\n"
		"float3 i = floor(p);\n"
		"float3 f = frac(p);\n"
		"// Quintic interpolation\n"
		"float3 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);\n"
		"// Hash corners\n"
		"float n = i.x + i.y * 157.0 + i.z * 113.0;\n"
		"float a = frac(sin(n) * 43758.5453);\n"
		"float b = frac(sin(n + 1.0) * 43758.5453);\n"
		"float c = frac(sin(n + 157.0) * 43758.5453);\n"
		"float d = frac(sin(n + 158.0) * 43758.5453);\n"
		"float e = frac(sin(n + 113.0) * 43758.5453);\n"
		"float ff = frac(sin(n + 114.0) * 43758.5453);\n"
		"float g = frac(sin(n + 270.0) * 43758.5453);\n"
		"float hh = frac(sin(n + 271.0) * 43758.5453);\n"
		"// Trilinear interpolation\n"
		"float x1 = lerp(a, b, u.x);\n"
		"float x2 = lerp(c, d, u.x);\n"
		"float x3 = lerp(e, ff, u.x);\n"
		"float x4 = lerp(g, hh, u.x);\n"
		"float y1 = lerp(x1, x2, u.y);\n"
		"float y2 = lerp(x3, x4, u.y);\n"
		"return lerp(y1, y2, u.z) * 2.0 - 1.0;\n"
	);
}

FString UARPGCustomMaterialExpression::GetFlowMapHLSL()
{
	return TEXT(
		"// Flow map UV distortion\n"
		"// Inputs: UV, FlowDir, Time, Phase\n"
		"float halfPhase = Phase * 0.5;\n"
		"float phase0 = frac(Time / Phase);\n"
		"float phase1 = frac(Time / Phase + 0.5);\n"
		"// Blend factor between two phases to hide snapping\n"
		"float blend = abs(phase0 - 0.5) * 2.0;\n"
		"// Offset UVs by flow direction, scaled by phase progress\n"
		"float2 uv0 = UV - FlowDir * phase0 * halfPhase;\n"
		"float2 uv1 = UV - FlowDir * phase1 * halfPhase;\n"
		"// Output two sets of UVs packed as float4, plus blend\n"
		"// Use: sample texture at uv0 and uv1, lerp by blend\n"
		"return float2(blend, 0);\n"
	);
}
