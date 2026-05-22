#include "ARPGQuestSubsystem.h"
#include "ARPGQuestDefinition.h"
#include "Player/ARPGPlayerCharacter.h"
#include "Inventory/ARPGInventoryComponent.h"
#include "Inventory/ARPGItemDefinition.h"
#include "Engine/AssetManager.h"
#include "Kismet/GameplayStatics.h"

void UARPGQuestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UARPGQuestSubsystem::RegisterQuestDefinition(UARPGQuestDefinition* Definition)
{
	if (!Definition) return;
	QuestDefinitions.Add(Definition->QuestID, Definition);
}

UARPGQuestDefinition* UARPGQuestSubsystem::FindQuestDefinition(FName QuestID) const
{
	const TObjectPtr<UARPGQuestDefinition>* Found = QuestDefinitions.Find(QuestID);
	return Found ? *Found : nullptr;
}

bool UARPGQuestSubsystem::AcceptQuest(FName QuestID)
{
	// Already tracking this quest
	if (QuestStates.Contains(QuestID)) return false;

	UARPGQuestDefinition* Def = FindQuestDefinition(QuestID);

	// Check prerequisites
	if (Def && !Def->PrerequisiteQuestID.IsNone())
	{
		if (!IsQuestComplete(Def->PrerequisiteQuestID)) return false;
	}

	// Check level requirement
	if (Def && Def->RequiredLevel > 0)
	{
		UWorld* World = GetGameInstance()->GetWorld();
		if (World)
		{
			APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
			if (PC)
			{
				if (AARPGPlayerCharacter* Player = Cast<AARPGPlayerCharacter>(PC->GetPawn()))
				{
					if (Player->GetPlayerLevel() < Def->RequiredLevel)
					{
						UE_LOG(LogTemp, Display, TEXT("[Quest] Cannot accept '%s' — requires level %d"), *QuestID.ToString(), Def->RequiredLevel);
						return false;
					}
				}
			}
		}
	}

	FARPGQuestState NewState;
	NewState.QuestID = QuestID;
	NewState.State = EARPGQuestState::Active;

	if (Def)
	{
		NewState.ObjectiveStates.SetNum(Def->Objectives.Num());
	}

	QuestStates.Add(QuestID, NewState);
	OnQuestAccepted.Broadcast(QuestID);
	return true;
}

void UARPGQuestSubsystem::CompleteQuest(FName QuestID)
{
	FARPGQuestState* State = QuestStates.Find(QuestID);
	if (!State)
	{
		// Quest wasn't active — create completed entry
		FARPGQuestState NewState;
		NewState.QuestID = QuestID;
		NewState.State = EARPGQuestState::Completed;
		QuestStates.Add(QuestID, NewState);
		OnQuestCompleted.Broadcast(QuestID);
		return;
	}

	if (State->State == EARPGQuestState::Completed) return;

	State->State = EARPGQuestState::Completed;

	// Mark all objectives complete
	for (FARPGQuestObjectiveState& ObjState : State->ObjectiveStates)
	{
		ObjState.bComplete = true;
	}

	// Grant rewards to the player
	GrantRewards(QuestID);

	OnQuestCompleted.Broadcast(QuestID);

	// Clear pin if this was the pinned quest
	if (PinnedQuestID == QuestID)
	{
		UnpinQuest();
	}

	// Notify quests waiting on this as prerequisite
	NotifyPrerequisiteCompleted(QuestID);
}

void UARPGQuestSubsystem::FailQuest(FName QuestID)
{
	FARPGQuestState* State = QuestStates.Find(QuestID);
	if (!State || State->State != EARPGQuestState::Active) return;

	State->State = EARPGQuestState::Failed;
	OnQuestFailed.Broadcast(QuestID);
}

bool UARPGQuestSubsystem::AbandonQuest(FName QuestID)
{
	FARPGQuestState* State = QuestStates.Find(QuestID);
	if (!State || State->State != EARPGQuestState::Active) return false;

	UARPGQuestDefinition* Def = FindQuestDefinition(QuestID);
	if (Def && !Def->bAbandonable) return false;

	QuestStates.Remove(QuestID);
	OnQuestAbandoned.Broadcast(QuestID);
	return true;
}

void UARPGQuestSubsystem::AddObjectiveProgress(FName QuestID, int32 ObjectiveIndex, int32 Amount)
{
	FARPGQuestState* State = QuestStates.Find(QuestID);
	if (!State || State->State != EARPGQuestState::Active) return;
	if (!State->ObjectiveStates.IsValidIndex(ObjectiveIndex)) return;

	FARPGQuestObjectiveState& ObjState = State->ObjectiveStates[ObjectiveIndex];
	if (ObjState.bComplete) return;

	UARPGQuestDefinition* Def = FindQuestDefinition(QuestID);
	int32 Required = 1;
	if (Def && Def->Objectives.IsValidIndex(ObjectiveIndex))
	{
		Required = Def->Objectives[ObjectiveIndex].RequiredCount;
	}

	ObjState.CurrentCount = FMath::Min(ObjState.CurrentCount + Amount, Required);
	if (ObjState.CurrentCount >= Required)
	{
		ObjState.bComplete = true;
	}

	OnObjectiveUpdated.Broadcast(QuestID, ObjectiveIndex, ObjState.CurrentCount);
	CheckAutoComplete(QuestID);
}

void UARPGQuestSubsystem::CompleteObjective(FName QuestID, int32 ObjectiveIndex)
{
	FARPGQuestState* State = QuestStates.Find(QuestID);
	if (!State || State->State != EARPGQuestState::Active) return;
	if (!State->ObjectiveStates.IsValidIndex(ObjectiveIndex)) return;

	FARPGQuestObjectiveState& ObjState = State->ObjectiveStates[ObjectiveIndex];
	if (ObjState.bComplete) return;

	UARPGQuestDefinition* Def = FindQuestDefinition(QuestID);
	if (Def && Def->Objectives.IsValidIndex(ObjectiveIndex))
	{
		ObjState.CurrentCount = Def->Objectives[ObjectiveIndex].RequiredCount;
	}
	ObjState.bComplete = true;

	OnObjectiveUpdated.Broadcast(QuestID, ObjectiveIndex, ObjState.CurrentCount);
	CheckAutoComplete(QuestID);
}

void UARPGQuestSubsystem::ReportEvent(EARPGObjectiveType EventType, FName TargetID, int32 Count)
{
	// Collect matching (QuestID, ObjectiveIndex) pairs first because
	// AddObjectiveProgress -> CheckAutoComplete -> CompleteQuest can modify QuestStates
	// and iterating a TMap while modifying it is undefined behavior.
	struct FPendingProgress { FName QuestID; int32 ObjIndex; };
	TArray<FPendingProgress> Pending;

	for (const auto& Pair : QuestStates)
	{
		if (Pair.Value.State != EARPGQuestState::Active) continue;

		UARPGQuestDefinition* Def = FindQuestDefinition(Pair.Key);
		if (!Def) continue;

		for (int32 i = 0; i < Def->Objectives.Num(); ++i)
		{
			const FARPGQuestObjective& Obj = Def->Objectives[i];
			if (Obj.Type != EventType) continue;
			if (Obj.TargetID != TargetID) continue;

			Pending.Add({ Pair.Key, i });
		}
	}

	for (const FPendingProgress& P : Pending)
	{
		AddObjectiveProgress(P.QuestID, P.ObjIndex, Count);
	}
}

bool UARPGQuestSubsystem::IsQuestActive(FName QuestID) const
{
	const FARPGQuestState* State = QuestStates.Find(QuestID);
	return State && State->State == EARPGQuestState::Active;
}

bool UARPGQuestSubsystem::IsQuestComplete(FName QuestID) const
{
	const FARPGQuestState* State = QuestStates.Find(QuestID);
	return State && State->State == EARPGQuestState::Completed;
}

bool UARPGQuestSubsystem::IsQuestFailed(FName QuestID) const
{
	const FARPGQuestState* State = QuestStates.Find(QuestID);
	return State && State->State == EARPGQuestState::Failed;
}

bool UARPGQuestSubsystem::IsObjectiveComplete(FName QuestID, int32 ObjectiveIndex) const
{
	const FARPGQuestState* State = QuestStates.Find(QuestID);
	if (!State) return false;
	if (!State->ObjectiveStates.IsValidIndex(ObjectiveIndex)) return false;
	return State->ObjectiveStates[ObjectiveIndex].bComplete;
}

EARPGQuestState UARPGQuestSubsystem::GetQuestState(FName QuestID) const
{
	const FARPGQuestState* State = QuestStates.Find(QuestID);
	return State ? State->State : EARPGQuestState::Unavailable;
}

const FARPGQuestState* UARPGQuestSubsystem::GetQuestRuntimeState(FName QuestID) const
{
	return QuestStates.Find(QuestID);
}

bool UARPGQuestSubsystem::GetQuestRuntimeStateCopy(FName QuestID, FARPGQuestState& OutState) const
{
	const FARPGQuestState* State = QuestStates.Find(QuestID);
	if (!State) return false;
	OutState = *State;
	return true;
}

TArray<FName> UARPGQuestSubsystem::GetActiveQuestIDs() const
{
	TArray<FName> Result;
	for (const auto& Pair : QuestStates)
	{
		if (Pair.Value.State == EARPGQuestState::Active)
		{
			Result.Add(Pair.Key);
		}
	}
	return Result;
}

TArray<FName> UARPGQuestSubsystem::GetCompletedQuestIDs() const
{
	TArray<FName> Result;
	for (const auto& Pair : QuestStates)
	{
		if (Pair.Value.State == EARPGQuestState::Completed)
		{
			Result.Add(Pair.Key);
		}
	}
	return Result;
}

TArray<FName> UARPGQuestSubsystem::GetFailedQuestIDs() const
{
	TArray<FName> Result;
	for (const auto& Pair : QuestStates)
	{
		if (Pair.Value.State == EARPGQuestState::Failed)
		{
			Result.Add(Pair.Key);
		}
	}
	return Result;
}

bool UARPGQuestSubsystem::HasQuestTurnInReady(FName QuestID) const
{
	const FARPGQuestState* State = QuestStates.Find(QuestID);
	if (!State || State->State != EARPGQuestState::Active) return false;

	UARPGQuestDefinition* Def = FindQuestDefinition(QuestID);
	if (!Def) return false;

	// Check if all required objectives are complete (but quest hasn't auto-completed yet — it's manual)
	for (int32 i = 0; i < Def->Objectives.Num(); ++i)
	{
		if (Def->Objectives[i].bOptional) continue;
		if (!State->ObjectiveStates.IsValidIndex(i)) return false;
		if (!State->ObjectiveStates[i].bComplete) return false;
	}
	return true;
}

TArray<FARPGQuestState> UARPGQuestSubsystem::GetAllQuestStates() const
{
	TArray<FARPGQuestState> Result;
	QuestStates.GenerateValueArray(Result);
	return Result;
}

void UARPGQuestSubsystem::RestoreQuestStates(const TArray<FARPGQuestState>& States, FName InPinnedQuestID)
{
	QuestStates.Empty();
	for (const FARPGQuestState& State : States)
	{
		QuestStates.Add(State.QuestID, State);
	}

	// Restore pinned quest (validate it's still active)
	if (!InPinnedQuestID.IsNone() && IsQuestActive(InPinnedQuestID))
	{
		PinnedQuestID = InPinnedQuestID;
	}
	else
	{
		PinnedQuestID = NAME_None;
	}
}

void UARPGQuestSubsystem::GrantRewards(FName QuestID)
{
	UARPGQuestDefinition* Def = FindQuestDefinition(QuestID);
	if (!Def || Def->Rewards.Num() == 0) return;

	// Find the local player character
	UWorld* World = GetGameInstance()->GetWorld();
	if (!World) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC) return;

	AARPGPlayerCharacter* Player = Cast<AARPGPlayerCharacter>(PC->GetPawn());
	if (!Player) return;

	for (const FARPGQuestReward& Reward : Def->Rewards)
	{
		// Grant XP
		if (Reward.XP > 0)
		{
			Player->AddExperience(static_cast<float>(Reward.XP));
		}

		// Grant items
		if (!Reward.ItemID.IsNone())
		{
			if (UARPGInventoryComponent* Inv = Player->FindComponentByClass<UARPGInventoryComponent>())
			{
				FPrimaryAssetId AssetId(TEXT("ItemDefinition"), Reward.ItemID);
				UObject* Loaded = UAssetManager::Get().GetPrimaryAssetObject(AssetId);
				if (UARPGItemDefinition* ItemDef = Cast<UARPGItemDefinition>(Loaded))
				{
					int32 Count = FMath::Max(Reward.ItemQuantity, 1);
					Inv->AddItem(ItemDef, Count);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[Quest] Reward item '%s' not found in asset manager"), *Reward.ItemID.ToString());
				}
			}
		}
	}
}

void UARPGQuestSubsystem::PinQuest(FName QuestID)
{
	if (!IsQuestActive(QuestID)) return;
	PinnedQuestID = QuestID;
	OnQuestPinChanged.Broadcast(PinnedQuestID);
}

void UARPGQuestSubsystem::UnpinQuest()
{
	PinnedQuestID = NAME_None;
	OnQuestPinChanged.Broadcast(PinnedQuestID);
}

void UARPGQuestSubsystem::NotifyPrerequisiteCompleted(FName CompletedQuestID)
{
	// Log for designers to know chain progression happened
	for (const auto& Pair : QuestDefinitions)
	{
		UARPGQuestDefinition* Def = Pair.Value;
		if (Def && Def->PrerequisiteQuestID == CompletedQuestID)
		{
			UE_LOG(LogTemp, Display, TEXT("[Quest] '%s' prerequisite met — quest '%s' now available"), *CompletedQuestID.ToString(), *Def->QuestID.ToString());
		}
	}
}

void UARPGQuestSubsystem::CheckAutoComplete(FName QuestID)
{
	FARPGQuestState* State = QuestStates.Find(QuestID);
	if (!State || State->State != EARPGQuestState::Active) return;

	UARPGQuestDefinition* Def = FindQuestDefinition(QuestID);
	if (!Def || !Def->bAutoComplete) return;

	for (int32 i = 0; i < Def->Objectives.Num(); ++i)
	{
		if (Def->Objectives[i].bOptional) continue;
		if (!State->ObjectiveStates.IsValidIndex(i)) return;
		if (!State->ObjectiveStates[i].bComplete) return;
	}

	CompleteQuest(QuestID);
}
