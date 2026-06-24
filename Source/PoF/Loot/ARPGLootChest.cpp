#include "Loot/ARPGLootChest.h"
#include "Loot/ARPGWorldItem.h"
#include "Inventory/ARPGLootTable.h"
#include "Inventory/ARPGItemInstance.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"

AARPGLootChest::AARPGLootChest()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	RootComponent = BaseMesh;

	LidMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LidMesh"));
	LidMesh->SetupAttachment(BaseMesh);

	InteractionTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionTrigger"));
	InteractionTrigger->SetupAttachment(BaseMesh);
	InteractionTrigger->SetSphereRadius(200.f);
	InteractionTrigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionTrigger->SetGenerateOverlapEvents(true);

	Tags.Add(FName("Interactable"));
	Tags.Add(FName("Chest"));
}

void AARPGLootChest::BeginPlay()
{
	Super::BeginPlay();
	CurrentLidAngle = 0.f;
	TargetLidAngle = 0.f;
}

bool AARPGLootChest::TryOpen(AActor* Opener)
{
	if (bIsOpened || !Opener)
	{
		return false;
	}

	bIsOpened = true;
	bIsAnimating = true;
	TargetLidAngle = LidOpenAngle;
	SetActorTickEnabled(true);

	UE_LOG(LogTemp, Log, TEXT("[Chest] %s opened by %s"), *GetName(), *Opener->GetName());

	OnChestOpened.Broadcast(this);

	// Spawn loot after a short delay to let the lid start opening
	FTimerHandle LootTimerHandle;
	GetWorldTimerManager().SetTimer(LootTimerHandle, this, &AARPGLootChest::SpawnLoot, 0.3f, false);

	// Schedule respawn if enabled
	if (bCanRespawn)
	{
		GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AARPGLootChest::ResetChest, RespawnDelay, false);
	}

	return true;
}

void AARPGLootChest::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsAnimating)
	{
		return;
	}

	// Interpolate lid toward target angle
	const float Direction = FMath::Sign(TargetLidAngle - CurrentLidAngle);
	CurrentLidAngle += Direction * LidOpenSpeed * DeltaTime;

	// Clamp and stop
	if ((Direction > 0.f && CurrentLidAngle >= TargetLidAngle) ||
		(Direction < 0.f && CurrentLidAngle <= TargetLidAngle))
	{
		CurrentLidAngle = TargetLidAngle;
		bIsAnimating = false;
		SetActorTickEnabled(false);
	}

	LidMesh->SetRelativeRotation(FRotator(CurrentLidAngle, 0.f, 0.f));
}

void AARPGLootChest::SpawnLoot()
{
	if (!LootTable)
	{
		return;
	}

	TArray<UARPGItemInstance*> AllItems;
	for (int32 Roll = 0; Roll < NumRolls; ++Roll)
	{
		TArray<UARPGItemInstance*> RolledItems = LootTable->RollLoot(this);
		AllItems.Append(RolledItems);
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector ChestLocation = GetActorLocation();
	const FVector Forward = GetActorForwardVector();

	for (int32 i = 0; i < AllItems.Num(); ++i)
	{
		float Angle = (AllItems.Num() > 1) ? (2.f * PI * i / AllItems.Num()) : 0.f;
		FVector SpawnOffset = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * ItemScatterRadius;
		SpawnOffset.Z = 50.f;
		// Bias scatter to the front of the chest
		SpawnOffset += Forward * 100.f;

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AARPGWorldItem* WorldItem = World->SpawnActor<AARPGWorldItem>(
			AARPGWorldItem::StaticClass(),
			ChestLocation + SpawnOffset,
			FRotator::ZeroRotator,
			SpawnParams);

		if (WorldItem)
		{
			// Chest loot routes into the looter's inventory (not the slice VFX-and-destroy).
			WorldItem->InitFromItemInstance(AllItems[i], /*bRouteToInventory=*/true);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Chest] %s spawned %d items"), *GetName(), AllItems.Num());
}

void AARPGLootChest::ResetChest()
{
	bIsOpened = false;
	bIsAnimating = true;
	TargetLidAngle = 0.f;
	SetActorTickEnabled(true);

	UE_LOG(LogTemp, Log, TEXT("[Chest] %s reset for respawn"), *GetName());
}
