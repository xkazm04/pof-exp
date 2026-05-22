#include "Materials/ARPGMaterialFunctionLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/MeshComponent.h"

// =====================================================================
// UV Manipulation
// =====================================================================

FVector2D UARPGMaterialFunctionLibrary::WorldAlignedUV(const FVector& WorldPosition, const FVector& Normal, float Scale)
{
	const FVector AbsNormal = Normal.GetAbs();
	const float InvScale = (Scale > KINDA_SMALL_NUMBER) ? (1.f / Scale) : 1.f;

	// Choose the dominant axis
	if (AbsNormal.Z >= AbsNormal.X && AbsNormal.Z >= AbsNormal.Y)
	{
		return FVector2D(WorldPosition.X, WorldPosition.Y) * InvScale;
	}
	else if (AbsNormal.X >= AbsNormal.Y)
	{
		return FVector2D(WorldPosition.Y, WorldPosition.Z) * InvScale;
	}
	else
	{
		return FVector2D(WorldPosition.X, WorldPosition.Z) * InvScale;
	}
}

FVector UARPGMaterialFunctionLibrary::TriplanarBlendWeights(const FVector& WorldNormal, float Sharpness)
{
	FVector Weights = WorldNormal.GetAbs();
	Weights.X = FMath::Pow(Weights.X, Sharpness);
	Weights.Y = FMath::Pow(Weights.Y, Sharpness);
	Weights.Z = FMath::Pow(Weights.Z, Sharpness);

	const float Sum = Weights.X + Weights.Y + Weights.Z;
	if (Sum > KINDA_SMALL_NUMBER)
	{
		Weights /= Sum;
	}
	return Weights;
}

FVector2D UARPGMaterialFunctionLibrary::RotateUV(const FVector2D& UV, float AngleDegrees)
{
	const float Rad = FMath::DegreesToRadians(AngleDegrees);
	const float CosA = FMath::Cos(Rad);
	const float SinA = FMath::Sin(Rad);

	// Rotate around center (0.5, 0.5)
	const float CX = UV.X - 0.5f;
	const float CY = UV.Y - 0.5f;

	return FVector2D(
		CX * CosA - CY * SinA + 0.5f,
		CX * SinA + CY * CosA + 0.5f
	);
}

FVector2D UARPGMaterialFunctionLibrary::PanUV(const FVector2D& UV, float Time, float SpeedU, float SpeedV)
{
	return FVector2D(
		UV.X + Time * SpeedU,
		UV.Y + Time * SpeedV
	);
}

FVector2D UARPGMaterialFunctionLibrary::TileOffsetUV(const FVector2D& UV, float TileU, float TileV, float OffsetU, float OffsetV)
{
	return FVector2D(
		UV.X * TileU + OffsetU,
		UV.Y * TileV + OffsetV
	);
}

// =====================================================================
// Noise / Procedural
// =====================================================================

// Simple hash-based 2D noise
static float Hash2D(float X, float Y)
{
	// Large primes for pseudo-random hash
	int32 IX = FMath::FloorToInt32(X);
	int32 IY = FMath::FloorToInt32(Y);
	IX = IX * 1619 + IY * 31337;
	IX = (IX >> 13) ^ IX;
	float Result = 1.f - ((IX * (IX * IX * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.f;
	return Result * 0.5f + 0.5f;
}

static float SmoothNoise2D(float X, float Y)
{
	const float IX = FMath::FloorToFloat(X);
	const float IY = FMath::FloorToFloat(Y);
	const float FX = X - IX;
	const float FY = Y - IY;

	// Smoothstep
	const float SX = FX * FX * (3.f - 2.f * FX);
	const float SY = FY * FY * (3.f - 2.f * FY);

	const float N00 = Hash2D(IX, IY);
	const float N10 = Hash2D(IX + 1.f, IY);
	const float N01 = Hash2D(IX, IY + 1.f);
	const float N11 = Hash2D(IX + 1.f, IY + 1.f);

	const float NX0 = FMath::Lerp(N00, N10, SX);
	const float NX1 = FMath::Lerp(N01, N11, SX);

	return FMath::Lerp(NX0, NX1, SY);
}

float UARPGMaterialFunctionLibrary::SimpleNoise2D(float X, float Y)
{
	return SmoothNoise2D(X, Y);
}

float UARPGMaterialFunctionLibrary::FBMNoise2D(FVector2D Position, int32 Octaves, float Lacunarity, float Gain)
{
	Octaves = FMath::Clamp(Octaves, 1, 8);

	float Value = 0.f;
	float Amplitude = 1.f;
	float Frequency = 1.f;
	float MaxValue = 0.f;

	for (int32 i = 0; i < Octaves; ++i)
	{
		Value += SmoothNoise2D(Position.X * Frequency, Position.Y * Frequency) * Amplitude;
		MaxValue += Amplitude;
		Amplitude *= Gain;
		Frequency *= Lacunarity;
	}

	return (MaxValue > 0.f) ? (Value / MaxValue) : 0.f;
}

float UARPGMaterialFunctionLibrary::VoronoiDistance2D(FVector2D Position, float CellSize)
{
	if (CellSize <= KINDA_SMALL_NUMBER)
	{
		CellSize = 1.f;
	}

	const FVector2D Scaled = Position / CellSize;
	const int32 CellX = FMath::FloorToInt32(Scaled.X);
	const int32 CellY = FMath::FloorToInt32(Scaled.Y);
	const FVector2D Frac(Scaled.X - CellX, Scaled.Y - CellY);

	float MinDist = 8.f;

	for (int32 DY = -1; DY <= 1; ++DY)
	{
		for (int32 DX = -1; DX <= 1; ++DX)
		{
			const FVector2D Neighbor(static_cast<float>(DX), static_cast<float>(DY));
			// Deterministic point position within neighbor cell
			const FVector2D Point(
				Hash2D(static_cast<float>(CellX + DX), static_cast<float>(CellY + DY)),
				Hash2D(static_cast<float>(CellX + DX) + 0.5f, static_cast<float>(CellY + DY) + 0.5f)
			);
			const FVector2D Diff = Neighbor + Point - Frac;
			const float Dist = Diff.SizeSquared();
			MinDist = FMath::Min(MinDist, Dist);
		}
	}

	return FMath::Sqrt(MinDist);
}

// =====================================================================
// Blending
// =====================================================================

float UARPGMaterialFunctionLibrary::HeightBlend(float HeightA, float HeightB, float BlendAlpha, float BlendContrast)
{
	BlendContrast = FMath::Max(BlendContrast, 0.001f);

	const float MA = HeightA + (1.f - BlendAlpha);
	const float MB = HeightB + BlendAlpha;

	const float MaxH = FMath::Max(MA, MB) - BlendContrast;
	const float WA = FMath::Max(MA - MaxH, 0.f);
	const float WB = FMath::Max(MB - MaxH, 0.f);
	const float Sum = WA + WB;

	return (Sum > KINDA_SMALL_NUMBER) ? (WB / Sum) : BlendAlpha;
}

float UARPGMaterialFunctionLibrary::AngleBlend(const FVector& SurfaceNormal, const FVector& UpDirection, float Falloff)
{
	const float Dot = FMath::Clamp(FVector::DotProduct(SurfaceNormal.GetSafeNormal(), UpDirection.GetSafeNormal()), 0.f, 1.f);
	return FMath::Pow(Dot, FMath::Max(Falloff, 0.01f));
}

float UARPGMaterialFunctionLibrary::DistanceFade(const FVector& WorldPosition, const FVector& Origin, float MaxDistance)
{
	if (MaxDistance <= KINDA_SMALL_NUMBER)
	{
		return 1.f;
	}
	const float Dist = FVector::Dist(WorldPosition, Origin);
	return FMath::Clamp(1.f - (Dist / MaxDistance), 0.f, 1.f);
}

// =====================================================================
// Convenience
// =====================================================================

void UARPGMaterialFunctionLibrary::SetScalarParameters(UMaterialInstanceDynamic* MID, const TMap<FName, float>& Parameters)
{
	if (!MID)
	{
		return;
	}
	for (const auto& Pair : Parameters)
	{
		MID->SetScalarParameterValue(Pair.Key, Pair.Value);
	}
}

void UARPGMaterialFunctionLibrary::SetVectorParameters(UMaterialInstanceDynamic* MID, const TMap<FName, FLinearColor>& Parameters)
{
	if (!MID)
	{
		return;
	}
	for (const auto& Pair : Parameters)
	{
		MID->SetVectorParameterValue(Pair.Key, Pair.Value);
	}
}

TArray<UMaterialInstanceDynamic*> UARPGMaterialFunctionLibrary::CreateDynamicMaterialsForMesh(UMeshComponent* MeshComp)
{
	TArray<UMaterialInstanceDynamic*> Result;
	if (!MeshComp)
	{
		return Result;
	}

	const int32 NumMats = MeshComp->GetNumMaterials();
	Result.Reserve(NumMats);

	for (int32 i = 0; i < NumMats; ++i)
	{
		UMaterialInterface* BaseMat = MeshComp->GetMaterial(i);
		if (BaseMat)
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, MeshComp);
			MeshComp->SetMaterial(i, MID);
			Result.Add(MID);
		}
		else
		{
			Result.Add(nullptr);
		}
	}

	return Result;
}

float UARPGMaterialFunctionLibrary::LerpScalarParameter(UMaterialInstanceDynamic* MID, FName ParamName, float TargetValue, float InterpSpeed, float DeltaTime)
{
	if (!MID)
	{
		return TargetValue;
	}

	float CurrentValue = 0.f;
	MID->GetScalarParameterValue(ParamName, CurrentValue);

	const float NewValue = FMath::FInterpTo(CurrentValue, TargetValue, DeltaTime, InterpSpeed);
	MID->SetScalarParameterValue(ParamName, NewValue);

	return NewValue;
}
