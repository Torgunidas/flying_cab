// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabNightshiftOffice.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AFlyingCabNightshiftOffice::AFlyingCabNightshiftOffice()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	Floor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Floor"));
	Ceiling = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ceiling"));
	LeftWall = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftWall"));
	RightWall = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightWall"));
	BackWall = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackWall"));
	Counter = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Counter"));

	for (UStaticMeshComponent* Surface : {Floor, Ceiling, LeftWall, RightWall, BackWall, Counter})
	{
		Surface->SetupAttachment(SceneRoot);
		Surface->SetMobility(EComponentMobility::Movable);
		Surface->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Surface->SetCollisionObjectType(ECC_WorldStatic);
		Surface->SetCollisionResponseToAllChannels(ECR_Block);
		if (CubeMesh.Succeeded())
		{
			Surface->SetStaticMesh(CubeMesh.Object);
		}
		if (BasicMaterial.Succeeded())
		{
			Surface->SetMaterial(0, BasicMaterial.Object);
		}
	}

	Floor->SetRelativeLocation(FVector::ZeroVector);
	Floor->SetRelativeScale3D(FVector(14.0f, 4.0f, 0.30f));
	Ceiling->SetRelativeLocation(FVector(0.0f, 0.0f, 760.0f));
	Ceiling->SetRelativeScale3D(FVector(14.0f, 4.0f, 0.24f));
	LeftWall->SetRelativeLocation(FVector(-700.0f, 0.0f, 380.0f));
	LeftWall->SetRelativeScale3D(FVector(0.24f, 4.0f, 7.6f));
	RightWall->SetRelativeLocation(FVector(700.0f, 0.0f, 380.0f));
	RightWall->SetRelativeScale3D(FVector(0.24f, 4.0f, 7.6f));
	BackWall->SetRelativeLocation(FVector(0.0f, -185.0f, 380.0f));
	BackWall->SetRelativeScale3D(FVector(14.0f, 0.24f, 7.6f));
	Counter->SetRelativeLocation(FVector(320.0f, -20.0f, 65.0f));
	Counter->SetRelativeScale3D(FVector(3.4f, 1.2f, 1.0f));

	for (UStaticMeshComponent* DarkSurface : {Floor, Ceiling, LeftWall, RightWall, BackWall})
	{
		DarkSurface->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.025f, 0.035f, 0.065f));
	}
	Counter->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.16f, 0.025f, 0.22f));

	OfficeLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("OfficeLabel"));
	OfficeLabel->SetupAttachment(SceneRoot);
	OfficeLabel->SetRelativeLocation(FVector(180.0f, 25.0f, 610.0f));
	OfficeLabel->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	OfficeLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	OfficeLabel->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	OfficeLabel->SetWorldSize(46.0f);
	OfficeLabel->SetTextRenderColor(FColor(210, 70, 255));
	OfficeLabel->SetText(FText::FromString(TEXT("NIGHTSHIFT OFFICE\nLICENSING AFTER DARK")));

	CyanLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CyanLight"));
	CyanLight->SetupAttachment(SceneRoot);
	CyanLight->SetRelativeLocation(FVector(-360.0f, 90.0f, 430.0f));
	CyanLight->SetLightColor(FLinearColor(0.02f, 0.75f, 1.0f));
	CyanLight->SetIntensity(2600.0f);
	CyanLight->SetAttenuationRadius(850.0f);
	CyanLight->SetCastShadows(false);

	MagentaLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MagentaLight"));
	MagentaLight->SetupAttachment(SceneRoot);
	MagentaLight->SetRelativeLocation(FVector(420.0f, 80.0f, 330.0f));
	MagentaLight->SetLightColor(FLinearColor(0.80f, 0.05f, 1.0f));
	MagentaLight->SetIntensity(2200.0f);
	MagentaLight->SetAttenuationRadius(750.0f);
	MagentaLight->SetCastShadows(false);
}

FVector AFlyingCabNightshiftOffice::GetEntryLocation() const
{
	return GetActorLocation() + FVector(-430.0f, 0.0f, 83.0f);
}

FVector AFlyingCabNightshiftOffice::GetExitPortalLocation() const
{
	return GetActorLocation() + FVector(-570.0f, 0.0f, 140.0f);
}

FVector AFlyingCabNightshiftOffice::GetTerminalLocation() const
{
	return GetActorLocation() + FVector(250.0f, 0.0f, 110.0f);
}
