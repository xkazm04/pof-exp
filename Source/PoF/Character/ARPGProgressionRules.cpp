#include "Character/ARPGProgressionRules.h"

int32 UARPGProgressionRules::XpToNextLevel(int32 Level)
{
	const int32 L = FMath::Clamp(Level, 1, SoftCapLevel);
	return FMath::RoundToInt(static_cast<double>(XpBase) * FMath::Pow(1.08, static_cast<double>(L)));
}
