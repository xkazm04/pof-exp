#include "Character/ARPGEnemyCharacter.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AI/ARPGAIController.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystem/Effects/GE_AwardXP.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "UI/EnemyHealthBarWidget.h"
#include "Loot/ARPGLootDropComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

AARPGEnemyCharacter::AARPGEnemyCharacter()
{
	// AI Controller
	AIControllerClass = AARPGAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Default enemy attribute row
	AttributeInitRowName = TEXT("Skeleton");

	// Loot drop component — auto-binds to OnEnemyDeath in BeginPlay
	LootDropComponent = CreateDefaultSubobject<UARPGLootDropComponent>(TEXT("LootDropComponent"));

	// Floating health bar
	HealthBarWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidget"));
	HealthBarWidgetComp->SetupAttachment(RootComponent);
	HealthBarWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	HealthBarWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarWidgetComp->SetDrawAtDesiredSize(true);
	HealthBarWidgetComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Widget class assigned in Blueprint via SetWidgetClass
}

void AARPGEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	// --- Sith lightsaber: turn the enemy's hand weapon into a glowing RED blade ---
	if (WeaponMesh)
	{
		if (UStaticMesh* Blade = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")))
		{
			WeaponMesh->SetStaticMesh(Blade);
		}
		if (UMaterialInterface* SaberMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/FX/M_Saber_Red.M_Saber_Red")))
		{
			WeaponMesh->SetMaterial(0, SaberMat);
		}
		WeaponMesh->SetRelativeScale3D(FVector(0.04f, 0.04f, 1.1f));
		WeaponMesh->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
		WeaponMesh->SetRelativeLocation(FVector(55.f, 0.f, 0.f));
		WeaponMesh->SetHiddenInGame(false);
		WeaponMesh->SetVisibility(true);
	}

	// --- Sith body: override the placeholder flat-red blob material so the enemy reads as a
	// dark-robed duelist (a believable Sith stand-in) instead of a glowing red shape. ---
	if (USkeletalMeshComponent* Body = GetMesh())
	{
		if (UMaterialInterface* SithMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/FX/M_Sith_Body.M_Sith_Body")))
		{
			const int32 NumMats = Body->GetNumMaterials();
			for (int32 i = 0; i < NumMats; ++i)
			{
				Body->SetMaterial(i, SithMat);
			}
		}
	}
}

void AARPGEnemyCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AbilitySystemComponent)
	{
		// For AI characters, both owner and avatar should be 'this' character
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	ApplyArchetypeDefaults();
	GrantAbilitiesToASC();
	WriteArchetypeToBlackboard();
	InitializeHealthBarWidget();
}

void AARPGEnemyCharacter::OnDeathFromAbility(AActor* KillingActor)
{
	if (!bIsAlive) return;

	bIsAlive = false;

	HandleDeathCleanup();

	// Fade out health bar
	if (HealthBarWidgetComp)
	{
		if (UEnemyHealthBarWidget* BarWidget = Cast<UEnemyHealthBarWidget>(HealthBarWidgetComp->GetWidget()))
		{
			BarWidget->HideForDeath();
		}
		else
		{
			HealthBarWidgetComp->SetVisibility(false);
		}
	}

	// --- Award XP to the killer ---
	if (KillingActor)
	{
		// Resolve the killer's ASC (could be the pawn or a controller that implements IAbilitySystemInterface)
		UAbilitySystemComponent* KillerASC = nullptr;
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(KillingActor))
		{
			KillerASC = ASI->GetAbilitySystemComponent();
		}
		// If the instigator is a controller, try the pawn
		if (!KillerASC)
		{
			if (AController* PC = Cast<AController>(KillingActor))
			{
				if (APawn* KillerPawn = PC->GetPawn())
				{
					if (IAbilitySystemInterface* PawnASI = Cast<IAbilitySystemInterface>(KillerPawn))
					{
						KillerASC = PawnASI->GetAbilitySystemComponent();
					}
				}
			}
		}

		if (KillerASC)
		{
			// Get killer's level from their attribute set for XP scaling
			int32 KillerLevel = 1;
			const UARPGAttributeSet* KillerAttribs = KillerASC->GetSet<UARPGAttributeSet>();
			if (KillerAttribs)
			{
				KillerLevel = FMath::RoundToInt32(KillerAttribs->GetCharacterLevel());
			}

			const float XPAmount = CalculateXPReward(KillerLevel);

			// Apply GE_AwardXP to the killer
			FGameplayEffectContextHandle Context = KillerASC->MakeEffectContext();
			Context.AddSourceObject(this);
			FGameplayEffectSpecHandle XPSpec = KillerASC->MakeOutgoingSpec(
				UGE_AwardXP::StaticClass(), 1.f, Context);
			if (XPSpec.IsValid())
			{
				XPSpec.Data->SetSetByCallerMagnitude(ARPGGameplayTags::Data_XP_Amount, XPAmount);
				KillerASC->ApplyGameplayEffectSpecToSelf(*XPSpec.Data.Get());
			}

			// Send Event.EnemyKilled to the killer's ASC
			FGameplayEventData KillPayload;
			KillPayload.EventTag = ARPGGameplayTags::Event_EnemyKilled;
			KillPayload.Instigator = KillingActor;
			KillPayload.Target = this;
			KillPayload.EventMagnitude = XPAmount;
			KillerASC->HandleGameplayEvent(ARPGGameplayTags::Event_EnemyKilled, &KillPayload);

			UE_LOG(LogTemp, Log, TEXT("[Enemy Death] %s killed by %s — awarded %.0f XP (EnemyLv=%d, KillerLv=%d)"),
				*GetName(), *KillingActor->GetName(), XPAmount, CharacterLevel, KillerLevel);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[Enemy Death] %s killed by %s — no ASC found on killer, no XP awarded"),
				*GetName(), *KillingActor->GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[Enemy Death] %s killed (no instigator — no XP awarded)"), *GetName());
	}

	// Broadcast for loot, quest hooks
	OnEnemyDeath.Broadcast(this);
}

float AARPGEnemyCharacter::GetBaseXPReward() const
{
	return GetArchetypeDefaults(Archetype).BaseXPReward;
}

float AARPGEnemyCharacter::CalculateXPReward(int32 KillerLevel) const
{
	const float BaseXP = GetBaseXPReward();

	// Scale by enemy level: higher-level enemies give more XP
	const float LevelScale = static_cast<float>(CharacterLevel);

	// Level difference bonus/penalty:
	// +50% if enemy is 5+ levels above killer, -50% if 5+ levels below, linear between
	const int32 LevelDiff = CharacterLevel - KillerLevel;
	const float DiffMultiplier = FMath::Clamp(1.f + LevelDiff * 0.1f, 0.1f, 2.0f);

	return FMath::Max(1.f, FMath::RoundToFloat(BaseXP * LevelScale * DiffMultiplier));
}

void AARPGEnemyCharacter::HandleDeathCleanup()
{
	// --- Disable collision ---
	// Drop the capsule to NoCollision so the corpse no longer blocks players,
	// projectiles, or navigation. The skeletal mesh keeps its collision so
	// GA_Death can still enable ragdoll afterwards.
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// --- Stop movement ---
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}

	// --- Stop AI ---
	StopBehaviorTree();
}

void AARPGEnemyCharacter::StopBehaviorTree()
{
	AARPGAIController* AIC = Cast<AARPGAIController>(GetController());
	if (!AIC) return;

	UBrainComponent* BrainComp = AIC->GetBrainComponent();
	if (UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(BrainComp))
	{
		BTComp->StopTree(EBTStopMode::Forced);
	}
	else if (BrainComp)
	{
		// Fallback for non-BT brains (StateTree, custom).
		BrainComp->StopLogic(TEXT("Enemy died"));
	}

	// Drop focus so the dead enemy stops rotating toward its target.
	AIC->ClearFocus(EAIFocusPriority::Gameplay);
}

FEnemyArchetypeDefaults AARPGEnemyCharacter::GetArchetypeDefaults(EEnemyArchetype InArchetype)
{
	FEnemyArchetypeDefaults D;
	switch (InArchetype)
	{
	case EEnemyArchetype::MeleeGrunt:
		D.AttackRange = 200.f;
		D.AttackCooldown = 2.0f;
		D.PrimaryAbilityTag = ARPGGameplayTags::Ability_Melee_LightAttack;
		D.LootNumRolls = 1;
		D.LootRarityBonusMultiplier = 1.f;
		D.BaseXPReward = 10.f;
		break;

	case EEnemyArchetype::RangedCaster:
		D.AttackRange = 800.f;
		D.PreferredCombatDistance = 800.f;
		D.RetreatDistance = 400.f;
		D.AttackCooldown = 3.0f;
		D.PrimaryAbilityTag = ARPGGameplayTags::Ability_Ranged_Fireball;
		D.LootNumRolls = 1;
		D.LootRarityBonusMultiplier = 1.2f;
		D.BaseXPReward = 10.f;
		break;

	case EEnemyArchetype::Brute:
		D.AttackRange = 250.f;
		D.AttackCooldown = 4.0f;
		D.PrimaryAbilityTag = ARPGGameplayTags::Ability_Melee_HeavyAttack;
		D.ChargeVulnerabilityDuration = 2.0f;
		D.ChargeSpeedMultiplier = 3.0f;
		D.LootNumRolls = 2;
		D.LootRarityBonusMultiplier = 2.f;
		D.BaseXPReward = 25.f;
		break;
	}
	return D;
}

void AARPGEnemyCharacter::ApplyArchetypeDefaults()
{
	const FEnemyArchetypeDefaults D = GetArchetypeDefaults(Archetype);

	AttackRange = D.AttackRange;
	PreferredCombatDistance = D.PreferredCombatDistance;
	RetreatDistance = D.RetreatDistance;
	AttackCooldown = D.AttackCooldown;
	PrimaryAbilityTag = D.PrimaryAbilityTag;
	ChargeVulnerabilityDuration = D.ChargeVulnerabilityDuration;
	ChargeSpeedMultiplier = D.ChargeSpeedMultiplier;
	if (LootDropComponent)
	{
		LootDropComponent->NumRolls = D.LootNumRolls;
		LootDropComponent->RarityBonusMultiplier = D.LootRarityBonusMultiplier;
	}

	UE_LOG(LogTemp, Log, TEXT("[Archetype] %s set to %d (AttackRange=%.0f, Cooldown=%.1f)"),
		*GetName(), static_cast<uint8>(Archetype), AttackRange, AttackCooldown);
}

void AARPGEnemyCharacter::GrantAbilitiesToASC()
{
	if (!AbilitySystemComponent) return;

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : GrantedAbilities)
	{
		if (AbilityClass)
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, CharacterLevel));
		}
	}
}

void AARPGEnemyCharacter::WriteArchetypeToBlackboard()
{
	AARPGAIController* AIC = Cast<AARPGAIController>(GetController());
	if (!AIC) return;

	UBlackboardComponent* BBComp = AIC->GetBlackboardComponent();
	if (!BBComp) return;

	BBComp->SetValueAsInt(AARPGAIController::KEY_Archetype, static_cast<int32>(Archetype));
	BBComp->SetValueAsFloat(AARPGAIController::KEY_AttackRange, AttackRange);
	BBComp->SetValueAsFloat(AARPGAIController::KEY_PreferredDistance, PreferredCombatDistance);
	BBComp->SetValueAsFloat(AARPGAIController::KEY_RetreatDistance, RetreatDistance);
	BBComp->SetValueAsFloat(AARPGAIController::KEY_AttackCooldown, AttackCooldown);
}

void AARPGEnemyCharacter::InitializeHealthBarWidget()
{
	if (!HealthBarWidgetComp || !HealthBarWidgetClass) return;

	HealthBarWidgetComp->SetWidgetClass(HealthBarWidgetClass);
	HealthBarWidgetComp->InitWidget();

	UEnemyHealthBarWidget* BarWidget = Cast<UEnemyHealthBarWidget>(HealthBarWidgetComp->GetWidget());
	if (!BarWidget) return;

	BarWidget->SetEnemyInfo(EnemyDisplayName, CharacterLevel);

	if (AbilitySystemComponent)
	{
		BarWidget->BindToAbilitySystem(AbilitySystemComponent);
	}

	// Color the level text based on level difference with the player
	if (UWorld* World = GetWorld())
	{
		APlayerController* PC = World->GetFirstPlayerController();
		if (PC)
		{
			if (AARPGCharacterBase* PlayerChar = Cast<AARPGCharacterBase>(PC->GetPawn()))
			{
				// Read player level from their ASC attribute set
				int32 PlayerLevel = 1;
				if (UAbilitySystemComponent* PlayerASC = PlayerChar->GetAbilitySystemComponent())
				{
					if (const UARPGAttributeSet* Attrs = PlayerASC->GetSet<UARPGAttributeSet>())
					{
						PlayerLevel = FMath::RoundToInt32(Attrs->GetCharacterLevel());
					}
				}
				BarWidget->UpdateLevelColor(PlayerLevel);
			}
		}
	}
}
