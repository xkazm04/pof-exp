#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ARPGReplicationTypes.h"
#include "ARPGReplicationComponent.generated.h"

class AARPGCharacterBase;

/** Broadcast when vitals are replicated to clients. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVitalsReplicated, const FARPGReplicatedVitals&, Vitals);

/** Broadcast when a combat event is replicated to clients. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatEventReplicated, const FARPGReplicatedCombatEvent&, CombatEvent);

/**
 * Replication component attached to characters.
 * Handles:
 *  - Replicated vitals snapshot (health, mana, stamina, level)
 *  - Server/Client/Multicast RPCs for combat events and ability activation
 *  - Server RPC validation
 */
UCLASS(ClassGroup = (Multiplayer), meta = (BlueprintSpawnableComponent))
class POF_API UARPGReplicationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARPGReplicationComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// =====================================================================
	// Replicated Properties
	// =====================================================================

	/** Current vitals snapshot, replicated to all clients. */
	UFUNCTION(BlueprintPure, Category = "Replication")
	const FARPGReplicatedVitals& GetReplicatedVitals() const { return ReplicatedVitals; }

	/** Is the owning character currently alive according to replicated state? */
	UFUNCTION(BlueprintPure, Category = "Replication")
	bool IsAliveReplicated() const { return ReplicatedVitals.Health > 0.f; }

	// =====================================================================
	// Server RPCs — called from owning client, executed on server
	// =====================================================================

	/** Client requests to use an ability. Server validates and executes. */
	UFUNCTION(Server, Reliable, WithValidation, Category = "Replication|RPC")
	void Server_RequestAbilityActivation(const FARPGReplicatedAbilityEvent& AbilityEvent);

	/** Client requests a dodge in a given direction. */
	UFUNCTION(Server, Reliable, WithValidation, Category = "Replication|RPC")
	void Server_RequestDodge(FVector2D MoveInput);

	/** Client requests sprint state change. */
	UFUNCTION(Server, Reliable, WithValidation, Category = "Replication|RPC")
	void Server_RequestSprintChange(bool bSprinting);

	/** Client requests interaction with an actor. */
	UFUNCTION(Server, Reliable, WithValidation, Category = "Replication|RPC")
	void Server_RequestInteraction(AActor* InteractTarget);

	// =====================================================================
	// Client RPCs — called from server, executed on owning client
	// =====================================================================

	/** Server notifies the owning client that an ability activation was rejected. */
	UFUNCTION(Client, Reliable, Category = "Replication|RPC")
	void Client_AbilityActivationRejected(FGameplayTag AbilityTag);

	/** Server force-corrects client vitals (e.g., after server-side heal/damage). */
	UFUNCTION(Client, Reliable, Category = "Replication|RPC")
	void Client_ForceVitalsUpdate(const FARPGReplicatedVitals& CorrectedVitals);

	// =====================================================================
	// Multicast RPCs — called from server, executed on all clients
	// =====================================================================

	/** Replicate a combat event (damage/heal) to all clients for VFX/SFX. */
	UFUNCTION(NetMulticast, Unreliable, Category = "Replication|RPC")
	void Multicast_CombatEvent(const FARPGReplicatedCombatEvent& CombatEvent);

	/** Replicate ability cosmetics (animation, VFX) to all clients. */
	UFUNCTION(NetMulticast, Unreliable, Category = "Replication|RPC")
	void Multicast_AbilityCosmetics(const FARPGReplicatedAbilityEvent& AbilityEvent);

	/** Replicate death event to all clients. */
	UFUNCTION(NetMulticast, Reliable, Category = "Replication|RPC")
	void Multicast_Death();

	/** Replicate respawn event to all clients. */
	UFUNCTION(NetMulticast, Reliable, Category = "Replication|RPC")
	void Multicast_Respawn(FVector RespawnLocation);

	// =====================================================================
	// Delegates
	// =====================================================================

	UPROPERTY(BlueprintAssignable, Category = "Events|Replication")
	FOnVitalsReplicated OnVitalsReplicated;

	UPROPERTY(BlueprintAssignable, Category = "Events|Replication")
	FOnCombatEventReplicated OnCombatEventReplicated;

protected:
	/** How often (seconds) to snapshot vitals for replication. Lower = more bandwidth. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication|Tuning", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float VitalsUpdateRate = 0.1f;

	UPROPERTY(ReplicatedUsing = OnRep_Vitals, BlueprintReadOnly, Category = "Replication")
	FARPGReplicatedVitals ReplicatedVitals;

	/** Whether the character is in combat (replicated for UI indicators). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Replication")
	bool bInCombat = false;

	/** Current dodge direction (replicated for animation sync). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Replication")
	uint8 ReplicatedDodgeDirection = 0;

	/** Whether sprinting (replicated for animation/speed sync). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Replication")
	bool bReplicatedSprinting = false;

private:
	UFUNCTION()
	void OnRep_Vitals();

	float VitalsUpdateTimer = 0.f;

	/** Sample current character state into the vitals struct. Server only. */
	void SnapshotVitals();
};
