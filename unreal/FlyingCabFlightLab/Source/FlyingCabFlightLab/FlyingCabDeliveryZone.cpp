// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabDeliveryZone.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "FlyingCabPawn.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float BaseZoneLightIntensity = 1800.0f;
	constexpr float PassengerStartX = -175.0f;
	constexpr float PassengerPathLimit = 235.0f;
	constexpr float PassengerVisualY = 70.0f;
}

AFlyingCabDeliveryZone::AFlyingCabDeliveryZone()
{
	PrimaryActorTick.bCanEverTick = true;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->InitBoxExtent(FVector(260.0f, 150.0f, 180.0f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);

	MarkerBase = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerBase"));
	MarkerLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerLeft"));
	MarkerRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerRight"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	for (UStaticMeshComponent* Marker : {MarkerBase, MarkerLeft, MarkerRight})
	{
		Marker->SetupAttachment(TriggerBox);
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
	}

	MarkerBase->SetRelativeLocation(FVector(0.0f, 0.0f, -172.0f));
	MarkerBase->SetRelativeScale3D(FVector(5.2f, 0.18f, 0.12f));
	MarkerLeft->SetRelativeLocation(FVector(-254.0f, 0.0f, 0.0f));
	MarkerLeft->SetRelativeScale3D(FVector(0.12f, 0.18f, 3.6f));
	MarkerRight->SetRelativeLocation(FVector(254.0f, 0.0f, 0.0f));
	MarkerRight->SetRelativeScale3D(FVector(0.12f, 0.18f, 3.6f));

	PassengerBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PassengerBody"));
	PassengerBody->SetupAttachment(TriggerBox);
	PassengerBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PassengerBody->SetMobility(EComponentMobility::Movable);
	PassengerBody->SetRelativeScale3D(FVector(0.18f, 0.18f, 0.70f));
	if (CylinderMesh.Succeeded())
	{
		PassengerBody->SetStaticMesh(CylinderMesh.Object);
	}
	if (BasicMaterial.Succeeded())
	{
		PassengerBody->SetMaterial(0, BasicMaterial.Object);
	}

	PassengerHead = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PassengerHead"));
	PassengerHead->SetupAttachment(TriggerBox);
	PassengerHead->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PassengerHead->SetMobility(EComponentMobility::Movable);
	PassengerHead->SetRelativeScale3D(FVector(0.22f));
	if (SphereMesh.Succeeded())
	{
		PassengerHead->SetStaticMesh(SphereMesh.Object);
	}
	if (BasicMaterial.Succeeded())
	{
		PassengerHead->SetMaterial(0, BasicMaterial.Object);
	}

	ZoneLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ZoneLabel"));
	ZoneLabel->SetupAttachment(TriggerBox);
	ZoneLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 225.0f));
	ZoneLabel->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	ZoneLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	ZoneLabel->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	ZoneLabel->SetWorldSize(44.0f);
	ZoneLabel->SetMobility(EComponentMobility::Movable);

	ZoneLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ZoneLight"));
	ZoneLight->SetupAttachment(TriggerBox);
	ZoneLight->SetRelativeLocation(FVector(0.0f, 80.0f, 0.0f));
	ZoneLight->SetIntensity(BaseZoneLightIntensity);
	ZoneLight->SetAttenuationRadius(650.0f);
	ZoneLight->SetCastShadows(false);
	ZoneLight->SetMobility(EComponentMobility::Movable);

	ApplyZoneAppearance();
	UpdateConfirmationVisuals();
}

void AFlyingCabDeliveryZone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bZoneActive || bTriggered)
	{
		return;
	}

	AFlyingCabPawn* ReadyPawn = nullptr;
	TArray<AActor*> OverlappingActors;
	TriggerBox->GetOverlappingActors(OverlappingActors, AFlyingCabPawn::StaticClass());
	for (AActor* OverlappingActor : OverlappingActors)
	{
		AFlyingCabPawn* Pawn = Cast<AFlyingCabPawn>(OverlappingActor);
		if (!Pawn)
		{
			continue;
		}

		const FVector Velocity = Pawn->GetVelocity();
		const float PlanarSpeed = FVector2D(Velocity.X, Velocity.Z).Size();
		if (PlanarSpeed <= ArrivalMaxPlanarSpeed)
		{
			ReadyPawn = Pawn;
			break;
		}
	}

	if (!ReadyPawn)
	{
		ResetConfirmation();
		return;
	}

	if (ConfirmationDuration <= UE_SMALL_NUMBER)
	{
		bTriggered = true;
		OnCabReady.Broadcast(this);
		return;
	}

	if (!bConfirmationInProgress)
	{
		CapturePassengerPath(ReadyPawn);
	}
	bConfirmationInProgress = true;
	ConfirmationElapsed = FMath::Min(ConfirmationElapsed + DeltaSeconds, ConfirmationDuration);
	UpdateConfirmationVisuals();
	if (ConfirmationElapsed >= ConfirmationDuration)
	{
		bTriggered = true;
		OnCabReady.Broadcast(this);
	}
}

void AFlyingCabDeliveryZone::Configure(
	EFlyingCabDeliveryZoneType InZoneType,
	float InArrivalMaxPlanarSpeed,
	float InConfirmationDuration)
{
	ZoneType = InZoneType;
	ArrivalMaxPlanarSpeed = FMath::Max(0.0f, InArrivalMaxPlanarSpeed);
	ConfirmationDuration = FMath::Max(0.0f, InConfirmationDuration);
	ResetConfirmation();
	ApplyZoneAppearance();
}

void AFlyingCabDeliveryZone::SetZoneActive(bool bNewActive)
{
	bZoneActive = bNewActive;
	bTriggered = false;
	ResetConfirmation();
	TriggerBox->SetCollisionEnabled(bZoneActive ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	MarkerBase->SetVisibility(bZoneActive, true);
	MarkerLeft->SetVisibility(bZoneActive, true);
	MarkerRight->SetVisibility(bZoneActive, true);
	ZoneLabel->SetVisibility(bZoneActive, true);
	ZoneLight->SetVisibility(bZoneActive, true);
	SetActorTickEnabled(bZoneActive);
	UpdateConfirmationVisuals();
}

bool AFlyingCabDeliveryZone::IsPawnInside(const AFlyingCabPawn* Pawn) const
{
	return bZoneActive && Pawn && TriggerBox->IsOverlappingActor(Pawn);
}

float AFlyingCabDeliveryZone::GetConfirmationAlpha() const
{
	return ConfirmationDuration > UE_SMALL_NUMBER
		? FMath::Clamp(ConfirmationElapsed / ConfirmationDuration, 0.0f, 1.0f)
		: 1.0f;
}

void AFlyingCabDeliveryZone::ApplyZoneAppearance()
{
	const bool bIsPickup = ZoneType == EFlyingCabDeliveryZoneType::Pickup;
	const FLinearColor ZoneColor = bIsPickup
		? FLinearColor(0.0f, 0.85f, 1.0f)
		: FLinearColor(1.0f, 0.18f, 0.04f);
	const FColor TextColor = ZoneColor.ToFColor(true);

	ZoneLabel->SetText(FText::FromString(bIsPickup ? TEXT("PICKUP") : TEXT("DROPOFF")));
	ZoneLabel->SetTextRenderColor(TextColor);
	ZoneLight->SetLightColor(ZoneColor);

	for (UStaticMeshComponent* Marker : {MarkerBase, MarkerLeft, MarkerRight})
	{
		Marker->SetVectorParameterValueOnMaterials(
			TEXT("Color"),
			FVector(ZoneColor.R, ZoneColor.G, ZoneColor.B));
	}

	for (UStaticMeshComponent* PassengerPart : {PassengerBody, PassengerHead})
	{
		PassengerPart->SetVectorParameterValueOnMaterials(
			TEXT("Color"),
			FVector(ZoneColor.R, ZoneColor.G, ZoneColor.B));
	}
	UpdateConfirmationVisuals();
}

void AFlyingCabDeliveryZone::ResetConfirmation()
{
	if (ConfirmationElapsed <= 0.0f && !bConfirmationInProgress)
	{
		return;
	}

	ConfirmationElapsed = 0.0f;
	bConfirmationInProgress = false;
	UpdateConfirmationVisuals();
}

void AFlyingCabDeliveryZone::UpdateConfirmationVisuals()
{
	if (!ZoneLabel || !ZoneLight || !PassengerBody || !PassengerHead)
	{
		return;
	}

	const bool bIsPickup = ZoneType == EFlyingCabDeliveryZoneType::Pickup;
	const float Alpha = GetConfirmationAlpha();
	if (bConfirmationInProgress)
	{
		const TCHAR* SequenceLabel = bIsPickup ? TEXT("LINK") : TEXT("EXIT");
		ZoneLabel->SetText(FText::FromString(FString::Printf(
			TEXT("%s %d%%"),
			SequenceLabel,
			FMath::RoundToInt(Alpha * 100.0f))));
		const float Pulse = 0.5f + 0.5f * FMath::Sin(ConfirmationElapsed * 30.0f);
		ZoneLight->SetIntensity(BaseZoneLightIntensity + Pulse * 1400.0f);
	}
	else
	{
		ZoneLabel->SetText(FText::FromString(bIsPickup ? TEXT("PICKUP") : TEXT("DROPOFF")));
		ZoneLight->SetIntensity(BaseZoneLightIntensity);
	}

	const bool bShowPassenger = bZoneActive && (bIsPickup || bConfirmationInProgress);
	PassengerBody->SetVisibility(bShowPassenger, true);
	PassengerHead->SetVisibility(bShowPassenger, true);

	const float PassengerX = bIsPickup
		? FMath::Lerp(PassengerStartX, PassengerCabX, Alpha)
		: FMath::Lerp(PassengerCabX, PassengerExitX, Alpha);
	PassengerBody->SetRelativeLocation(FVector(PassengerX, PassengerVisualY, -120.0f));
	PassengerHead->SetRelativeLocation(FVector(PassengerX, PassengerVisualY, -67.0f));
}

void AFlyingCabDeliveryZone::CapturePassengerPath(const AFlyingCabPawn* Pawn)
{
	if (!Pawn)
	{
		PassengerCabX = 0.0f;
		PassengerExitX = PassengerPathLimit;
		return;
	}

	PassengerCabX = FMath::Clamp(
		Pawn->GetActorLocation().X - GetActorLocation().X,
		-PassengerPathLimit,
		PassengerPathLimit);
	PassengerExitX = PassengerCabX < 0.0f ? -PassengerPathLimit : PassengerPathLimit;
}
