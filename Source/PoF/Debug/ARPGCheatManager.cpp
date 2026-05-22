#include "Debug/ARPGCheatManager.h"
#include "Debug/ARPGLogCategories.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "Character/ARPGEnemyCharacter.h"
#include "Player/ARPGPlayerCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

// =============================================================================
// God Mode
// =============================================================================

void UARPGCheatManager::ARPGGod()
{
	bGodMode = !bGodMode;

	APlayerController* PC = GetOuterAPlayerController();
	if (!PC) return;

	APawn* Pawn = PC->GetPawn();
	if (!Pawn) return;

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Pawn);
	if (!ASI) return;

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC) return;

	if (bGodMode)
	{
		// Restore full vitals
		UARPGAttributeSet* Attrs = const_cast<UARPGAttributeSet*>(ASC->GetSet<UARPGAttributeSet>());
		if (Attrs)
		{
			Attrs->SetHealth(Attrs->GetMaxHealth());
			Attrs->SetMana(Attrs->GetMaxMana());
		}

		UE_LOG(LogARPGCombat, Display, TEXT("God mode ENABLED — invulnerable + infinite mana"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("GOD MODE ON"));
		}
	}
	else
	{
		UE_LOG(LogARPGCombat, Display, TEXT("God mode DISABLED"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("GOD MODE OFF"));
		}
	}
}

// =============================================================================
// Progression
// =============================================================================

void UARPGCheatManager::ARPGSetLevel(int32 NewLevel)
{
	NewLevel = FMath::Max(1, NewLevel);

	APlayerController* PC = GetOuterAPlayerController();
	if (!PC || !PC->GetPawn()) return;

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PC->GetPawn());
	if (!ASI) return;

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC) return;

	UARPGAttributeSet* Attrs = const_cast<UARPGAttributeSet*>(ASC->GetSet<UARPGAttributeSet>());
	if (!Attrs) return;

	Attrs->SetCharacterLevel(static_cast<float>(NewLevel));
	Attrs->SetCurrentXP(0.f);

	UE_LOG(LogARPGAbility, Display, TEXT("Player level set to %d"), NewLevel);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan,
			FString::Printf(TEXT("Level set to %d"), NewLevel));
	}
}

void UARPGCheatManager::ARPGGiveXP(float Amount)
{
	APlayerController* PC = GetOuterAPlayerController();
	if (!PC || !PC->GetPawn()) return;

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PC->GetPawn());
	if (!ASI) return;

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC) return;

	UARPGAttributeSet* Attrs = const_cast<UARPGAttributeSet*>(ASC->GetSet<UARPGAttributeSet>());
	if (!Attrs) return;

	const float OldXP = Attrs->GetCurrentXP();
	Attrs->SetCurrentXP(OldXP + Amount);

	UE_LOG(LogARPGAbility, Display, TEXT("Awarded %.0f XP (%.0f -> %.0f)"), Amount, OldXP, OldXP + Amount);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
			FString::Printf(TEXT("+%.0f XP"), Amount));
	}
}

// =============================================================================
// Vitals
// =============================================================================

void UARPGCheatManager::ARPGHeal()
{
	APlayerController* PC = GetOuterAPlayerController();
	if (!PC || !PC->GetPawn()) return;

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PC->GetPawn());
	if (!ASI) return;

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC) return;

	UARPGAttributeSet* Attrs = const_cast<UARPGAttributeSet*>(ASC->GetSet<UARPGAttributeSet>());
	if (!Attrs) return;

	Attrs->SetHealth(Attrs->GetMaxHealth());
	Attrs->SetMana(Attrs->GetMaxMana());

	UE_LOG(LogARPGCombat, Display, TEXT("Player fully healed (HP=%.0f, MP=%.0f)"),
		Attrs->GetMaxHealth(), Attrs->GetMaxMana());
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("Fully healed"));
	}
}

// =============================================================================
// Combat
// =============================================================================

void UARPGCheatManager::ARPGKillAll()
{
	UWorld* World = GetWorld();
	if (!World) return;

	int32 KillCount = 0;
	for (TActorIterator<AARPGEnemyCharacter> It(World); It; ++It)
	{
		AARPGEnemyCharacter* Enemy = *It;
		if (!Enemy || Enemy->IsDead()) continue;

		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Enemy);
		if (!ASI) continue;

		UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
		if (!ASC) continue;

		UARPGAttributeSet* Attrs = const_cast<UARPGAttributeSet*>(ASC->GetSet<UARPGAttributeSet>());
		if (Attrs)
		{
			Attrs->SetHealth(0.f);
			++KillCount;
		}
	}

	UE_LOG(LogARPGCombat, Display, TEXT("KillAll: Killed %d enemies"), KillCount);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
			FString::Printf(TEXT("Killed %d enemies"), KillCount));
	}
}

void UARPGCheatManager::ARPGSpawnEnemy(const FString& BlueprintPath, int32 EnemyLevel)
{
	APlayerController* PC = GetOuterAPlayerController();
	if (!PC || !PC->GetPawn()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// Resolve the class from path (e.g., "/Game/Blueprints/BP_Enemy_Grunt.BP_Enemy_Grunt_C")
	UClass* EnemyClass = LoadClass<AARPGEnemyCharacter>(nullptr, *BlueprintPath);
	if (!EnemyClass)
	{
		// Try appending _C suffix
		EnemyClass = LoadClass<AARPGEnemyCharacter>(nullptr, *(BlueprintPath + TEXT("_C")));
	}
	if (!EnemyClass)
	{
		UE_LOG(LogARPGAI, Warning, TEXT("SpawnEnemy: Could not load class '%s'"), *BlueprintPath);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
				FString::Printf(TEXT("Failed to load: %s"), *BlueprintPath));
		}
		return;
	}

	// Spawn 500 units in front of the player
	const FVector Forward = PC->GetPawn()->GetActorForwardVector();
	const FVector SpawnLoc = PC->GetPawn()->GetActorLocation() + Forward * 500.f;
	const FRotator SpawnRot = (-Forward).Rotation();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AARPGEnemyCharacter* Spawned = World->SpawnActor<AARPGEnemyCharacter>(EnemyClass, SpawnLoc, SpawnRot, Params);
	if (Spawned)
	{
		Spawned->SetCharacterLevel(FMath::Max(1, EnemyLevel));

		UE_LOG(LogARPGAI, Display, TEXT("Spawned %s at level %d"), *Spawned->GetName(), EnemyLevel);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
				FString::Printf(TEXT("Spawned %s (Lv%d)"), *EnemyClass->GetName(), EnemyLevel));
		}
	}
}

void UARPGCheatManager::ARPGDamageTarget(float Amount)
{
	APlayerController* PC = GetOuterAPlayerController();
	if (!PC) return;

	// Line trace from camera center
	FVector Start;
	FRotator ViewRot;
	PC->GetPlayerViewPoint(Start, ViewRot);
	const FVector End = Start + ViewRot.Vector() * 5000.f;

	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(PC->GetPawn());

	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, QueryParams))
	{
		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Hit.GetActor());
		if (ASI)
		{
			UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
			if (ASC)
			{
				UARPGAttributeSet* Attrs = const_cast<UARPGAttributeSet*>(ASC->GetSet<UARPGAttributeSet>());
				if (Attrs)
				{
					const float OldHP = Attrs->GetHealth();
					Attrs->SetHealth(FMath::Max(0.f, OldHP - Amount));

					UE_LOG(LogARPGCombat, Display, TEXT("Dealt %.0f damage to %s (%.0f -> %.0f)"),
						Amount, *Hit.GetActor()->GetName(), OldHP, Attrs->GetHealth());
					if (GEngine)
					{
						GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
							FString::Printf(TEXT("%.0f dmg -> %s"), Amount, *Hit.GetActor()->GetName()));
					}
					return;
				}
			}
		}
	}

	UE_LOG(LogARPGCombat, Display, TEXT("DamageTarget: No valid target under crosshair"));
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Silver, TEXT("No target under crosshair"));
	}
}

// =============================================================================
// Diagnostics
// =============================================================================

void UARPGCheatManager::ARPGShowStats()
{
	APlayerController* PC = GetOuterAPlayerController();
	if (!PC || !PC->GetPawn()) return;

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PC->GetPawn());
	if (!ASI) return;

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC) return;

	const UARPGAttributeSet* Attrs = ASC->GetSet<UARPGAttributeSet>();
	if (!Attrs) return;

	const FString Stats = FString::Printf(
		TEXT("=== Player Stats ===\n"
			 "Level: %.0f | XP: %.0f/%.0f\n"
			 "HP: %.0f/%.0f | MP: %.0f/%.0f\n"
			 "STR: %.0f | DEX: %.0f | INT: %.0f\n"
			 "Armor: %.0f | AttackPower: %.0f\n"
			 "Crit: %.1f%% | CritDmg: %.1fx\n"
			 "Resist — Fire: %.0f%% Ice: %.0f%% Lightning: %.0f%%"),
		Attrs->GetCharacterLevel(),
		Attrs->GetCurrentXP(), Attrs->GetXPToNextLevel(),
		Attrs->GetHealth(), Attrs->GetMaxHealth(),
		Attrs->GetMana(), Attrs->GetMaxMana(),
		Attrs->GetStrength(), Attrs->GetDexterity(), Attrs->GetIntelligence(),
		Attrs->GetArmor(), Attrs->GetAttackPower(),
		Attrs->GetCriticalChance() * 100.f, Attrs->GetCriticalDamage(),
		Attrs->GetFireResistance() * 100.f,
		Attrs->GetIceResistance() * 100.f,
		Attrs->GetLightningResistance() * 100.f);

	UE_LOG(LogARPGAbility, Display, TEXT("%s"), *Stats);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::White, Stats);
	}
}
