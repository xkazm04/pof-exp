#include "ARPGGameInstance.h"
#include "Framework/ARPGSaveSubsystem.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

void UARPGGameInstance::Init()
{
	Super::Init();

	// Start a repeating 1-second timer to accumulate play time
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(PlayTimeTimerHandle, this,
			&UARPGGameInstance::TickPlayTime, 1.0f, true);
	}

	// Bind to map load events
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UARPGGameInstance::HandlePostLoadMap);

	UE_LOG(LogTemp, Log, TEXT("[ARPGGameInstance] Init — PlayerLevel=%d, CurrentZone=%s"),
		PlayerLevel, *CurrentZone);
}

void UARPGGameInstance::Shutdown()
{
	// Clear timer
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PlayTimeTimerHandle);
	}

	// Unbind map load delegate
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	UE_LOG(LogTemp, Log, TEXT("[ARPGGameInstance] Shutdown — TotalPlayTime=%.0fs"), TotalPlayTime);

	Super::Shutdown();
}

// =========================================================================
// Play Time
// =========================================================================

void UARPGGameInstance::TickPlayTime()
{
	TotalPlayTime += 1.0f;
}

// =========================================================================
// Map Transitions
// =========================================================================

void UARPGGameInstance::TravelToMap(const FString& MapName)
{
	const FString CurrentMap = GetWorld() ? GetWorld()->GetMapName() : TEXT("Unknown");

	UE_LOG(LogTemp, Log, TEXT("[ARPGGameInstance] TravelToMap: %s -> %s"), *CurrentMap, *MapName);

	// Broadcast pre-change
	OnPreMapChange.Broadcast(CurrentMap, MapName);

	// Update zone tracking
	CurrentZone = MapName;

	// Perform the travel (seamless if possible, otherwise hard travel)
	UGameplayStatics::OpenLevel(this, FName(*MapName));
}

void UARPGGameInstance::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (!LoadedWorld) return;

	const FString NewMapName = LoadedWorld->GetMapName();
	CurrentZone = NewMapName;

	OnPostMapChange.Broadcast(NewMapName);

	UE_LOG(LogTemp, Log, TEXT("[ARPGGameInstance] PostMapLoad: %s"), *NewMapName);
}

// =========================================================================
// Save System Access
// =========================================================================

UARPGSaveSubsystem* UARPGGameInstance::GetSaveSubsystem() const
{
	return GetSubsystem<UARPGSaveSubsystem>();
}

bool UARPGGameInstance::QuickSave()
{
	if (UARPGSaveSubsystem* SaveSub = GetSaveSubsystem())
	{
		return SaveSub->QuickSave();
	}
	return false;
}

bool UARPGGameInstance::QuickLoad()
{
	if (UARPGSaveSubsystem* SaveSub = GetSaveSubsystem())
	{
		return SaveSub->QuickLoad();
	}
	return false;
}

// =========================================================================
// Progression Sync
// =========================================================================

void UARPGGameInstance::SyncProgressionToSubsystem()
{
	// The subsystem reads our public properties directly.
	// This method exists as a hook for subclasses to push additional data.
}

void UARPGGameInstance::SyncProgressionFromSubsystem(const FARPGPlayerProgression& LoadedProgression)
{
	PlayerLevel = LoadedProgression.PlayerLevel;
	Experience = LoadedProgression.Experience;
	MaxHealth = LoadedProgression.MaxHealth;
	MaxMana = LoadedProgression.MaxMana;
	CurrentZone = LoadedProgression.CurrentZone;
	TotalPlayTime = LoadedProgression.TotalPlayTime;

	UE_LOG(LogTemp, Log, TEXT("[ARPGGameInstance] SyncFromSubsystem — Lv%d, XP=%.0f, HP=%.0f, MP=%.0f, Zone=%s, PlayTime=%.0fs"),
		PlayerLevel, Experience, MaxHealth, MaxMana, *CurrentZone, TotalPlayTime);
}
