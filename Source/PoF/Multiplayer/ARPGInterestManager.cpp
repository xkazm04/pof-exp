#include "ARPGInterestManager.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

UARPGInterestManager::UARPGInterestManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f; // 10Hz is sufficient for relevancy checks
	SetIsReplicatedByDefault(false); // Relevancy is server-side only

	InitDefaultTierConfigs();
}

void UARPGInterestManager::InitDefaultTierConfigs()
{
	// AlwaysRelevant
	{
		FARPGRelevancyTierConfig Config;
		Config.MaxRelevancyDistance = 0.f; // Infinite
		Config.UpdateFrequency = 100.f;
		Config.PriorityMultiplier = 3.f;
		TierConfigs.Add(EARPGRelevancyTier::AlwaysRelevant, Config);
	}

	// Critical
	{
		FARPGRelevancyTierConfig Config;
		Config.MaxRelevancyDistance = 15000.f;
		Config.UpdateFrequency = 60.f;
		Config.PriorityMultiplier = 2.5f;
		TierConfigs.Add(EARPGRelevancyTier::Critical, Config);
	}

	// Standard
	{
		FARPGRelevancyTierConfig Config;
		Config.MaxRelevancyDistance = 8000.f;
		Config.UpdateFrequency = 30.f;
		Config.PriorityMultiplier = 1.f;
		TierConfigs.Add(EARPGRelevancyTier::Standard, Config);
	}

	// Background
	{
		FARPGRelevancyTierConfig Config;
		Config.MaxRelevancyDistance = 15000.f;
		Config.UpdateFrequency = 10.f;
		Config.PriorityMultiplier = 0.5f;
		TierConfigs.Add(EARPGRelevancyTier::Background, Config);
	}

	// Dormant
	{
		FARPGRelevancyTierConfig Config;
		Config.MaxRelevancyDistance = 5000.f;
		Config.UpdateFrequency = 1.f;
		Config.PriorityMultiplier = 0.1f;
		TierConfigs.Add(EARPGRelevancyTier::Dormant, Config);
	}
}

void UARPGInterestManager::BeginPlay()
{
	Super::BeginPlay();

	CurrentTier = BaseTier;

	// Only server needs to run relevancy calculations
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		PrimaryComponentTick.SetTickFunctionEnable(false);
		return;
	}

	ApplyTierSettings();
}

void UARPGInterestManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	DistanceCheckTimer += DeltaTime;
	if (DistanceCheckTimer >= DistanceCheckInterval)
	{
		DistanceCheckTimer = 0.f;
		UpdateDistanceToPlayers();

		if (bAutoTierByDistance && !bInCombatRelevancy && !bIsDormant)
		{
			UpdateAutoTier();
		}
	}
}

// =====================================================================
// Tier Management
// =====================================================================

void UARPGInterestManager::SetRelevancyTier(EARPGRelevancyTier NewTier)
{
	if (CurrentTier == NewTier) return;

	CurrentTier = NewTier;
	ApplyTierSettings();
}

FARPGRelevancyTierConfig UARPGInterestManager::GetTierConfig(EARPGRelevancyTier Tier) const
{
	if (const FARPGRelevancyTierConfig* Config = TierConfigs.Find(Tier))
	{
		return *Config;
	}
	return FARPGRelevancyTierConfig();
}

void UARPGInterestManager::ApplyTierSettings()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	FARPGRelevancyTierConfig Config = GetTierConfig(CurrentTier);

	// Apply net update frequency to the owning actor
	Owner->SetNetUpdateFrequency(Config.UpdateFrequency);
	Owner->NetPriority = Config.PriorityMultiplier;

	// Set always-relevant flag
	Owner->bAlwaysRelevant = (CurrentTier == EARPGRelevancyTier::AlwaysRelevant);

	// Net cull distance — squared distance for relevancy check
	if (Config.MaxRelevancyDistance > 0.f)
	{
		Owner->SetNetCullDistanceSquared(Config.MaxRelevancyDistance * Config.MaxRelevancyDistance);
	}
	else
	{
		Owner->SetNetCullDistanceSquared(0.f); // No culling
	}

	// Dormancy
	if (CurrentTier == EARPGRelevancyTier::Dormant)
	{
		Owner->SetNetDormancy(DORM_DormantAll);
	}
	else if (bIsDormant)
	{
		// Was dormant, now waking up
		Owner->SetNetDormancy(DORM_Awake);
	}
}

// =====================================================================
// Dynamic State
// =====================================================================

void UARPGInterestManager::EnterCombatRelevancy()
{
	if (bInCombatRelevancy) return;

	bInCombatRelevancy = true;
	SetRelevancyTier(EARPGRelevancyTier::Critical);
}

void UARPGInterestManager::ExitCombatRelevancy()
{
	if (!bInCombatRelevancy) return;

	bInCombatRelevancy = false;
	SetRelevancyTier(BaseTier);
}

void UARPGInterestManager::EnterDormancy()
{
	if (bIsDormant) return;

	bIsDormant = true;
	SetRelevancyTier(EARPGRelevancyTier::Dormant);
}

void UARPGInterestManager::WakeFromDormancy()
{
	if (!bIsDormant) return;

	bIsDormant = false;

	if (bInCombatRelevancy)
	{
		SetRelevancyTier(EARPGRelevancyTier::Critical);
	}
	else
	{
		SetRelevancyTier(BaseTier);
	}
}

// =====================================================================
// Distance
// =====================================================================

void UARPGInterestManager::UpdateDistanceToPlayers()
{
	AActor* Owner = GetOwner();
	UWorld* World = Owner ? Owner->GetWorld() : nullptr;
	if (!World) return;

	FVector OwnerLoc = Owner->GetActorLocation();
	float MinDist = TNumericLimits<float>::Max();
	bool bFoundPlayer = false;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC) continue;

		APawn* Pawn = PC->GetPawn();
		if (!Pawn) continue;

		float Dist = FVector::Dist(OwnerLoc, Pawn->GetActorLocation());
		if (Dist < MinDist)
		{
			MinDist = Dist;
			bFoundPlayer = true;
		}
	}

	DistanceToNearestPlayer = bFoundPlayer ? MinDist : TNumericLimits<float>::Max();

	// Check relevancy against current tier's max distance
	FARPGRelevancyTierConfig Config = GetTierConfig(CurrentTier);
	bIsRelevant = (Config.MaxRelevancyDistance <= 0.f) || (DistanceToNearestPlayer <= Config.MaxRelevancyDistance);
}

void UARPGInterestManager::UpdateAutoTier()
{
	if (DistanceToNearestPlayer <= CriticalDistanceBand)
	{
		if (CurrentTier != EARPGRelevancyTier::Critical)
		{
			SetRelevancyTier(EARPGRelevancyTier::Critical);
		}
	}
	else if (DistanceToNearestPlayer <= StandardDistanceBand)
	{
		if (CurrentTier != EARPGRelevancyTier::Standard)
		{
			SetRelevancyTier(EARPGRelevancyTier::Standard);
		}
	}
	else if (DistanceToNearestPlayer <= BackgroundDistanceBand)
	{
		if (CurrentTier != EARPGRelevancyTier::Background)
		{
			SetRelevancyTier(EARPGRelevancyTier::Background);
		}
	}
	else
	{
		// Beyond all bands — dormant
		if (CurrentTier != EARPGRelevancyTier::Dormant)
		{
			SetRelevancyTier(EARPGRelevancyTier::Dormant);
		}
	}
}
