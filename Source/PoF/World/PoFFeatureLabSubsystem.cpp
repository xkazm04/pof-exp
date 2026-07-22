#include "World/PoFFeatureLabSubsystem.h"
#include "Dialogue/ARPGNPCActor.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"

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
		const FVector Pos = Anchor + Entry.Offset;
		const FRotator Facing = (Anchor - Pos).Rotation();
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AActor* Spawned = InWorld.SpawnActor<AActor>(Cls, Pos, FRotator(0.f, Facing.Yaw, 0.f), Params);
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
		UE_LOG(LogTemp, Log, TEXT("[FeatureLab] spawned %s at %s"), *Entry.Label, *Pos.ToCompactString());
	}
}
