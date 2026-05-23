#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Player/ARPGPlayerCharacter.h"
#include "Character/ARPGEnemyCharacter.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystem/Effects/GE_Damage.h"

// ARPG.Verify.<System> console commands — ad-hoc, operator-runnable system
// checks usable live in PIE *and* in a cooked Shipping build (FAutoConsoleCommand
// survives shipping; UCheatManager Exec does not). The cooked smoke-pak runs
// `-ExecCmds="ARPG.Verify.Slice"` to self-check the gameplay loop in the
// packaged exe — bridging SP-E's process-level check and the PIE functional test.
//
// Every command logs a single discriminating result line:
//   "ARPG.Verify.<System>: PASS"  or  "ARPG.Verify.<System>: FAIL (<reason>)"
// so a harness can grep the log regardless of process exit code (lesson: prove
// by the log line, don't trust the exit code).

DEFINE_LOG_CATEGORY_STATIC(LogARPGVerify, Log, All);

namespace ARPGVerify
{
	/** Accumulates per-check results and emits the single PASS/FAIL summary line. */
	struct FReport
	{
		const TCHAR* System;
		bool bOk = true;
		FString FirstFailure;

		explicit FReport(const TCHAR* InSystem) : System(InSystem) {}

		void Check(bool bCondition, const FString& FailReason)
		{
			if (!bCondition && bOk)
			{
				bOk = false;
				FirstFailure = FailReason;
			}
		}

		bool Finish() const
		{
			if (bOk)
			{
				UE_LOG(LogARPGVerify, Display, TEXT("ARPG.Verify.%s: PASS"), System);
			}
			else
			{
				UE_LOG(LogARPGVerify, Error, TEXT("ARPG.Verify.%s: FAIL (%s)"), System, *FirstFailure);
			}
			return bOk;
		}
	};

	static AARPGPlayerCharacter* FindPlayer(UWorld* World)
	{
		return Cast<AARPGPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
	}

	static AARPGEnemyCharacter* FindEnemy(UWorld* World)
	{
		for (TActorIterator<AARPGEnemyCharacter> It(World); It; ++It)
		{
			return *It;
		}
		return nullptr;
	}

	/** Apply real GE_Damage from source to target ASC; returns false if it couldn't. */
	static bool ApplyDamage(AActor* Source, AActor* Target, float Amount)
	{
		UAbilitySystemComponent* SrcASC = Source ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Source) : nullptr;
		UAbilitySystemComponent* TgtASC = Target ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target) : nullptr;
		if (!SrcASC || !TgtASC)
		{
			return false;
		}
		FGameplayEffectContextHandle Context = SrcASC->MakeEffectContext();
		Context.AddSourceObject(Source);
		FGameplayEffectSpecHandle Spec = SrcASC->MakeOutgoingSpec(UGE_Damage::StaticClass(), 1.f, Context);
		if (!Spec.IsValid())
		{
			return false;
		}
		Spec.Data->SetSetByCallerMagnitude(ARPGGameplayTags::Data_Damage_Base, Amount);
		Spec.Data->AddDynamicAssetTag(ARPGGameplayTags::Damage_Physical);
		SrcASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TgtASC);
		return true;
	}

	static float GetHealth(AActor* Actor)
	{
		UAbilitySystemComponent* ASC = Actor ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor) : nullptr;
		return ASC ? ASC->GetNumericAttribute(UARPGAttributeSet::GetHealthAttribute()) : 0.f;
	}

	// --- Verb implementations -------------------------------------------------

	static bool VerifyCharacters(UWorld* World)
	{
		FReport R(TEXT("Characters"));
		AARPGPlayerCharacter* Player = FindPlayer(World);
		AARPGEnemyCharacter* Enemy = FindEnemy(World);
		R.Check(Player != nullptr, TEXT("no player character in world"));
		R.Check(Enemy != nullptr, TEXT("no enemy character in world"));
		R.Check(Player == nullptr || Enemy == nullptr || Player->GetClass() != Enemy->GetClass(),
			TEXT("player and enemy share the same class (not distinct)"));
		return R.Finish();
	}

	static bool VerifyHUD(UWorld* World)
	{
		FReport R(TEXT("HUD"));
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		R.Check(PC != nullptr, TEXT("no player controller"));
		R.Check(PC != nullptr && PC->GetHUD() != nullptr, TEXT("player controller has no AHUD instance"));
		return R.Finish();
	}

	static bool VerifyCombat(UWorld* World)
	{
		FReport R(TEXT("Combat"));
		AARPGPlayerCharacter* Player = FindPlayer(World);
		AARPGEnemyCharacter* Enemy = FindEnemy(World);
		R.Check(Player != nullptr, TEXT("no player"));
		R.Check(Enemy != nullptr, TEXT("no enemy"));
		if (Player && Enemy)
		{
			const float Before = GetHealth(Enemy);
			const bool bApplied = ApplyDamage(Player, Enemy, 25.f);
			R.Check(bApplied, TEXT("could not build/apply GE_Damage (missing ASC?)"));
			const float After = GetHealth(Enemy);
			R.Check(After < Before,
				FString::Printf(TEXT("enemy Health did not drop (%.1f -> %.1f)"), Before, After));
		}
		return R.Finish();
	}

	/** Full slice self-check: structure + a real synchronous damage-pipeline proof. */
	static bool VerifySlice(UWorld* World)
	{
		FReport R(TEXT("Slice"));
		AARPGPlayerCharacter* Player = FindPlayer(World);
		AARPGEnemyCharacter* Enemy = FindEnemy(World);
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;

		R.Check(Player != nullptr, TEXT("no player character"));
		R.Check(Enemy != nullptr, TEXT("no enemy character"));
		R.Check(PC != nullptr && PC->GetHUD() != nullptr, TEXT("no HUD"));

		if (Player && Enemy)
		{
			const float Before = GetHealth(Enemy);
			const bool bApplied = ApplyDamage(Player, Enemy, 25.f);
			R.Check(bApplied, TEXT("damage pipeline unavailable"));
			const float After = GetHealth(Enemy);
			R.Check(After < Before,
				FString::Printf(TEXT("damage had no effect (%.1f -> %.1f)"), Before, After));
		}
		return R.Finish();
	}
}

static FAutoConsoleCommandWithWorld GVerifyCharacters(
	TEXT("ARPG.Verify.Characters"),
	TEXT("Verify the player and a distinct enemy character exist. Logs PASS/FAIL."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World) { ARPGVerify::VerifyCharacters(World); }));

static FAutoConsoleCommandWithWorld GVerifyHUD(
	TEXT("ARPG.Verify.HUD"),
	TEXT("Verify a player controller + HUD instance exist. Logs PASS/FAIL."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World) { ARPGVerify::VerifyHUD(World); }));

static FAutoConsoleCommandWithWorld GVerifyCombat(
	TEXT("ARPG.Verify.Combat"),
	TEXT("Apply real GE_Damage to an enemy and verify its Health drops. Logs PASS/FAIL."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World) { ARPGVerify::VerifyCombat(World); }));

static FAutoConsoleCommandWithWorld GVerifySlice(
	TEXT("ARPG.Verify.Slice"),
	TEXT("Full slice self-check (structure + damage pipeline). Logs ARPG.Verify.Slice: PASS/FAIL. Safe to run live in PIE."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World) { ARPGVerify::VerifySlice(World); }));

// CI / smoke-pak variant: runs the same check, then requests process exit with
// the verdict as the exit code (0 = PASS, 1 = FAIL). The exit code is the
// reliable signal in a cooked Shipping build, where logging is compiled out
// (bUseLoggingInShipping=false) so the PASS log line never appears. Do NOT use
// this one live in the editor — it quits the process.
static FAutoConsoleCommandWithWorld GVerifySliceCI(
	TEXT("ARPG.Verify.SliceCI"),
	TEXT("Run the slice self-check and exit the process with the verdict as exit code (0=PASS,1=FAIL). For the cooked smoke-pak."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		const bool bPass = ARPGVerify::VerifySlice(World);
		FPlatformMisc::RequestExitWithStatus(/*Force=*/true, bPass ? 0 : 1);
	}));
