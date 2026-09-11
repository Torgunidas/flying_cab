#include "FlyingCabHighwayTile.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "FlyingCabCityData.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AFlyingCabHighwayTile::AFlyingCabHighwayTile()
{
 PrimaryActorTick.bCanEverTick = false;
 Zone = CreateDefaultSubobject<UBoxComponent>(TEXT("TurboZone"));
 SetRootComponent(Zone);
 Zone->SetBoxExtent(FVector(1400,200,450));
 Zone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Zone->SetGenerateOverlapEvents(false);
 Zone->ShapeColor = FColor(195,255,65);
 Lane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Lane"));
 Lane->SetupAttachment(Zone);
 Markings = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Markings"));
 Markings->SetupAttachment(Zone);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
 for (UStaticMeshComponent* Mesh : {Lane.Get(), static_cast<UStaticMeshComponent*>(Markings.Get())})
 {
  Mesh->SetStaticMesh(Cube.Object);
  Mesh->SetMaterial(0,Material.Object);
  Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Mesh->SetGenerateOverlapEvents(false);
  Mesh->SetCastShadow(false);
 }
}

void AFlyingCabHighwayTile::OnConstruction(const FTransform& Transform)
{
 Super::OnConstruction(Transform);
 const float L = FMath::Max(100.f,Length), W = FMath::Max(100.f,Width);
 Zone->SetBoxExtent(FVector(L*.5f,FMath::Max(10.f,Depth)*.5f,W*.5f));
 // Visuals sit behind the flight plane; the actor origin is the gameplay centre (Y=0).
 Lane->SetRelativeLocation(FVector(0,-520,0));
 Lane->SetRelativeScale3D(FVector(L/100.f,.08f,W/100.f));
 Lane->SetVectorParameterValueOnMaterials(TEXT("Color"),FVector(.025,.09,.12));
 Markings->ClearInstances();
 const int32 Count = FMath::Clamp(FMath::RoundToInt(L/700.f),1,256);
 const float Step = L/Count;
 for (int32 I=0; I<Count; ++I)
  Markings->AddInstance(FTransform(FQuat::Identity,FVector(-L*.5f+Step*(I+.5f),-480,0),FVector(Step*.4f/100.f,.1f,.12f)));
 Markings->SetVectorParameterValueOnMaterials(TEXT("Color"),FVector(.08,.45,.55));
}

bool AFlyingCabHighwayTile::ContainsLocation(const FVector& Location) const
{
 if (!bEnabled || IsActorBeingDestroyed() || GetActorScale3D().GetAbs().GetMin() < UE_SMALL_NUMBER) return false;
 const FVector P = Zone->GetComponentTransform().InverseTransformPosition(Location).GetAbs();
 const FVector E = Zone->GetUnscaledBoxExtent();
 return P.X <= E.X && P.Y <= E.Y && P.Z <= E.Z;
}

bool AFlyingCabHighwayTile::GetBonuses(UWorld* World, const FVector& Location, float LegacySpeed,
 float LegacyFuel, float& OutSpeed, float& OutFuel)
{
 bool bFound = FlyingCabCityData::IsOnHighway(Location);
 OutSpeed = bFound ? FMath::Clamp(LegacySpeed,1.f,2.f) : 1.f;
 OutFuel = bFound ? FMath::Clamp(LegacyFuel,.1f,1.f) : 1.f;
 if (World)
  for (TActorIterator<AFlyingCabHighwayTile> It(World); It; ++It)
   if (It->ContainsLocation(Location))
   {
    bFound = true;
    OutSpeed = FMath::Max(OutSpeed,FMath::Clamp(It->SpeedMultiplier,1.f,2.f));
    OutFuel = FMath::Min(OutFuel,FMath::Clamp(It->FuelConsumptionMultiplier,.1f,1.f));
   }
 return bFound;
}
