#include "ARPGReplicationComponent.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "Character/ARPGCharacterBase.h"
#include "Player/ARPGPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UARPGReplicationComponent::UARPGReplicationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UARPGReplicationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Vitals change frequently on server — skip owner (server already knows), send to simulated proxies
	DOREPLIFETIME_CONDITION(UARPGReplicationComponent, ReplicatedVitals, COND_SkipOwner);
	// Combat state — all clients need it for animation/UI
	DOREPLIFETIME_CONDITION(UARPGReplicationComponent, bInCombat, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(UARPGReplicationComponent, ReplicatedDodgeDirection, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(UARPGReplicationComponent, bReplicatedSprinting, COND_SkipOwner);
}

void UARPGReplicationComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		SnapshotVitals();
	}
	else
	{
		// Clients don't need to tick — only server snapshots vitals
		PrimaryComponentTick.SetTickFunctionEnable(false);
	}
}

void UARPGReplicationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	VitalsUpdateTimer += DeltaTime;
	if (VitalsUpdateTimer >= VitalsUpdateRate)
	{
		VitalsUpdateTimer = 0.f;
		SnapshotVitals();
	}
}

void UARPGReplicationComponent::SnapshotVitals()
{
	AARPGCharacterBase* CharBase = Cast<AARPGCharacterBase>(GetOwner());
	if (!CharBase) return;

	UAbilitySystemComponent* ASC = CharBase->GetAbilitySystemComponent();

	// Read vitals from GAS attributes — works for all character types (player, enemy, boss)
	if (ASC)
	{
		bool bFound = false;
		ReplicatedVitals.Health = ASC->GetGameplayAttributeValue(UARPGAttributeSet::GetHealthAttribute(), bFound);
		if (!bFound) ReplicatedVitals.Health = 0.f;

		ReplicatedVitals.MaxHealth = ASC->GetGameplayAttributeValue(UARPGAttributeSet::GetMaxHealthAttribute(), bFound);
		if (!bFound) ReplicatedVitals.MaxHealth = 0.f;

		ReplicatedVitals.Mana = ASC->GetGameplayAttributeValue(UARPGAttributeSet::GetManaAttribute(), bFound);
		if (!bFound) ReplicatedVitals.Mana = 0.f;

		ReplicatedVitals.MaxMana = ASC->GetGameplayAttributeValue(UARPGAttributeSet::GetMaxManaAttribute(), bFound);
		if (!bFound) ReplicatedVitals.MaxMana = 0.f;
	}

	// Stamina is on the base character directly (not a GAS attribute)
	ReplicatedVitals.Stamina = CharBase->GetStamina();
	float Ratio = CharBase->GetStaminaRatio();
	ReplicatedVitals.MaxStamina = Ratio > 0.f ? CharBase->GetStamina() / Ratio : 0.f;

	// Player-specific: level
	if (AARPGPlayerCharacter* Player = Cast<AARPGPlayerCharacter>(CharBase))
	{
		ReplicatedVitals.CharacterLevel = Player->GetPlayerLevel();
	}
	else
	{
		ReplicatedVitals.CharacterLevel = 1;
	}

	bReplicatedSprinting = CharBase->IsSprinting();
	bInCombat = CharBase->IsAttacking();
	ReplicatedDodgeDirection = static_cast<uint8>(CharBase->GetDodgeDirection());
}

void UARPGReplicationComponent::OnRep_Vitals()
{
	OnVitalsReplicated.Broadcast(ReplicatedVitals);
}

// =====================================================================
// Server RPCs
// =====================================================================

bool UARPGReplicationComponent::Server_RequestAbilityActivation_Validate(const FARPGReplicatedAbilityEvent& AbilityEvent)
{
	// Basic validation: tag must be valid
	return AbilityEvent.AbilityTag.IsValid();
}

void UARPGReplicationComponent::Server_RequestAbilityActivation_Implementation(const FARPGReplicatedAbilityEvent& AbilityEvent)
{
	AARPGCharacterBase* CharBase = Cast<AARPGCharacterBase>(GetOwner());
	if (!CharBase) return;

	// The actual ability activation goes through GAS on the server
	UAbilitySystemComponent* ASC = CharBase->GetAbilitySystemComponent();
	if (!ASC) return;

	// Try to activate by tag
	FGameplayTagContainer TagContainer;
	TagContainer.AddTag(AbilityEvent.AbilityTag);
	if (!ASC->TryActivateAbilitiesByTag(TagContainer))
	{
		// Rejected — notify the owning client
		Client_AbilityActivationRejected(AbilityEvent.AbilityTag);
		return;
	}

	// Broadcast cosmetics to all clients
	Multicast_AbilityCosmetics(AbilityEvent);
}

bool UARPGReplicationComponent::Server_RequestDodge_Validate(FVector2D MoveInput)
{
	// Reasonable input magnitude
	return MoveInput.SizeSquared() <= 2.f;
}

void UARPGReplicationComponent::Server_RequestDodge_Implementation(FVector2D MoveInput)
{
	AARPGCharacterBase* CharBase = Cast<AARPGCharacterBase>(GetOwner());
	if (!CharBase) return;

	CharBase->TryDodge(MoveInput);
}

bool UARPGReplicationComponent::Server_RequestSprintChange_Validate(bool bSprinting)
{
	return true;
}

void UARPGReplicationComponent::Server_RequestSprintChange_Implementation(bool bSprinting)
{
	AARPGCharacterBase* CharBase = Cast<AARPGCharacterBase>(GetOwner());
	if (!CharBase) return;

	if (bSprinting)
	{
		CharBase->StartSprinting();
	}
	else
	{
		CharBase->StopSprinting();
	}
}

bool UARPGReplicationComponent::Server_RequestInteraction_Validate(AActor* InteractTarget)
{
	// Target must exist — nullptr is allowed (deselect)
	return true;
}

void UARPGReplicationComponent::Server_RequestInteraction_Implementation(AActor* InteractTarget)
{
	AARPGPlayerCharacter* Player = Cast<AARPGPlayerCharacter>(GetOwner());
	if (!Player) return;

	Player->PerformInteraction();
}

// =====================================================================
// Client RPCs
// =====================================================================

void UARPGReplicationComponent::Client_AbilityActivationRejected_Implementation(FGameplayTag AbilityTag)
{
	// Clients can bind to this to show "ability blocked" UI feedback
	UE_LOG(LogNet, Verbose, TEXT("Ability activation rejected: %s"), *AbilityTag.ToString());
}

void UARPGReplicationComponent::Client_ForceVitalsUpdate_Implementation(const FARPGReplicatedVitals& CorrectedVitals)
{
	ReplicatedVitals = CorrectedVitals;
	OnVitalsReplicated.Broadcast(ReplicatedVitals);
}

// =====================================================================
// Multicast RPCs
// =====================================================================

void UARPGReplicationComponent::Multicast_CombatEvent_Implementation(const FARPGReplicatedCombatEvent& CombatEvent)
{
	OnCombatEventReplicated.Broadcast(CombatEvent);
}

void UARPGReplicationComponent::Multicast_AbilityCosmetics_Implementation(const FARPGReplicatedAbilityEvent& AbilityEvent)
{
	// Skip on authority — already played locally via GAS
	if (GetOwner() && GetOwner()->HasAuthority()) return;
	// Skip on autonomous proxy — owning client already played locally
	AARPGCharacterBase* CharBase = Cast<AARPGCharacterBase>(GetOwner());
	if (!CharBase) return;
	if (CharBase->GetLocalRole() == ROLE_AutonomousProxy) return;

	// Cosmetic-only path for simulated proxies: broadcast a gameplay cue
	// so VFX/SFX play without activating the full GAS ability
	UAbilitySystemComponent* ASC = CharBase->GetAbilitySystemComponent();
	if (ASC)
	{
		FGameplayCueParameters CueParams;
		CueParams.Location = AbilityEvent.TargetLocation;
		CueParams.RawMagnitude = static_cast<float>(AbilityEvent.MontageSection);
		ASC->ExecuteGameplayCue(AbilityEvent.AbilityTag, CueParams);
	}
}

void UARPGReplicationComponent::Multicast_Death_Implementation()
{
	AARPGCharacterBase* CharBase = Cast<AARPGCharacterBase>(GetOwner());
	if (!CharBase) return;

	// Play death montage on simulated proxies
	if (CharBase->GetLocalRole() == ROLE_SimulatedProxy)
	{
		UAnimMontage* DeathMontage = CharBase->GetDeathMontage();
		if (DeathMontage)
		{
			CharBase->PlayAnimMontage(DeathMontage);
		}
	}
}

void UARPGReplicationComponent::Multicast_Respawn_Implementation(FVector RespawnLocation)
{
	AARPGCharacterBase* CharBase = Cast<AARPGCharacterBase>(GetOwner());
	if (!CharBase) return;

	if (CharBase->GetLocalRole() == ROLE_SimulatedProxy)
	{
		CharBase->SetActorLocation(RespawnLocation);
	}
}
