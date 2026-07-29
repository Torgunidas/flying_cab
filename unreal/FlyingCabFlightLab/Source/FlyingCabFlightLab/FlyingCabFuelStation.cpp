// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabFuelStation.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "FlyingCabGameMode.h"
#include "FlyingCabPawn.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabFuel, Log, All);

namespace
{
	constexpr float IdleLightIntensity = 1700.0f;
}

AFlyingCabFuelStation::AFlyingCabFuelStation()
{
	PrimaryActorTick.bCanEverTick = true;

	ServiceZone = CreateDefaultSubobject<UBoxComponent>(TEXT("ServiceZone"));
	SetRootComponent(ServiceZone);
	ServiceZone->InitBoxExtent(FVector(300.0f, 150.0f, 175.0f));
	ServiceZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ServiceZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	ServiceZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ServiceZone->SetGenerateOverlapEvents(true);

	ServiceBase = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ServiceBase"));
	ServicePillar = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ServicePillar"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	for (UStaticMeshComponent* Marker : {ServiceBase, ServicePillar})
	{
		Marker->SetupAttachment(ServiceZone);
		Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Marker->SetMobility(EComponentMobility::Movable);
		if (CubeMesh.Succeeded())
		{
			Marker->SetStaticMesh(CubeMesh.Object);
		}
		if (BasicMaterial.Succeeded())
		{
			Marker->SetMaterial(0, BasicMaterial.Object);
		}
		Marker->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.05f, 1.0f, 0.30f));
	}

	ServiceBase->SetRelativeLocation(FVector(0.0f, 85.0f, -170.0f));
	ServiceBase->SetRelativeScale3D(FVector(3.5f, 0.12f, 0.08f));
	ServicePillar->SetRelativeLocation(FVector(-270.0f, 85.0f, 0.0f));
	ServicePillar->SetRelativeScale3D(FVector(0.10f, 0.12f, 3.4f));

	ServiceLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ServiceLabel"));
	ServiceLabel->SetupAttachment(ServiceZone);
	ServiceLabel->SetRelativeLocation(FVector(0.0f, 85.0f, 220.0f));
	ServiceLabel->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	ServiceLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	ServiceLabel->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	ServiceLabel->SetWorldSize(34.0f);
	ServiceLabel->SetTextRenderColor(FColor(45, 255, 105));
	ServiceLabel->SetText(FText::FromString(ServiceName));

	ServiceLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ServiceLight"));
	ServiceLight->SetupAttachment(ServiceZone);
	ServiceLight->SetRelativeLocation(FVector(0.0f, 90.0f, 0.0f));
	ServiceLight->SetLightColor(FLinearColor(0.05f, 1.0f, 0.30f));
	ServiceLight->SetIntensity(IdleLightIntensity);
	ServiceLight->SetAttenuationRadius(600.0f);
	ServiceLight->SetCastShadows(false);
}

void AFlyingCabFuelStation::Configure(const FString& InServiceName)
{
	ServiceName = InServiceName.IsEmpty() ? FString(TEXT("FUEL SERVICE")) : InServiceName;
	if (ServiceLabel)
	{
		ServiceLabel->SetText(FText::FromString(ServiceName));
	}
}

void AFlyingCabFuelStation::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AFlyingCabPawn* EligiblePawn = nullptr;
	TArray<AActor*> OverlappingActors;
	ServiceZone->GetOverlappingActors(OverlappingActors, AFlyingCabPawn::StaticClass());
	for (AActor* Actor : OverlappingActors)
	{
		AFlyingCabPawn* Pawn = Cast<AFlyingCabPawn>(Actor);
		if (!Pawn || Pawn->IsDestroyed())
		{
			continue;
		}

		const FVector Velocity = Pawn->GetVelocity();
		const float PlanarSpeed = FVector2D(Velocity.X, Velocity.Z).Size();
		if (PlanarSpeed <= RefuelMaxPlanarSpeed)
		{
			EligiblePawn = Pawn;
			break;
		}
	}

	if (ContextPawn.Get() != EligiblePawn)
	{
		ClearContextPawn();
		ContextPawn = EligiblePawn;
	}

	if (!EligiblePawn)
	{
		RefuelUnitAccumulator = 0.0f;
		ServiceLabel->SetText(FText::FromString(ServiceName));
		ServiceLight->SetIntensity(IdleLightIntensity);
		return;
	}

	const bool bNeedsFuel = EligiblePawn->GetFuelNeeded() > UE_SMALL_NUMBER;
	EligiblePawn->SetRefuelAvailable(bNeedsFuel, FuelPricePerUnit);
	if (!bNeedsFuel)
	{
		RefuelUnitAccumulator = 0.0f;
		ServiceLabel->SetText(FText::FromString(TEXT("TANK FULL")));
		ServiceLight->SetIntensity(IdleLightIntensity);
		return;
	}

	ServiceLabel->SetText(FText::FromString(FString::Printf(
		TEXT("HOLD E // %d CR PER UNIT"),
		FuelPricePerUnit)));

	const bool bRefueling = EligiblePawn->IsRefuelRequested();
	ServiceLight->SetIntensity(
		bRefueling
			? IdleLightIntensity + 1000.0f + 700.0f * FMath::Sin(GetWorld()->GetTimeSeconds() * 20.0f)
			: IdleLightIntensity);
	if (!bRefueling)
	{
		RefuelUnitAccumulator = 0.0f;
		return;
	}

	RefuelUnitAccumulator += DeltaSeconds * RefuelUnitsPerSecond;
	const int32 RequestedUnits = FMath::FloorToInt(RefuelUnitAccumulator);
	if (RequestedUnits <= 0)
	{
		return;
	}
	RefuelUnitAccumulator -= RequestedUnits;

	AFlyingCabGameMode* GameMode = GetWorld()->GetAuthGameMode<AFlyingCabGameMode>();
	const int32 PurchasedUnits = GameMode
		? GameMode->TryPurchaseFuel(EligiblePawn, RequestedUnits, FuelPricePerUnit)
		: 0;
	if (PurchasedUnits > 0)
	{
		UE_LOG(
			LogFlyingCabFuel,
			Verbose,
			TEXT("Purchased %d fuel units at %d credits each."),
			PurchasedUnits,
			FuelPricePerUnit);
	}
	else
	{
		RefuelUnitAccumulator = 0.0f;
	}
}

void AFlyingCabFuelStation::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearContextPawn();
	Super::EndPlay(EndPlayReason);
}

void AFlyingCabFuelStation::ClearContextPawn()
{
	if (AFlyingCabPawn* Pawn = ContextPawn.Get())
	{
		Pawn->SetRefuelAvailable(false, FuelPricePerUnit);
	}
	ContextPawn.Reset();
}
