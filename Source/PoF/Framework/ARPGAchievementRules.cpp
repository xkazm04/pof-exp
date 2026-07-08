#include "Framework/ARPGAchievementRules.h"

bool UARPGAchievementRules::ShouldUnlock(int32 KillCount, int32 Threshold, bool bAlreadyUnlocked)
{
	return !bAlreadyUnlocked && KillCount >= Threshold;
}
