#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h"
#include "FlyingCabCityData.h"
#include "FlyingCabHighwayMapLayer.h"
#include "FlyingCabHighwayTile.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyingCabHighwayMapTest,
 "FlyingCab.Core.City.HighwayTileMinimap", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FFlyingCabHighwayMapTest::RunTest(const FString&)
{
 UWorld* World = UWorld::CreateWorld(EWorldType::Game,false);
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
 const FVector2D Min = FlyingCabCityData::GetMinimapWorldMin();
 const FVector2D Max = FlyingCabCityData::GetMinimapWorldMax();
 const FVector2D Mid = (Min+Max)*.5;
 const FVector Center(Mid.X,0,Mid.Y);
 TestEqual(TEXT("No tile means no additional road"),UFlyingCabHighwayMapLayer::GetRoadSegments(World).Num(),0);
 auto* Tile = World->SpawnActor<AFlyingCabHighwayTile>(Center,FRotator::ZeroRotator);
 auto Roads = UFlyingCabHighwayMapLayer::GetRoadSegments(World);
 TestEqual(TEXT("Placed tile creates one line"),Roads.Num(),1);
 if (Roads.Num()==1)
 {
  TestTrue(TEXT("Horizontal midpoint matches city midpoint"),((Roads[0].Key+Roads[0].Value)*.5).Equals(FVector2D(.5,.5),1.e-6));
  TestTrue(TEXT("Line reflects world length"),FMath::IsNearlyEqual(Roads[0].Value.X-Roads[0].Key.X,2800./(Max.X-Min.X),1.e-6));
 }
 Tile->SetActorScale3D(FVector(2,1,1));
 Tile->SetActorRotation(FRotator(45,0,0));
 Roads = UFlyingCabHighwayMapLayer::GetRoadSegments(World);
 if (TestEqual(TEXT("Rotated tile still one line"),Roads.Num(),1))
 {
  TestTrue(TEXT("Rotation produces a diagonal"),!FMath::IsNearlyEqual(Roads[0].Key.X,Roads[0].Value.X)
   && !FMath::IsNearlyEqual(Roads[0].Key.Y,Roads[0].Value.Y));
  const FVector2D Delta = Roads[0].Value-Roads[0].Key;
  const FVector2D WorldDelta(Delta.X*(Max.X-Min.X),Delta.Y*(Max.Y-Min.Y));
  TestTrue(TEXT("Scale doubles line length"),FMath::IsNearlyEqual(WorldDelta.Length(),5600.,.01));
 }
 Tile->bEnabled = false;
 TestEqual(TEXT("Disabled bonus retains visible road"),UFlyingCabHighwayMapLayer::GetRoadSegments(World).Num(),1);
 auto* Copy = World->SpawnActor<AFlyingCabHighwayTile>(Center+FVector(0,0,2000),FRotator::ZeroRotator);
 TestEqual(TEXT("Second tile adds a second line"),UFlyingCabHighwayMapLayer::GetRoadSegments(World).Num(),2);
 Copy->Destroy();
 TestEqual(TEXT("Deleting tile removes line"),UFlyingCabHighwayMapLayer::GetRoadSegments(World).Num(),1);
 Tile->SetActorScale3D(FVector::OneVector);
 Tile->SetActorRotation(FRotator::ZeroRotator);
 Tile->SetActorLocation(FVector(Max.X,0,Mid.Y));
 Roads = UFlyingCabHighwayMapLayer::GetRoadSegments(World);
 if (TestEqual(TEXT("Crossing boundary is clipped"),Roads.Num(),1))
  TestTrue(TEXT("Line ends at map edge"),FMath::IsNearlyEqual(Roads[0].Value.X,1.,1.e-6));
 Tile->SetActorLocation(FVector(Max.X+10000,0,Mid.Y));
 TestEqual(TEXT("Outside tile makes no false border line"),UFlyingCabHighwayMapLayer::GetRoadSegments(World).Num(),0);
 TestEqual(TEXT("No world cannot leak other world's roads"),UFlyingCabHighwayMapLayer::GetRoadSegments(nullptr).Num(),0);
 World->DestroyWorld(false);
 GEngine->DestroyWorldContext(World);
 return true;
}
#endif
