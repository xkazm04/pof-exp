#include "Test/ARPGFunctionalTestBase.h"
#include "Player/ARPGPlayerCharacter.h"
#include "Character/ARPGEnemyCharacter.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystem/Effects/GE_Damage.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

AARPGFunctionalTestBase::AARPGFunctionalTestBase()
{
	PrimaryActorTick.bCanEverTick = true;
	TimeLimit = 30.f;

	// Gray-box slice: empty montages / no skeletal mesh produce incidental anim
	// *warnings* that are not failures. Ignore warning-level log output so it
	// can't fail the run; Error-level output still does.
	LogWarningHandling = EFunctionalTestLogHandling::OutputIgnored;
}

void AARPGFunctionalTestBase::StartTest()
{
	Super::StartTest();

	Player = GetPlayerCharacter();
	Enemy = GetFirstEnemy();

	if (!Player.IsValid())
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("Player character missing in the slice level"));
		return;
	}

	CurrentPhase = 0;
	PhaseTime = 0.f;
	OnTestStarted();
}

void AARPGFunctionalTestBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsRunning())
	{
		return;
	}

	if (CurrentPhase >= Phases.Num())
	{
		FinishTest(EFunctionalTestResult::Default, TEXT("all phases passed"));
		return;
	}

	PhaseTime += DeltaSeconds;

	const FName PhaseName = Phases[CurrentPhase];
	const EARPGPhaseResult PhaseOutcome = RunPhase(CurrentPhase, PhaseName, DeltaSeconds);

	if (PhaseOutcome == EARPGPhaseResult::Fail)
	{
		FinishTest(EFunctionalTestResult::Failed,
			FString::Printf(TEXT("phase '%s' (%d) failed"), *PhaseName.ToString(), CurrentPhase));
		return;
	}

	if (PhaseOutcome == EARPGPhaseResult::Advance)
	{
		++CurrentPhase;
		PhaseTime = 0.f;
		if (CurrentPhase >= Phases.Num())
		{
			FinishTest(EFunctionalTestResult::Default, TEXT("all phases passed"));
		}
		return;
	}

	// Running — enforce the optional per-phase timeout.
	if (PhaseTimeout > 0.f && PhaseTime > PhaseTimeout)
	{
		FinishTest(EFunctionalTestResult::Failed,
			FString::Printf(TEXT("phase '%s' (%d) exceeded %.1fs timeout"), *PhaseName.ToString(), CurrentPhase, PhaseTimeout));
	}
}

EARPGPhaseResult AARPGFunctionalTestBase::RunPhase(int32 /*PhaseIndex*/, FName /*PhaseName*/, float /*DeltaSeconds*/)
{
	return EARPGPhaseResult::Advance;
}

AARPGPlayerCharacter* AARPGFunctionalTestBase::GetPlayerCharacter() const
{
	if (Player.IsValid())
	{
		return Player.Get();
	}
	return Cast<AARPGPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
}

AARPGEnemyCharacter* AARPGFunctionalTestBase::GetFirstEnemy() const
{
	if (Enemy.IsValid())
	{
		return Enemy.Get();
	}
	if (const UWorld* World = GetWorld())
	{
		for (TActorIterator<AARPGEnemyCharacter> It(World); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}

UAbilitySystemComponent* AARPGFunctionalTestBase::GetPlayerASC() const
{
	AARPGPlayerCharacter* P = GetPlayerCharacter();
	return P ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(P) : nullptr;
}

UAbilitySystemComponent* AARPGFunctionalTestBase::GetEnemyASC() const
{
	AARPGEnemyCharacter* E = GetFirstEnemy();
	return E ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(E) : nullptr;
}

void AARPGFunctionalTestBase::ApplyDamage(AActor* Target, float Amount)
{
	UAbilitySystemComponent* SourceASC = GetPlayerASC();
	UAbilitySystemComponent* TargetASC = Target ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target) : nullptr;
	if (!SourceASC || !TargetASC)
	{
		return;
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(GetPlayerCharacter());

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(UGE_Damage::StaticClass(), 1.f, Context);
	if (SpecHandle.IsValid())
	{
		FGameplayEffectSpec* Spec = SpecHandle.Data.Get();
		Spec->SetSetByCallerMagnitude(ARPGGameplayTags::Data_Damage_Base, Amount);
		Spec->AddDynamicAssetTag(ARPGGameplayTags::Damage_Physical);
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec, TargetASC);
	}
}

float AARPGFunctionalTestBase::GetHealth(AActor* Actor) const
{
	UAbilitySystemComponent* ASC = Actor ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor) : nullptr;
	return ASC ? ASC->GetNumericAttribute(UARPGAttributeSet::GetHealthAttribute()) : 0.f;
}

EARPGWait AARPGFunctionalTestBase::WaitForCondition(TFunctionRef<bool()> Predicate, float TimeoutSeconds) const
{
	if (Predicate())
	{
		return EARPGWait::Satisfied;
	}
	if (PhaseTime >= TimeoutSeconds)
	{
		return EARPGWait::TimedOut;
	}
	return EARPGWait::Pending;
}
