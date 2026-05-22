#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARPGNPCActor.generated.h"

class UARPGDialogueComponent;
class UARPGDialogueTree;
class UStaticMeshComponent;
class UWidgetComponent;
class USphereComponent;
class UBillboardComponent;
class UTextBlock;

/**
 * Role/archetype for visual indicators above the NPC.
 */
UENUM(BlueprintType)
enum class EARPGNPCRole : uint8
{
	None,
	QuestGiver,
	Merchant,
	Trainer,
	Innkeeper,
	Lorekeeper
};

/**
 * Interactable NPC actor with dialogue support and role indicator.
 * Place in the level, assign dialogue trees and a role.
 * The player's interaction scan picks this up via the "Interactable" tag.
 */
UCLASS(Blueprintable)
class POF_API AARPGNPCActor : public AActor
{
	GENERATED_BODY()

public:
	AARPGNPCActor();

	virtual void BeginPlay() override;

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC")
	TObjectPtr<UStaticMeshComponent> MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC")
	TObjectPtr<USphereComponent> InteractionTrigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC")
	TObjectPtr<UARPGDialogueComponent> DialogueComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC")
	TObjectPtr<UWidgetComponent> RoleIndicatorComp;

	// --- Configuration ---

	/** Unique NPC identifier (used for TalkTo objective matching). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FName NPCID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	EARPGNPCRole NPCRole = EARPGNPCRole::None;

	/** Whether this NPC auto-faces the player during dialogue. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	bool bFacePlayerInDialogue = true;

	// --- API ---

	UFUNCTION(BlueprintPure, Category = "NPC")
	FText GetRoleDisplayText() const;

	UFUNCTION(BlueprintPure, Category = "NPC")
	FLinearColor GetRoleColor() const;

	/** Returns true if this NPC has any valid dialogue for the given player. */
	UFUNCTION(BlueprintPure, Category = "NPC")
	bool HasDialogueForPlayer(class AARPGPlayerCharacter* Player) const;

	/** Get the quest-giver indicator symbol. */
	UFUNCTION(BlueprintPure, Category = "NPC")
	FString GetIndicatorSymbol() const;

	/** Get dynamic indicator: "?" for turn-in ready, "!" for quest available, or role default. */
	UFUNCTION(BlueprintPure, Category = "NPC")
	FString GetDynamicIndicatorSymbol() const;

	/** Get dynamic indicator color matching the dynamic symbol. */
	UFUNCTION(BlueprintPure, Category = "NPC")
	FLinearColor GetDynamicIndicatorColor() const;

	/** Refresh the floating indicator widget (call after quest state changes). */
	UFUNCTION(BlueprintCallable, Category = "NPC")
	void RefreshIndicator();

private:
	void UpdateRoleIndicator();

	UFUNCTION()
	void OnDialogueStartedForNPC(UARPGDialogueTree* Dialogue);

	UFUNCTION()
	void OnDialogueEndedForNPC();

	UFUNCTION()
	void OnQuestStateChanged(FName QuestID);

	UPROPERTY()
	TObjectPtr<UTextBlock> IndicatorTextBlock;
};
