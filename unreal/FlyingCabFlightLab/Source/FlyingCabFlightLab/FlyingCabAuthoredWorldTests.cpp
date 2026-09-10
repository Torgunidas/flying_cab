#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "EngineUtils.h"
#include "FlyingCabAuthoredWorld.h"
#include "FlyingCabCityExpansion.h"
#include "FlyingCabCityData.h"
#include "FlyingCabFuelStation.h"
#include "FlyingCabOnFootPortal.h"
#include "FlyingCabWorldBootstrap.h"
#include "Components/StaticMeshComponent.h"

namespace
{
class FAuthoredWorldCheck : public IAutomationLatentCommand
{
 FAutomationTestBase* Test;
 double Started = FPlatformTime::Seconds();
public:
 explicit FAuthoredWorldCheck(FAutomationTestBase* InTest) : Test(InTest) {}
 bool Update() override
 {
  UWorld* World = AutomationCommon::GetAnyGameWorld();
  if (!World || !World->HasBegunPlay())
  {
   if (FPlatformTime::Seconds()-Started<20) return false;
   Test->AddError(TEXT("Authored map did not begin play")); return true;
  }
  auto* Authored = AFlyingCabAuthoredWorld::Find(World);
  if (!Test->TestNotNull(TEXT("Saved authored-world settings loaded"),Authored)) return true;
  int32 Cities=0, Geometry=0;
  for (TActorIterator<AFlyingCabCityExpansion> It(World); It; ++It)
  {
   ++Cities;
   Test->TestTrue(TEXT("City controller uses baked geometry"),It->bBakedToLevel);
   TArray<UStaticMeshComponent*> Meshes;
   It->GetComponents(Meshes);
   Test->TestEqual(TEXT("BeginPlay did not regenerate geometry on controller"),Meshes.Num(),0);
  }
  for (TActorIterator<AFlyingCabWorldGeometry> It(World); It; ++It) ++Geometry;
  Test->TestEqual(TEXT("Exactly one city controller"),Cities,1);
  Test->TestTrue(TEXT("Separate saved geometry actors survived reload"),Geometry>250);
  Test->TestEqual(TEXT("All taxi stops are read from this map"),FlyingCabCityData::GetWorldDistricts(World).Num(),24);
  Test->TestEqual(TEXT("No duplicate fuel services spawned"),FlyingCabCityData::GetWorldFuelStations(World).Num(),4);
  Test->TestNotNull(TEXT("Portal destination reference survived serialization"),Authored->Entrance ? Authored->Entrance->DestinationActor.Get() : nullptr);
  for (TActorIterator<AFlyingCabDistrictAnchor> It(World); It; ++It)
  {
   const FVector Before = It->GetActorLocation();
   const FVector Delta(100,0,200);
   It->SetActorLocation(Before+Delta);
   const auto Stops = FlyingCabCityData::GetWorldDistricts(World);
   const auto* Stop = Stops.FindByPredicate([&It](const auto& D) { return D.DistrictId==It->Definition.DistrictId; });
   Test->TestTrue(TEXT("Moved parent drives gameplay stop coordinates"),Stop && Stop->StopLocation.Equals(Before+Delta));
   const auto Defaults = FlyingCabCityData::GetDistricts();
   const auto* Original = Defaults.FindByPredicate([&It](const auto& D) { return D.DistrictId==It->Definition.DistrictId; });
   Test->TestTrue(TEXT("World edit leaves shared source asset unchanged"),Original && !Original->StopLocation.Equals(Before+Delta));
   It->SetActorLocation(Before);
   break;
  }
  for (TActorIterator<AFlyingCabFuelStation> It(World); It; ++It)
  {
   const FVector Before = It->GetActorLocation();
   It->SetActorLocation(Before+FVector(120,0,80));
   Test->TestTrue(TEXT("Service minimap data follows the actual actor"),FlyingCabCityData::GetWorldFuelStations(World).ContainsByPredicate(
    [&It](const auto& S) { return S.DisplayName==It->GetServiceName() && S.Location.Equals(It->GetActorLocation()); }));
   It->SetActorLocation(Before);
   break;
  }
  return true;
 }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyingCabAuthoredWorldTest,"FlyingCab.Functional.PIE.AuthoredWorld",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FFlyingCabAuthoredWorldTest::RunTest(const FString&)
{
 if (!AutomationOpenMap(TEXT("/Game/Maps/FlightLab"),true)) return false;
 ADD_LATENT_AUTOMATION_COMMAND(FAuthoredWorldCheck(this));
 return true;
}
#endif
