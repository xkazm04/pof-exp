#include "World/ARPGBossEncounter.h"
#include "Character/ARPGBossCharacter.h"
#include "Player/ARPGPlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"

AARPGBossEncounter::AARPGBossEncounter()
{
	PrimaryActorTick.bCanEverTick = false;

	ArenaTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("ArenaTrigger"));
	ArenaTrigger->SetBoxExtent(FVector(1000.f, 1000.f, 300.f));
	ArenaTrigger->SetCollisionProfileName(TEXT("Trigger"));
	ArenaTrigger->SetGenerateOverlapEvents(true);
	SetRootComponent(ArenaTrigger);
}

void AARPGBossEncounter::BeginPlay()
{
	Super::BeginPlay();

	if (bTriggerOnOverlap)
	{
		ArenaTrigger->OnComponentBeginOverlap.AddDynamic(this, &AARPGBossEncounter::OnArenaOverlap);
	}

	// Hide fog walls initially
	ActivateFogWalls(false);
}

void AARPGBossEncounter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RetriggerTimerHandle);

	// Unbind player death delegate
	if (AARPGPlayerCharacter* Player = Cast<AARPGPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		Player->OnPlayerDeath.RemoveDynamic(this, &AARPGBossEncounter::OnPlayerDied);
	}

	Super::EndPlay(EndPlayReason);
}

void AARPGBossEncounter::OnArenaOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (OtherActor != PlayerPawn) return;

	StartEncounter();
}

void AARPGBossEncounter::StartEncounter()
{
	if (bEncounterActive) return;
	if (bEncounterCompleted && !bCanRetrigger) return;

	bEncounterActive = true;
	bEncounterCompleted = false;

	UE_LOG(LogTemp, Log, TEXT("[BossEncounter] %s starting"), *GetName());

	// Activate fog walls
	ActivateFogWalls(true);

	// Spawn or activate the boss
	SpawnBoss();

	AARPGBossCharacter* Boss = GetBoss();
	if (Boss)
	{
		// Clear any previous binding before adding (safe for retrigger scenarios)
		Boss->OnEnemyDeath.RemoveDynamic(this, &AARPGBossEncounter::OnBossDied);
		Boss->OnEnemyDeath.AddDynamic(this, &AARPGBossEncounter::OnBossDied);
		OnEncounterStarted.Broadcast(Boss);
	}

	// Bind to player death — reset encounter so player isn't trapped behind fog walls
	if (AARPGPlayerCharacter* Player = Cast<AARPGPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		Player->OnPlayerDeath.AddDynamic(this, &AARPGBossEncounter::OnPlayerDied);
	}
}

AARPGBossCharacter* AARPGBossEncounter::GetBoss() const
{
	if (SpawnedBoss) return SpawnedBoss;
	return PrePlacedBoss;
}

void AARPGBossEncounter::SpawnBoss()
{
	// Use pre-placed boss if available
	if (PrePlacedBoss)
	{
		SpawnedBoss = nullptr;
		return;
	}

	if (!BossClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BossEncounter] %s: No BossClass or PrePlacedBoss set"), *GetName());
		return;
	}

	const FVector SpawnLocation = GetActorLocation() + BossSpawnOffset;
	const FRotator SpawnRotation = GetActorRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	SpawnedBoss = GetWorld()->SpawnActor<AARPGBossCharacter>(BossClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (SpawnedBoss && BossLevelOverride > 0)
	{
		SpawnedBoss->SetCharacterLevel(BossLevelOverride);
	}

	UE_LOG(LogTemp, Log, TEXT("[BossEncounter] Spawned boss %s"), SpawnedBoss ? *SpawnedBoss->GetName() : TEXT("FAILED"));
}

void AARPGBossEncounter::ActivateFogWalls(bool bActivate)
{
	for (AActor* FogWall : FogWallActors)
	{
		if (FogWall)
		{
			FogWall->SetActorHiddenInGame(!bActivate);
			FogWall->SetActorEnableCollision(bActivate);
		}
	}
}

void AARPGBossEncounter::OnBossDied(AARPGEnemyCharacter* DeadEnemy)
{
	if (!bEncounterActive) return;

	UE_LOG(LogTemp, Log, TEXT("[BossEncounter] Boss defeated!"));
	CompleteEncounter();
}

void AARPGBossEncounter::OnPlayerDied()
{
	if (!bEncounterActive) return;

	UE_LOG(LogTemp, Log, TEXT("[BossEncounter] %s: Player died — resetting encounter"), *GetName());

	bEncounterActive = false;

	// Unbind player death delegate to prevent double binding on retrigger
	if (AARPGPlayerCharacter* Player = Cast<AARPGPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		Player->OnPlayerDeath.RemoveDynamic(this, &AARPGBossEncounter::OnPlayerDied);
	}

	// Drop fog walls so player isn't trapped on respawn
	ActivateFogWalls(false);

	// Clean up spawned boss (pre-placed boss stays but will be re-used on retrigger)
	if (SpawnedBoss)
	{
		SpawnedBoss->Destroy();
		SpawnedBoss = nullptr;
	}
}

void AARPGBossEncounter::CompleteEncounter()
{
	bEncounterActive = false;
	bEncounterCompleted = true;

	// Unbind player death delegate — fight is over
	if (AARPGPlayerCharacter* Player = Cast<AARPGPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		Player->OnPlayerDeath.RemoveDynamic(this, &AARPGBossEncounter::OnPlayerDied);
	}

	// Remove fog walls
	ActivateFogWalls(false);

	AARPGBossCharacter* Boss = GetBoss();
	OnEncounterCompleted.Broadcast(Boss);

	if (bCanRetrigger)
	{
		GetWorldTimerManager().SetTimer(
			RetriggerTimerHandle,
			[this]()
			{
				bEncounterCompleted = false;
				// Clean up spawned boss so it re-spawns on next trigger
				if (SpawnedBoss)
				{
					SpawnedBoss->Destroy();
					SpawnedBoss = nullptr;
				}
				// Pre-placed bosses cannot retrigger — they only exist once
				if (PrePlacedBoss)
				{
					UE_LOG(LogTemp, Warning, TEXT("[BossEncounter] %s: Pre-placed boss cannot retrigger"), *GetName());
					bEncounterCompleted = true;
				}
			},
			RetriggerDelay,
			false);
	}

	UE_LOG(LogTemp, Log, TEXT("[BossEncounter] %s completed"), *GetName());
}
