#include "ARPGDialogueComponent.h"
#include "ARPGDialogueTree.h"
#include "Player/ARPGPlayerCharacter.h"
#include "Quest/ARPGQuestSubsystem.h"
#include "Inventory/ARPGInventoryComponent.h"
#include "Inventory/ARPGItemDefinition.h"
#include "AbilitySystemComponent.h"
#include "Engine/AssetManager.h"

const FARPGDialogueNode UARPGDialogueComponent::EmptyNode = FARPGDialogueNode();

UARPGDialogueComponent::UARPGDialogueComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UARPGDialogueComponent::StartDialogue(AARPGPlayerCharacter* Player)
{
	if (!Player || bInDialogue) return false;

	UARPGDialogueTree* Tree = SelectDialogue(Player);
	if (!Tree) return false;

	ActiveDialogue = Tree;
	DialoguePlayer = Player;
	bInDialogue = true;

	OnDialogueStarted.Broadcast(ActiveDialogue);
	GoToNode(Tree->StartNodeIndex);
	return true;
}

void UARPGDialogueComponent::SelectChoice(int32 ChoiceIndex)
{
	if (!bInDialogue || !ActiveDialogue) return;

	const FARPGDialogueNode* Node = ActiveDialogue->GetNode(CurrentNodeIndex);
	if (!Node) return;

	TArray<FARPGDialogueChoice> Available = GetAvailableChoices();
	if (!Available.IsValidIndex(ChoiceIndex))
	{
		EndDialogue();
		return;
	}

	const FARPGDialogueChoice& Choice = Available[ChoiceIndex];

	if (AARPGPlayerCharacter* Player = DialoguePlayer.Get())
	{
		ExecuteActions(Choice.Actions, Player);
	}

	GoToNode(Choice.NextNodeIndex);
}

void UARPGDialogueComponent::AdvanceDialogue()
{
	if (!bInDialogue || !ActiveDialogue) return;

	const FARPGDialogueNode* Node = ActiveDialogue->GetNode(CurrentNodeIndex);
	if (!Node) return;

	if (Node->Choices.Num() > 0) return; // Has choices, must use SelectChoice

	GoToNode(Node->NextNodeIndex);
}

void UARPGDialogueComponent::EndDialogue()
{
	if (!bInDialogue) return;

	if (ActiveDialogue && ActiveDialogue->bOneShot)
	{
		UsedOneShotDialogues.Add(ActiveDialogue->DialogueID);
	}

	bInDialogue = false;
	CurrentNodeIndex = -1;
	ActiveDialogue = nullptr;
	DialoguePlayer.Reset();

	OnDialogueEnded.Broadcast();
}

const FARPGDialogueNode& UARPGDialogueComponent::GetCurrentNode() const
{
	if (ActiveDialogue)
	{
		const FARPGDialogueNode* Node = ActiveDialogue->GetNode(CurrentNodeIndex);
		if (Node) return *Node;
	}
	return EmptyNode;
}

TArray<FARPGDialogueChoice> UARPGDialogueComponent::GetAvailableChoices() const
{
	TArray<FARPGDialogueChoice> Result;
	if (!ActiveDialogue) return Result;

	const FARPGDialogueNode* Node = ActiveDialogue->GetNode(CurrentNodeIndex);
	if (!Node) return Result;

	AARPGPlayerCharacter* Player = DialoguePlayer.Get();

	for (const FARPGDialogueChoice& Choice : Node->Choices)
	{
		if (Player && !EvaluateConditions(Choice.Conditions, Player)) continue;
		Result.Add(Choice);
	}

	return Result;
}

UARPGDialogueTree* UARPGDialogueComponent::SelectDialogue(AARPGPlayerCharacter* Player) const
{
	for (UARPGDialogueTree* Tree : DialogueTrees)
	{
		if (!Tree || !Tree->IsValid()) continue;
		if (Tree->bOneShot && UsedOneShotDialogues.Contains(Tree->DialogueID)) continue;

		const FARPGDialogueNode* StartNode = Tree->GetNode(Tree->StartNodeIndex);
		if (StartNode && EvaluateConditions(StartNode->EntryConditions, Player))
		{
			return Tree;
		}
	}
	return nullptr;
}

void UARPGDialogueComponent::GoToNode(int32 NodeIndex)
{
	if (NodeIndex < 0 || !ActiveDialogue)
	{
		EndDialogue();
		return;
	}

	const FARPGDialogueNode* Node = ActiveDialogue->GetNode(NodeIndex);
	if (!Node)
	{
		EndDialogue();
		return;
	}

	CurrentNodeIndex = NodeIndex;

	if (AARPGPlayerCharacter* Player = DialoguePlayer.Get())
	{
		ExecuteActions(Node->EntryActions, Player);
	}

	OnDialogueNodeEntered.Broadcast(CurrentNodeIndex, *Node);
}

bool UARPGDialogueComponent::EvaluateCondition(const FARPGDialogueCondition& Condition, AARPGPlayerCharacter* Player) const
{
	bool bResult = false;

	switch (Condition.Type)
	{
	case EARPGDialogueConditionType::None:
		bResult = true;
		break;

	case EARPGDialogueConditionType::HasQuest:
	{
		if (UARPGQuestSubsystem* QS = GetWorld()->GetGameInstance()->GetSubsystem<UARPGQuestSubsystem>())
		{
			bResult = QS->IsQuestActive(Condition.ParamName) || QS->IsQuestComplete(Condition.ParamName);
		}
		break;
	}

	case EARPGDialogueConditionType::QuestActive:
	{
		if (UARPGQuestSubsystem* QS = GetWorld()->GetGameInstance()->GetSubsystem<UARPGQuestSubsystem>())
		{
			bResult = QS->IsQuestActive(Condition.ParamName);
		}
		break;
	}

	case EARPGDialogueConditionType::QuestComplete:
	{
		if (UARPGQuestSubsystem* QS = GetWorld()->GetGameInstance()->GetSubsystem<UARPGQuestSubsystem>())
		{
			bResult = QS->IsQuestComplete(Condition.ParamName);
		}
		break;
	}

	case EARPGDialogueConditionType::QuestObjectiveComplete:
	{
		if (UARPGQuestSubsystem* QS = GetWorld()->GetGameInstance()->GetSubsystem<UARPGQuestSubsystem>())
		{
			bResult = QS->IsObjectiveComplete(Condition.ParamName, Condition.ParamInt);
		}
		break;
	}

	case EARPGDialogueConditionType::HasItem:
	{
		if (UARPGInventoryComponent* Inv = Player->FindComponentByClass<UARPGInventoryComponent>())
		{
			// Look up item definition by primary asset ID
			FPrimaryAssetId AssetId(TEXT("ItemDefinition"), Condition.ParamName);
			UObject* Loaded = UAssetManager::Get().GetPrimaryAssetObject(AssetId);
			if (UARPGItemDefinition* ItemDef = Cast<UARPGItemDefinition>(Loaded))
			{
				int32 RequiredCount = FMath::Max(Condition.ParamInt, 1);
				bResult = Inv->HasItem(ItemDef, RequiredCount);
			}
		}
		break;
	}

	case EARPGDialogueConditionType::PlayerLevelMin:
		bResult = Player->GetPlayerLevel() >= Condition.ParamInt;
		break;

	case EARPGDialogueConditionType::HasGameplayTag:
	{
		if (UAbilitySystemComponent* ASC = Player->FindComponentByClass<UAbilitySystemComponent>())
		{
			bResult = ASC->HasMatchingGameplayTag(Condition.ParamTag);
		}
		break;
	}
	}

	return Condition.bInvert ? !bResult : bResult;
}

bool UARPGDialogueComponent::EvaluateConditions(const TArray<FARPGDialogueCondition>& Conditions, AARPGPlayerCharacter* Player) const
{
	for (const FARPGDialogueCondition& Cond : Conditions)
	{
		if (!EvaluateCondition(Cond, Player)) return false;
	}
	return true;
}

void UARPGDialogueComponent::ExecuteAction(const FARPGDialogueAction& Action, AARPGPlayerCharacter* Player)
{
	switch (Action.Type)
	{
	case EARPGDialogueActionType::None:
		break;

	case EARPGDialogueActionType::GiveQuest:
	{
		if (UARPGQuestSubsystem* QS = GetWorld()->GetGameInstance()->GetSubsystem<UARPGQuestSubsystem>())
		{
			QS->AcceptQuest(Action.ParamName);
		}
		break;
	}

	case EARPGDialogueActionType::CompleteQuest:
	{
		if (UARPGQuestSubsystem* QS = GetWorld()->GetGameInstance()->GetSubsystem<UARPGQuestSubsystem>())
		{
			QS->CompleteQuest(Action.ParamName);
		}
		break;
	}

	case EARPGDialogueActionType::FailQuest:
	{
		if (UARPGQuestSubsystem* QS = GetWorld()->GetGameInstance()->GetSubsystem<UARPGQuestSubsystem>())
		{
			QS->FailQuest(Action.ParamName);
		}
		break;
	}

	case EARPGDialogueActionType::GiveItem:
	{
		if (UARPGInventoryComponent* Inv = Player->FindComponentByClass<UARPGInventoryComponent>())
		{
			FPrimaryAssetId AssetId(TEXT("ItemDefinition"), Action.ParamName);
			UObject* Loaded = UAssetManager::Get().GetPrimaryAssetObject(AssetId);
			if (UARPGItemDefinition* ItemDef = Cast<UARPGItemDefinition>(Loaded))
			{
				int32 Count = FMath::Max(Action.ParamInt, 1);
				Inv->AddItem(ItemDef, Count);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[Dialogue] GiveItem: could not find item '%s' in asset manager"), *Action.ParamName.ToString());
			}
		}
		break;
	}

	case EARPGDialogueActionType::RemoveItem:
	{
		if (UARPGInventoryComponent* Inv = Player->FindComponentByClass<UARPGInventoryComponent>())
		{
			FPrimaryAssetId AssetId(TEXT("ItemDefinition"), Action.ParamName);
			UObject* Loaded = UAssetManager::Get().GetPrimaryAssetObject(AssetId);
			if (UARPGItemDefinition* ItemDef = Cast<UARPGItemDefinition>(Loaded))
			{
				int32 Count = FMath::Max(Action.ParamInt, 1);
				Inv->RemoveItemByDefinition(ItemDef, Count);
			}
		}
		break;
	}

	case EARPGDialogueActionType::GiveXP:
		Player->AddExperience(static_cast<float>(Action.ParamInt));
		break;

	case EARPGDialogueActionType::AddGameplayTag:
	{
		if (UAbilitySystemComponent* ASC = Player->FindComponentByClass<UAbilitySystemComponent>())
		{
			ASC->AddLooseGameplayTag(Action.ParamTag);
		}
		break;
	}

	case EARPGDialogueActionType::RemoveGameplayTag:
	{
		if (UAbilitySystemComponent* ASC = Player->FindComponentByClass<UAbilitySystemComponent>())
		{
			ASC->RemoveLooseGameplayTag(Action.ParamTag);
		}
		break;
	}
	}
}

void UARPGDialogueComponent::ExecuteActions(const TArray<FARPGDialogueAction>& Actions, AARPGPlayerCharacter* Player)
{
	for (const FARPGDialogueAction& Action : Actions)
	{
		ExecuteAction(Action, Player);
	}
}
