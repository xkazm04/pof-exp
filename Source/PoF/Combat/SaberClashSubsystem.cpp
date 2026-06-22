#include "Combat/SaberClashSubsystem.h"

#include "Character/ARPGCharacterBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/PointLight.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

namespace
{
	constexpr float CLASH_THRESHOLD = 30.f;    // cm between blade segments to count as a clash
	constexpr float CLASH_COOLDOWN = 0.18f;    // s between successive clashes
	constexpr float HITSTOP_SECONDS = 0.06f;   // real-time hit-stop duration
	constexpr float HITSTOP_DILATION = 0.08f;  // time scale during the hit-stop
	constexpr float FX_LIFESPAN = 0.25f;
}

bool USaberClashSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId USaberClashSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USaberClashSubsystem, STATGROUP_Tickables);
}

void USaberClashSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	bHadClashThisFrame = false;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Restore the hit-stop using REAL time (unaffected by the dilation we applied).
	if (bHitStopActive && World->GetRealTimeSeconds() >= HitStopUntilRealTime)
	{
		UGameplayStatics::SetGlobalTimeDilation(World, 1.0f);
		bHitStopActive = false;
	}

	if (ClashCooldown > 0.f)
	{
		ClashCooldown -= DeltaTime;
		return;
	}

	// Collect every visible saber blade segment.
	TArray<TPair<FVector, FVector>> Blades;
	for (TActorIterator<AARPGCharacterBase> It(World); It; ++It)
	{
		FVector S, E;
		if (It->GetSaberSegment(S, E))
		{
			Blades.Emplace(S, E);
		}
	}

	DiagAccum += DeltaTime;
	const bool bLogDiag = (Blades.Num() >= 2) && (DiagAccum >= 0.25f);
	if (bLogDiag) { DiagAccum = 0.f; }
	float MinDist = FLT_MAX;

	for (int32 i = 0; i < Blades.Num(); ++i)
	{
		for (int32 j = i + 1; j < Blades.Num(); ++j)
		{
			FVector P1, P2;
			FMath::SegmentDistToSegmentSafe(Blades[i].Key, Blades[i].Value, Blades[j].Key, Blades[j].Value, P1, P2);
			MinDist = FMath::Min(MinDist, (float)FVector::Dist(P1, P2));
			if (FVector::Dist(P1, P2) <= CLASH_THRESHOLD)
			{
				const FVector ClashLoc = (P1 + P2) * 0.5f;
				LastClashLocation = ClashLoc;
				bHadClashThisFrame = true;

				SpawnClashFX(World, ClashLoc);
				ClashCooldown = CLASH_COOLDOWN;
				UGameplayStatics::SetGlobalTimeDilation(World, HITSTOP_DILATION);
				HitStopUntilRealTime = World->GetRealTimeSeconds() + HITSTOP_SECONDS;
				bHitStopActive = true;
				UE_LOG(LogTemp, Display, TEXT("[SaberClash] clash at %s"), *ClashLoc.ToString());
				return;  // one clash per tick
			}
		}
	}

	if (bLogDiag)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[SaberClash] blades=%d minDist=%.0f"), Blades.Num(), MinDist);
	}
}

void USaberClashSubsystem::SpawnClashFX(UWorld* World, const FVector& Loc)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Bright point-light flash at the contact point.
	if (APointLight* Light = World->SpawnActor<APointLight>(APointLight::StaticClass(), Loc, FRotator::ZeroRotator, Params))
	{
		if (UPointLightComponent* LC = Cast<UPointLightComponent>(Light->GetLightComponent()))
		{
			LC->SetMobility(EComponentMobility::Movable);
			LC->SetIntensity(70000.f);
			LC->SetAttenuationRadius(400.f);
			LC->SetLightColor(FLinearColor(0.8f, 0.9f, 1.0f));
		}
		Light->SetLifeSpan(FX_LIFESPAN);
	}

	// Emissive spark ball.
	if (UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")))
	{
		if (AStaticMeshActor* Flash = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Loc, FRotator::ZeroRotator, Params))
		{
			UStaticMeshComponent* MC = Flash->GetStaticMeshComponent();
			MC->SetMobility(EComponentMobility::Movable);
			MC->SetStaticMesh(Sphere);
			MC->SetWorldScale3D(FVector(0.35f));
			MC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/FX/M_Saber_Blue.M_Saber_Blue")))
			{
				MC->SetMaterial(0, M);
			}
			Flash->SetLifeSpan(FX_LIFESPAN);
		}
	}
}
