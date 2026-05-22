#pragma once

#include "CoreMinimal.h"
#include "NavAreas/NavArea.h"
#include "NavArea_Hazard.generated.h"

/**
 * NavArea with very high traversal cost for environmental hazards.
 * AI strongly avoids these areas unless no alternative path exists.
 */
UCLASS()
class POF_API UNavArea_Hazard : public UNavArea
{
	GENERATED_BODY()

public:
	UNavArea_Hazard();
};
