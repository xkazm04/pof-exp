#include "World/NavArea_Hazard.h"

UNavArea_Hazard::UNavArea_Hazard()
{
	DefaultCost = 10.0f;  // Very high cost — AI strongly avoids hazard zones
	DrawColor = FColor(255, 50, 50);  // Red for hazards
}
