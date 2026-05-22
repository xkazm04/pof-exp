#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ARPGSquadManager.generated.h"

class AARPGEnemyCharacter;
class AActor;

/**
 * Manages a group of enemies as a coordinated squad.
 * Handles attack token distribution (limits simultaneous attackers),
 * flanking assignment, and focus-fire coordination.
 *
 * Created per-encounter by spawn volumes or level scripts.
 */
UCLASS(BlueprintType, Blueprintable)
class POF_API UARPGSquadManager : public UObject
{
	GENERATED_BODY()

public:
	UARPGSquadManager();

	// =====================================================================
	// Squad Membership
	// =====================================================================

	UFUNCTION(BlueprintCallable, Category = "AI|Squad")
	void RegisterMember(AARPGEnemyCharacter* Enemy);

	UFUNCTION(BlueprintCallable, Category = "AI|Squad")
	void UnregisterMember(AARPGEnemyCharacter* Enemy);

	UFUNCTION(BlueprintPure, Category = "AI|Squad")
	int32 GetMemberCount() const { return Members.Num(); }

	UFUNCTION(BlueprintPure, Category = "AI|Squad")
	TArray<AARPGEnemyCharacter*> GetAliveMembers() const;

	// =====================================================================
	// Attack Tokens — limits simultaneous attackers
	// =====================================================================

	/** Try to claim an attack token. Returns true if the enemy may attack. */
	UFUNCTION(BlueprintCallable, Category = "AI|Squad|Tokens")
	bool RequestAttackToken(AARPGEnemyCharacter* Requester);

	/** Release a previously claimed attack token. */
	UFUNCTION(BlueprintCallable, Category = "AI|Squad|Tokens")
	void ReleaseAttackToken(AARPGEnemyCharacter* Holder);

	/** Check if an enemy currently holds an attack token. */
	UFUNCTION(BlueprintPure, Category = "AI|Squad|Tokens")
	bool HasAttackToken(AARPGEnemyCharacter* Enemy) const;

	/** Maximum number of enemies that can attack simultaneously. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Squad|Tokens", meta = (ClampMin = "1"))
	int32 MaxSimultaneousAttackers = 2;

	// =====================================================================
	// Flanking
	// =====================================================================

	/**
	 * Assign a flank angle offset (in degrees) for the given enemy relative to the target.
	 * Enemies spread evenly around the target to avoid clustering.
	 * Returns the world position the enemy should move to for flanking.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Squad|Flank")
	FVector GetFlankPosition(AARPGEnemyCharacter* Enemy, AActor* Target, float DesiredDistance) const;

	// =====================================================================
	// Focus Fire
	// =====================================================================

	/** Set a shared focus target for the entire squad. */
	UFUNCTION(BlueprintCallable, Category = "AI|Squad|Focus")
	void SetFocusTarget(AActor* NewTarget);

	UFUNCTION(BlueprintPure, Category = "AI|Squad|Focus")
	AActor* GetFocusTarget() const { return FocusTarget.Get(); }

	// =====================================================================
	// Retreat Coordination
	// =====================================================================

	/**
	 * Signal that an enemy should retreat. If enough squad members are low HP,
	 * triggers a coordinated retreat for remaining members.
	 * Returns true if a coordinated retreat was triggered.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Squad|Retreat")
	bool SignalRetreat(AARPGEnemyCharacter* Retreater);

	/** Health ratio below which a member is counted as "low HP" for retreat decisions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Squad|Retreat", meta = (ClampMin = "0", ClampMax = "1"))
	float RetreatHealthThreshold = 0.25f;

	/** Fraction of squad that must be low HP to trigger coordinated retreat (0.5 = half). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Squad|Retreat", meta = (ClampMin = "0", ClampMax = "1"))
	float CoordinatedRetreatFraction = 0.5f;

private:
	UPROPERTY()
	TArray<TWeakObjectPtr<AARPGEnemyCharacter>> Members;

	UPROPERTY()
	TArray<TWeakObjectPtr<AARPGEnemyCharacter>> AttackTokenHolders;

	TWeakObjectPtr<AActor> FocusTarget;

	void CleanupStaleMembers();
};
