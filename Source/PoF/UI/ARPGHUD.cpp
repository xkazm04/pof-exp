#include "UI/ARPGHUD.h"
#include "UI/ARPGHUDWidget.h"
#include "UI/AbilityBarWidget.h"
#include "UI/AbilityUnlockWidget.h"
#include "UI/InventoryScreenWidget.h"
#include "UI/CharacterStatsWidget.h"
#include "UI/AttributeAllocationWidget.h"
#include "UI/NotificationWidget.h"
#include "UI/DialogueWidget.h"
#include "UI/QuestTrackerWidget.h"
#include "UI/QuestLogWidget.h"
#include "UI/SaveSlotWidget.h"
#include "Player/ARPGPlayerController.h"
#include "Player/ARPGPlayerCharacter.h"
#include "Inventory/ARPGInventoryComponent.h"
#include "Dialogue/ARPGDialogueComponent.h"
#include "Quest/ARPGQuestSubsystem.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"

void AARPGHUD::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return;

	// Create always-on HUD widgets
	if (HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UARPGHUDWidget>(PC, HUDWidgetClass);
		if (HUDWidget) HUDWidget->AddToViewport(0);
	}

	if (AbilityBarClass)
	{
		AbilityBar = CreateWidget<UAbilityBarWidget>(PC, AbilityBarClass);
		if (AbilityBar) AbilityBar->AddToViewport(0);
	}

	if (NotificationWidgetClass)
	{
		NotificationWidget = CreateWidget<UNotificationWidget>(PC, NotificationWidgetClass);
		if (NotificationWidget) NotificationWidget->AddToViewport(50);
	}

	// Dialogue widget (hidden until dialogue starts)
	if (DialogueWidgetClass)
	{
		DialogueWidget = CreateWidget<UDialogueWidget>(PC, DialogueWidgetClass);
		if (DialogueWidget) DialogueWidget->AddToViewport(20);
	}

	// Quest tracker (always visible, top-right)
	if (QuestTrackerClass)
	{
		QuestTracker = CreateWidget<UQuestTrackerWidget>(PC, QuestTrackerClass);
		if (QuestTracker) QuestTracker->AddToViewport(0);
	}

	// Quest log (overlay, hidden by default)
	if (QuestLogClass)
	{
		QuestLog = CreateWidget<UQuestLogWidget>(PC, QuestLogClass);
		if (QuestLog)
		{
			QuestLog->AddToViewport(10);
			QuestLog->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// Create overlay screens (hidden by default)
	if (InventoryScreenClass)
	{
		InventoryScreen = CreateWidget<UInventoryScreenWidget>(PC, InventoryScreenClass);
		if (InventoryScreen)
		{
			InventoryScreen->AddToViewport(10);
			InventoryScreen->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (CharacterStatsClass)
	{
		CharacterStats = CreateWidget<UCharacterStatsWidget>(PC, CharacterStatsClass);
		if (CharacterStats)
		{
			CharacterStats->AddToViewport(10);
			CharacterStats->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (AttributeAllocationClass)
	{
		AttributeAllocation = CreateWidget<UAttributeAllocationWidget>(PC, AttributeAllocationClass);
		if (AttributeAllocation)
		{
			AttributeAllocation->AddToViewport(10);
			AttributeAllocation->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (AbilityUnlockClass)
	{
		AbilityUnlockScreen = CreateWidget<UAbilityUnlockWidget>(PC, AbilityUnlockClass);
		if (AbilityUnlockScreen)
		{
			AbilityUnlockScreen->AddToViewport(10);
			AbilityUnlockScreen->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (SaveSlotScreenClass)
	{
		SaveSlotScreen = CreateWidget<USaveSlotWidget>(PC, SaveSlotScreenClass);
		if (SaveSlotScreen)
		{
			SaveSlotScreen->AddToViewport(15);
			SaveSlotScreen->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// Bind HUD widgets to the player's ASC and character
	BindWidgetsToPlayer();
}

// ---------------------------------------------------------------------------
// Widget <-> Player binding
// ---------------------------------------------------------------------------

void AARPGHUD::BindWidgetsToPlayer()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return;

	AARPGPlayerCharacter* Player = Cast<AARPGPlayerCharacter>(PC->GetPawn());
	if (!Player) return;

	UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();

	// Bind HUD widget to GAS attributes + player character (stamina/XP)
	if (HUDWidget)
	{
		if (ASC)
		{
			HUDWidget->BindToAbilitySystem(ASC);
		}
		HUDWidget->BindToPlayerCharacter(Player);
	}

	// Bind ability bar to ASC and refresh from current loadout
	if (AbilityBar)
	{
		if (ASC)
		{
			AbilityBar->BindToAbilitySystem(ASC);
		}
		AbilityBar->RefreshFromLoadout(Player);
	}

	// Bind inventory screen to the player's inventory component
	if (InventoryScreen)
	{
		if (UARPGInventoryComponent* Inventory = Player->FindComponentByClass<UARPGInventoryComponent>())
		{
			InventoryScreen->BindToInventory(Inventory);
		}
	}

	// Bind character stats to ASC
	if (CharacterStats && ASC)
	{
		CharacterStats->BindToAbilitySystem(ASC);
	}

	// Bind attribute allocation to player
	if (AttributeAllocation)
	{
		AttributeAllocation->BindToPlayer(Player);
	}

	// Bind skill tree / ability unlock to player
	if (AbilityUnlockScreen)
	{
		AbilityUnlockScreen->BindToPlayer(Player);
	}

	// Bind quest tracker and quest log to quest subsystem
	if (UGameInstance* GI = GetWorld()->GetGameInstance())
	{
		if (UARPGQuestSubsystem* QS = GI->GetSubsystem<UARPGQuestSubsystem>())
		{
			if (QuestTracker)
			{
				QuestTracker->BindToQuestSubsystem(QS);
			}
			if (QuestLog)
			{
				QuestLog->BindToQuestSubsystem(QS);
			}
		}
	}

	// Listen for loadout changes to auto-refresh ability bar
	Player->OnLoadoutChanged.AddDynamic(this, &AARPGHUD::OnLoadoutChanged);
}

void AARPGHUD::OnLoadoutChanged(int32 SlotIndex)
{
	if (!AbilityBar) return;

	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return;

	if (AARPGPlayerCharacter* Player = Cast<AARPGPlayerCharacter>(PC->GetPawn()))
	{
		AbilityBar->RefreshFromLoadout(Player);
	}
}

// ---------------------------------------------------------------------------
// Screen toggles
// ---------------------------------------------------------------------------

void AARPGHUD::ToggleInventory()
{
	if (!InventoryScreen) return;

	bInventoryOpen = !bInventoryOpen;
	InventoryScreen->SetVisibility(bInventoryOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (bInventoryOpen)
		OnScreenOpened();
	else
		OnScreenClosed();
}

void AARPGHUD::ToggleQuestLog()
{
	if (!QuestLog) return;

	bQuestLogOpen = !bQuestLogOpen;
	QuestLog->SetVisibility(bQuestLogOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (bQuestLogOpen)
	{
		QuestLog->RefreshDisplay();
		OnScreenOpened();
	}
	else
	{
		OnScreenClosed();
	}
}

void AARPGHUD::ToggleCharacterStats()
{
	if (!CharacterStats) return;

	bStatsOpen = !bStatsOpen;
	CharacterStats->SetVisibility(bStatsOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (bStatsOpen)
		OnScreenOpened();
	else
		OnScreenClosed();
}

void AARPGHUD::ToggleAttributeAllocation()
{
	if (!AttributeAllocation) return;

	bAttributeOpen = !bAttributeOpen;
	AttributeAllocation->SetVisibility(bAttributeOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (bAttributeOpen)
		OnScreenOpened();
	else
		OnScreenClosed();
}

void AARPGHUD::ToggleSkillTree()
{
	if (!AbilityUnlockScreen) return;

	bSkillTreeOpen = !bSkillTreeOpen;
	AbilityUnlockScreen->SetVisibility(bSkillTreeOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (bSkillTreeOpen)
	{
		AbilityUnlockScreen->RefreshDisplay();
		OnScreenOpened();
	}
	else
	{
		OnScreenClosed();
	}
}

void AARPGHUD::ToggleSaveScreen(ESaveScreenMode InMode)
{
	if (!SaveSlotScreen) return;

	bSaveScreenOpen = !bSaveScreenOpen;

	if (bSaveScreenOpen)
	{
		SaveSlotScreen->OpenScreen(InMode);
		OnScreenOpened();
	}
	else
	{
		SaveSlotScreen->CloseScreen();
		OnScreenClosed();
	}
}

void AARPGHUD::CloseAllScreens()
{
	if (bInventoryOpen && InventoryScreen)
	{
		bInventoryOpen = false;
		InventoryScreen->SetVisibility(ESlateVisibility::Collapsed);
		OnScreenClosed();
	}
	if (bQuestLogOpen && QuestLog)
	{
		bQuestLogOpen = false;
		QuestLog->SetVisibility(ESlateVisibility::Collapsed);
		OnScreenClosed();
	}
	if (bStatsOpen && CharacterStats)
	{
		bStatsOpen = false;
		CharacterStats->SetVisibility(ESlateVisibility::Collapsed);
		OnScreenClosed();
	}
	if (bAttributeOpen && AttributeAllocation)
	{
		bAttributeOpen = false;
		AttributeAllocation->SetVisibility(ESlateVisibility::Collapsed);
		OnScreenClosed();
	}
	if (bSkillTreeOpen && AbilityUnlockScreen)
	{
		bSkillTreeOpen = false;
		AbilityUnlockScreen->SetVisibility(ESlateVisibility::Collapsed);
		OnScreenClosed();
	}
	if (bSaveScreenOpen && SaveSlotScreen)
	{
		bSaveScreenOpen = false;
		SaveSlotScreen->CloseScreen();
		OnScreenClosed();
	}
}

void AARPGHUD::SetHUDVisible(bool bVisible)
{
	const ESlateVisibility Vis = bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	if (HUDWidget) HUDWidget->SetVisibility(Vis);
	if (AbilityBar) AbilityBar->SetVisibility(Vis);
}

void AARPGHUD::ShowDialogue(UARPGDialogueComponent* DialogueComp)
{
	if (DialogueWidget && DialogueComp)
	{
		DialogueWidget->BindToDialogue(DialogueComp);
		OnScreenOpened();
	}
}

void AARPGHUD::HideDialogue()
{
	if (DialogueWidget)
	{
		DialogueWidget->UnbindFromDialogue();
		OnScreenClosed();
	}
}

void AARPGHUD::ShowNotification(const FText& Message, float Duration)
{
	if (NotificationWidget)
	{
		NotificationWidget->ShowToast(Message, Duration);
	}
}

// ---------------------------------------------------------------------------
// Input mode management
// ---------------------------------------------------------------------------

void AARPGHUD::OnScreenOpened()
{
	++OpenScreenCount;
	if (OpenScreenCount == 1)
	{
		if (AARPGPlayerController* PC = Cast<AARPGPlayerController>(GetOwningPlayerController()))
		{
			PC->SetInputModeGameAndUI();
		}
	}
}

void AARPGHUD::OnScreenClosed()
{
	OpenScreenCount = FMath::Max(OpenScreenCount - 1, 0);
	if (OpenScreenCount == 0)
	{
		if (AARPGPlayerController* PC = Cast<AARPGPlayerController>(GetOwningPlayerController()))
		{
			PC->SetInputModeGameOnly();
		}
	}
}
