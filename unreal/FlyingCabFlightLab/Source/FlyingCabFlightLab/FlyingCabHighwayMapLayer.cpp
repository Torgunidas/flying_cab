#include "FlyingCabHighwayMapLayer.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "FlyingCabCityData.h"
#include "FlyingCabHighwayTile.h"
#include "Rendering/DrawElements.h"

TArray<TPair<FVector2D,FVector2D>> UFlyingCabHighwayMapLayer::GetRoadSegments(UWorld* World)
{
 TArray<TPair<FVector2D,FVector2D>> Result;
 if (!World) return Result;
 const FVector2D Min = FlyingCabCityData::GetMinimapWorldMin();
 const FVector2D Size = FlyingCabCityData::GetMinimapWorldMax()-Min;
 if (Size.X <= 0 || Size.Y <= 0) return Result;
 for (TActorIterator<AFlyingCabHighwayTile> It(World); It; ++It)
 {
  // Enabled controls the bonus, not the existence of the visible road.
  if (It->IsActorBeingDestroyed() || It->GetActorScale3D().GetAbs().GetMin() < UE_SMALL_NUMBER) continue;
  const FTransform Transform = It->Zone->GetComponentTransform();
  const double HalfLength = It->Zone->GetUnscaledBoxExtent().X;
  const FVector WA = Transform.TransformPosition(FVector(-HalfLength,0,0));
  const FVector WB = Transform.TransformPosition(FVector(HalfLength,0,0));
  const FVector2D A((WA.X-Min.X)/Size.X,1.-(WA.Z-Min.Y)/Size.Y);
  const FVector2D B((WB.X-Min.X)/Size.X,1.-(WB.Z-Min.Y)/Size.Y);
  const FVector2D D = B-A;
  if (D.IsNearlyZero()) continue;
  // Clip the segment, not its endpoints independently (which invents roads on map edges).
  double Enter = 0, Exit = 1;
  bool bVisible = true;
  for (int32 Axis=0; Axis<2; ++Axis)
  {
   if (FMath::IsNearlyZero(D[Axis]))
   {
    if (A[Axis]<0 || A[Axis]>1) bVisible = false;
   }
   else
   {
    const double T0 = -A[Axis]/D[Axis], T1 = (1.-A[Axis])/D[Axis];
    Enter = FMath::Max(Enter,FMath::Min(T0,T1));
    Exit = FMath::Min(Exit,FMath::Max(T0,T1));
   }
  }
  if (bVisible && Enter < Exit) Result.Emplace(A+D*Enter,A+D*Exit);
 }
 return Result;
}

int32 UFlyingCabHighwayMapLayer::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
 const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
 const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
 const int32 BaseLayer = Super::NativePaint(Args,AllottedGeometry,MyCullingRect,OutDrawElements,LayerId,InWidgetStyle,bParentEnabled);
 const FVector2D Size = AllottedGeometry.GetLocalSize();
 for (const auto& Road : GetRoadSegments(GetWorld()))
 {
  TArray<FVector2D> Points{Road.Key*Size,Road.Value*Size};
  FSlateDrawElement::MakeLines(OutDrawElements,BaseLayer+1,AllottedGeometry.ToPaintGeometry(),
   Points,ESlateDrawEffect::None,FLinearColor(.06f,.25f,.3f,.75f)*InWidgetStyle.GetColorAndOpacityTint(),true,4.f);
 }
 return BaseLayer+1;
}
