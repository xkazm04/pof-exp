#if WITH_GAMEPLAY_DEBUGGER

#include "AI/Debug/GameplayDebuggerCategory_ARPG.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "Character/ARPGEnemyCharacter.h"
#include "AI/ARPGAIController.h"
#include "GameplayTagContainer.h"

FGameplayDebuggerCategory_ARPG::FGameplayDebuggerCategory_ARPG()
{
	SetDataPackReplication<FRepData>(&DataPack);
	CollectDataInterval = 0.2f;
}

TSharedRef<FGameplayDebuggerCategory> FGameplayDebuggerCategory_ARPG::MakeInstance()
{
	return MakeShareable(new FGameplayDebuggerCategory_ARPG());
}

void FGameplayDebuggerCategory_ARPG::FRepData::Serialize(FArchive& Ar)
{
	Ar << ArchetypeName;
	Ar << ActiveBTTask;
	Ar << TargetName;
	Ar << DistanceToTarget;
	Ar << bHasLOS;
	Ar << HealthPercent;
	Ar << AttackRange;
	Ar << AttackCooldown;
	Ar << ActiveGASTags;
	Ar << bHasAttackToken;
	Ar << SquadMemberCount;
	Ar << AIState;
}

void FGameplayDebuggerCategory_ARPG::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
	if (!DebugActor) return;

	const AARPGEnemyCharacter* Enemy = Cast<AARPGEnemyCharacter>(DebugActor);
	if (!Enemy) return;

	// --- Archetype ---
	const UEnum* ArchEnum = StaticEnum<EEnemyArchetype>();
	DataPack.ArchetypeName = ArchEnum
		? ArchEnum->GetDisplayNameTextByValue(static_cast<int64>(Enemy->GetArchetype())).ToString()
		: TEXT("Unknown");

	// --- Health ---
	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(const_cast<AARPGEnemyCharacter*>(Enemy)))
	{
		if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
		{
			const UARPGAttributeSet* Attrs = ASC->GetSet<UARPGAttributeSet>();
			if (Attrs && Attrs->GetMaxHealth() > 0.f)
			{
				DataPack.HealthPercent = (Attrs->GetHealth() / Attrs->GetMaxHealth()) * 100.f;
			}

			// --- GAS Tags ---
			FGameplayTagContainer OwnedTags;
			ASC->GetOwnedGameplayTags(OwnedTags);
			DataPack.ActiveGASTags = OwnedTags.ToStringSimple();
		}
	}

	// --- AI Controller data ---
	AARPGAIController* AIC = Cast<AARPGAIController>(Enemy->GetController());
	if (!AIC) return;

	// Blackboard
	UBlackboardComponent* BBComp = AIC->GetBlackboardComponent();
	if (BBComp)
	{
		AActor* Target = Cast<AActor>(BBComp->GetValueAsObject(AARPGAIController::KEY_TargetActor));
		DataPack.TargetName = Target ? Target->GetName() : TEXT("None");
		DataPack.DistanceToTarget = BBComp->GetValueAsFloat(AARPGAIController::KEY_DistanceToTarget);
		DataPack.bHasLOS = BBComp->GetValueAsBool(AARPGAIController::KEY_HasLineOfSight);
		DataPack.AttackRange = BBComp->GetValueAsFloat(AARPGAIController::KEY_AttackRange);
		DataPack.AttackCooldown = BBComp->GetValueAsFloat(AARPGAIController::KEY_AttackCooldown);
	}

	// --- Active BT task ---
	UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(AIC->GetBrainComponent());
	if (BTComp)
	{
		const FString BTDesc = BTComp->DescribeActiveTasks();
		DataPack.ActiveBTTask = BTDesc.Len() > 0 ? BTDesc : TEXT("None");
	}

	// --- Determine AI state ---
	if (Enemy->IsDead())
	{
		DataPack.AIState = TEXT("Dead");
	}
	else if (DataPack.TargetName != TEXT("None"))
	{
		DataPack.AIState = DataPack.bHasLOS ? TEXT("Combat") : TEXT("Searching");
	}
	else
	{
		DataPack.AIState = TEXT("Idle/Patrol");
	}
}

void FGameplayDebuggerCategory_ARPG::DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext)
{
	CanvasContext.Printf(TEXT("{yellow}=== ARPG AI Debug ==="));
	CanvasContext.Printf(TEXT("{white}State: {green}%s"), *DataPack.AIState);
	CanvasContext.Printf(TEXT("{white}Archetype: {cyan}%s"), *DataPack.ArchetypeName);
	CanvasContext.Printf(TEXT("{white}Health: %s%.0f%%"),
		DataPack.HealthPercent > 50.f ? TEXT("{green}") : (DataPack.HealthPercent > 25.f ? TEXT("{yellow}") : TEXT("{red}")),
		DataPack.HealthPercent);
	CanvasContext.Printf(TEXT(""));
	CanvasContext.Printf(TEXT("{yellow}--- Combat ---"));
	CanvasContext.Printf(TEXT("{white}Target: {cyan}%s"), *DataPack.TargetName);
	CanvasContext.Printf(TEXT("{white}Distance: {cyan}%.0f {white}| LOS: %s"),
		DataPack.DistanceToTarget,
		DataPack.bHasLOS ? TEXT("{green}YES") : TEXT("{red}NO"));
	CanvasContext.Printf(TEXT("{white}AttackRange: {cyan}%.0f {white}| Cooldown: {cyan}%.1fs"),
		DataPack.AttackRange, DataPack.AttackCooldown);
	CanvasContext.Printf(TEXT(""));
	CanvasContext.Printf(TEXT("{yellow}--- Behavior Tree ---"));
	CanvasContext.Printf(TEXT("{white}Active: {cyan}%s"), *DataPack.ActiveBTTask);
	CanvasContext.Printf(TEXT(""));
	CanvasContext.Printf(TEXT("{yellow}--- GAS Tags ---"));
	CanvasContext.Printf(TEXT("{white}%s"),
		DataPack.ActiveGASTags.Len() > 0 ? *DataPack.ActiveGASTags : TEXT("(none)"));
}

#endif // WITH_GAMEPLAY_DEBUGGER
