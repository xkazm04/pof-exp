#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PoFDuelSelectionSubsystem.generated.h"

/**
 * Pre-game duel selections (UE mirror of the browser preview's menu state):
 * which saber the player chose before entering the arena. Survives map travel
 * (GameInstance scope); consumed by whatever realizes the blade visuals.
 */
UCLASS()
class POF_API UPoFDuelSelectionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** 'Crimson' / 'Azure' / NAME_None (fight with the default blade). */
	UPROPERTY(BlueprintReadOnly, Category = "Duel")
	FName SelectedSaber;

	/** Quest palette id ('lords-challenge' / 'echoes-order') or NAME_None. */
	UPROPERTY(BlueprintReadOnly, Category = "Duel")
	FName SelectedQuest;

	UFUNCTION(BlueprintCallable, Category = "Duel")
	void SelectQuest(FName Quest)
	{
		SelectedQuest = Quest;
		UE_LOG(LogTemp, Log, TEXT("[PreGameMenu] quest selected: %s"), *Quest.ToString());
	}

	UFUNCTION(BlueprintCallable, Category = "Duel")
	void SelectSaber(FName Saber)
	{
		SelectedSaber = Saber;
		UE_LOG(LogTemp, Log, TEXT("[PreGameMenu] saber selected: %s"), *Saber.ToString());
	}
};
