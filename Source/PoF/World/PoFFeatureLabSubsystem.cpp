#include "World/PoFFeatureLabSubsystem.h"
#include "Dialogue/ARPGNPCActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "UI/PreGameMenuWidget.h"

bool UPoFFeatureLabSubsystem::ShouldPopulate(const FString& MapName)
{
	// PIE maps arrive as "UEDPIE_0_FeatureLab"; standalone as "FeatureLab".
	return MapName.Contains(TEXT("FeatureLab"));
}

TArray<FFeatureLabEntry> UPoFFeatureLabSubsystem::GetRoster()
{
	TArray<FFeatureLabEntry> Roster;

	// Dialog-trees: the Duel Challenge speaker (dialog-duel-intro) — the NPC
	// self-builds its tree in AARPGNPCActor::BeginPlay when NPCID == Malgrave.
	FFeatureLabEntry Malgrave;
	Malgrave.ClassPath = TEXT("/Script/PoF.ARPGNPCActor");
	Malgrave.Label = TEXT("NPC_DarthMalgrave");
	Malgrave.Offset = FVector(400.f, 150.f, 0.f);
	Malgrave.NPCID = FName(TEXT("Malgrave"));
	Roster.Add(Malgrave);

	// Next features land here as the plan progresses (bestiary duel Sith,
	// saber pickups, arena props, ...) — one entry each, gate-asserted.
	return Roster;
}

void UPoFFeatureLabSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (!ShouldPopulate(InWorld.GetMapName()))
	{
		return;
	}

	// Pre-game menu (UE mirror of the browser staging shell): saber choice + Enter.
	if (APlayerController* PC = InWorld.GetFirstPlayerController())
	{
		if (UPreGameMenuWidget* Menu = CreateWidget<UPreGameMenuWidget>(PC, UPreGameMenuWidget::StaticClass()))
		{
			Menu->AddToViewport(30);
			UE_LOG(LogTemp, Log, TEXT("[PreGameMenu] shown (built=%s)"), Menu->IsMenuBuilt() ? TEXT("yes") : TEXT("NO"));
		}
	}

	// Anchor on the PlayerStart so the roster surrounds the player's spawn.
	FVector Anchor = FVector(0.f, 0.f, 100.f);
	for (TActorIterator<APlayerStart> It(&InWorld); It; ++It)
	{
		Anchor = It->GetActorLocation();
		break;
	}

	for (const FFeatureLabEntry& Entry : GetRoster())
	{
		UClass* Cls = LoadClass<AActor>(nullptr, *Entry.ClassPath);
		if (!Cls)
		{
			UE_LOG(LogTemp, Warning, TEXT("[FeatureLab] class not found: %s"), *Entry.ClassPath);
			continue;
		}
		FVector Pos = Anchor + Entry.Offset;
		// Ground the spawn: static actors don't fall, and a PlayerStart sits well
		// above the floor — trace down and rest the stand-in on the surface.
		FHitResult Floor;
		const FVector TraceFrom = Pos + FVector(0.f, 0.f, 300.f);
		if (InWorld.LineTraceSingleByChannel(Floor, TraceFrom, TraceFrom - FVector(0.f, 0.f, 2000.f), ECC_Visibility))
		{
			Pos.Z = Floor.ImpactPoint.Z + 92.f; // cylinder half-height at the lab scale
		}
		const FRotator Facing = (Anchor - Pos).Rotation();
		// DEFERRED spawn: NPCID must be set BEFORE BeginPlay fires, or the NPC
		// begins play as None and never self-attaches its dialogue tree.
		const FTransform SpawnTM(FRotator(0.f, Facing.Yaw, 0.f), Pos);
		AActor* Spawned = InWorld.SpawnActorDeferred<AActor>(
			Cls, SpawnTM, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!Spawned)
		{
			UE_LOG(LogTemp, Warning, TEXT("[FeatureLab] spawn failed: %s"), *Entry.Label);
			continue;
		}
#if WITH_EDITOR
		Spawned->SetActorLabel(Entry.Label);
#endif
		if (!Entry.NPCID.IsNone())
		{
			if (AARPGNPCActor* NPC = Cast<AARPGNPCActor>(Spawned))
			{
				NPC->NPCID = Entry.NPCID;
				NPC->DisplayName = FText::FromString(TEXT("Darth Malgrave"));
			}
		}
		Spawned->FinishSpawning(SpawnTM);
		// Code-spawned NPCs have NO mesh assigned (designers set it per map
		// instance) — an invisible actor reads as "the feature is missing".
		// Give meshless spawns a visible engine-capsule stand-in.
		UStaticMeshComponent* MeshComp = Spawned->FindComponentByClass<UStaticMeshComponent>();
		if (!MeshComp)
		{
			UE_LOG(LogTemp, Warning, TEXT("[FeatureLab] %s: no StaticMeshComponent found"), *Entry.Label);
		}
		else if (MeshComp->GetStaticMesh())
		{
			UE_LOG(LogTemp, Log, TEXT("[FeatureLab] %s already has mesh %s"),
				*Entry.Label, *MeshComp->GetStaticMesh()->GetName());
		}
		else if (UStaticMesh* Capsule = LoadObject<UStaticMesh>(
				nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")))
		{
			MeshComp->SetStaticMesh(Capsule);
			MeshComp->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.8f));
			UE_LOG(LogTemp, Log, TEXT("[FeatureLab] %s had no mesh - cylinder stand-in applied"), *Entry.Label);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[FeatureLab] %s: engine cylinder failed to load"), *Entry.Label);
		}
		UE_LOG(LogTemp, Log, TEXT("[FeatureLab] spawned %s at %s"), *Entry.Label, *Pos.ToCompactString());
	}
}
