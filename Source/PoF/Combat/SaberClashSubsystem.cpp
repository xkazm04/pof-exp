#include "Combat/SaberClashSubsystem.h"

#include "Character/ARPGCharacterBase.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystemComponent.h"
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
	constexpr float CLASH_THRESHOLD = 30.f;    // cm between blade segments for a free clash
	constexpr float PARRY_RANGE = 170.f;       // saber range within which a timed block deflects
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

	// Collect every visible saber blade segment + its owner.
	struct FBlade { FVector Start; FVector End; AARPGCharacterBase* Owner; };
	TArray<FBlade> Blades;
	for (TActorIterator<AARPGCharacterBase> It(World); It; ++It)
	{
		FVector S, E;
		if (It->GetSaberSegment(S, E))
		{
			Blades.Add(FBlade{S, E, *It});
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
			FMath::SegmentDistToSegmentSafe(Blades[i].Start, Blades[i].End, Blades[j].Start, Blades[j].End, P1, P2);
			const float D = FVector::Dist(P1, P2);
			MinDist = FMath::Min(MinDist, D);

			// A parry: one fighter is blocking (in the parry window) while the other is mid-attack.
			// Use a generous saber range rather than exact blade contact — at duel spacing the
			// blades rarely cross to within a few cm, but a well-timed block still deflects the
			// swing. This is gameplay timing (block active + attacker swinging + in range), and
			// the FX still fires at the closest point between the two blades.
			AARPGCharacterBase* A = Blades[i].Owner;
			AARPGCharacterBase* B = Blades[j].Owner;
			AARPGCharacterBase* Defender = nullptr;
			AARPGCharacterBase* Attacker = nullptr;
			if (A && B)
			{
				if (A->IsParrying() && B->IsAttacking()) { Defender = A; Attacker = B; }
				else if (B->IsParrying() && A->IsAttacking()) { Defender = B; Attacker = A; }
			}
			const bool bParry = (Attacker != nullptr) && (D < PARRY_RANGE);

			if (bParry || D <= CLASH_THRESHOLD)
			{
				// For a parry, fire the FX ON the blocking blade (where it catches the swing)
				// rather than the midpoint, so the spark reads as the saber deflecting. P1 is
				// the closest point on Blades[i], P2 on Blades[j].
				FVector ClashLoc = (P1 + P2) * 0.5f;
				if (bParry)
				{
					ClashLoc = (Defender == A) ? P1 : P2;
				}
				LastClashLocation = ClashLoc;
				bHadClashThisFrame = true;

				SpawnClashFX(World, ClashLoc);
				ClashCooldown = CLASH_COOLDOWN;
				UGameplayStatics::SetGlobalTimeDilation(World, HITSTOP_DILATION);
				HitStopUntilRealTime = World->GetRealTimeSeconds() + HITSTOP_SECONDS;
				bHitStopActive = true;

				if (bParry)
				{
					if (UAbilitySystemComponent* AttASC = Attacker->GetAbilitySystemComponent())
					{
						FGameplayTagContainer MeleeTags;
						MeleeTags.AddTag(ARPGGameplayTags::Ability_Melee_LightAttack);
						MeleeTags.AddTag(ARPGGameplayTags::Ability_Enemy_Melee);
						AttASC->CancelAbilities(&MeleeTags);
					}
					Attacker->SetAttacking(false);
					UE_LOG(LogTemp, Display, TEXT("[SaberClash] PARRY! %s deflected %s (d=%.0f)"),
						*Defender->GetName(), *Attacker->GetName(), D);
				}
				else
				{
					UE_LOG(LogTemp, Display, TEXT("[SaberClash] clash at %s"), *ClashLoc.ToString());
				}
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
