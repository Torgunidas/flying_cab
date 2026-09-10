#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FlyingCabWorldAuthoring.generated.h"

/** Explicit one-time authoring operation. Never called by game startup. */
UCLASS()
class UFlyingCabWorldAuthoring : public UBlueprintFunctionLibrary
{
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable, Category="Flying Cab|World Authoring")
 static bool BakeCurrentWorld();
};
