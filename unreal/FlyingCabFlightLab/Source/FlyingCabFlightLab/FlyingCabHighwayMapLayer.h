#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FlyingCabHighwayMapLayer.generated.h"

/** Read-only live road overlay, separate from the touch-input implementation. */
UCLASS()
class FLYINGCABFLIGHTLAB_API UFlyingCabHighwayMapLayer : public UUserWidget
{
 GENERATED_BODY()
public:
 // Normalized, clipped map endpoints. Z is inverted for screen coordinates.
 static TArray<TPair<FVector2D,FVector2D>> GetRoadSegments(UWorld* World);
protected:
 virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
  const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
  const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
};
