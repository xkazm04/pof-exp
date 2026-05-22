#include "Combat/ARPGCombatTestDummy.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystem/GA_HitReact.h"
#include "AbilitySystem/GA_Death.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UI/EnemyHealthBarWidget.h"

AARPGCombatTestDummy::AARPGCombatTestDummy()
{
	// No AI needed
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Default attribute row (uses generic enemy stats)
	AttributeInitRowName = TEXT("Skeleton");

	// Floating health bar
	HealthBarWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidget"));
	HealthBarWidgetComp->SetupAttachment(RootComponent);
	HealthBarWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	HealthBarWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarWidgetComp->SetDrawAtDesiredSize(true);
	HealthBarWidgetComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AARPGCombatTestDummy::BeginPlay()
{
	Super::BeginPlay();

	// Remember spawn transform for respawning
	SpawnLocation = GetActorLocation();
	SpawnRotation = GetActorRotation();
}

void AARPGCombatTestDummy::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AbilitySystemComponent)
	{
		// For non-player characters, both owner and avatar should be 'this'
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	GrantAbilities();
	InitializeHealthBarWidget();

	// Disable movement — test dummy stands still
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->DisableMovement();
	}
}

void AARPGCombatTestDummy::OnDeathFromAbility(AActor* KillingActor)
{
	if (!bIsAlive) return;

	bIsAlive = false;

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

	UE_LOG(LogTemp, Warning, TEXT("[TestDummy] %s has died. Killer: %s"),
		*GetName(),
		KillingActor ? *KillingActor->GetName() : TEXT("None"));

	// Schedule respawn if configured
	if (bAutoRespawn)
	{
		GetWorldTimerManager().SetTimer(
			RespawnTimerHandle,
			this,
			&AARPGCombatTestDummy::ExecuteRespawn,
			RespawnDelay,
			false);
	}
}

void AARPGCombatTestDummy::GrantAbilities()
{
	if (!AbilitySystemComponent) return;

	// Grant hit react and death abilities so the dummy responds to damage
	AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UGA_HitReact::StaticClass(), 1));
	AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UGA_Death::StaticClass(), 1));
}

void AARPGCombatTestDummy::InitializeHealthBarWidget()
{
	if (!HealthBarWidgetComp || !HealthBarWidgetClass) return;

	HealthBarWidgetComp->SetWidgetClass(HealthBarWidgetClass);
	HealthBarWidgetComp->InitWidget();

	UEnemyHealthBarWidget* BarWidget = Cast<UEnemyHealthBarWidget>(HealthBarWidgetComp->GetWidget());
	if (!BarWidget) return;

	BarWidget->SetEnemyInfo(DisplayName, CharacterLevel);

	if (AbilitySystemComponent)
	{
		BarWidget->BindToAbilitySystem(AbilitySystemComponent);
	}
}

void AARPGCombatTestDummy::ExecuteRespawn()
{
	bIsAlive = true;

	// Reset position and rotation
	SetActorLocation(SpawnLocation);
	SetActorRotation(SpawnRotation);

	// Re-enable collision (GA_Death disables it)
	SetActorEnableCollision(true);

	// Reset mesh state (undo ragdoll)
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetAllBodiesSimulatePhysics(false);
		MeshComp->SetSimulatePhysics(false);
		MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MeshComp->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		MeshComp->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), FRotator::ZeroRotator);
	}

	// Cancel the death ability so State.Dead is removed
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->CancelAllAbilities();

		// Re-grant abilities (they may have been removed during death)
		GrantAbilities();

		// Re-initialize attributes to full health
		InitializeAttributes();
	}

	// Re-initialize health bar
	if (HealthBarWidgetComp)
	{
		HealthBarWidgetComp->SetVisibility(true);
		if (UEnemyHealthBarWidget* BarWidget = Cast<UEnemyHealthBarWidget>(HealthBarWidgetComp->GetWidget()))
		{
			BarWidget->SetEnemyInfo(DisplayName, CharacterLevel);
			if (AbilitySystemComponent)
			{
				BarWidget->BindToAbilitySystem(AbilitySystemComponent);
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[TestDummy] %s has respawned."), *GetName());
}
