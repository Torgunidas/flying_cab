// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabRepairStation.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "FlyingCabGameMode.h"
#include "FlyingCabPawn.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabRepair, Log, All);

namespace
{
	constexpr float IdleLightIntensity = 1800.0f;
}

AFlyingCabRepairStation::AFlyingCabRepairStation()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	ServiceZone = CreateDefaultSubobject<UBoxComponent>(TEXT("ServiceZone"));
	SetRootComponent(ServiceZone);
	ServiceZone->InitBoxExtent(FVector(320.0f, 150.0f, 175.0f));
	ServiceZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ServiceZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	ServiceZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ServiceZone->SetGenerateOverlapEvents(true);
	ServiceZone->OnComponentBeginOverlap.AddDynamic(
		this,
		&AFlyingCabRepairStation::HandleServiceZoneBeginOverlap);
	ServiceZone->OnComponentEndOverlap.AddDynamic(
		this,
		&AFlyingCabRepairStation::HandleServiceZoneEndOverlap);

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
		Marker->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.75f, 0.10f, 1.0f));
	}

	ServiceBase->SetRelativeLocation(FVector(0.0f, 85.0f, -170.0f));
	ServiceBase->SetRelativeScale3D(FVector(4.0f, 0.12f, 0.08f));
	ServicePillar->SetRelativeLocation(FVector(0.0f, 85.0f, 0.0f));
	ServicePillar->SetRelativeScale3D(FVector(0.10f, 0.12f, 3.4f));

	ServiceLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ServiceLabel"));
	ServiceLabel->SetupAttachment(ServiceZone);
	ServiceLabel->SetRelativeLocation(FVector(0.0f, 85.0f, 220.0f));
	ServiceLabel->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	ServiceLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	ServiceLabel->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	ServiceLabel->SetWorldSize(34.0f);
	ServiceLabel->SetTextRenderColor(FColor(205, 70, 255));
	ServiceLabel->SetText(FText::FromString(ServiceName));

	ServiceLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ServiceLight"));
	ServiceLight->SetupAttachment(ServiceZone);
	ServiceLight->SetRelativeLocation(FVector(0.0f, 90.0f, 0.0f));
	ServiceLight->SetLightColor(FLinearColor(0.75f, 0.10f, 1.0f));
	ServiceLight->SetIntensity(IdleLightIntensity);
	ServiceLight->SetAttenuationRadius(650.0f);
	ServiceLight->SetCastShadows(false);
}

void AFlyingCabRepairStation::BeginPlay()
{
	Super::BeginPlay();

	// Catch actors that were already inside when the runtime station spawned.
	TArray<AActor*> InitialOverlaps;
	ServiceZone->GetOverlappingActors(InitialOverlaps, AFlyingCabPawn::StaticClass());
	for (AActor* Actor : InitialOverlaps)
	{
		if (AFlyingCabPawn* Pawn = Cast<AFlyingCabPawn>(Actor))
		{
			OverlappingPawns.Add(Pawn);
		}
	}
	RefreshTickState();
}

void AFlyingCabRepairStation::Configure(const FString& InServiceName, int32 InPricePerUnit)
{
	if (!InServiceName.IsEmpty())
	{
		ServiceName = InServiceName;
	}
	RepairPricePerHullUnit = FMath::Max(1, InPricePerUnit);
	if (ServiceLabel)
	{
		ServiceLabel->SetText(FText::FromString(ServiceName));
	}
}

void AFlyingCabRepairStation::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AFlyingCabPawn* EligiblePawn = nullptr;
	for (auto It = OverlappingPawns.CreateIterator(); It; ++It)
	{
		AFlyingCabPawn* Pawn = It->Get();
		if (!Pawn)
		{
			It.RemoveCurrent();
			continue;
		}
		if (!Pawn->IsPlayerControlled() || Pawn->IsDestroyed())
		{
			continue;
		}

		const FVector Velocity = Pawn->GetVelocity();
		if (FVector2D(Velocity.X, Velocity.Z).Size() <= RepairMaxPlanarSpeed)
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
		ResetServiceState();
		RefreshTickState();
		return;
	}

	const bool bNeedsRepair = EligiblePawn->GetHullNeeded() > UE_SMALL_NUMBER;
	EligiblePawn->SetRepairAvailable(bNeedsRepair, RepairPricePerHullUnit);
	if (!bNeedsRepair)
	{
		RepairUnitAccumulator = 0.0f;
		ServiceLabel->SetText(FText::FromString(TEXT("HULL FULL")));
		ServiceLight->SetIntensity(IdleLightIntensity);
		return;
	}

	ServiceLabel->SetText(FText::FromString(FString::Printf(
		TEXT("HOLD E // %d CR PER HULL"),
		RepairPricePerHullUnit)));

	const bool bRepairing = EligiblePawn->IsRepairRequested();
	ServiceLight->SetIntensity(
		bRepairing
			? IdleLightIntensity + 1100.0f + 700.0f * FMath::Sin(GetWorld()->GetTimeSeconds() * 18.0f)
			: IdleLightIntensity);
	if (!bRepairing)
	{
		RepairUnitAccumulator = 0.0f;
		return;
	}

	RepairUnitAccumulator += DeltaSeconds * RepairHullPerSecond;
	const int32 RequestedUnits = FMath::FloorToInt(RepairUnitAccumulator);
	if (RequestedUnits <= 0)
	{
		return;
	}
	RepairUnitAccumulator -= RequestedUnits;

	AFlyingCabGameMode* GameMode = GetWorld()->GetAuthGameMode<AFlyingCabGameMode>();
	const int32 RepairedUnits = GameMode
		? GameMode->TryPurchaseRepair(EligiblePawn, RequestedUnits, RepairPricePerHullUnit)
		: 0;
	if (RepairedUnits > 0)
	{
		UE_LOG(
			LogFlyingCabRepair,
			Verbose,
			TEXT("Repaired %d hull units at %d credits each."),
			RepairedUnits,
			RepairPricePerHullUnit);
	}
	else
	{
		RepairUnitAccumulator = 0.0f;
	}
}

void AFlyingCabRepairStation::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearContextPawn();
	OverlappingPawns.Reset();
	Super::EndPlay(EndPlayReason);
}

void AFlyingCabRepairStation::HandleServiceZoneBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (AFlyingCabPawn* Pawn = Cast<AFlyingCabPawn>(OtherActor))
	{
		OverlappingPawns.Add(Pawn);
		SetActorTickEnabled(true);
	}
}

void AFlyingCabRepairStation::HandleServiceZoneEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex)
{
	if (AFlyingCabPawn* Pawn = Cast<AFlyingCabPawn>(OtherActor))
	{
		OverlappingPawns.Remove(Pawn);
		if (ContextPawn == Pawn)
		{
			ClearContextPawn();
		}
		RefreshTickState();
	}
}

void AFlyingCabRepairStation::ClearContextPawn()
{
	if (AFlyingCabPawn* Pawn = ContextPawn.Get())
	{
		Pawn->SetRepairAvailable(false, RepairPricePerHullUnit);
	}
	ContextPawn.Reset();
}

void AFlyingCabRepairStation::ResetServiceState()
{
	RepairUnitAccumulator = 0.0f;
	ServiceLabel->SetText(FText::FromString(ServiceName));
	ServiceLight->SetIntensity(IdleLightIntensity);
}

void AFlyingCabRepairStation::RefreshTickState()
{
	for (auto It = OverlappingPawns.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}
	const bool bHasNearbyPawn = !OverlappingPawns.IsEmpty();
	SetActorTickEnabled(bHasNearbyPawn);
	if (!bHasNearbyPawn)
	{
		ClearContextPawn();
		ResetServiceState();
	}
}
