#include "Audio/ARPGMusicRules.h"

EARPGMusicLayer UARPGMusicRules::LayerForEvent(FName MusicEvent)
{
	if (MusicEvent == FName(TEXT("MusicEvent.AmbientStart"))) return EARPGMusicLayer::AmbientTension;
	if (MusicEvent == FName(TEXT("MusicEvent.CombatStart")))  return EARPGMusicLayer::CombatLow;
	if (MusicEvent == FName(TEXT("MusicEvent.EliteSpawned"))) return EARPGMusicLayer::CombatHigh;
	if (MusicEvent == FName(TEXT("MusicEvent.BossSwell")))    return EARPGMusicLayer::BossSwell;
	return EARPGMusicLayer::Silence;
}
