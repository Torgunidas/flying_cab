#include "FlyingCabWorldAuthoring.h"
#include "Editor.h"
#include "Engine/TargetPoint.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "FlyingCabAuthoredWorld.h"
#include "FlyingCabCityExpansion.h"
#include "FlyingCabSupercarData.h"
#include "FlyingCabFuelStation.h"
#include "FlyingCabRepairStation.h"
#include "FlyingCabNightshiftOffice.h"
#include "FlyingCabOnFootPortal.h"
#include "FlyingCabAccessTerminal.h"
#include "FlyingCabQuestGiver.h"
#include "FlyingCabQuestHubData.h"
#include "FlyingCabLivingRoute.h"
#include "FlyingCabLivingWorldProfile.h"
#include "ScopedTransaction.h"

namespace
{
template<class T> T* Place(UWorld* World,const FVector& Location,const FString& Label,const FName Folder,AActor* Parent=nullptr)
{
 FActorSpawnParameters Params;
 Params.ObjectFlags |= RF_Transactional;
 Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
 T* Actor = World->SpawnActor<T>(Location, FRotator::ZeroRotator, Params);
 if (!Actor) return nullptr;
 Actor->SetActorLabel(Label);
 Actor->SetFolderPath(Folder);
 if (Parent) Actor->AttachToActor(Parent, FAttachmentTransformRules::KeepWorldTransform);
 return Actor;
}
}

bool UFlyingCabWorldAuthoring::BakeCurrentWorld()
{
 UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
 if (!World || World->WorldType != EWorldType::Editor || AFlyingCabAuthoredWorld::Find(World)) return false;
 const FScopedTransaction Transaction(NSLOCTEXT("FlyingCab", "BakeWorld", "Create editable Flying Cab world"));
 World->Modify();
 auto* City = Place<AFlyingCabCityExpansion>(World,FVector::ZeroVector,TEXT("City signal controller"),TEXT("Flying Cab/System"));
 City->BakeToLevel();
 auto* Authored = Place<AFlyingCabAuthoredWorld>(World,FVector::ZeroVector,TEXT("Flying Cab - level settings"),TEXT("Flying Cab/System"));
 for (TActorIterator<AFlyingCabDistrictAnchor> It(World); It; ++It)
 {
  const auto D = It->GetWorldDefinition();
  const FVector Roof = D.StopLocation + FVector(0,0,D.ResidentialTowerHeight);
  if (!D.FuelStationName.IsEmpty())
  {
   auto* Fuel = Place<AFlyingCabFuelStation>(World,Roof,D.FuelStationName,TEXT("Flying Cab/Stops"),*It);
   Fuel->Configure(D.FuelStationName);
  }
  if (!D.RepairStationName.IsEmpty())
  {
   auto* Repair = Place<AFlyingCabRepairStation>(World,Roof,D.RepairStationName,TEXT("Flying Cab/Stops"),*It);
   Repair->Configure(D.RepairStationName);
  }
  const bool bLeftBay = D.DistrictId == TEXT("District.YellowProjects") || D.DistrictId == TEXT("District.NeonDocks");
  const bool bRightBay = D.DistrictId == TEXT("District.AshlineMarket") || D.DistrictId == TEXT("District.GlasswardTransit");
  if (bLeftBay || bRightBay)
  {
   auto* Spawn = Place<ATargetPoint>(World,D.StopLocation+FVector(FlyingCabSupercarData::GetBayOffsetX(D.DistrictId),0,-117),
    TEXT("A_R7 spawn - ")+D.DisplayName,TEXT("Flying Cab/Stops"),*It);
   Spawn->Tags.Add(D.DistrictId);
   Authored->SupercarSpawns.Add(Spawn);
  }
 }
 // Preserve the NPC profiles and their stable IDs; actor positions now override the roster.
 const auto Hubs = FlyingCabQuestHubData::GetQuestHubs();
 for (const auto& Hub : Hubs)
 {
  AFlyingCabQuestGiver* Npc = nullptr;
  for (TActorIterator<AFlyingCabQuestGiver> It(World); It; ++It) if(It->GetNpcId()==Hub.HubId) { Npc=*It; break; }
  if (!Npc && Hub.Profile)
  {
   Npc = Place<AFlyingCabQuestGiver>(World,Hub.WorldLocation,Hub.DisplayName,TEXT("Flying Cab/NPC"));
   Npc->ConfigureProfile(Hub.Profile);
  }
 }
 Authored->Office = Place<AFlyingCabNightshiftOffice>(World,FVector(50000,0,650),TEXT("Nightshift Office"),TEXT("Flying Cab/Office"));
 Authored->Entrance = Place<AFlyingCabOnFootPortal>(World,FVector(-9600,0,10120),TEXT("Office entrance"),TEXT("Flying Cab/Office"));
 Authored->Exit = Place<AFlyingCabOnFootPortal>(World,Authored->Office->GetExitPortalLocation(),TEXT("Office exit"),TEXT("Flying Cab/Office"),Authored->Office);
 Authored->Terminal = Place<AFlyingCabAccessTerminal>(World,Authored->Office->GetTerminalLocation(),TEXT("Service access terminal"),TEXT("Flying Cab/Office"),Authored->Office);
 auto* InteriorArrival = Place<ATargetPoint>(World,Authored->Office->GetEntryLocation(),TEXT("Office arrival"),TEXT("Flying Cab/Office"),Authored->Office);
 auto* ExteriorArrival = Place<ATargetPoint>(World,FVector(-9450,0,10068),TEXT("City return position"),TEXT("Flying Cab/Office"),Authored->Entrance);
 Authored->Entrance->Configure(TEXT("NIGHTSHIFT OFFICE"),FText::FromString(TEXT("Q // ENTER NIGHTSHIFT OFFICE")),InteriorArrival->GetActorLocation(),FLinearColor(.8f,.08f,1.f));
 Authored->Entrance->DestinationActor = InteriorArrival;
 Authored->Exit->Configure(TEXT("CITY PLATFORM"),FText::FromString(TEXT("Q // RETURN TO CITY")),ExteriorArrival->GetActorLocation(),FLinearColor(.05f,.78f,1.f));
 Authored->Exit->DestinationActor = ExteriorArrival;
 Authored->ServiceVehicleSpawn = Place<ATargetPoint>(World,FVector(-10600,0,10050),TEXT("Service cab spawn"),TEXT("Flying Cab/Vehicles"));
 auto* Profile = UFlyingCabLivingWorldProfile::LoadDefaultAsset();
 const auto Definitions = Profile ? Profile->Routes : UFlyingCabLivingWorldProfile::BuildCityRoutes();
 for (const auto& Definition : Definitions)
 {
  auto* Route = Place<AFlyingCabLivingRoute>(World,FVector::ZeroVector,Definition.RouteId.ToString(),TEXT("Flying Cab/Routes"));
  Route->Configure(Definition);
 }
 // Replace transient material instances with saved colour assets for editor reload and cooking.
 TMap<FString, UMaterialInstanceConstant*> Palette;
 for (TActorIterator<AActor> It(World); It; ++It)
 {
  if (!It->GetFolderPath().ToString().StartsWith(TEXT("Flying Cab/"))) continue;
  TArray<UStaticMeshComponent*> Meshes;
  It->GetComponents(Meshes);
  for (auto* Mesh : Meshes)
  {
   auto* Dynamic = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0));
   if (!Dynamic) continue;
   FLinearColor Color;
   if (!Dynamic->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Color")),Color)) continue;
   const FString Key = FString::Printf(TEXT("M_City_%08X"),GetTypeHash(Color));
   auto*& Material = Palette.FindOrAdd(Key);
   if (!Material)
   {
    const FString PackageName = TEXT("/Game/World/Materials/")+Key;
    UPackage* Package = CreatePackage(*PackageName);
    Material = FindObject<UMaterialInstanceConstant>(Package,*Key);
    if (!Material) Material = NewObject<UMaterialInstanceConstant>(Package,*Key,RF_Public|RF_Standalone);
    UMaterialInterface* Parent = Dynamic->Parent;
    while (auto* ParentDynamic = Cast<UMaterialInstanceDynamic>(Parent)) Parent = ParentDynamic->Parent;
    Material->SetParentEditorOnly(Parent);
    Material->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Color")),Color);
    Material->PostEditChange();
    FAssetRegistryModule::AssetCreated(Material);
    Package->MarkPackageDirty();
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public|RF_Standalone;
    if (!UPackage::SavePackage(Package,Material,*FPackageName::LongPackageNameToFilename(PackageName,FPackageName::GetAssetPackageExtension()),Args)) return false;
   }
   Mesh->SetMaterial(0,Material);
  }
 }
 World->MarkPackageDirty();
 return true;
}
