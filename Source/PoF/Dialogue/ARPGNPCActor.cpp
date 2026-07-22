#include "ARPGNPCActor.h"
#include "ARPGDialogueComponent.h"
#include "ARPGDialogueTree.h"
#include "ARPGDialogueTypes.h"
#include "ARPGDuelIntroDialogue.h"
#include "Player/ARPGPlayerCharacter.h"
#include "Quest/ARPGQuestSubsystem.h"
#include "Quest/ARPGQuestTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/TextBlock.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/CoreStyle.h"

AARPGNPCActor::AARPGNPCActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Root mesh
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;
	MeshComp->SetCollisionProfileName(TEXT("BlockAll"));

	// Interaction trigger sphere
	InteractionTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionTrigger"));
	InteractionTrigger->SetupAttachment(RootComponent);
	InteractionTrigger->SetSphereRadius(200.f);
	InteractionTrigger->SetCollisionProfileName(TEXT("OverlapAll"));
	InteractionTrigger->SetGenerateOverlapEvents(true);

	// Dialogue component
	DialogueComp = CreateDefaultSubobject<UARPGDialogueComponent>(TEXT("DialogueComp"));

	// Role indicator widget (floating above head)
	RoleIndicatorComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("RoleIndicatorComp"));
	RoleIndicatorComp->SetupAttachment(RootComponent);
	RoleIndicatorComp->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	RoleIndicatorComp->SetWidgetSpace(EWidgetSpace::Screen);
	RoleIndicatorComp->SetDrawSize(FVector2D(100.f, 40.f));

	// Tag for interaction scan
	Tags.Add(FName("Interactable"));
}

void AARPGNPCActor::BeginPlay()
{
	Super::BeginPlay();
	UpdateRoleIndicator();

	if (DialogueComp)
	{
		// Darth Malgrave self-carries the Duel Challenge tree (dialog-trees catalog:
		// dialog-duel-intro) from the code-as-data builder — placing/spawning an NPC
		// with NPCID "Malgrave" needs no .uasset authoring to speak the pre-duel beat.
		if (DialogueComp->DialogueTrees.Num() == 0 && NPCID == FName(TEXT("Malgrave")))
		{
			UARPGDialogueTree* DuelIntro = NewObject<UARPGDialogueTree>(this, TEXT("DuelIntroTree"));
			ARPGDuelIntroDialogue::Populate(*DuelIntro);
			DialogueComp->DialogueTrees.Add(DuelIntro);
			UE_LOG(LogTemp, Log, TEXT("[DuelIntro] %s self-attached the Duel Challenge tree (%d nodes)"),
				*GetName(), DuelIntro->Nodes.Num());
		}

		// Report TalkTo quest event when dialogue starts on this NPC
		if (!NPCID.IsNone())
		{
			DialogueComp->OnDialogueStarted.AddDynamic(this, &AARPGNPCActor::OnDialogueStartedForNPC);
		}

		// Face player during dialogue and refresh indicator when dialogue ends
		DialogueComp->OnDialogueEnded.AddDynamic(this, &AARPGNPCActor::OnDialogueEndedForNPC);
	}

	// Bind to quest state changes so indicator updates dynamically
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UARPGQuestSubsystem* QS = GI->GetSubsystem<UARPGQuestSubsystem>())
		{
			QS->OnQuestAccepted.AddDynamic(this, &AARPGNPCActor::OnQuestStateChanged);
			QS->OnQuestCompleted.AddDynamic(this, &AARPGNPCActor::OnQuestStateChanged);
			QS->OnQuestFailed.AddDynamic(this, &AARPGNPCActor::OnQuestStateChanged);
		}
	}
}

FText AARPGNPCActor::GetRoleDisplayText() const
{
	switch (NPCRole)
	{
	case EARPGNPCRole::QuestGiver:  return FText::FromString(TEXT("Quest"));
	case EARPGNPCRole::Merchant:    return FText::FromString(TEXT("Merchant"));
	case EARPGNPCRole::Trainer:     return FText::FromString(TEXT("Trainer"));
	case EARPGNPCRole::Innkeeper:   return FText::FromString(TEXT("Inn"));
	case EARPGNPCRole::Lorekeeper:  return FText::FromString(TEXT("Lore"));
	default:                        return FText::GetEmpty();
	}
}

FLinearColor AARPGNPCActor::GetRoleColor() const
{
	switch (NPCRole)
	{
	case EARPGNPCRole::QuestGiver:  return FLinearColor(1.f, 0.85f, 0.f);    // Gold
	case EARPGNPCRole::Merchant:    return FLinearColor(0.3f, 0.8f, 1.f);    // Blue
	case EARPGNPCRole::Trainer:     return FLinearColor(0.4f, 1.f, 0.4f);    // Green
	case EARPGNPCRole::Innkeeper:   return FLinearColor(1.f, 0.6f, 0.3f);    // Orange
	case EARPGNPCRole::Lorekeeper:  return FLinearColor(0.8f, 0.5f, 1.f);    // Purple
	default:                        return FLinearColor::White;
	}
}

bool AARPGNPCActor::HasDialogueForPlayer(AARPGPlayerCharacter* Player) const
{
	if (!DialogueComp || !Player) return false;

	// Check if any dialogue tree passes its entry conditions for this player
	for (UARPGDialogueTree* Tree : DialogueComp->DialogueTrees)
	{
		if (!Tree || !Tree->IsValid()) continue;
		if (Tree->bOneShot && DialogueComp->HasUsedOneShot(Tree->DialogueID)) continue;

		const FARPGDialogueNode* StartNode = Tree->GetNode(Tree->StartNodeIndex);
		if (StartNode && DialogueComp->CheckConditions(StartNode->EntryConditions, Player))
		{
			return true;
		}
	}
	return false;
}

FString AARPGNPCActor::GetIndicatorSymbol() const
{
	switch (NPCRole)
	{
	case EARPGNPCRole::QuestGiver:  return TEXT("!");
	case EARPGNPCRole::Merchant:    return TEXT("$");
	case EARPGNPCRole::Trainer:     return TEXT("*");
	case EARPGNPCRole::Innkeeper:   return TEXT("~");
	case EARPGNPCRole::Lorekeeper:  return TEXT("?");
	default:                        return TEXT("");
	}
}

void AARPGNPCActor::OnDialogueStartedForNPC(UARPGDialogueTree* /*Dialogue*/)
{
	if (NPCID.IsNone()) return;

	if (UARPGQuestSubsystem* QS = GetGameInstance()->GetSubsystem<UARPGQuestSubsystem>())
	{
		QS->ReportEvent(EARPGObjectiveType::TalkTo, NPCID, 1);
	}

	// Face the player during dialogue
	if (bFacePlayerInDialogue)
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				FVector Dir = Pawn->GetActorLocation() - GetActorLocation();
				Dir.Z = 0.f;
				if (!Dir.IsNearlyZero())
				{
					SetActorRotation(Dir.Rotation());
				}
			}
		}
	}
}

void AARPGNPCActor::OnDialogueEndedForNPC()
{
	RefreshIndicator();
}

void AARPGNPCActor::OnQuestStateChanged(FName /*QuestID*/)
{
	RefreshIndicator();
}

FString AARPGNPCActor::GetDynamicIndicatorSymbol() const
{
	if (NPCRole == EARPGNPCRole::QuestGiver)
	{
		// Check if any quest assigned through this NPC's dialogues is turn-in ready
		if (DialogueComp)
		{
			UARPGQuestSubsystem* QS = nullptr;
			if (UGameInstance* GI = GetGameInstance())
			{
				QS = GI->GetSubsystem<UARPGQuestSubsystem>();
			}

			if (QS)
			{
				for (UARPGDialogueTree* Tree : DialogueComp->DialogueTrees)
				{
					if (!Tree) continue;
					for (const FARPGDialogueNode& Node : Tree->Nodes)
					{
						// Check entry actions for GiveQuest actions
						for (const FARPGDialogueAction& Action : Node.EntryActions)
						{
							if (Action.Type == EARPGDialogueActionType::GiveQuest && QS->HasQuestTurnInReady(Action.ParamName))
							{
								return TEXT("?");
							}
						}
						for (const FARPGDialogueChoice& Choice : Node.Choices)
						{
							for (const FARPGDialogueAction& Action : Choice.Actions)
							{
								if (Action.Type == EARPGDialogueActionType::GiveQuest && QS->HasQuestTurnInReady(Action.ParamName))
								{
									return TEXT("?");
								}
							}
						}
					}
				}
			}
		}
	}
	return GetIndicatorSymbol();
}

FLinearColor AARPGNPCActor::GetDynamicIndicatorColor() const
{
	FString Symbol = GetDynamicIndicatorSymbol();
	if (Symbol == TEXT("?"))
	{
		return FLinearColor(0.3f, 1.f, 0.3f); // Green for turn-in
	}
	return GetRoleColor();
}

void AARPGNPCActor::RefreshIndicator()
{
	if (!IndicatorTextBlock) return;

	if (NPCRole == EARPGNPCRole::None)
	{
		if (RoleIndicatorComp) RoleIndicatorComp->SetVisibility(false);
		return;
	}

	IndicatorTextBlock->SetText(FText::FromString(GetDynamicIndicatorSymbol()));
	IndicatorTextBlock->SetColorAndOpacity(FSlateColor(GetDynamicIndicatorColor()));
	if (RoleIndicatorComp) RoleIndicatorComp->SetVisibility(true);
}

void AARPGNPCActor::UpdateRoleIndicator()
{
	if (!RoleIndicatorComp) return;

	if (NPCRole == EARPGNPCRole::None)
	{
		RoleIndicatorComp->SetVisibility(false);
		return;
	}

	// Create a simple text widget for the role indicator
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	UUserWidget* IndicatorWidget = CreateWidget<UUserWidget>(PC);
	if (!IndicatorWidget) return;

	IndicatorTextBlock = NewObject<UTextBlock>(IndicatorWidget);
	IndicatorTextBlock->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 28));
	IndicatorTextBlock->SetText(FText::FromString(GetDynamicIndicatorSymbol()));
	IndicatorTextBlock->SetColorAndOpacity(FSlateColor(GetDynamicIndicatorColor()));
	IndicatorTextBlock->SetJustification(ETextJustify::Center);

	if (IndicatorWidget->WidgetTree)
	{
		IndicatorWidget->WidgetTree->RootWidget = IndicatorTextBlock;
	}

	RoleIndicatorComp->SetWidget(IndicatorWidget);
	RoleIndicatorComp->SetVisibility(true);
}
