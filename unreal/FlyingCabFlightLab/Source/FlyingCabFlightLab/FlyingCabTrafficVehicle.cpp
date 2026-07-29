// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabTrafficVehicle.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "FlyingCabPawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AFlyingCabTrafficVehicle::AFlyingCabTrafficVehicle()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionBody = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBody"));
	SetRootComponent(CollisionBody);
	CollisionBody->InitBoxExtent(FVector(130.0f, 50.0f, 42.0f));
	CollisionBody->SetMobility(EComponentMobility::Movable);
	CollisionBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBody->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBody->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBody->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	CollisionBody->SetGenerateOverlapEvents(false);
	CollisionBody->SetCanEverAffectNavigation(false);
	CollisionBody->OnComponentHit.AddDynamic(this, &AFlyingCabTrafficVehicle::HandleCollisionHit);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(CollisionBody);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetMobility(EComponentMobility::Movable);
	VisualMesh->SetRelativeScale3D(FVector(2.6f, 1.0f, 0.84f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (CubeMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CubeMesh.Object);
	}
	if (BasicMaterial.Succeeded())
	{
		VisualMesh->SetMaterial(0, BasicMaterial.Object);
	}

	RunningLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("RunningLight"));
	RunningLight->SetupAttachment(CollisionBody);
	RunningLight->SetRelativeLocation(FVector(125.0f, 55.0f, 0.0f));
	RunningLight->SetIntensity(1300.0f);
	RunningLight->SetAttenuationRadius(380.0f);
	RunningLight->SetCastShadows(false);
}

void AFlyingCabTrafficVehicle::Configure(
	const FVector& InRouteStart,
	const FVector& InRouteEnd,
	float InCruiseSpeed,
	float InitialRouteAlpha,
	const FLinearColor& VehicleColor)
{
	RouteStart = InRouteStart;
	RouteEnd = InRouteEnd;
	const FVector RouteDelta = RouteEnd - RouteStart;
	RouteLength = RouteDelta.Size();
	RouteDirection = RouteLength > UE_SMALL_NUMBER
		? RouteDelta / RouteLength
		: FVector::ForwardVector;
	CruiseSpeed = FMath::Max(0.0f, InCruiseSpeed);
	RouteDistance = FMath::Clamp(InitialRouteAlpha, 0.0f, 0.999f) * RouteLength;

	SetActorLocation(
		RouteStart + RouteDirection * RouteDistance,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	VisualMesh->SetVectorParameterValueOnMaterials(
		TEXT("Color"),
		FVector(VehicleColor.R, VehicleColor.G, VehicleColor.B));
	RunningLight->SetLightColor(VehicleColor);
	RunningLight->SetRelativeLocation(FVector(
		RouteDirection.X >= 0.0f ? 125.0f : -125.0f,
		55.0f,
		0.0f));
}

void AFlyingCabTrafficVehicle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	NearMissCooldownRemaining = FMath::Max(0.0f, NearMissCooldownRemaining - DeltaSeconds);
	if (RouteLength <= UE_SMALL_NUMBER || CruiseSpeed <= UE_SMALL_NUMBER)
	{
		return;
	}

	float CandidateDistance = RouteDistance + CruiseSpeed * DeltaSeconds;
	if (CandidateDistance >= RouteLength)
	{
		ResetNearMissTracking();
		CandidateDistance = FMath::Fmod(CandidateDistance, RouteLength);
		RouteDistance = CandidateDistance;
		SetActorLocation(
			RouteStart + RouteDirection * RouteDistance,
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
		return;
	}

	FHitResult Hit;
	SetActorLocation(
		RouteStart + RouteDirection * CandidateDistance,
		true,
		&Hit,
		ETeleportType::None);
	if (Hit.IsValidBlockingHit())
	{
		if (Cast<AFlyingCabPawn>(Hit.GetActor()))
		{
			bEncounterInvalidated = true;
		}
		RouteDistance = FMath::Clamp(
			FVector::DotProduct(GetActorLocation() - RouteStart, RouteDirection),
			0.0f,
			RouteLength);
	}
	else
	{
		RouteDistance = CandidateDistance;
	}

	UpdateNearMissTracking(Cast<AFlyingCabPawn>(UGameplayStatics::GetPlayerPawn(this, 0)));
}

void AFlyingCabTrafficVehicle::UpdateNearMissTracking(AFlyingCabPawn* Pawn)
{
	if (!Pawn || Pawn->IsDestroyed() || NearMissCooldownRemaining > 0.0f)
	{
		ResetNearMissTracking();
		return;
	}

	const float HorizontalSeparation = Pawn->GetActorLocation().X - GetActorLocation().X;
	const float AbsoluteHorizontalSeparation = FMath::Abs(HorizontalSeparation);
	const float VerticalSeparation = FMath::Abs(Pawn->GetActorLocation().Z - GetActorLocation().Z);
	const float RelativeSpeed = FMath::Abs(Pawn->GetVelocity().X - GetTrafficVelocity().X);

	if (!bTrackingNearMiss)
	{
		if (AbsoluteHorizontalSeparation > NearMissDetectionHalfWidth
			|| VerticalSeparation > NearMissMaximumVerticalSeparation)
		{
			return;
		}

		bTrackingNearMiss = true;
		bEncounterInvalidated = false;
		PreviousEncounterHorizontalSeparation = HorizontalSeparation;
		EncounterMinimumVerticalSeparation = VerticalSeparation;
		EncounterMaximumRelativeSpeed = RelativeSpeed;
		return;
	}

	EncounterMinimumVerticalSeparation = FMath::Min(
		EncounterMinimumVerticalSeparation,
		VerticalSeparation);
	EncounterMaximumRelativeSpeed = FMath::Max(EncounterMaximumRelativeSpeed, RelativeSpeed);

	const bool bCrossed = (PreviousEncounterHorizontalSeparation < 0.0f && HorizontalSeparation >= 0.0f)
		|| (PreviousEncounterHorizontalSeparation > 0.0f && HorizontalSeparation <= 0.0f);
	const bool bLeftEncounter = AbsoluteHorizontalSeparation > NearMissDetectionHalfWidth * 1.25f;
	PreviousEncounterHorizontalSeparation = HorizontalSeparation;

	if (!bCrossed && !bLeftEncounter)
	{
		return;
	}

	const bool bQualified = bCrossed
		&& !bEncounterInvalidated
		&& EncounterMinimumVerticalSeparation >= NearMissMinimumVerticalSeparation
		&& EncounterMinimumVerticalSeparation <= NearMissMaximumVerticalSeparation
		&& EncounterMaximumRelativeSpeed >= NearMissMinimumRelativeSpeed;
	if (bQualified)
	{
		OnNearMiss.Broadcast(this, Pawn);
		NearMissCooldownRemaining = NearMissCooldown;
	}
	ResetNearMissTracking();
}

void AFlyingCabTrafficVehicle::ResetNearMissTracking()
{
	bTrackingNearMiss = false;
	bEncounterInvalidated = false;
	PreviousEncounterHorizontalSeparation = 0.0f;
	EncounterMinimumVerticalSeparation = TNumericLimits<float>::Max();
	EncounterMaximumRelativeSpeed = 0.0f;
}

void AFlyingCabTrafficVehicle::HandleCollisionHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (Cast<AFlyingCabPawn>(OtherActor))
	{
		bEncounterInvalidated = true;
	}
}
