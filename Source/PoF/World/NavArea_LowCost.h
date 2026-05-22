#pragma once

#include "CoreMinimal.h"
#include "NavAreas/NavArea.h"
#include "NavArea_LowCost.generated.h"

/**
 * NavArea with reduced traversal cost.
 * Used for preferred patrol routes and corridors that AI should favor.
 */
UCLASS()
class POF_API UNavArea_LowCost : public UNavArea
{
	GENERATED_BODY()

public:
	UNavArea_LowCost();
};
