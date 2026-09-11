#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "FlyingCabHighwayTile.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyingCabHighwayTileTest,
 "FlyingCab.Core.City.HighwayTile", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FFlyingCabHighwayTileTest::RunTest(const FString&)
{
 UWorld* World = UWorld::CreateWorld(EWorldType::Game,false);
 const FVector Center(80000,0,80000); // Outside every legacy highway.
 auto* Tile = World->SpawnActor<AFlyingCabHighwayTile>(Center,FRotator::ZeroRotator);
 if (!TestNotNull(TEXT("Tile spawns"),Tile)) { World->DestroyWorld(false); return false; }
 Tile->SetActorScale3D(FVector(2,1,.5));
 Tile->SetActorRotation(FRotator(90,0,0));
 Tile->SpeedMultiplier = 1.8f;
 Tile->FuelConsumptionMultiplier = .3f;
 const FTransform T = Tile->GetActorTransform();
 TestTrue(TEXT("Rotated scaled edge inside"),Tile->ContainsLocation(T.TransformPosition(FVector(1399,199,449))));
 TestFalse(TEXT("Outside length"),Tile->ContainsLocation(T.TransformPosition(FVector(1401,0,0))));
 TestFalse(TEXT("Outside width"),Tile->ContainsLocation(T.TransformPosition(FVector(0,0,451))));
 TestFalse(TEXT("Outside flight depth"),Tile->ContainsLocation(T.TransformPosition(FVector(0,201,0))));
 float Speed, Fuel;
 TestTrue(TEXT("New area grants bonus"),AFlyingCabHighwayTile::GetBonuses(World,Center,1.5,.5,Speed,Fuel));
 TestEqual(TEXT("Tile speed"),Speed,1.8f);
 TestEqual(TEXT("Tile fuel"),Fuel,.3f);
 auto* Second = World->SpawnActor<AFlyingCabHighwayTile>(Center,FRotator::ZeroRotator);
 TestTrue(TEXT("Overlapping tiles found"),AFlyingCabHighwayTile::GetBonuses(World,Center,1.5,.5,Speed,Fuel));
 TestEqual(TEXT("No speed stacking"),Speed,1.8f);
 TestEqual(TEXT("No fuel stacking"),Fuel,.3f);
 Tile->SetActorLocation(Center+FVector(10000,0,0));
 Second->bEnabled = false;
 TestFalse(TEXT("Moving tile leaves no ghost bonus"),AFlyingCabHighwayTile::GetBonuses(World,Center,1.5,.5,Speed,Fuel));
 TestEqual(TEXT("Ordinary fuel outside"),Fuel,1.f);
 Tile->Destroy();
 TestFalse(TEXT("Destroyed tile grants no bonus"),AFlyingCabHighwayTile::GetBonuses(World,Center+FVector(10000,0,0),1.5,.5,Speed,Fuel));
 World->DestroyWorld(false);
 return true;
}
#endif
