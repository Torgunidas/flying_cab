#include "FlyingCabAuthoredWorld.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "EngineUtils.h"

AFlyingCabWorldGeometry::AFlyingCabWorldGeometry()
{
 PrimaryActorTick.bCanEverTick = false;
 Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Geometry"));
 SetRootComponent(Mesh);
 Mesh->SetMobility(EComponentMobility::Movable);
 Mesh->SetGenerateOverlapEvents(false);
 Windows = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Windows"));
 Windows->SetupAttachment(Mesh);
 Windows->SetMobility(EComponentMobility::Movable);
 Windows->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Windows->SetGenerateOverlapEvents(false);
 Windows->SetCastShadow(false);
 Tags.Add(TEXT("FlyingCab.CityGeometry"));
}
void AFlyingCabWorldGeometry::ApplyColor()
{
 const FVector Value(Color.R, Color.G, Color.B);
 Mesh->SetVectorParameterValueOnMaterials(TEXT("Color"), Value);
 Windows->SetVectorParameterValueOnMaterials(TEXT("Color"), Value);
}
void AFlyingCabWorldGeometry::OnConstruction(const FTransform& Transform)
{
 Super::OnConstruction(Transform);
 ApplyColor();
}
void AFlyingCabWorldGeometry::BeginPlay()
{
 Super::BeginPlay();
 ApplyColor();
}
AFlyingCabDistrictAnchor::AFlyingCabDistrictAnchor()
{
 SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("TaxiStop")));
 PrimaryActorTick.bCanEverTick = false;
}
FFlyingCabDistrictDefinition AFlyingCabDistrictAnchor::GetWorldDefinition() const
{
 FFlyingCabDistrictDefinition Result = Definition;
 Result.StopLocation = GetActorLocation();
 return Result;
}
AFlyingCabAuthoredWorld::AFlyingCabAuthoredWorld()
{
 SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("World")));
 PrimaryActorTick.bCanEverTick = false;
}
AFlyingCabAuthoredWorld* AFlyingCabAuthoredWorld::Find(const UWorld* World)
{
 if (World) for (TActorIterator<AFlyingCabAuthoredWorld> It(const_cast<UWorld*>(World)); It; ++It) return *It;
 return nullptr;
}
