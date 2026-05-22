#include "Spawning/EnemySpawnPoint.h"
#include "Character/ARPGEnemyCharacter.h"
#include "World/ARPGZoneDefinition.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystem/Effects/GE_DifficultyScaling.h"
#include "Components/BillboardComponent.h"
#include "NavigationSystem.h"

AEnemySpawnPoint::AEnemySpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

#if WITH_EDITORONLY_DATA
	EditorSprite = CreateDefaultSubobject<UBillboardComponent>(TEXT("EditorSprite"));
	EditorSprite->SetupAttachment(Root);
	EditorSprite->SetHiddenInGame(true);
#endif
}

void AEnemySpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	if (bSpawnOnBeginPlay && EnemyClass)
	{
		SpawnEnemy();
	}
}

void AEnemySpawnPoint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RespawnTimerHandle);
	Super::EndPlay(EndPlayReason);
}

AARPGEnemyCharacter* AEnemySpawnPoint::SpawnEnemy()
{
	return SpawnEnemyWithDifficulty(1.0f);
}

AARPGEnemyCharacter* AEnemySpawnPoint::SpawnEnemyWithDifficulty(float DifficultyMultiplier)
{
	if (!EnemyClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SpawnPoint] %s: No EnemyClass set"), *GetName());
		return nullptr;
	}

	const FVector Location = GetSpawnLocation();
	const FRotator Rotation = GetActorRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AARPGEnemyCharacter* Enemy = GetWorld()->SpawnActor<AARPGEnemyCharacter>(EnemyClass, Location, Rotation, SpawnParams);
	if (!Enemy)
	{
		UE_LOG(LogTemp, Error, TEXT("[SpawnPoint] %s: Failed to spawn %s"), *GetName(), *EnemyClass->GetName());
		return nullptr;
	}

	// Override level: explicit override > zone-based > class default
	if (EnemyLevelOverride > 0)
	{
		Enemy->SetCharacterLevel(EnemyLevelOverride);
	}
	else if (ZoneDefinition)
	{
		Enemy->SetCharacterLevel(ZoneDefinition->BaseEnemyLevel);
	}

	// Bind death callback for respawning
	Enemy->OnEnemyDeath.AddDynamic(this, &AEnemySpawnPoint::OnEnemyDied);

	CurrentEnemy = Enemy;

	// Apply difficulty scaling after a short delay to let PossessedBy/InitializeAttributes complete
	if (DifficultyMultiplier > 1.0f)
	{
		// Use a timer to ensure ASC init is done (PossessedBy is called during SpawnActor for AutoPossess)
		FTimerHandle DifficultyTimerHandle;
		FTimerDelegate TimerDel;
		TimerDel.BindUObject(this, &AEnemySpawnPoint::ApplyDifficultyScaling, Enemy, DifficultyMultiplier);
		GetWorld()->GetTimerManager().SetTimer(DifficultyTimerHandle, TimerDel, 0.1f, false);
	}

	UE_LOG(LogTemp, Log, TEXT("[SpawnPoint] %s: Spawned %s (difficulty=%.2f)"),
		*GetName(), *Enemy->GetName(), DifficultyMultiplier);

	return Enemy;
}

bool AEnemySpawnPoint::HasLivingEnemy() const
{
	return CurrentEnemy.IsValid() && CurrentEnemy->IsAlive();
}

void AEnemySpawnPoint::OnEnemyDied(AARPGEnemyCharacter* DeadEnemy)
{
	UE_LOG(LogTemp, Log, TEXT("[SpawnPoint] %s: Enemy died"), *GetName());
	CurrentEnemy = nullptr;
	ScheduleRespawn();
}

void AEnemySpawnPoint::ScheduleRespawn()
{
	if (RespawnDelay <= 0.f) return;

	GetWorld()->GetTimerManager().SetTimer(
		RespawnTimerHandle,
		[this]() { SpawnEnemy(); },
		RespawnDelay,
		false);

	UE_LOG(LogTemp, Log, TEXT("[SpawnPoint] %s: Respawn in %.1fs"), *GetName(), RespawnDelay);
}

FVector AEnemySpawnPoint::GetSpawnLocation() const
{
	FVector Location = GetActorLocation();

	if (SpawnRadius > 0.f)
	{
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		const float Dist = FMath::FRandRange(0.f, SpawnRadius);
		Location += FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.f);

		// Project onto nav mesh for valid placement
		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
		if (NavSys)
		{
			FNavLocation NavLoc;
			if (NavSys->ProjectPointToNavigation(Location, NavLoc, FVector(SpawnRadius, SpawnRadius, 200.f)))
			{
				Location = NavLoc.Location;
			}
		}
	}

	return Location;
}

void AEnemySpawnPoint::ApplyDifficultyScaling(AARPGEnemyCharacter* Enemy, float Multiplier)
{
	if (!Enemy || !Enemy->IsAlive()) return;

	UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
	if (!ASC) return;

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(
		UGE_DifficultyScaling::StaticClass(), 1.f, Context);

	if (!SpecHandle.IsValid()) return;

	SpecHandle.Data->SetSetByCallerMagnitude(ARPGGameplayTags::Data_DifficultyMultiplier, Multiplier);
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	UE_LOG(LogTemp, Log, TEXT("[SpawnPoint] Applied difficulty scaling x%.2f to %s"),
		Multiplier, *Enemy->GetName());
}
