#include "UI/VSHUD.h"
#include "UI/VSHUDWidget.h"
#include "Player/ARPGPlayerCharacter.h"
#include "Character/ARPGEnemyCharacter.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

void AVSHUD::BeginPlay()
{
	Super::BeginPlay();

	HUDWidget = CreateWidget<UVSHUDWidget>(GetWorld(), UVSHUDWidget::StaticClass());
	if (HUDWidget)
	{
		HUDWidget->AddToViewport();
		UE_LOG(LogTemp, Log, TEXT("[VSHUD] HUD widget created"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[VSHUD] failed to create HUD widget"));
	}

	TryBind();
}

void AVSHUD::TryBind()
{
	// --- Resolve the player ASC ---
	UAbilitySystemComponent* PlayerASC = nullptr;
	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (IAbilitySystemInterface* AsiPlayer = Cast<IAbilitySystemInterface>(PlayerPawn))
		{
			PlayerASC = AsiPlayer->GetAbilitySystemComponent();
		}
	}

	// --- Resolve the enemy ASC (first AARPGEnemyCharacter in the world) ---
	UAbilitySystemComponent* EnemyASC = nullptr;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AARPGEnemyCharacter> It(World); It; ++It)
		{
			if (AARPGEnemyCharacter* Enemy = *It)
			{
				EnemyASC = Enemy->GetAbilitySystemComponent();
				break;
			}
		}
	}

	if (HUDWidget && PlayerASC && EnemyASC)
	{
		HUDWidget->BindPlayer(PlayerASC);
		HUDWidget->BindEnemy(EnemyASC, TEXT("Enemy"));
		UE_LOG(LogTemp, Log, TEXT("[VSHUD] bound player + enemy"));
		return;
	}

	if (BindAttempts < 10)
	{
		++BindAttempts;
		GetWorldTimerManager().SetTimer(BindTimerHandle, this, &AVSHUD::TryBind, 0.5f, false);
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[VSHUD] bind incomplete after %d attempts (player=%s enemy=%s) — binding what is available"),
		BindAttempts,
		PlayerASC ? TEXT("ok") : TEXT("null"),
		EnemyASC ? TEXT("ok") : TEXT("null"));

	if (HUDWidget && PlayerASC)
	{
		HUDWidget->BindPlayer(PlayerASC);
	}
	if (HUDWidget && EnemyASC)
	{
		HUDWidget->BindEnemy(EnemyASC, TEXT("Enemy"));
	}
}
