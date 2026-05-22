#include "Debug/ARPGDebugDrawHelpers.h"
#include "Engine/World.h"

static TAutoConsoleVariable<int32> CVarARPGDebugDraw(
	TEXT("ARPG.Debug.Draw"),
	0,
	TEXT("Toggle ARPG debug drawing (0 = off, 1 = on)"),
	ECVF_Cheat);

bool UARPGDebugDrawHelpers::IsDebugDrawEnabled()
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return CVarARPGDebugDraw.GetValueOnGameThread() != 0;
#endif
}

void UARPGDebugDrawHelpers::DrawWeaponTrace(const UObject* WorldContextObject,
	const FVector& Start, const FVector& End, float Radius, bool bHit, float Duration)
{
#if ENABLE_DRAW_DEBUG
	if (!IsDebugDrawEnabled()) return;
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World) return;

	const FColor Color = bHit ? FColor::Red : FColor::Green;
	DrawDebugCapsule(World, (Start + End) * 0.5f, (End - Start).Size() * 0.5f, Radius,
		FRotationMatrix::MakeFromZ(End - Start).Rotator().Quaternion(), Color, false, Duration);
	if (bHit)
	{
		DrawDebugPoint(World, End, 10.f, FColor::Red, false, Duration);
	}
#endif
}

void UARPGDebugDrawHelpers::DrawAbilityRange(const UObject* WorldContextObject,
	const FVector& Center, float Radius, FLinearColor Color, float Duration)
{
#if ENABLE_DRAW_DEBUG
	if (!IsDebugDrawEnabled()) return;
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World) return;

	DrawDebugSphere(World, Center, Radius, 24, Color.ToFColor(true), false, Duration);
#endif
}

void UARPGDebugDrawHelpers::DrawHitImpact(const UObject* WorldContextObject,
	const FVector& Location, const FVector& Normal, float Size, float Duration)
{
#if ENABLE_DRAW_DEBUG
	if (!IsDebugDrawEnabled()) return;
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World) return;

	DrawDebugPoint(World, Location, 8.f, FColor::Red, false, Duration);
	DrawDebugDirectionalArrow(World, Location, Location + Normal * Size, 5.f, FColor::Yellow, false, Duration);
#endif
}

void UARPGDebugDrawHelpers::DrawAIPath(const UObject* WorldContextObject,
	const TArray<FVector>& PathPoints, FLinearColor Color, float Duration)
{
#if ENABLE_DRAW_DEBUG
	if (!IsDebugDrawEnabled()) return;
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World) return;

	const FColor DrawColor = Color.ToFColor(true);
	for (int32 i = 0; i < PathPoints.Num(); ++i)
	{
		DrawDebugSphere(World, PathPoints[i], 15.f, 8, DrawColor, false, Duration);
		if (i > 0)
		{
			DrawDebugLine(World, PathPoints[i - 1], PathPoints[i], DrawColor, false, Duration, 0, 2.f);
		}
	}
#endif
}

void UARPGDebugDrawHelpers::DrawPerceptionRange(const UObject* WorldContextObject,
	const FVector& Center, float SightRange, float HearingRange, float Duration)
{
#if ENABLE_DRAW_DEBUG
	if (!IsDebugDrawEnabled()) return;
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World) return;

	DrawDebugSphere(World, Center, SightRange, 16, FColor::Blue, false, Duration);
	if (HearingRange > 0.f)
	{
		DrawDebugSphere(World, Center, HearingRange, 16, FColor::Purple, false, Duration);
	}
#endif
}

void UARPGDebugDrawHelpers::DrawAttackCone(const UObject* WorldContextObject,
	const FVector& Origin, const FVector& Direction, float Range, float HalfAngleDeg,
	FLinearColor Color, float Duration)
{
#if ENABLE_DRAW_DEBUG
	if (!IsDebugDrawEnabled()) return;
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World) return;

	const FColor DrawColor = Color.ToFColor(true);
	const FVector Dir = Direction.GetSafeNormal();
	const float HalfAngleRad = FMath::DegreesToRadians(HalfAngleDeg);
	constexpr int32 NumSegments = 12;
	const FVector Right = FVector::CrossProduct(Dir, FVector::UpVector).GetSafeNormal();
	const FVector Up = FVector::CrossProduct(Right, Dir);

	for (int32 i = 0; i <= NumSegments; ++i)
	{
		const float Angle = 2.f * PI * static_cast<float>(i) / static_cast<float>(NumSegments);
		const FVector RimDir = (Dir * FMath::Cos(HalfAngleRad)
			+ (Right * FMath::Cos(Angle) + Up * FMath::Sin(Angle)) * FMath::Sin(HalfAngleRad)).GetSafeNormal();
		DrawDebugLine(World, Origin, Origin + RimDir * Range, DrawColor, false, Duration, 0, 1.f);
	}
#endif
}

void UARPGDebugDrawHelpers::DrawSpawnPoint(const UObject* WorldContextObject,
	const FVector& Location, const FString& Label, FLinearColor Color, float Duration)
{
#if ENABLE_DRAW_DEBUG
	if (!IsDebugDrawEnabled()) return;
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World) return;

	const FColor DrawColor = Color.ToFColor(true);
	const FVector Top = Location + FVector(0, 0, 100.f);
	DrawDebugDirectionalArrow(World, Top, Location, 20.f, DrawColor, false, Duration, 0, 3.f);
	DrawDebugString(World, Top + FVector(0, 0, 20.f), Label, nullptr, DrawColor, Duration);
#endif
}

void UARPGDebugDrawHelpers::DrawBoxVolume(const UObject* WorldContextObject,
	const FVector& Center, const FVector& Extent, const FRotator& Rotation, FLinearColor Color, float Duration)
{
#if ENABLE_DRAW_DEBUG
	if (!IsDebugDrawEnabled()) return;
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World) return;

	DrawDebugBox(World, Center, Extent, Rotation.Quaternion(), Color.ToFColor(true), false, Duration, 0, 2.f);
#endif
}

void UARPGDebugDrawHelpers::DrawOverlapSphere(const UObject* WorldContextObject,
	const FVector& Center, float Radius, const TArray<FVector>& HitLocations, float Duration)
{
#if ENABLE_DRAW_DEBUG
	if (!IsDebugDrawEnabled()) return;
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World) return;

	DrawDebugSphere(World, Center, Radius, 16, FColor::Cyan, false, Duration);
	for (const FVector& Loc : HitLocations)
	{
		DrawDebugPoint(World, Loc, 8.f, FColor::Red, false, Duration);
		DrawDebugLine(World, Center, Loc, FColor::Yellow, false, Duration, 0, 1.f);
	}
#endif
}
