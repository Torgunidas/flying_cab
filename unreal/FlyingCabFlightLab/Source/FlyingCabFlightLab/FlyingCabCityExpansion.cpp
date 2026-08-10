// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabCityExpansion.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabCityExpansion, Log, All);

AFlyingCabCityExpansion::AFlyingCabCityExpansion()
{
	PrimaryActorTick.bCanEverTick = false;

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
	OpenExistingEasternBoundary();
	BuildExpansionGeometry();
}

void AFlyingCabCityExpansion::OpenExistingEasternBoundary()
{
	int32 OpenedBoundaries = 0;
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		AStaticMeshActor* MeshActor = *It;
		if (!MeshActor)
		{
			continue;
		}

		FVector BoundsOrigin = FVector::ZeroVector;
		FVector BoundsExtent = FVector::ZeroVector;
		MeshActor->GetActorBounds(false, BoundsOrigin, BoundsExtent);
		bool bIsEasternBoundary = FMath::Abs(BoundsOrigin.X - 4950.0f) <= 350.0f
			&& BoundsExtent.X <= 350.0f
			&& BoundsExtent.Z >= 2200.0f;
#if WITH_EDITOR
		bIsEasternBoundary = bIsEasternBoundary
			|| MeshActor->GetActorLabel().Equals(TEXT("Arena_RightBoundary"));
#endif
		if (!bIsEasternBoundary)
		{
			continue;
		}

		if (UStaticMeshComponent* Mesh = MeshActor->GetStaticMeshComponent())
		{
			Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Mesh->SetVisibility(false, true);
			++OpenedBoundaries;
		}
	}

	if (OpenedBoundaries > 0)
	{
		UE_LOG(
			LogFlyingCabCityExpansion,
			Display,
			TEXT("Opened %d existing eastern arena boundary component(s)."),
			OpenedBoundaries);
	}
	else
	{
		UE_LOG(
			LogFlyingCabCityExpansion,
			Warning,
			TEXT("Existing eastern arena boundary was not found."));
	}
}

void AFlyingCabCityExpansion::BuildExpansionGeometry()
{
	const FLinearColor Structure(0.025f, 0.045f, 0.075f);
	const FLinearColor Roadbed(0.035f, 0.075f, 0.095f);
	const FLinearColor Cyan(0.02f, 0.62f, 0.82f);
	const FLinearColor Amber(0.92f, 0.32f, 0.04f);
	const FLinearColor Magenta(0.72f, 0.06f, 0.70f);
	const FLinearColor Green(0.08f, 0.62f, 0.32f);

	// The original arena spans roughly -5000..5000. This adds another 10000 cm.
	AddBlock(TEXT("EastFloor"), FVector(10000.0f, 0.0f, -50.0f), FVector(100.0f, 6.0f, 1.0f), Structure);
	AddBlock(TEXT("EastCeiling"), FVector(10000.0f, 0.0f, 6550.0f), FVector(100.0f, 6.0f, 1.0f), Structure);
	AddBlock(TEXT("NewEastBoundary"), FVector(15000.0f, 0.0f, 3250.0f), FVector(1.0f, 6.0f, 65.0f), Structure);

	// Ground and ceiling silhouettes establish readable districts without closing flight lanes.
	AddBlock(TEXT("GlasswardBase"), FVector(5650.0f, 0.0f, 330.0f), FVector(9.0f, 5.2f, 6.6f), Cyan);
	AddBlock(TEXT("RainlineBase"), FVector(8050.0f, 0.0f, 470.0f), FVector(11.0f, 5.2f, 9.4f), Amber);
	AddBlock(TEXT("CobaltBase"), FVector(10600.0f, 0.0f, 620.0f), FVector(10.0f, 5.2f, 12.4f), Magenta);
	AddBlock(TEXT("OrbitalBase"), FVector(13900.0f, 0.0f, 430.0f), FVector(12.0f, 5.2f, 8.6f), Green);
	AddBlock(TEXT("GlasswardCanopy"), FVector(7100.0f, 0.0f, 6200.0f), FVector(10.0f, 5.2f, 7.0f), Cyan);
	AddBlock(TEXT("CobaltCanopy"), FVector(10850.0f, 0.0f, 6050.0f), FVector(13.0f, 5.2f, 10.0f), Magenta);
	AddBlock(TEXT("OrbitalCanopy"), FVector(13750.0f, 0.0f, 6250.0f), FVector(9.0f, 5.2f, 6.0f), Green);

	// Curbside platforms for the four new passenger districts.
	AddBlock(TEXT("PlatformGlassward"), FVector(6500.0f, 0.0f, 960.0f), FVector(15.0f, 4.8f, 0.8f), Cyan);
	AddBlock(TEXT("PlatformRainline"), FVector(8650.0f, 0.0f, 2510.0f), FVector(16.0f, 4.8f, 0.8f), Amber);
	AddBlock(TEXT("PlatformCobalt"), FVector(11150.0f, 0.0f, 3760.0f), FVector(16.0f, 4.8f, 0.8f), Magenta);
	AddBlock(TEXT("PlatformOrbital"), FVector(13250.0f, 0.0f, 5260.0f), FVector(17.0f, 4.8f, 0.8f), Green);

	// A few narrow bridges make the extension a navigable space rather than an empty box.
	AddBlock(TEXT("RainlineBridge"), FVector(7450.0f, 0.0f, 3450.0f), FVector(7.0f, 4.6f, 0.65f), Roadbed);
	AddBlock(TEXT("CobaltBridge"), FVector(10150.0f, 0.0f, 4850.0f), FVector(7.5f, 4.6f, 0.65f), Roadbed);
	AddBlock(TEXT("OrbitalBridge"), FVector(12450.0f, 0.0f, 2100.0f), FVector(7.0f, 4.6f, 0.65f), Roadbed);

	AddDistrictLabel(TEXT("GLASSWARD TRANSIT"), FVector(6500.0f, 80.0f, 1320.0f), Cyan);
	AddDistrictLabel(TEXT("RAINLINE BAZAAR"), FVector(8650.0f, 80.0f, 2870.0f), Amber);
	AddDistrictLabel(TEXT("COBALT HEIGHTS"), FVector(11150.0f, 80.0f, 4120.0f), Magenta);
	AddDistrictLabel(TEXT("ORBITAL GARDENS"), FVector(13250.0f, 80.0f, 5620.0f), Green);

	UE_LOG(
		LogFlyingCabCityExpansion,
		Display,
		TEXT("East city extension built with %d blocks and %d district labels; city width is now approximately 20000 cm."),
		RuntimeBlocks.Num(),
		RuntimeLabels.Num());
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
