#include "Character/ARPGBossCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"

const FName AARPGBossCharacter::KEY_BossPhase = TEXT("BossPhase");

AARPGBossCharacter::AARPGBossCharacter()
{
	// Boss defaults
	EnemyDisplayName = FText::FromString(TEXT("Boss"));
	AttackCooldown = 1.5f;
}

void AARPGBossCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Listen for health changes to drive phase transitions
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC)
	{
		HealthChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(
			UARPGAttributeSet::GetHealthAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				if (!bInPhaseTransition)
				{
					CheckPhaseTransition();
				}
			});
	}

	// Start at phase -1 (no phase). First check will enter phase 0 or appropriate phase.
	CurrentPhaseIndex = -1;
	WritePhaseToBB();

	// Enter phase 0 if phases exist
	if (Phases.Num() > 0)
	{
		EnterPhase(0);
	}
}

float AARPGBossCharacter::GetHealthPercentage() const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return 1.f;

	bool bFound = false;
	float Health = ASC->GetGameplayAttributeValue(UARPGAttributeSet::GetHealthAttribute(), bFound);
	float MaxHealth = ASC->GetGameplayAttributeValue(UARPGAttributeSet::GetMaxHealthAttribute(), bFound);

	return MaxHealth > 0.f ? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 0.f;
}

void AARPGBossCharacter::CheckPhaseTransition()
{
	if (Phases.Num() == 0 || IsDead()) return;

	const float HealthPct = GetHealthPercentage();
	int32 TargetPhase = CurrentPhaseIndex;

	// Find the highest phase index whose threshold is above current health
	for (int32 i = Phases.Num() - 1; i > CurrentPhaseIndex; --i)
	{
		if (HealthPct <= Phases[i].HealthThreshold)
		{
			TargetPhase = i;
			break;
		}
	}

	if (TargetPhase > CurrentPhaseIndex)
	{
		EnterPhase(TargetPhase);
	}
}

void AARPGBossCharacter::ForcePhaseTransition(int32 PhaseIndex)
{
	if (Phases.IsValidIndex(PhaseIndex) && PhaseIndex != CurrentPhaseIndex)
	{
		EnterPhase(PhaseIndex);
	}
}

void AARPGBossCharacter::EnterPhase(int32 PhaseIndex)
{
	if (!Phases.IsValidIndex(PhaseIndex)) return;

	bInPhaseTransition = true;
	CurrentPhaseIndex = PhaseIndex;
	const FBossPhaseConfig& Phase = Phases[PhaseIndex];

	UE_LOG(LogTemp, Log, TEXT("[Boss] %s entering phase %d/%d (%s)"),
		*BossName.ToString(), PhaseIndex + 1, Phases.Num(),
		*Phase.PhaseName.ToString());

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();

	// Apply invulnerability during transition
	if (bInvulnerableDuringTransition && ASC)
	{
		ASC->AddLooseGameplayTag(ARPGGameplayTags::State_Invulnerable);
	}

	// Play transition montage
	if (Phase.PhaseTransitionMontage)
	{
		PlayAnimMontage(Phase.PhaseTransitionMontage);
	}

	// Grant phase abilities
	if (ASC)
	{
		for (const TSubclassOf<UGameplayAbility>& AbilityClass : Phase.PhaseAbilities)
		{
			if (AbilityClass)
			{
				ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
			}
		}

		// Apply phase buff
		if (Phase.PhaseBuffEffect)
		{
			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			Context.AddSourceObject(this);
			ASC->ApplyGameplayEffectToSelf(Phase.PhaseBuffEffect.GetDefaultObject(), 1.f, Context);
		}
	}

	// Override attack cooldown
	if (Phase.AttackCooldownOverride > 0.f)
	{
		AttackCooldown = Phase.AttackCooldownOverride;
	}

	WritePhaseToBB();

	OnBossPhaseChanged.Broadcast(PhaseIndex, Phases.Num());

	// End transition after duration
	if (bInvulnerableDuringTransition)
	{
		GetWorldTimerManager().SetTimer(
			TransitionTimerHandle,
			this, &AARPGBossCharacter::OnPhaseTransitionEnd,
			TransitionInvulnDuration,
			false);
	}
	else
	{
		bInPhaseTransition = false;
	}
}

void AARPGBossCharacter::OnPhaseTransitionEnd()
{
	bInPhaseTransition = false;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC)
	{
		ASC->RemoveLooseGameplayTag(ARPGGameplayTags::State_Invulnerable);
	}

	UE_LOG(LogTemp, Log, TEXT("[Boss] %s phase transition complete"), *BossName.ToString());
}

void AARPGBossCharacter::WritePhaseToBB()
{
	AAIController* AIC = Cast<AAIController>(GetController());
	if (!AIC) return;

	UBlackboardComponent* BB = AIC->GetBlackboardComponent();
	if (BB)
	{
		BB->SetValueAsInt(KEY_BossPhase, CurrentPhaseIndex);
	}
}
