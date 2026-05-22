#include "World/NavArea_Chokepoint.h"

UNavArea_Chokepoint::UNavArea_Chokepoint()
{
	DefaultCost = 3.0f;  // Higher than Obstacle (1.5) to strongly discourage crowding
	DrawColor = FColor(255, 165, 0);  // Orange for chokepoints
}
