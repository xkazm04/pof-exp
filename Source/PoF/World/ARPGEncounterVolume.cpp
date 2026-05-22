#include "World/ARPGEncounterVolume.h"
#include "World/ARPGZoneDefinition.h"

AARPGEncounterVolume::AARPGEncounterVolume()
{
}

void AARPGEncounterVolume::BeginPlay()
{
	// Generate waves before Super::BeginPlay triggers overlap logic
	if (bUseZoneEncounters && Waves.Num() == 0 && ZoneDefinition)
	{
		GenerateWavesFromZone();
	}

	ApplyZoneDifficulty();

	Super::BeginPlay();
}

void AARPGEncounterVolume::ApplyZoneDifficulty()
{
	if (!ZoneDefinition) return;

	// Override base difficulty from zone definition
	BaseDifficultyLevel = ZoneDefinition->BaseEnemyLevel;

	// Apply zone's max concurrent enemies cap if not already set
	if (MaxConcurrentEnemies == 0 && ZoneDefinition->MaxConcurrentEnemies > 0)
	{
		MaxConcurrentEnemies = ZoneDefinition->MaxConcurrentEnemies;
	}

	// Apply position-based difficulty arc on top of zone difficulty
	const float PositionMult = GetPositionDifficultyMultiplier();
	for (FSpawnWaveConfig& Wave : Waves)
	{
		Wave.WaveDifficultyMultiplier *= ZoneDefinition->ZoneDifficultyMultiplier * PositionMult;
	}
}

void AARPGEncounterVolume::GenerateWavesFromZone()
{
	if (!ZoneDefinition || ZoneDefinition->EncounterGroups.Num() == 0) return;

	Waves.Empty();

	for (int32 i = 0; i < AutoWaveCount; ++i)
	{
		const FZoneEncounterGroup& Group = ZoneDefinition->GetRandomEncounterGroup();

		FSpawnWaveConfig WaveConfig;
		if (Group.EnemyClasses.Num() > 0)
		{
			WaveConfig.EnemyClass = Group.EnemyClasses[FMath::RandRange(0, Group.EnemyClasses.Num() - 1)];
		}
		WaveConfig.Count = FMath::RandRange(Group.MinCount, Group.MaxCount);
		WaveConfig.SpawnInterval = 0.5f;
		WaveConfig.DelayBeforeWave = (i == 0) ? 0.f : 2.0f;
		WaveConfig.WaveDifficultyMultiplier = 1.0f + (static_cast<float>(i) * 0.15f);

		Waves.Add(WaveConfig);
	}

	UE_LOG(LogTemp, Log, TEXT("[EncounterVolume] %s: Generated %d waves from zone %s"),
		*GetName(), AutoWaveCount, *ZoneDefinition->ZoneID.ToString());
}

float AARPGEncounterVolume::GetPositionDifficultyMultiplier() const
{
	switch (EncounterPosition)
	{
	case EEncounterPosition::Entrance: return 0.8f;
	case EEncounterPosition::Middle:   return 1.0f;
	case EEncounterPosition::Deep:     return 1.3f;
	case EEncounterPosition::Boss:     return 1.6f;
	default:                           return 1.0f;
	}
}
