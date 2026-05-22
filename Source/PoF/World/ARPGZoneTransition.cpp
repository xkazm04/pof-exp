#include "World/ARPGZoneTransition.h"
#include "World/ARPGZoneDefinition.h"
#include "World/ARPGZoneManager.h"
#include "Quest/ARPGQuestSubsystem.h"
#include "Framework/ARPGGameInstance.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "Kismet/GameplayStatics.h"

AARPGZoneTransition::AARPGZoneTransition()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	TriggerVolume->SetBoxExtent(FVector(200.f, 200.f, 200.f));
	TriggerVolume->SetCollisionProfileName(TEXT("Trigger"));
	TriggerVolume->SetGenerateOverlapEvents(true);
	TriggerVolume->SetupAttachment(Root);

	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));
	PortalMesh->SetupAttachment(Root);
	PortalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	DirectionArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("DirectionArrow"));
	DirectionArrow->SetupAttachment(Root);
	DirectionArrow->SetHiddenInGame(true);
}

void AARPGZoneTransition::BeginPlay()
{
	Super::BeginPlay();

	if (bTriggerOnOverlap)
	{
		TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AARPGZoneTransition::OnPlayerOverlap);
	}
	else
	{
		// Non-overlap mode: tag as interactable so player controller can find it
		Tags.AddUnique(FName("Interactable"));
	}
}

void AARPGZoneTransition::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(CooldownTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void AARPGZoneTransition::TryInteractTransition()
{
	ExecuteTransition();
}

bool AARPGZoneTransition::CanPlayerTransition() const
{
	if (!TargetZone) return false;

	// Level check
	if (RequiredLevel > 0)
	{
		const UARPGGameInstance* GI = Cast<UARPGGameInstance>(UGameplayStatics::GetGameInstance(this));
		if (GI && GI->PlayerLevel < RequiredLevel)
		{
			return false;
		}
	}

	// Quest check
	if (!RequiredQuestID.IsNone())
	{
		UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
		if (GI)
		{
			UARPGQuestSubsystem* QuestSys = GI->GetSubsystem<UARPGQuestSubsystem>();
			if (QuestSys && !QuestSys->GetCompletedQuestIDs().Contains(RequiredQuestID))
			{
				return false;
			}
		}
	}

	return true;
}

FText AARPGZoneTransition::GetBlockedReason() const
{
	if (!TargetZone)
	{
		return FText::FromString(TEXT("No destination"));
	}

	if (RequiredLevel > 0)
	{
		const UARPGGameInstance* GI = Cast<UARPGGameInstance>(UGameplayStatics::GetGameInstance(this));
		if (GI && GI->PlayerLevel < RequiredLevel)
		{
			return FText::FromString(FString::Printf(TEXT("Requires level %d"), RequiredLevel));
		}
	}

	if (!RequiredQuestID.IsNone())
	{
		UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
		if (GI)
		{
			UARPGQuestSubsystem* QuestSys = GI->GetSubsystem<UARPGQuestSubsystem>();
			if (QuestSys && !QuestSys->GetCompletedQuestIDs().Contains(RequiredQuestID))
			{
				return FText::FromString(FString::Printf(TEXT("Requires quest: %s"), *RequiredQuestID.ToString()));
			}
		}
	}

	return FText::GetEmpty();
}

void AARPGZoneTransition::OnPlayerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (OtherActor != PlayerPawn) return;

	ExecuteTransition();
}

void AARPGZoneTransition::ExecuteTransition()
{
	if (bOnCooldown) return;

	if (!CanPlayerTransition())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ZoneTransition] %s: Player cannot transition — %s"),
			*GetName(), *GetBlockedReason().ToString());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[ZoneTransition] %s: Transitioning to %s"),
		*GetName(), *TargetZone->ZoneID.ToString());

	// Start cooldown to prevent rapid re-trigger from overlap oscillation
	bOnCooldown = true;
	GetWorldTimerManager().SetTimer(CooldownTimerHandle, [this]() { bOnCooldown = false; }, TransitionCooldown, false);

	OnTransitionTriggered.Broadcast(TargetZone, this);

	UGameInstance* GI = GetGameInstance();
	UARPGZoneManager* ZoneMgr = GI ? GI->GetSubsystem<UARPGZoneManager>() : nullptr;
	if (ZoneMgr)
	{
		ZoneMgr->RequestZoneTransitionToSpawn(TargetZone, TargetSpawnPointTag);
	}
}
