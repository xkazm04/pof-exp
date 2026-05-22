#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PofAssetManifest.h"
#include "PofBlueprintIntrospector.generated.h"

UCLASS()
class UPofBlueprintIntrospector : public UObject
{
    GENERATED_BODY()

public:
    /** Introspect a Blueprint by asset path and return JSON. */
    FString IntrospectBlueprintByPath(const FString& AssetPath);

    /** Introspect a loaded Blueprint and return structured data. */
    FPofBlueprintEntry IntrospectBlueprint(class UBlueprint* Blueprint);
};
