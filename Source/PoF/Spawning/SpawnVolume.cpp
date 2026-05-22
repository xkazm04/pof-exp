#include "Spawning/SpawnVolume.h"
#include "Character/ARPGEnemyCharacter.h"
#include "Framework/ARPGGameInstance.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystem/Effects/GE_DifficultyScaling.h"
#include "Components/BoxComponent.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"

ASpawnVolume::ASpawnVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	TriggerVolume->SetBoxExtent(FVector(500.f, 500.f, 200.f));
	TriggerVolume->SetCollisionProfileName(TEXT("Trigger"));
	TriggerVolume->SetGenerateOverlapEvents(true);
	SetRootComponent(TriggerVolume);
}

void ASpawnVolume::BeginPlay()
{
	Super::BeginPlay();

	if (bTriggerOnOverlap)
	{
		TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ASpawnVolume::OnTriggerBeginOverlap);
	}
}

void ASpawnVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(WaveDelayTimerHandle);
	GetWorldTimerManager().ClearTimer(SpawnIntervalTimerHandle);
	GetWorldTimerManager().ClearTimer(RetriggerTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void ASpawnVolume::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Only trigger for the player pawn
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (OtherActor != PlayerPawn) return;

	if (bIsSpawning) return;

	if (bHasTriggered && !bCanRetrigger) return;

	StartWaves();
}

void ASpawnVolume::StartWaves()
{
	if (Waves.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SpawnVolume] %s: No waves configured"), *GetName());
		return;
	}

	bIsSpawning = true;
	bHasTriggered = true;
	bWaitingForKills = false;
	CurrentWaveIndex = -1;
	SpawnedEnemies.Empty();
	CurrentWaveEnemies.Empty();

	BeginNextWave();
}

void ASpawnVolume::BeginNextWave()
{
	CurrentWaveIndex++;

	if (!Waves.IsValidIndex(CurrentWaveIndex))
	{
		// All waves done
		bIsSpawning = false;
		OnAllWavesComplete.Broadcast();

		if (bCanRetrigger)
		{
			GetWorld()->GetTimerManager().SetTimer(
				RetriggerTimerHandle,
				[this]() { bHasTriggered = false; },
				RetriggerDelay,
				false);
		}

		UE_LOG(LogTemp, Log, TEXT("[SpawnVolume] %s: All waves complete"), *GetName());
		return;
	}

	const FSpawnWaveConfig& Wave = Waves[CurrentWaveIndex];
	CurrentWaveSpawnedCount = 0;
	CurrentWaveEnemies.Empty();

	OnWaveStarted.Broadcast(CurrentWaveIndex, Waves.Num());

	UE_LOG(LogTemp, Log, TEXT("[SpawnVolume] %s: Wave %d/%d starting (count=%d, class=%s)"),
		*GetName(), CurrentWaveIndex + 1, Waves.Num(), Wave.Count,
		Wave.EnemyClass ? *Wave.EnemyClass->GetName() : TEXT("null"));

	if (Wave.DelayBeforeWave > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			WaveDelayTimerHandle,
			this, &ASpawnVolume::SpawnNextInWave,
			Wave.DelayBeforeWave,
			false);
	}
	else
	{
		SpawnNextInWave();
	}
}

void ASpawnVolume::SpawnNextInWave()
{
	if (!Waves.IsValidIndex(CurrentWaveIndex)) return;

	const FSpawnWaveConfig& Wave = Waves[CurrentWaveIndex];

	if (CurrentWaveSpawnedCount >= Wave.Count)
	{
		GetWorld()->GetTimerManager().ClearTimer(SpawnIntervalTimerHandle);

		if (bWaitForKillsBeforeNextWave)
		{
			// Wait for all current wave enemies to die before advancing
			bWaitingForKills = true;
			CheckWaveCompletion();
		}
		else
		{
			// Advance immediately (overlapping waves)
			BeginNextWave();
		}
		return;
	}

	// Respect max concurrent enemy cap — defer spawn if at limit
	if (MaxConcurrentEnemies > 0 && GetLivingEnemyCount() >= MaxConcurrentEnemies)
	{
		GetWorld()->GetTimerManager().SetTimer(
			SpawnIntervalTimerHandle,
			this, &ASpawnVolume::SpawnNextInWave,
			1.0f, false);
		return;
	}

	AARPGEnemyCharacter* Enemy = SpawnEnemyInVolume(Wave.EnemyClass);
	CurrentWaveSpawnedCount++;

	if (Enemy)
	{
		SpawnedEnemies.Add(Enemy);
		CurrentWaveEnemies.Add(Enemy);
		Enemy->OnEnemyDeath.AddDynamic(this, &ASpawnVolume::OnSpawnedEnemyDied);

		// Apply difficulty scaling: base volume difficulty * per-wave multiplier
		const float BaseMultiplier = ComputeDifficultyMultiplier();
		const float FinalMultiplier = BaseMultiplier * Wave.WaveDifficultyMultiplier;
		if (FinalMultiplier > 1.0f)
		{
			FTimerHandle DifficultyTimerHandle;
			FTimerDelegate TimerDel;
			TimerDel.BindUObject(this, &ASpawnVolume::ApplyDifficultyScaling, Enemy, FinalMultiplier);
			GetWorld()->GetTimerManager().SetTimer(DifficultyTimerHandle, TimerDel, 0.1f, false);
		}
	}

	// Schedule the next enemy in this wave
	if (CurrentWaveSpawnedCount < Wave.Count)
	{
		if (Wave.SpawnInterval > 0.f)
		{
			GetWorld()->GetTimerManager().SetTimer(
				SpawnIntervalTimerHandle,
				this, &ASpawnVolume::SpawnNextInWave,
				Wave.SpawnInterval,
				false);
		}
		else
		{
			SpawnNextInWave();
		}
	}
	else if (bWaitForKillsBeforeNextWave)
	{
		// Last enemy spawned — wait for kills to advance
		bWaitingForKills = true;
		CheckWaveCompletion();
	}
	else
	{
		// Last enemy spawned, no kill-wait — advance immediately
		BeginNextWave();
	}
}

AARPGEnemyCharacter* ASpawnVolume::SpawnEnemyInVolume(TSubclassOf<AARPGEnemyCharacter> EnemyClass)
{
	if (!EnemyClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SpawnVolume] %s: Wave has null EnemyClass"), *GetName());
		return nullptr;
	}

	const FVector Location = GetRandomSpawnLocationInVolume();
	const FRotator Rotation = FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AARPGEnemyCharacter* Enemy = GetWorld()->SpawnActor<AARPGEnemyCharacter>(EnemyClass, Location, Rotation, SpawnParams);
	return Enemy;
}

FVector ASpawnVolume::GetRandomSpawnLocationInVolume() const
{
	const FVector Extent = TriggerVolume->GetScaledBoxExtent();
	const FVector Origin = TriggerVolume->GetComponentLocation();

	FVector RandomPoint = Origin + FVector(
		FMath::FRandRange(-Extent.X, Extent.X),
		FMath::FRandRange(-Extent.Y, Extent.Y),
		0.f);

	// Project onto nav mesh for valid placement
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (NavSys)
	{
		FNavLocation NavLoc;
		if (NavSys->ProjectPointToNavigation(RandomPoint, NavLoc, FVector(Extent.X, Extent.Y, 500.f)))
		{
			RandomPoint = NavLoc.Location;
		}
	}

	return RandomPoint;
}

float ASpawnVolume::ComputeDifficultyMultiplier() const
{
	const UARPGGameInstance* GI = Cast<UARPGGameInstance>(UGameplayStatics::GetGameInstance(this));
	if (!GI) return 1.0f;

	const int32 PlayerLevel = GI->PlayerLevel;
	const float RawMultiplier = 1.f + static_cast<float>(PlayerLevel - BaseDifficultyLevel) * DifficultyScalePerLevel;

	return FMath::Clamp(RawMultiplier, 1.0f, MaxDifficultyMultiplier);
}

void ASpawnVolume::ApplyDifficultyScaling(AARPGEnemyCharacter* Enemy, float Multiplier)
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

	UE_LOG(LogTemp, Log, TEXT("[SpawnVolume] Applied difficulty x%.2f to %s (PlayerLvl vs Base %d)"),
		Multiplier, *Enemy->GetName(), BaseDifficultyLevel);
}

int32 ASpawnVolume::GetLivingEnemyCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AARPGEnemyCharacter>& EnemyPtr : SpawnedEnemies)
	{
		if (EnemyPtr.IsValid() && EnemyPtr->IsAlive())
		{
			Count++;
		}
	}
	return Count;
}

void ASpawnVolume::OnSpawnedEnemyDied(AARPGEnemyCharacter* DeadEnemy)
{
	CheckWaveCompletion();
}

void ASpawnVolume::CheckWaveCompletion()
{
	// Clean up dead references from all-time list
	SpawnedEnemies.RemoveAll([](const TWeakObjectPtr<AARPGEnemyCharacter>& Ptr)
	{
		return !Ptr.IsValid() || !Ptr->IsAlive();
	});

	// If waiting for kills, check if the current wave's enemies are all dead
	if (bWaitingForKills)
	{
		const bool bAllWaveEnemiesDead = !CurrentWaveEnemies.ContainsByPredicate(
			[](const TWeakObjectPtr<AARPGEnemyCharacter>& Ptr)
			{
				return Ptr.IsValid() && Ptr->IsAlive();
			});

		if (bAllWaveEnemiesDead)
		{
			bWaitingForKills = false;
			UE_LOG(LogTemp, Log, TEXT("[SpawnVolume] %s: Wave %d cleared — advancing"),
				*GetName(), CurrentWaveIndex + 1);
			BeginNextWave();
		}
	}
}
