#pragma once

#include "CoreMinimal.h"
#include "NavAreas/NavArea.h"
#include "NavArea_Chokepoint.generated.h"

/**
 * NavArea with elevated traversal cost.
 * Used for narrow passages and chokepoints to discourage AI crowding.
 * Higher cost than Obstacle so most AI prefers wider paths.
 */
UCLASS()
class POF_API UNavArea_Chokepoint : public UNavArea
{
	GENERATED_BODY()

public:
	UNavArea_Chokepoint();
};
