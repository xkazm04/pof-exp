#include "AbilitySystem/ARPGAbilityUnlockComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/ARPGGameplayAbility.h"
#include "Player/ARPGPlayerCharacter.h"
#include "Engine/DataTable.h"

UARPGAbilityUnlockComponent::UARPGAbilityUnlockComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// =========================================================================
// Learn / Upgrade
// =========================================================================

bool UARPGAbilityUnlockComponent::LearnAbility(FName AbilityRowName)
{
	if (!CanLearnAbility(AbilityRowName))
	{
		return false;
	}

	const FAbilityUnlockRow* Row = FindRow(AbilityRowName);
	if (!Row)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC)
	{
		return false;
	}

	// Spend skill points
	SkillPoints -= Row->SkillPointCost;
	OnSkillPointsChanged.Broadcast(SkillPoints);

	// Grant the ability at level 1
	FGameplayAbilitySpec AbilitySpec(Row->AbilityClass, 1);
	ASC->GiveAbility(AbilitySpec);

	// Track it
	LearnedAbilities.Add(AbilityRowName, 1);

	// Auto-assign to first empty hotbar slot
	if (AARPGPlayerCharacter* Player = Cast<AARPGPlayerCharacter>(GetOwner()))
	{
		const int32 SlotCount = Player->GetAbilitySlotCount();
		for (int32 i = 0; i < SlotCount; ++i)
		{
			if (!Player->GetSlotAbility(i))
			{
				Player->AssignAbilityToSlot(i, Row->AbilityClass);
				UE_LOG(LogTemp, Log, TEXT("[AbilityUnlock] Auto-assigned '%s' to hotbar slot %d"),
					*AbilityRowName.ToString(), i);
				break;
			}
		}
	}

	OnAbilityLearned.Broadcast(AbilityRowName, 1);

	UE_LOG(LogTemp, Log, TEXT("[AbilityUnlock] Learned ability '%s' (Rank 1). Skill points remaining: %d"),
		*AbilityRowName.ToString(), SkillPoints);

	return true;
}

bool UARPGAbilityUnlockComponent::UpgradeAbility(FName AbilityRowName)
{
	if (!CanUpgradeAbility(AbilityRowName))
	{
		return false;
	}

	const FAbilityUnlockRow* Row = FindRow(AbilityRowName);
	if (!Row)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC)
	{
		return false;
	}

	// Spend skill points
	SkillPoints -= Row->UpgradeSkillPointCost;
	OnSkillPointsChanged.Broadcast(SkillPoints);

	// Increase rank
	int32& CurrentRank = LearnedAbilities.FindChecked(AbilityRowName);
	CurrentRank++;

	// Update the ability spec level in the ASC
	FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromClass(Row->AbilityClass);
	if (Spec)
	{
		Spec->Level = CurrentRank;
		ASC->MarkAbilitySpecDirty(*Spec);
	}

	OnAbilityUpgraded.Broadcast(AbilityRowName, CurrentRank);

	UE_LOG(LogTemp, Log, TEXT("[AbilityUnlock] Upgraded ability '%s' to Rank %d. Skill points remaining: %d"),
		*AbilityRowName.ToString(), CurrentRank, SkillPoints);

	return true;
}

// =========================================================================
// Queries
// =========================================================================

bool UARPGAbilityUnlockComponent::CanLearnAbility(FName AbilityRowName) const
{
	// Already learned?
	if (LearnedAbilities.Contains(AbilityRowName))
	{
		return false;
	}

	const FAbilityUnlockRow* Row = FindRow(AbilityRowName);
	if (!Row || !Row->AbilityClass)
	{
		return false;
	}

	// Level check
	if (GetPlayerLevel() < Row->RequiredLevel)
	{
		return false;
	}

	// Skill point check
	if (SkillPoints < Row->SkillPointCost)
	{
		return false;
	}

	return true;
}

bool UARPGAbilityUnlockComponent::CanUpgradeAbility(FName AbilityRowName) const
{
	const int32* RankPtr = LearnedAbilities.Find(AbilityRowName);
	if (!RankPtr)
	{
		return false; // Not learned
	}

	const FAbilityUnlockRow* Row = FindRow(AbilityRowName);
	if (!Row)
	{
		return false;
	}

	// Max rank check
	if (*RankPtr >= Row->MaxRank)
	{
		return false;
	}

	// Skill point check
	if (SkillPoints < Row->UpgradeSkillPointCost)
	{
		return false;
	}

	return true;
}

bool UARPGAbilityUnlockComponent::HasLearnedAbility(FName AbilityRowName) const
{
	return LearnedAbilities.Contains(AbilityRowName);
}

int32 UARPGAbilityUnlockComponent::GetAbilityRank(FName AbilityRowName) const
{
	const int32* RankPtr = LearnedAbilities.Find(AbilityRowName);
	return RankPtr ? *RankPtr : 0;
}

bool UARPGAbilityUnlockComponent::GetAbilityUnlockInfo(FName AbilityRowName, FAbilityUnlockRow& OutRow) const
{
	const FAbilityUnlockRow* Row = FindRow(AbilityRowName);
	if (!Row)
	{
		return false;
	}
	OutRow = *Row;
	return true;
}

TArray<FName> UARPGAbilityUnlockComponent::GetAllAbilityRowNames() const
{
	TArray<FName> Names;
	if (AbilityUnlockTable)
	{
		Names = AbilityUnlockTable->GetRowNames();
	}
	return Names;
}

TArray<FName> UARPGAbilityUnlockComponent::GetLearnedAbilityRowNames() const
{
	TArray<FName> Names;
	LearnedAbilities.GetKeys(Names);
	return Names;
}

// =========================================================================
// Skill Points
// =========================================================================

void UARPGAbilityUnlockComponent::AddSkillPoints(int32 Amount)
{
	if (Amount <= 0) return;

	SkillPoints += Amount;
	OnSkillPointsChanged.Broadcast(SkillPoints);

	UE_LOG(LogTemp, Log, TEXT("[AbilityUnlock] Gained %d skill point(s). Total: %d"), Amount, SkillPoints);
}

// =========================================================================
// Save / Load
// =========================================================================

void UARPGAbilityUnlockComponent::RestoreLearnedAbilities(const TMap<FName, int32>& SavedAbilities)
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC) return;

	LearnedAbilities = SavedAbilities;

	// Re-grant each ability at the saved rank
	for (const auto& Pair : LearnedAbilities)
	{
		const FAbilityUnlockRow* Row = FindRow(Pair.Key);
		if (!Row || !Row->AbilityClass) continue;

		FGameplayAbilitySpec AbilitySpec(Row->AbilityClass, Pair.Value);
		ASC->GiveAbility(AbilitySpec);

		UE_LOG(LogTemp, Log, TEXT("[AbilityUnlock] Restored ability '%s' at Rank %d"),
			*Pair.Key.ToString(), Pair.Value);
	}
}

// =========================================================================
// Internal
// =========================================================================

const FAbilityUnlockRow* UARPGAbilityUnlockComponent::FindRow(FName AbilityRowName) const
{
	if (!AbilityUnlockTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AbilityUnlock] AbilityUnlockTable is not set!"));
		return nullptr;
	}

	return AbilityUnlockTable->FindRow<FAbilityUnlockRow>(AbilityRowName, TEXT("AbilityUnlock"));
}

UAbilitySystemComponent* UARPGAbilityUnlockComponent::GetASC() const
{
	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		return ASI->GetAbilitySystemComponent();
	}
	return nullptr;
}

int32 UARPGAbilityUnlockComponent::GetPlayerLevel() const
{
	if (const AARPGPlayerCharacter* Player = Cast<AARPGPlayerCharacter>(GetOwner()))
	{
		return Player->GetPlayerLevel();
	}
	return 1;
}
