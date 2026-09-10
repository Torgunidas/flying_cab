// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabCityExpansion.h"
#include "FlyingCabAuthoredWorld.h"
#include "Engine/TextRenderActor.h"

#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "FlyingCabCityData.h"
#include "FlyingCabSupercarData.h"
#include "FlyingCabQuestHubData.h"
#include "FlyingCabTrafficSignals.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabCityExpansion, Log, All);

AFlyingCabCityExpansion::AFlyingCabCityExpansion()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = .2f;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CubeMesh = CubeFinder.Object;
	BasicMaterial = MaterialFinder.Object;
}

void AFlyingCabCityExpansion::BeginPlay()
{
	Super::BeginPlay();
	if (!bBakedToLevel) BuildExpansionGeometry();
}

#if WITH_EDITOR
void AFlyingCabCityExpansion::BakeToLevel()
{
 if (bBakedToLevel) return;
 bBaking = true;
 BuildExpansionGeometry();
 bBaking = false;
 BakeParent = nullptr;
 bBakedToLevel = true;
 MarkPackageDirty();
}
#endif

void AFlyingCabCityExpansion::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!SignalLabel) return;
	const double Time = GetWorld()->GetTimeSeconds();
	const bool H = FlyingCabTrafficSignals::IsGreen(TEXT("Horizontal"),Time);
	const bool V = FlyingCabTrafficSignals::IsGreen(TEXT("Vertical"),Time);
	SignalLabel->SetText(FText::FromString(H ? TEXT("E-W  GO  //  N-S  WAIT")
		: V ? TEXT("E-W  WAIT  //  N-S  GO") : TEXT("CLEARING CROSSING // WAIT")));
	SignalLabel->SetTextRenderColor(H || V ? FColor(80,240,180) : FColor(255,110,70));
}

void AFlyingCabCityExpansion::BuildExpansionGeometry()
{
	const FLinearColor Structure(.025f,.045f,.075f);
	const FLinearColor Lane(.035f,.18f,.22f);
	const FVector2D Min = FlyingCabCityData::GetMinimapWorldMin();
	const FVector2D Max = FlyingCabCityData::GetMinimapWorldMax();
	const FVector2D Mid = (Min + Max) * .5;
	const FVector2D Size = Max - Min;
	AddBlock(TEXT("MetroFloor"), FVector(Mid.X,0,Min.Y-50), FVector(Size.X/100,12,1), Structure);
	AddBlock(TEXT("MetroCeiling"), FVector(Mid.X,0,Max.Y+50), FVector(Size.X/100,12,1), Structure);
	AddBlock(TEXT("MetroWestBoundary"), FVector(Min.X-50,0,Mid.Y), FVector(1,12,Size.Y/100), Structure);
	AddBlock(TEXT("MetroEastBoundary"), FVector(Max.X+50,0,Mid.Y), FVector(1,12,Size.Y/100), Structure);

	// Lane markings are behind the flight plane and never collide with traffic or the cab.
	for (const FFlyingCabHighwayStrip& Strip : FlyingCabCityData::GetHighwayStrips())
	{
		AddBlock(Strip.Name, FVector(Strip.Center.X,-520,Strip.Center.Y),
			FVector(Strip.HalfSize.X/50, .08, Strip.HalfSize.Y/50), Lane, false);
	}
	// Regular luminous dashes give speed and scale cues along long empty corridors.
	for (double X=Min.X+2200; X<Max.X-1500; X+=1400)
	{
		for (double Z : {Mid.Y, Min.Y+1250, Max.Y-1250})
			AddBlock(TEXT("LaneDash"), FVector(X,-480,Z), FVector(2.8,.1,.12), FLinearColor(.08f,.45f,.55f),false);
	}
	for (double Z=Min.Y+2200; Z<Max.Y-1500; Z+=1400)
	{
		for (double X : {Mid.X, Min.X+1250, Max.X-1250})
			AddBlock(TEXT("LaneDashVertical"), FVector(X,-480,Z), FVector(.12,.1,2.8), FLinearColor(.08f,.45f,.55f),false);
	}
	if (!bBaking)
 {
	ResidentialWindows = NewObject<UInstancedStaticMeshComponent>(this,TEXT("ResidentialWindows"));
	ResidentialWindows->SetupAttachment(SceneRoot);
	ResidentialWindows->SetStaticMesh(CubeMesh);
	ResidentialWindows->SetMaterial(0,BasicMaterial);
	ResidentialWindows->SetMobility(EComponentMobility::Movable);
	ResidentialWindows->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ResidentialWindows->SetGenerateOverlapEvents(false);
	ResidentialWindows->SetCastShadow(false);
	AddInstanceComponent(ResidentialWindows);
	ResidentialWindows->RegisterComponent();
	ResidentialWindows->SetVectorParameterValueOnMaterials(TEXT("Color"),FVector(.85,.65,.24));
 }
 for (const FFlyingCabDistrictDefinition& District : FlyingCabCityData::GetDistricts())
	{
#if WITH_EDITOR
  if (bBaking)
  {
   auto* Anchor = GetWorld()->SpawnActor<AFlyingCabDistrictAnchor>(District.StopLocation, FRotator::ZeroRotator);
   Anchor->Definition = District;
   Anchor->SetActorLabel(TEXT("STOP - ")+District.DisplayName);
   Anchor->SetFolderPath(TEXT("Flying Cab/Stops"));
   BakeParent = Anchor;
   auto* WindowActor = GetWorld()->SpawnActor<AFlyingCabWorldGeometry>();
   WindowActor->SetActorLabel(TEXT("Windows - ")+District.DisplayName);
   WindowActor->SetFolderPath(TEXT("Flying Cab/Stops"));
   WindowActor->AttachToActor(Anchor,FAttachmentTransformRules::KeepWorldTransform);
   WindowActor->Color = FLinearColor(.85f,.65f,.24f);
   ResidentialWindows = WindowActor->Windows;
   ResidentialWindows->SetStaticMesh(CubeMesh);
   ResidentialWindows->SetMaterial(0,BasicMaterial);
   WindowActor->ApplyColor();
  }
#endif
  const FVector Stop = District.StopLocation;
		const float Height = District.ResidentialTowerHeight;
		const FString Code = District.MinimapCode;
		AddBlock(TEXT("CurbsidePlatform")+District.MinimapCode,
			Stop-FVector(0,0,190), FVector(FMath::Max(District.RuntimePlatformHalfWidth,22.f),4.8,.8), District.AccentColor);
		if (FlyingCabSupercarData::GetDistrictIds().Contains(District.DistrictId))
		{
			// A connected bay beyond the passenger curb leaves pickup/dropoff space clear.
			const FVector Bay = Stop + FVector(FlyingCabSupercarData::GetBayOffsetX(District.DistrictId),0,0);
			AddBlock(TEXT("SupercarBay")+Code, Bay-FVector(0,0,190), FVector(5.2,4.8,.8), District.AccentColor);
		}
		// Shallow solid foundations add readable silhouettes without closing approach lanes.
		AddBlock(TEXT("CurbsideFoundation")+District.MinimapCode,
			Stop-FVector(0,0,280), FVector(15,4.4,1), Structure);
		// This is a real obstacle in the flight plane, not a background facade.
		AddBlock(TEXT("ResidentialTower")+Code,Stop+FVector(0,0,Height*.5f-150),
			FVector(8.5,4.8,Height/100),Structure+District.AccentColor*.13f);
		AddBlock(TEXT("ResidentialRoofTrim")+Code,Stop+FVector(0,246,Height-155),
			FVector(8.5,.06,.12),District.AccentColor,false);
		AddDistrictLabel(District.DisplayName,Stop+FVector(0,260,Height*.45f),District.AccentColor);
		for (float Z=0; Z<Height-220; Z+=115)
		{
			if (FMath::Abs(Z-Height*.45f)<60) continue; // Reserve a clean strip for the block name.
			for (double X : {-300.,-150.,0.,150.,300.})
			{
				ResidentialWindows->AddInstance(FTransform(FRotator::ZeroRotator,
					Stop+FVector(X,245,Z),FVector(.60,.045,.40)));
			}
		}
		for (bool bRight : {false,true})
		{
			const FVector Curb = bRight ? FlyingCabCityData::GetPassengerDropoffLocation(Stop)
				: FlyingCabCityData::GetPassengerPickupLocation(Stop);
			const FLinearColor CurbColor = bRight ? FLinearColor(1.f,.18f,.04f) : FLinearColor(0.f,.85f,1.f);
			const FString Side = bRight ? TEXT("Right") : TEXT("Left");
			AddBlock(TEXT("ResidentialCurb")+Code+Side,Curb+FVector(0,247,-155),FVector(5.2,.05,.12),CurbColor,false);
			AddDistrictLabel(bRight ? TEXT("DROPOFF") : TEXT("PICKUP"),Curb+FVector(0,260,-95),CurbColor);
			AddBlock(TEXT("ResidentialDoor")+Code+Side,Stop+FVector(bRight ? 350 : -350,247,-65),
				FVector(.9,.04,1.65),FLinearColor(.02f,.06f,.08f),false);
			// Named, non-interactive anchors for a later interior/foot-quest stage.
			auto* Entrance = NewObject<USceneComponent>(bBaking ? BakeParent : this,FName(*(TEXT("HomeEntry_")+Code+TEXT("_")+Side)));
			Entrance->SetupAttachment(bBaking ? BakeParent->GetRootComponent() : SceneRoot.Get());
			Entrance->SetRelativeLocation(FlyingCabCityData::GetResidentialEntranceLocation(District,bRight) - (bBaking ? Stop : FVector::ZeroVector));
			Entrance->ComponentTags = {TEXT("FutureResidentialInterior"),District.DistrictId};
			(bBaking ? BakeParent : this)->AddInstanceComponent(Entrance);
			Entrance->RegisterComponent();
		}
		if (!District.FuelStationName.IsEmpty() || !District.RepairStationName.IsEmpty())
		{
			const FLinearColor ServiceColor = !District.FuelStationName.IsEmpty()
				? FLinearColor(.05f,1.f,.3f) : FLinearColor(.75f,.1f,1.f);
			AddBlock(TEXT("ResidentialServiceRoof")+Code,FlyingCabCityData::GetDistrictServiceLocation(District)+FVector(0,0,-148),
				FVector(7.8,4.8,.04),ServiceColor,false);
		}
	}
	BakeParent = nullptr;
	for (const FFlyingCabNeighborhoodDefinition& Estate : FlyingCabCityData::GetNeighborhoods())
	{
		const FVector C = Estate.Center;
		AddBlock(TEXT("TransitTerrace")+Estate.NeighborhoodId.ToString(), C-FVector(0,0,290), FVector(43,5,.8), Estate.Color);
		AddDistrictLabel(TEXT("NPC TRANSIT // ")+Estate.DisplayName, C+FVector(0,-300,-150), Estate.Color);
		// Building facades remain in the background: pedestrians disappear through their doors.
		for (double Offset : {-1800.,1800.})
		{
			AddBlock(TEXT("TransitBuilding"), C+FVector(Offset,-350,0), FVector(4,1,5), Estate.Color*.45f, false);
			AddBlock(TEXT("TransitDoor"), C+FVector(Offset,-280,-140), FVector(.9,.1,1.8), Estate.Color,false);
		}
		AddDistrictLabel(Estate.DisplayName, C+FVector(0,-350,1300), Estate.Color);
	}
	// Dedicated quest plazas: no passenger offer, destination or service trigger on these pads.
	for (const FFlyingCabQuestHubDefinition& Hub : FlyingCabQuestHubData::GetQuestHubs(GetWorld()))
	{
		AddBlock(TEXT("QuestPlaza")+Hub.DisplayName, Hub.WorldLocation-FVector(0,0,110),
			FVector(20,6,1), FLinearColor(.12f,.15f,.2f));
		AddDistrictLabel(Hub.DisplayName+TEXT(" // DISPATCH"), Hub.WorldLocation+FVector(0,-200,200), FLinearColor::White);
	}
	AddDistrictLabel(TEXT("CROSS CENTRAL // SIGNAL CONTROL"), FVector(Mid.X,-300,Mid.Y+800), FLinearColor(.7f,.8f,.85f));
	if (!bBaking) SignalLabel = RuntimeLabels.Last();
	UE_LOG(LogFlyingCabCityExpansion, Display, TEXT("Metro city built: %.0f x %.0f cm, %d neighborhoods, %d taxi stops."),
		Size.X, Size.Y, FlyingCabCityData::GetNeighborhoods().Num(), FlyingCabCityData::GetDistricts().Num());
}

void AFlyingCabCityExpansion::AddBlock(
	const FString& Name,
	const FVector& Location,
	const FVector& Scale,
	const FLinearColor& Color,
	bool bCollisionEnabled)
{
	if (!CubeMesh)
	{
		return;
	}

#if WITH_EDITOR
 if (bBaking)
 {
  auto* Piece = GetWorld()->SpawnActor<AFlyingCabWorldGeometry>(Location,FRotator::ZeroRotator);
  Piece->SetActorLabel(Name);
  Piece->SetFolderPath(BakeParent ? TEXT("Flying Cab/Stops") : TEXT("Flying Cab/City"));
  Piece->Mesh->SetStaticMesh(CubeMesh);
  Piece->Mesh->SetMaterial(0,BasicMaterial);
  Piece->Mesh->SetCollisionProfileName(bCollisionEnabled ? TEXT("BlockAll") : TEXT("NoCollision"));
  Piece->Color = Color;
  Piece->ApplyColor();
  Piece->SetActorScale3D(Scale);
  if (BakeParent) Piece->AttachToActor(BakeParent,FAttachmentTransformRules::KeepWorldTransform);
  return;
 }
#endif
 UStaticMeshComponent* Block = NewObject<UStaticMeshComponent>(
		this,
		MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), FName(*Name)));
	Block->SetupAttachment(SceneRoot);
	Block->SetStaticMesh(CubeMesh);
	Block->SetMobility(EComponentMobility::Movable);
	Block->SetCollisionProfileName(bCollisionEnabled ? TEXT("BlockAll") : TEXT("NoCollision"));
	Block->SetGenerateOverlapEvents(false);
	Block->SetRelativeLocation(Location);
	Block->SetRelativeScale3D(Scale);
	if (BasicMaterial)
	{
		Block->SetMaterial(0, BasicMaterial);
	}
	AddInstanceComponent(Block);
	Block->RegisterComponent();
	Block->SetVectorParameterValueOnMaterials(
		TEXT("Color"),
		FVector(Color.R, Color.G, Color.B));
	RuntimeBlocks.Add(Block);
}

void AFlyingCabCityExpansion::AddDistrictLabel(
	const FString& Name,
	const FVector& Location,
	const FLinearColor& Color)
{
#if WITH_EDITOR
 if (bBaking)
 {
  auto* TextActor = GetWorld()->SpawnActor<ATextRenderActor>(Location,FRotator(0,90,0));
  TextActor->SetActorLabel(Name);
  TextActor->SetFolderPath(BakeParent ? TEXT("Flying Cab/Stops") : TEXT("Flying Cab/City"));
  auto* Text = TextActor->GetTextRender();
  Text->SetMobility(EComponentMobility::Movable);
  Text->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
  Text->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
  Text->SetWorldSize(48);
  Text->SetTextRenderColor(Color.ToFColor(true));
  Text->SetText(FText::FromString(Name));
  if (BakeParent) TextActor->AttachToActor(BakeParent,FAttachmentTransformRules::KeepWorldTransform);
  if (Name == TEXT("CROSS CENTRAL // SIGNAL CONTROL")) SignalLabel = Text;
  return;
 }
#endif
 UTextRenderComponent* Label = NewObject<UTextRenderComponent>(
		this,
		MakeUniqueObjectName(this, UTextRenderComponent::StaticClass(), FName(*Name)));
	Label->SetupAttachment(SceneRoot);
	Label->SetMobility(EComponentMobility::Movable);
	Label->SetRelativeLocation(Location);
	Label->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	Label->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	Label->SetWorldSize(48.0f);
	Label->SetTextRenderColor(Color.ToFColor(true));
	Label->SetText(FText::FromString(Name));
	AddInstanceComponent(Label);
	Label->RegisterComponent();
	RuntimeLabels.Add(Label);
}
