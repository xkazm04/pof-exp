#include "ProgressionTestCommandlet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "AbilitySystem/ARPGAbilityUnlockComponent.h"
#include "AbilitySystem/ARPGAbilityUnlockData.h"
#include "AbilitySystem/ARPGGameplayAbility.h"
#include "AbilitySystem/GA_Fireball.h"
#include "AbilitySystem/GA_GroundSlam.h"
#include "AbilitySystem/GA_DashStrike.h"
#include "AbilitySystem/GA_WarCry.h"
#include "Player/ARPGPlayerCharacter.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"

UProgressionTestCommandlet::UProgressionTestCommandlet()
{
	LogToConsole = true;
	IsClient = false;
	IsServer = false;
	IsEditor = true;
}

bool UProgressionTestCommandlet::Check(bool bCondition, const FString& TestName)
{
	if (bCondition)
	{
		TestsPassed++;
		UE_LOG(LogTemp, Display, TEXT("  [PASS] %s"), *TestName);
	}
	else
	{
		TestsFailed++;
		UE_LOG(LogTemp, Error, TEXT("  [FAIL] %s"), *TestName);
	}
	return bCondition;
}

// =========================================================================
// Helper: create a test world + spawn player with ASC initialized
// =========================================================================
struct FTestContext
{
	UWorld* World = nullptr;
	AARPGPlayerCharacter* Player = nullptr;
};

static FTestContext CreateTestWorld()
{
	FTestContext Ctx;
	Ctx.World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& WC = GEngine->CreateNewWorldContext(EWorldType::Game);
	WC.SetCurrentWorld(Ctx.World);
	Ctx.World->InitializeActorsForPlay(FURL());
	Ctx.World->BeginPlay();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	Ctx.Player = Ctx.World->SpawnActor<AARPGPlayerCharacter>(
		AARPGPlayerCharacter::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

	if (Ctx.Player)
	{
		// Initialize ASC without a controller (commandlet has no controller).
		// Use the player as both owner and avatar.
		UAbilitySystemComponent* ASC = Ctx.Player->GetAbilitySystemComponent();
		if (ASC)
		{
			ASC->InitAbilityActorInfo(Ctx.Player, Ctx.Player);

			// Create and register the attribute set (normally done in PossessedBy/InitializeAttributes)
			UARPGAttributeSet* AttribSet = NewObject<UARPGAttributeSet>(Ctx.Player, TEXT("TestAttributeSet"));
			ASC->AddSpawnedAttribute(AttribSet);
			ASC->ForceReplication();
		}
	}

	return Ctx;
}

static void DestroyTestWorld(FTestContext& Ctx)
{
	GEngine->DestroyWorldContext(Ctx.World);
	Ctx.World->DestroyWorld(false);
	Ctx.World = nullptr;
	Ctx.Player = nullptr;
}

// =========================================================================
// Test 1: XP Award → Level Up
// =========================================================================
bool UProgressionTestCommandlet::TestXPAndLevelUp()
{
	UE_LOG(LogTemp, Display, TEXT(""));
	UE_LOG(LogTemp, Display, TEXT("=== Test 1: XP Award and Level Up ==="));

	FTestContext Ctx = CreateTestWorld();
	bool bAllPassed = true;

	if (!Check(Ctx.Player != nullptr, TEXT("Player character spawned")))
	{
		DestroyTestWorld(Ctx);
		return false;
	}

	AARPGPlayerCharacter* Player = Ctx.Player;

	// Verify initial state
	bAllPassed &= Check(Player->GetPlayerLevel() == 1, TEXT("Initial level is 1"));
	bAllPassed &= Check(FMath::IsNearlyZero(Player->GetExperience()), TEXT("Initial XP is 0"));
	bAllPassed &= Check(Player->GetExperienceToNextLevel() > 0.f, TEXT("XP to next level > 0"));

	const float XPNeeded = Player->GetExperienceToNextLevel();
	const float InitialMaxHP = Player->GetMaxHealth();
	const float InitialMaxMP = Player->GetMaxMana();

	// Add just under the threshold — should NOT level up
	Player->AddExperience(XPNeeded - 1.f);
	bAllPassed &= Check(Player->GetPlayerLevel() == 1, TEXT("Under-threshold XP does not level up"));
	bAllPassed &= Check(Player->GetExperience() > 0.f, TEXT("XP accumulated correctly"));

	// Add the remaining 1 XP — should level up to 2
	Player->AddExperience(1.f);
	bAllPassed &= Check(Player->GetPlayerLevel() == 2, TEXT("Level up to 2 on exact threshold"));
	bAllPassed &= Check(Player->GetMaxHealth() > InitialMaxHP, TEXT("MaxHealth increased on level up"));
	bAllPassed &= Check(Player->GetMaxMana() > InitialMaxMP, TEXT("MaxMana increased on level up"));
	bAllPassed &= Check(Player->GetHealth() == Player->GetMaxHealth(), TEXT("Full heal on level up"));
	bAllPassed &= Check(Player->GetMana() == Player->GetMaxMana(), TEXT("Full mana restore on level up"));

	// Verify skill points awarded
	UARPGAbilityUnlockComponent* UnlockComp = Player->GetAbilityUnlockComponent();
	bAllPassed &= Check(UnlockComp != nullptr, TEXT("AbilityUnlockComponent exists"));
	if (UnlockComp)
	{
		bAllPassed &= Check(UnlockComp->GetSkillPoints() > 0, TEXT("Skill points awarded on level up"));
		UE_LOG(LogTemp, Display, TEXT("    Skill points: %d"), UnlockComp->GetSkillPoints());
	}

	// Verify attribute points awarded
	bAllPassed &= Check(Player->GetUnspentAttributePoints() > 0, TEXT("Attribute points awarded on level up"));
	UE_LOG(LogTemp, Display, TEXT("    Attribute points: %d"), Player->GetUnspentAttributePoints());

	DestroyTestWorld(Ctx);
	return bAllPassed;
}

// =========================================================================
// Test 2: Attribute Point Allocation
// =========================================================================
bool UProgressionTestCommandlet::TestAttributeAllocation()
{
	UE_LOG(LogTemp, Display, TEXT(""));
	UE_LOG(LogTemp, Display, TEXT("=== Test 2: Attribute Point Allocation ==="));

	FTestContext Ctx = CreateTestWorld();
	bool bAllPassed = true;

	if (!Check(Ctx.Player != nullptr, TEXT("Player spawned for attribute test")))
	{
		DestroyTestWorld(Ctx);
		return false;
	}

	AARPGPlayerCharacter* Player = Ctx.Player;

	// Cannot allocate with 0 points
	bAllPassed &= Check(!Player->AllocateStrength(), TEXT("Cannot allocate STR with 0 points"));
	bAllPassed &= Check(!Player->AllocateDexterity(), TEXT("Cannot allocate DEX with 0 points"));
	bAllPassed &= Check(!Player->AllocateIntelligence(), TEXT("Cannot allocate INT with 0 points"));

	// Level up to get points
	const float XPNeeded = Player->GetExperienceToNextLevel();
	Player->AddExperience(XPNeeded);
	const int32 PointsAfterLevelUp = Player->GetUnspentAttributePoints();
	bAllPassed &= Check(PointsAfterLevelUp > 0, TEXT("Have attribute points after level up"));

	// Read GAS attributes before allocation
	UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	bAllPassed &= Check(ASC != nullptr, TEXT("ASC exists on player"));

	if (ASC)
	{
		const UARPGAttributeSet* Attribs = ASC->GetSet<UARPGAttributeSet>();
		bAllPassed &= Check(Attribs != nullptr, TEXT("AttributeSet exists on ASC"));

		if (Attribs)
		{
			const float APBefore = Attribs->GetAttackPower();
			const float CritBefore = Attribs->GetCriticalChance();
			const float ManaBefore = Attribs->GetMaxMana();

			// Allocate STR
			bAllPassed &= Check(Player->AllocateStrength(), TEXT("AllocateStrength succeeds"));
			bAllPassed &= Check(Player->GetAllocatedStrength() == 1, TEXT("AllocatedStrength == 1"));
			bAllPassed &= Check(Attribs->GetAttackPower() > APBefore, TEXT("AttackPower increased after STR allocation"));
			UE_LOG(LogTemp, Display, TEXT("    AttackPower: %.1f -> %.1f (+%.1f)"),
				APBefore, Attribs->GetAttackPower(), Attribs->GetAttackPower() - APBefore);

			// Allocate DEX (if we have points)
			if (Player->GetUnspentAttributePoints() > 0)
			{
				bAllPassed &= Check(Player->AllocateDexterity(), TEXT("AllocateDexterity succeeds"));
				bAllPassed &= Check(Player->GetAllocatedDexterity() == 1, TEXT("AllocatedDexterity == 1"));
				bAllPassed &= Check(Attribs->GetCriticalChance() > CritBefore, TEXT("CriticalChance increased after DEX allocation"));
				UE_LOG(LogTemp, Display, TEXT("    CriticalChance: %.3f -> %.3f"), CritBefore, Attribs->GetCriticalChance());
			}

			// Allocate INT (if we have points)
			if (Player->GetUnspentAttributePoints() > 0)
			{
				bAllPassed &= Check(Player->AllocateIntelligence(), TEXT("AllocateIntelligence succeeds"));
				bAllPassed &= Check(Player->GetAllocatedIntelligence() == 1, TEXT("AllocatedIntelligence == 1"));
				bAllPassed &= Check(Attribs->GetMaxMana() > ManaBefore, TEXT("MaxMana increased after INT allocation"));
				UE_LOG(LogTemp, Display, TEXT("    MaxMana: %.1f -> %.1f"), ManaBefore, Attribs->GetMaxMana());
			}

			// Points should be spent
			bAllPassed &= Check(Player->GetUnspentAttributePoints() < PointsAfterLevelUp,
				TEXT("Unspent points decreased after allocations"));
		}
	}

	DestroyTestWorld(Ctx);
	return bAllPassed;
}

// =========================================================================
// Test 3: Ability Unlock
// =========================================================================
bool UProgressionTestCommandlet::TestAbilityUnlock()
{
	UE_LOG(LogTemp, Display, TEXT(""));
	UE_LOG(LogTemp, Display, TEXT("=== Test 3: Ability Unlock System ==="));

	FTestContext Ctx = CreateTestWorld();
	bool bAllPassed = true;

	if (!Check(Ctx.Player != nullptr, TEXT("Player spawned for unlock test")))
	{
		DestroyTestWorld(Ctx);
		return false;
	}

	AARPGPlayerCharacter* Player = Ctx.Player;
	UARPGAbilityUnlockComponent* UnlockComp = Player->GetAbilityUnlockComponent();
	bAllPassed &= Check(UnlockComp != nullptr, TEXT("UnlockComponent exists"));

	if (UnlockComp)
	{
		// Create a test DataTable programmatically
		UDataTable* TestDT = NewObject<UDataTable>(GetTransientPackage(), TEXT("TestAbilityUnlockTable"));
		TestDT->RowStruct = FAbilityUnlockRow::StaticStruct();

		// Add a Fireball row
		FAbilityUnlockRow FireballRow;
		FireballRow.AbilityClass = UGA_Fireball::StaticClass();
		FireballRow.RequiredLevel = 1;
		FireballRow.SkillPointCost = 1;
		FireballRow.MaxRank = 3;
		FireballRow.UpgradeSkillPointCost = 1;
		FireballRow.DisplayName = FText::FromString(TEXT("Fireball"));
		FireballRow.Description = FText::FromString(TEXT("Test fireball"));
		TestDT->AddRow(FName("Fireball"), FireballRow);

		// Add a GroundSlam row (requires level 3)
		FAbilityUnlockRow SlamRow;
		SlamRow.AbilityClass = UGA_GroundSlam::StaticClass();
		SlamRow.RequiredLevel = 3;
		SlamRow.SkillPointCost = 2;
		SlamRow.MaxRank = 3;
		SlamRow.UpgradeSkillPointCost = 2;
		SlamRow.DisplayName = FText::FromString(TEXT("Ground Slam"));
		SlamRow.Description = FText::FromString(TEXT("Test ground slam"));
		TestDT->AddRow(FName("GroundSlam"), SlamRow);

		// Inject the data table into the component via reflection (it's EditDefaultsOnly)
		FObjectPropertyBase* DTProp = CastField<FObjectPropertyBase>(
			UARPGAbilityUnlockComponent::StaticClass()->FindPropertyByName(TEXT("AbilityUnlockTable")));
		if (DTProp)
		{
			DTProp->SetObjectPropertyValue_InContainer(UnlockComp, TestDT);
		}

		// With 0 skill points, cannot learn anything
		bAllPassed &= Check(!UnlockComp->CanLearnAbility(FName("Fireball")), TEXT("Cannot learn Fireball with 0 skill points"));

		// Give skill points
		UnlockComp->AddSkillPoints(10);
		bAllPassed &= Check(UnlockComp->GetSkillPoints() == 10, TEXT("10 skill points added"));

		// Learn Fireball
		bAllPassed &= Check(UnlockComp->CanLearnAbility(FName("Fireball")), TEXT("Can learn Fireball at level 1"));
		bAllPassed &= Check(UnlockComp->LearnAbility(FName("Fireball")), TEXT("LearnAbility(Fireball) succeeds"));
		bAllPassed &= Check(UnlockComp->HasLearnedAbility(FName("Fireball")), TEXT("HasLearnedAbility(Fireball) is true"));
		bAllPassed &= Check(UnlockComp->GetAbilityRank(FName("Fireball")) == 1, TEXT("Fireball at rank 1"));
		bAllPassed &= Check(UnlockComp->GetSkillPoints() == 9, TEXT("1 skill point spent (9 remaining)"));

		// Cannot learn again
		bAllPassed &= Check(!UnlockComp->CanLearnAbility(FName("Fireball")), TEXT("Cannot learn Fireball again"));

		// Upgrade Fireball to rank 2
		bAllPassed &= Check(UnlockComp->CanUpgradeAbility(FName("Fireball")), TEXT("Can upgrade Fireball"));
		bAllPassed &= Check(UnlockComp->UpgradeAbility(FName("Fireball")), TEXT("UpgradeAbility(Fireball) succeeds"));
		bAllPassed &= Check(UnlockComp->GetAbilityRank(FName("Fireball")) == 2, TEXT("Fireball at rank 2"));

		// GroundSlam requires level 3 — should fail at level 1
		bAllPassed &= Check(!UnlockComp->CanLearnAbility(FName("GroundSlam")), TEXT("Cannot learn GroundSlam at level 1 (requires 3)"));

		// Level up to 3
		Player->AddExperience(Player->GetExperienceToNextLevel()); // -> level 2
		Player->AddExperience(Player->GetExperienceToNextLevel()); // -> level 3
		bAllPassed &= Check(Player->GetPlayerLevel() == 3, TEXT("Player is now level 3"));

		// Now can learn GroundSlam
		bAllPassed &= Check(UnlockComp->CanLearnAbility(FName("GroundSlam")), TEXT("Can learn GroundSlam at level 3"));
		bAllPassed &= Check(UnlockComp->LearnAbility(FName("GroundSlam")), TEXT("LearnAbility(GroundSlam) succeeds"));
		bAllPassed &= Check(UnlockComp->HasLearnedAbility(FName("GroundSlam")), TEXT("HasLearnedAbility(GroundSlam) is true"));

		// Verify ASC has both abilities granted
		UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
		if (Check(ASC != nullptr, TEXT("ASC exists")))
		{
			FGameplayAbilitySpec* FireballSpec = ASC->FindAbilitySpecFromClass(UGA_Fireball::StaticClass());
			bAllPassed &= Check(FireballSpec != nullptr, TEXT("ASC has Fireball spec"));
			if (FireballSpec)
			{
				bAllPassed &= Check(FireballSpec->Level == 2, TEXT("Fireball spec level == 2 (after upgrade)"));
			}

			FGameplayAbilitySpec* SlamSpec = ASC->FindAbilitySpecFromClass(UGA_GroundSlam::StaticClass());
			bAllPassed &= Check(SlamSpec != nullptr, TEXT("ASC has GroundSlam spec"));
		}

		UE_LOG(LogTemp, Display, TEXT("    Learned abilities: %d"), UnlockComp->GetLearnedAbilityRowNames().Num());
		UE_LOG(LogTemp, Display, TEXT("    Remaining skill points: %d"), UnlockComp->GetSkillPoints());
	}

	DestroyTestWorld(Ctx);
	return bAllPassed;
}

// =========================================================================
// Test 4: Ability Loadout
// =========================================================================
bool UProgressionTestCommandlet::TestAbilityLoadout()
{
	UE_LOG(LogTemp, Display, TEXT(""));
	UE_LOG(LogTemp, Display, TEXT("=== Test 4: Ability Loadout ==="));

	FTestContext Ctx = CreateTestWorld();
	bool bAllPassed = true;

	if (!Check(Ctx.Player != nullptr, TEXT("Player spawned for loadout test")))
	{
		DestroyTestWorld(Ctx);
		return false;
	}

	AARPGPlayerCharacter* Player = Ctx.Player;

	// All slots should start empty
	for (int32 i = 0; i < Player->GetAbilitySlotCount(); ++i)
	{
		bAllPassed &= Check(Player->GetSlotAbility(i) == nullptr,
			FString::Printf(TEXT("Slot %d starts empty"), i));
	}

	// Assign abilities to slots
	Player->AssignAbilityToSlot(0, UGA_Fireball::StaticClass());
	Player->AssignAbilityToSlot(1, UGA_GroundSlam::StaticClass());
	Player->AssignAbilityToSlot(2, UGA_DashStrike::StaticClass());
	Player->AssignAbilityToSlot(3, UGA_WarCry::StaticClass());

	// Verify assignments
	bAllPassed &= Check(Player->GetSlotAbility(0) == UGA_Fireball::StaticClass(), TEXT("Slot 0 = Fireball"));
	bAllPassed &= Check(Player->GetSlotAbility(1) == UGA_GroundSlam::StaticClass(), TEXT("Slot 1 = GroundSlam"));
	bAllPassed &= Check(Player->GetSlotAbility(2) == UGA_DashStrike::StaticClass(), TEXT("Slot 2 = DashStrike"));
	bAllPassed &= Check(Player->GetSlotAbility(3) == UGA_WarCry::StaticClass(), TEXT("Slot 3 = WarCry"));

	// Verify loadout map
	const TMap<int32, TSubclassOf<UGameplayAbility>>& Loadout = Player->GetLoadout();
	bAllPassed &= Check(Loadout.Num() == 4, TEXT("Loadout has 4 entries"));

	// Re-assign slot 0 to WarCry (swap test)
	Player->AssignAbilityToSlot(0, UGA_WarCry::StaticClass());
	bAllPassed &= Check(Player->GetSlotAbility(0) == UGA_WarCry::StaticClass(), TEXT("Slot 0 reassigned to WarCry"));

	// Clear slot 2
	Player->ClearAbilitySlot(2);
	bAllPassed &= Check(Player->GetSlotAbility(2) == nullptr, TEXT("Slot 2 cleared successfully"));
	bAllPassed &= Check(Player->GetLoadout().Num() == 3, TEXT("Loadout has 3 entries after clear"));

	// Invalid slot indices
	Player->AssignAbilityToSlot(-1, UGA_Fireball::StaticClass()); // should silently fail
	Player->AssignAbilityToSlot(99, UGA_Fireball::StaticClass()); // should silently fail
	bAllPassed &= Check(Player->GetSlotAbility(-1) == nullptr, TEXT("Invalid slot -1 returns null"));
	bAllPassed &= Check(Player->GetSlotAbility(99) == nullptr, TEXT("Invalid slot 99 returns null"));

	// CDO metadata test (ability icons, costs, tags)
	const UGA_Fireball* FireballCDO = GetDefault<UGA_Fireball>();
	bAllPassed &= Check(FireballCDO != nullptr, TEXT("Fireball CDO accessible"));
	if (FireballCDO)
	{
		bAllPassed &= Check(FireballCDO->GetAbilityManaCost() > 0.f, TEXT("Fireball has mana cost > 0"));
		bAllPassed &= Check(FireballCDO->GetAbilityCooldownTag().IsValid(), TEXT("Fireball has cooldown tag"));
		UE_LOG(LogTemp, Display, TEXT("    Fireball ManaCost=%.0f, CooldownTag=%s"),
			FireballCDO->GetAbilityManaCost(),
			*FireballCDO->GetAbilityCooldownTag().ToString());
	}

	DestroyTestWorld(Ctx);
	return bAllPassed;
}

// =========================================================================
// Test 5: Multi-level XP (large XP → multiple level-ups in one call)
// =========================================================================
bool UProgressionTestCommandlet::TestMultiLevelXP()
{
	UE_LOG(LogTemp, Display, TEXT(""));
	UE_LOG(LogTemp, Display, TEXT("=== Test 5: Multi-Level XP Jump ==="));

	FTestContext Ctx = CreateTestWorld();
	bool bAllPassed = true;

	if (!Check(Ctx.Player != nullptr, TEXT("Player spawned for multi-level test")))
	{
		DestroyTestWorld(Ctx);
		return false;
	}

	AARPGPlayerCharacter* Player = Ctx.Player;

	// Give a massive XP dump — should trigger multiple level-ups
	// XP curve: 100 * Level^1.5
	// Level 1->2: 100, Level 2->3: ~283, Level 3->4: ~520, Level 4->5: ~894 = total ~1797
	Player->AddExperience(5000.f);
	bAllPassed &= Check(Player->GetPlayerLevel() > 3, TEXT("Multiple level-ups from large XP (level > 3)"));
	UE_LOG(LogTemp, Display, TEXT("    Final level: %d (from 5000 XP dump)"), Player->GetPlayerLevel());
	UE_LOG(LogTemp, Display, TEXT("    Remaining XP: %.0f / %.0f"), Player->GetExperience(), Player->GetExperienceToNextLevel());

	// Verify accumulated points scale with levels gained
	const int32 LevelsGained = Player->GetPlayerLevel() - 1;
	bAllPassed &= Check(Player->GetUnspentAttributePoints() == LevelsGained * 3,
		FString::Printf(TEXT("Attribute points = %d levels * 3 = %d"),
			LevelsGained, LevelsGained * 3));

	UARPGAbilityUnlockComponent* UnlockComp = Player->GetAbilityUnlockComponent();
	if (UnlockComp)
	{
		bAllPassed &= Check(UnlockComp->GetSkillPoints() == LevelsGained * UnlockComp->GetSkillPointsPerLevel(),
			FString::Printf(TEXT("Skill points = %d levels * %d/level = %d"),
				LevelsGained, UnlockComp->GetSkillPointsPerLevel(),
				LevelsGained * UnlockComp->GetSkillPointsPerLevel()));
	}

	DestroyTestWorld(Ctx);
	return bAllPassed;
}

// =========================================================================
// Test 6: Full Progression Loop (integrated)
// =========================================================================
bool UProgressionTestCommandlet::TestFullLoop()
{
	UE_LOG(LogTemp, Display, TEXT(""));
	UE_LOG(LogTemp, Display, TEXT("=== Test 6: Full Progression Loop (Integrated) ==="));

	FTestContext Ctx = CreateTestWorld();
	bool bAllPassed = true;

	if (!Check(Ctx.Player != nullptr, TEXT("Player spawned for full loop")))
	{
		DestroyTestWorld(Ctx);
		return false;
	}

	AARPGPlayerCharacter* Player = Ctx.Player;
	UARPGAbilityUnlockComponent* UnlockComp = Player->GetAbilityUnlockComponent();
	UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();

	if (!Check(UnlockComp != nullptr && ASC != nullptr, TEXT("Core components exist")))
	{
		DestroyTestWorld(Ctx);
		return false;
	}

	// --- Set up ability data table ---
	UDataTable* TestDT = NewObject<UDataTable>(GetTransientPackage(), TEXT("TestDT_FullLoop"));
	TestDT->RowStruct = FAbilityUnlockRow::StaticStruct();

	FAbilityUnlockRow FireballRow;
	FireballRow.AbilityClass = UGA_Fireball::StaticClass();
	FireballRow.RequiredLevel = 2;
	FireballRow.SkillPointCost = 1;
	FireballRow.MaxRank = 3;
	FireballRow.UpgradeSkillPointCost = 1;
	FireballRow.DisplayName = FText::FromString(TEXT("Fireball"));
	TestDT->AddRow(FName("Fireball"), FireballRow);

	FObjectPropertyBase* DTProp = CastField<FObjectPropertyBase>(
		UARPGAbilityUnlockComponent::StaticClass()->FindPropertyByName(TEXT("AbilityUnlockTable")));
	if (DTProp)
	{
		DTProp->SetObjectPropertyValue_InContainer(UnlockComp, TestDT);
	}

	// --- Step 1: Simulate killing enemies (XP awards) ---
	UE_LOG(LogTemp, Display, TEXT("  Step 1: Kill enemies for XP"));
	const float EnemyXP = 35.f; // Simulated XP per kill
	int32 KillCount = 0;
	while (Player->GetPlayerLevel() < 2)
	{
		Player->AddExperience(EnemyXP);
		KillCount++;
	}
	bAllPassed &= Check(Player->GetPlayerLevel() == 2,
		FString::Printf(TEXT("Reached level 2 after %d kills"), KillCount));

	// --- Step 2: Allocate attribute points ---
	UE_LOG(LogTemp, Display, TEXT("  Step 2: Allocate attribute points"));
	const UARPGAttributeSet* Attribs = ASC->GetSet<UARPGAttributeSet>();
	bAllPassed &= Check(Attribs != nullptr, TEXT("AttributeSet exists for allocation"));

	float APBefore = Attribs ? Attribs->GetAttackPower() : 0.f;

	bAllPassed &= Check(Player->AllocateStrength(), TEXT("Allocate 1 STR"));
	bAllPassed &= Check(Player->AllocateStrength(), TEXT("Allocate 2 STR"));
	bAllPassed &= Check(Player->AllocateDexterity(), TEXT("Allocate 1 DEX"));

	if (Attribs)
	{
		bAllPassed &= Check(Attribs->GetAttackPower() > APBefore,
			FString::Printf(TEXT("AttackPower %.0f -> %.0f"), APBefore, Attribs->GetAttackPower()));
	}

	// --- Step 3: Unlock an ability ---
	UE_LOG(LogTemp, Display, TEXT("  Step 3: Unlock Fireball ability"));
	bAllPassed &= Check(UnlockComp->CanLearnAbility(FName("Fireball")), TEXT("Can learn Fireball at level 2"));
	bAllPassed &= Check(UnlockComp->LearnAbility(FName("Fireball")), TEXT("Learned Fireball"));

	// --- Step 4: Assign to loadout ---
	UE_LOG(LogTemp, Display, TEXT("  Step 4: Assign Fireball to hotbar slot 0"));
	Player->AssignAbilityToSlot(0, UGA_Fireball::StaticClass());
	bAllPassed &= Check(Player->GetSlotAbility(0) == UGA_Fireball::StaticClass(), TEXT("Slot 0 = Fireball"));

	// Verify the ability was granted to ASC
	FGameplayAbilitySpec* FireballSpec = ASC->FindAbilitySpecFromClass(UGA_Fireball::StaticClass());
	bAllPassed &= Check(FireballSpec != nullptr, TEXT("Fireball granted in ASC"));

	// --- Step 5: Try activating the slot ---
	UE_LOG(LogTemp, Display, TEXT("  Step 5: Try activating hotbar slot"));
	bool bActivated = Player->TryActivateAbilitySlot(0);
	UE_LOG(LogTemp, Display, TEXT("    TryActivateAbilitySlot(0) = %s (headless may fail — OK)"),
		bActivated ? TEXT("true") : TEXT("false"));

	// --- Step 6: Upgrade the ability ---
	UE_LOG(LogTemp, Display, TEXT("  Step 6: Upgrade Fireball"));
	if (UnlockComp->GetSkillPoints() > 0)
	{
		bAllPassed &= Check(UnlockComp->UpgradeAbility(FName("Fireball")), TEXT("Upgraded Fireball to rank 2"));
		bAllPassed &= Check(UnlockComp->GetAbilityRank(FName("Fireball")) == 2, TEXT("Fireball rank == 2"));

		FireballSpec = ASC->FindAbilitySpecFromClass(UGA_Fireball::StaticClass());
		if (FireballSpec)
		{
			bAllPassed &= Check(FireballSpec->Level == 2, TEXT("ASC Fireball spec level == 2"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("    Skipping upgrade — no skill points remaining"));
	}

	// --- Summary ---
	UE_LOG(LogTemp, Display, TEXT(""));
	UE_LOG(LogTemp, Display, TEXT("  Full Loop Summary:"));
	UE_LOG(LogTemp, Display, TEXT("    Player Level: %d"), Player->GetPlayerLevel());
	UE_LOG(LogTemp, Display, TEXT("    XP: %.0f / %.0f"), Player->GetExperience(), Player->GetExperienceToNextLevel());
	UE_LOG(LogTemp, Display, TEXT("    Allocated: STR=%d DEX=%d INT=%d"),
		Player->GetAllocatedStrength(), Player->GetAllocatedDexterity(), Player->GetAllocatedIntelligence());
	UE_LOG(LogTemp, Display, TEXT("    Unspent Attr Pts: %d"), Player->GetUnspentAttributePoints());
	UE_LOG(LogTemp, Display, TEXT("    Skill Points: %d"), UnlockComp->GetSkillPoints());
	UE_LOG(LogTemp, Display, TEXT("    Learned Abilities: %d"), UnlockComp->GetLearnedAbilityRowNames().Num());
	UE_LOG(LogTemp, Display, TEXT("    Loadout Slots Used: %d"), Player->GetLoadout().Num());
	if (Attribs)
	{
		UE_LOG(LogTemp, Display, TEXT("    AttackPower: %.1f | CritChance: %.3f | MaxMana: %.0f"),
			Attribs->GetAttackPower(), Attribs->GetCriticalChance(), Attribs->GetMaxMana());
	}

	DestroyTestWorld(Ctx);
	return bAllPassed;
}

// =========================================================================
// Main
// =========================================================================
int32 UProgressionTestCommandlet::Main(const FString& Params)
{
	UE_LOG(LogTemp, Display, TEXT(""));
	UE_LOG(LogTemp, Display, TEXT("================================================================"));
	UE_LOG(LogTemp, Display, TEXT("  PoF Progression Test Suite"));
	UE_LOG(LogTemp, Display, TEXT("================================================================"));

	TestXPAndLevelUp();
	TestAttributeAllocation();
	TestAbilityUnlock();
	TestAbilityLoadout();
	TestMultiLevelXP();
	TestFullLoop();

	UE_LOG(LogTemp, Display, TEXT(""));
	UE_LOG(LogTemp, Display, TEXT("================================================================"));
	UE_LOG(LogTemp, Display, TEXT("  Results: %d PASSED, %d FAILED"), TestsPassed, TestsFailed);
	UE_LOG(LogTemp, Display, TEXT("================================================================"));

	if (TestsFailed > 0)
	{
		UE_LOG(LogTemp, Error, TEXT("  PROGRESSION TEST SUITE FAILED"));
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("  ALL TESTS PASSED"));
	return 0;
}
