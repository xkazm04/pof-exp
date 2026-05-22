#include "World/ARPGZoneVolume.h"
#include "World/ARPGZoneDefinition.h"
#include "World/ARPGZoneManager.h"
#include "Components/BoxComponent.h"
#include "Components/BillboardComponent.h"
#include "Kismet/GameplayStatics.h"

AARPGZoneVolume::AARPGZoneVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	ZoneBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("ZoneBounds"));
	ZoneBounds->SetBoxExtent(FVector(2000.f, 2000.f, 500.f));
	ZoneBounds->SetCollisionProfileName(TEXT("Trigger"));
	ZoneBounds->SetGenerateOverlapEvents(true);
	SetRootComponent(ZoneBounds);

#if WITH_EDITORONLY_DATA
	EditorSprite = CreateDefaultSubobject<UBillboardComponent>(TEXT("EditorSprite"));
	EditorSprite->SetupAttachment(ZoneBounds);
	EditorSprite->SetHiddenInGame(true);
#endif
}

void AARPGZoneVolume::BeginPlay()
{
	Super::BeginPlay();

	if (!ZoneDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ZoneVolume] %s: No ZoneDefinition assigned"), *GetName());
		return;
	}

	// Register with the zone manager
	UGameInstance* GI = GetGameInstance();
	UARPGZoneManager* ZoneMgr = GI ? GI->GetSubsystem<UARPGZoneManager>() : nullptr;
	if (ZoneMgr)
	{
		ZoneMgr->RegisterZone(ZoneDefinition);

		if (bIsStartingZone)
		{
			ZoneMgr->SetCurrentZone(ZoneDefinition);
		}
	}

	// Bind overlap
	ZoneBounds->OnComponentBeginOverlap.AddDynamic(this, &AARPGZoneVolume::OnPlayerEnterZone);

	// Check if the player is already inside this zone on level load
	if (!bIsStartingZone && ZoneMgr)
	{
		TArray<AActor*> Overlapping;
		ZoneBounds->GetOverlappingActors(Overlapping);
		APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
		if (PlayerPawn && Overlapping.Contains(PlayerPawn))
		{
			ZoneMgr->SetCurrentZone(ZoneDefinition);
		}
	}
}

void AARPGZoneVolume::OnPlayerEnterZone(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (OtherActor != PlayerPawn) return;

	UGameInstance* GI = GetGameInstance();
	UARPGZoneManager* ZoneMgr = GI ? GI->GetSubsystem<UARPGZoneManager>() : nullptr;
	if (ZoneMgr && ZoneDefinition)
	{
		ZoneMgr->SetCurrentZone(ZoneDefinition);
	}
}
