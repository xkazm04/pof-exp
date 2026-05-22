#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "VSHUD.generated.h"

/** Minimal AHUD for the vertical slice: spawns the UVSHUDWidget and binds it
 *  to the player and the (single) enemy. Set as HUDClass on BP_VSGameMode. */
UCLASS()
class POF_API AVSHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

private:
	void TryBind();

	UPROPERTY() class UVSHUDWidget* HUDWidget = nullptr;
	int32 BindAttempts = 0;
	FTimerHandle BindTimerHandle;
};
