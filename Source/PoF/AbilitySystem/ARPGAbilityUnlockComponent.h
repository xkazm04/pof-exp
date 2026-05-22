#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AbilitySystem/ARPGAbilityUnlockData.h"
#include "ARPGAbilityUnlockComponent.generated.h"

class UAbilitySystemComponent;
class UDataTable;

/** Broadcast when an ability is learned: (AbilityRowName, Rank) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAbilityLearned, FName, AbilityRowName, int32, Rank);

/** Broadcast when an ability is upgraded: (AbilityRowName, NewRank) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAbilityUpgraded, FName, AbilityRowName, int32, NewRank);

/** Broadcast when skill points change: (CurrentSkillPoints) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillPointsChanged, int32, CurrentSkillPoints);

/**
 * Component that manages the player's ability unlock and upgrade system.
 *
 * Tracks which abilities have been learned and their current rank.
 * Interfaces with the Ability System Component to grant/update abilities.
 * Consumes skill points to learn and upgrade abilities.
 */
UCLASS(ClassGroup = "AbilitySystem", meta = (BlueprintSpawnableComponent))
class POF_API UARPGAbilityUnlockComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARPGAbilityUnlockComponent();

	// =====================================================================
	// Learn / Upgrade
	// =====================================================================

	/**
	 * Learn an ability from the data table by row name.
	 * Spends skill points, grants the ability to the ASC at level 1.
	 * @param AbilityRowName  Row name in the AbilityUnlockTable.
	 * @return true if the ability was successfully learned.
	 */
	UFUNCTION(BlueprintCallable, Category = "AbilityUnlock")
	bool LearnAbility(FName AbilityRowName);

	/**
	 * Upgrade an already-learned ability to the next rank.
	 * Spends skill points, increases the ability spec level in the ASC.
	 * @param AbilityRowName  Row name in the AbilityUnlockTable.
	 * @return true if the ability was successfully upgraded.
	 */
	UFUNCTION(BlueprintCallable, Category = "AbilityUnlock")
	bool UpgradeAbility(FName AbilityRowName);

	// =====================================================================
	// Queries
	// =====================================================================

	/** Whether the player can learn this ability (has level, points, and hasn't learned it). */
	UFUNCTION(BlueprintPure, Category = "AbilityUnlock")
	bool CanLearnAbility(FName AbilityRowName) const;

	/** Whether the player can upgrade this ability (learned, not max rank, has points). */
	UFUNCTION(BlueprintPure, Category = "AbilityUnlock")
	bool CanUpgradeAbility(FName AbilityRowName) const;

	/** Whether the player has learned this ability. */
	UFUNCTION(BlueprintPure, Category = "AbilityUnlock")
	bool HasLearnedAbility(FName AbilityRowName) const;

	/** Get the current rank of a learned ability. Returns 0 if not learned. */
	UFUNCTION(BlueprintPure, Category = "AbilityUnlock")
	int32 GetAbilityRank(FName AbilityRowName) const;

	/** Get the data table row for an ability. Returns null if not found. */
	UFUNCTION(BlueprintPure, Category = "AbilityUnlock")
	bool GetAbilityUnlockInfo(FName AbilityRowName, FAbilityUnlockRow& OutRow) const;

	/** Get all row names from the ability unlock table. */
	UFUNCTION(BlueprintPure, Category = "AbilityUnlock")
	TArray<FName> GetAllAbilityRowNames() const;

	/** Get all learned ability row names. */
	UFUNCTION(BlueprintPure, Category = "AbilityUnlock")
	TArray<FName> GetLearnedAbilityRowNames() const;

	// =====================================================================
	// Skill Points
	// =====================================================================

	UFUNCTION(BlueprintPure, Category = "AbilityUnlock")
	int32 GetSkillPoints() const { return SkillPoints; }

	UFUNCTION(BlueprintCallable, Category = "AbilityUnlock")
	void AddSkillPoints(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "AbilityUnlock")
	void SetSkillPoints(int32 Amount) { SkillPoints = Amount; OnSkillPointsChanged.Broadcast(SkillPoints); }

	UFUNCTION(BlueprintPure, Category = "AbilityUnlock")
	int32 GetSkillPointsPerLevel() const { return SkillPointsPerLevel; }

	// =====================================================================
	// Save / Load
	// =====================================================================

	/** Get the learned abilities map for saving. */
	const TMap<FName, int32>& GetLearnedAbilitiesMap() const { return LearnedAbilities; }

	/** Restore learned abilities from saved data. Re-grants abilities to the ASC. */
	void RestoreLearnedAbilities(const TMap<FName, int32>& SavedAbilities);

	// =====================================================================
	// Delegates
	// =====================================================================

	UPROPERTY(BlueprintAssignable, Category = "AbilityUnlock|Events")
	FOnAbilityLearned OnAbilityLearned;

	UPROPERTY(BlueprintAssignable, Category = "AbilityUnlock|Events")
	FOnAbilityUpgraded OnAbilityUpgraded;

	UPROPERTY(BlueprintAssignable, Category = "AbilityUnlock|Events")
	FOnSkillPointsChanged OnSkillPointsChanged;

protected:
	/** Data Table containing FAbilityUnlockRow entries. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityUnlock")
	TObjectPtr<UDataTable> AbilityUnlockTable;

	/** Current skill points available to spend. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilityUnlock")
	int32 SkillPoints = 0;

	/** Skill points awarded per level-up. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityUnlock", meta = (ClampMin = "1"))
	int32 SkillPointsPerLevel = 2;

private:
	/** Map of learned abilities: RowName -> CurrentRank */
	UPROPERTY()
	TMap<FName, int32> LearnedAbilities;

	/** Find the data table row. Returns nullptr if table or row is missing. */
	const FAbilityUnlockRow* FindRow(FName AbilityRowName) const;

	/** Get the owning actor's ASC. */
	UAbilitySystemComponent* GetASC() const;

	/** Get the owning player's level. */
	int32 GetPlayerLevel() const;
};
