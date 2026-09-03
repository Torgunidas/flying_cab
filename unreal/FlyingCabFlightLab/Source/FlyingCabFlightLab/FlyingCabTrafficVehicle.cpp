// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabTrafficVehicle.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "FlyingCabLivingPedestrian.h"
#include "FlyingCabLivingRoute.h"
#include "FlyingCabPawn.h"
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
	CurrentSpeed = CruiseSpeed;
	RouteDistance = FMath::Clamp(InitialRouteAlpha, 0.0f, 0.999f) * RouteLength;
	LivingRoute = nullptr;
	NextLivingNodeIndex = INDEX_NONE;
	MovementState = EFlyingCabTrafficMovementState::Cruising;

	SetActorLocation(
		RouteStart + RouteDirection * RouteDistance,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	VisualMesh->SetVectorParameterValueOnMaterials(
		TEXT("Color"),
		FVector(VehicleColor.R, VehicleColor.G, VehicleColor.B));
	RunningLight->SetLightColor(VehicleColor);
	ResetRoutePresentation();
	UpdateRoutePresentation(0.0f);
}

void AFlyingCabTrafficVehicle::ConfigureLivingRoute(
	AFlyingCabLivingRoute* InRoute,
	float InitialRouteAlpha,
	const FLinearColor& VehicleColor)
{
	LivingRoute = InRoute;
	if (!LivingRoute)
	{
		RouteLength = 0.0f;
		CurrentSpeed = 0.0f;
		return;
	}

	RouteLength = LivingRoute->GetRouteLength();
	CruiseSpeed = FMath::Max(0.0f, LivingRoute->GetCruiseSpeed());
	RouteDistance = LivingRoute->NormalizeDistance(
		FMath::Clamp(InitialRouteAlpha, 0.0f, 0.999f) * RouteLength);
	CurrentSpeed = 0.0f;
	DwellRemaining = 0.0f;
	MovementState = EFlyingCabTrafficMovementState::Cruising;
	NextLivingNodeIndex = LivingRoute->FindNodeAtOrAhead(RouteDistance, 2.0f);

	CollisionBody->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionBody->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	SetActorLocation(
		LivingRoute->GetWorldLocationAtDistance(RouteDistance),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	VisualMesh->SetVectorParameterValueOnMaterials(
		TEXT("Color"),
		FVector(VehicleColor.R, VehicleColor.G, VehicleColor.B));
	RunningLight->SetLightColor(VehicleColor);
	ResetRoutePresentation();
	UpdateRoutePresentation(0.0f);
}

FName AFlyingCabTrafficVehicle::GetLivingRouteId() const
{
	return LivingRoute ? LivingRoute->GetRouteId() : NAME_None;
}

void AFlyingCabTrafficVehicle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	NearMissCooldownRemaining = FMath::Max(0.0f, NearMissCooldownRemaining - DeltaSeconds);
	if (RouteLength <= UE_SMALL_NUMBER || CruiseSpeed <= UE_SMALL_NUMBER)
	{
		CurrentSpeed = 0.0f;
		return;
	}
	if (LivingRoute)
	{
		TickLivingRoute(DeltaSeconds);
	}
	else
	{
		TickLegacyRoute(DeltaSeconds);
	}
	UpdateRoutePresentation(DeltaSeconds);
	UpdateNearMissTracking(TrackedPawn);
}

void AFlyingCabTrafficVehicle::TickLegacyRoute(float DeltaSeconds)
{
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
		CurrentSpeed = 0.0f;
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
		CurrentSpeed = 0.0f;
	}
	else
	{
		RouteDistance = CandidateDistance;
		CurrentSpeed = CruiseSpeed;
	}
}

void AFlyingCabTrafficVehicle::TickLivingRoute(float DeltaSeconds)
{
	if (!LivingRoute || NextLivingNodeIndex == INDEX_NONE)
	{
		CurrentSpeed = 0.0f;
		return;
	}

	if (DwellRemaining > 0.0f)
	{
		DwellRemaining = FMath::Max(0.0f, DwellRemaining - DeltaSeconds);
		CurrentSpeed = 0.0f;
		MovementState = EFlyingCabTrafficMovementState::Dwelling;
		if (DwellRemaining <= 0.0f)
		{
			CurrentLivingStopId = NAME_None;
			AdvanceLivingRouteNode();
			MovementState = EFlyingCabTrafficMovementState::Cruising;
		}
		return;
	}

	const FFlyingCabLivingRouteNode* NextNode = LivingRoute->GetNode(NextLivingNodeIndex);
	if (!NextNode)
	{
		CurrentSpeed = 0.0f;
		return;
	}
	const float DistanceToNode = LivingRoute->GetForwardDistanceToNode(
		RouteDistance,
		NextLivingNodeIndex);
	const bool bStopsAtNode = IsVehicleStopAction(NextNode->Action);
	const float EffectiveDeceleration = FMath::Max(1.0f, LivingRoute->GetDeceleration());
	const float BrakingDistance = FMath::Square(CurrentSpeed) / (2.0f * EffectiveDeceleration) + 35.0f;
	const float SensorDistance = FMath::Max(
		LivingRoute->GetMinimumSpacing(),
		CurrentSpeed * 0.65f + 180.0f);
	const bool bBlocked = HasLivingRouteObstacle(SensorDistance);

	float DesiredSpeed = CruiseSpeed;
	if (bStopsAtNode && DistanceToNode <= BrakingDistance)
	{
		const float RemainingForBraking = FMath::Max(0.0f, DistanceToNode - 4.0f);
		DesiredSpeed = FMath::Min(
			DesiredSpeed,
			FMath::Sqrt(2.0f * EffectiveDeceleration * RemainingForBraking));
		MovementState = EFlyingCabTrafficMovementState::ApproachingStop;
	}
	else
	{
		MovementState = EFlyingCabTrafficMovementState::Cruising;
	}
	if (bBlocked)
	{
		DesiredSpeed = 0.0f;
		MovementState = EFlyingCabTrafficMovementState::WaitingForObstacle;
	}

	const float ChangeRate = DesiredSpeed < CurrentSpeed
		? EffectiveDeceleration
		: FMath::Max(1.0f, LivingRoute->GetAcceleration());
	CurrentSpeed = FMath::FInterpConstantTo(CurrentSpeed, DesiredSpeed, DeltaSeconds, ChangeRate);
	const float RequestedAdvance = CurrentSpeed * DeltaSeconds;
	const float Advance = FMath::Min(RequestedAdvance, DistanceToNode);
	const float CandidateDistance = LivingRoute->NormalizeDistance(RouteDistance + Advance);

	FHitResult Hit;
	SetActorLocation(
		LivingRoute->GetWorldLocationAtDistance(CandidateDistance),
		true,
		&Hit,
		ETeleportType::None);
	if (Hit.IsValidBlockingHit())
	{
		CurrentSpeed = 0.0f;
		MovementState = EFlyingCabTrafficMovementState::WaitingForObstacle;
		if (Cast<AFlyingCabPawn>(Hit.GetActor()))
		{
			bEncounterInvalidated = true;
		}
		return;
	}

	RouteDistance = CandidateDistance;
	if (DistanceToNode <= FMath::Max(4.0f, RequestedAdvance + 1.0f))
	{
		RouteDistance = LivingRoute->GetNodeDistance(NextLivingNodeIndex);
		SetActorLocation(
			LivingRoute->GetWorldLocationAtDistance(RouteDistance),
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
		ProcessLivingRouteNode();
	}
}

bool AFlyingCabTrafficVehicle::HasLivingRouteObstacle(float LookAheadDistance) const
{
	if (!LivingRoute || !GetWorld() || LookAheadDistance <= 0.0f)
	{
		return false;
	}
	const FVector Direction = LivingRoute->GetWorldDirectionAtDistance(RouteDistance).GetSafeNormal();
	const FVector Start = GetActorLocation() + Direction * 155.0f;
	const FVector End = LivingRoute->GetWorldLocationAtDistance(RouteDistance + LookAheadDistance);
	FCollisionObjectQueryParams ObjectTypes;
	ObjectTypes.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectTypes.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectTypes.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FlyingCabLivingTrafficSensor), false, this);
	TArray<FHitResult> Hits;
	if (!GetWorld()->SweepMultiByObjectType(
		Hits,
		Start,
		End,
		FQuat::Identity,
		ObjectTypes,
		FCollisionShape::MakeSphere(72.0f),
		QueryParams))
	{
		return false;
	}
	for (const FHitResult& Hit : Hits)
	{
		const AActor* HitActor = Hit.GetActor();
		const UPrimitiveComponent* HitComponent = Hit.GetComponent();
		if (Cast<AFlyingCabPawn>(HitActor)
			|| Cast<AFlyingCabTrafficVehicle>(HitActor)
			|| Cast<AFlyingCabLivingPedestrian>(HitActor)
			|| (HitComponent
				&& HitComponent->GetCollisionResponseToChannel(ECC_WorldDynamic) == ECR_Block))
		{
			return true;
		}
	}
	return false;
}

void AFlyingCabTrafficVehicle::ProcessLivingRouteNode()
{
	if (!LivingRoute)
	{
		return;
	}
	const FFlyingCabLivingRouteNode* Node = LivingRoute->GetNode(NextLivingNodeIndex);
	if (!Node)
	{
		return;
	}
	if (!Node->StopId.IsNone())
	{
		OnLivingStopReached.Broadcast(this, Node->StopId, Node->Action);
	}
	if (IsVehicleStopAction(Node->Action))
	{
		CurrentLivingStopId = Node->StopId;
		CurrentSpeed = 0.0f;
		DwellRemaining = FMath::Max(0.05f, Node->WaitDuration);
		MovementState = EFlyingCabTrafficMovementState::Dwelling;
		return;
	}
	AdvanceLivingRouteNode();
}

void AFlyingCabTrafficVehicle::AdvanceLivingRouteNode()
{
	NextLivingNodeIndex = LivingRoute
		? LivingRoute->FindNextNodeIndex(NextLivingNodeIndex)
		: INDEX_NONE;
}

float AFlyingCabTrafficVehicle::GetVisualPitchDegrees() const
{
	return VisualMesh ? VisualMesh->GetRelativeRotation().Pitch : 0.0f;
}

void AFlyingCabTrafficVehicle::UpdateRoutePresentation(float DeltaSeconds)
{
	if (LivingRoute)
	{
		RouteDirection = LivingRoute->GetWorldDirectionAtDistance(RouteDistance).GetSafeNormal();
	}

	const FVector PresentationVelocity = RouteDirection * CurrentSpeed;
	const FVector PresentationAcceleration = bHasPreviousPresentationVelocity
		&& DeltaSeconds > UE_SMALL_NUMBER
		? (PresentationVelocity - PreviousPresentationVelocity) / DeltaSeconds
		: FVector::ZeroVector;
	PreviousPresentationVelocity = PresentationVelocity;
	bHasPreviousPresentationVelocity = true;
	PresentationElapsedTime += FMath::Max(0.0f, DeltaSeconds);

	const float AccelerationRatio = VisualPitchFullAcceleration > UE_SMALL_NUMBER
		? FMath::Clamp(
			PresentationAcceleration.X / VisualPitchFullAcceleration,
			-1.0f,
			1.0f)
		: 0.0f;
	const float TargetPitch = -AccelerationRatio * MaxVisualPitchDegrees;
	const float CurrentPitch = VisualMesh->GetRelativeRotation().Pitch;
	const bool bReturningToLevel = FMath::IsNearlyZero(TargetPitch)
		|| FMath::Abs(TargetPitch) < FMath::Abs(CurrentPitch);
	const float PitchInterpSpeed = bReturningToLevel
		? VisualPitchReturnSpeed
		: VisualPitchResponseSpeed;
	const float NewPitch = FMath::FInterpTo(
		CurrentPitch,
		TargetPitch,
		DeltaSeconds,
		PitchInterpSpeed);
	VisualMesh->SetRelativeRotation(FRotator(NewPitch, 0.0f, 0.0f));

	const FVector PlaneAcceleration(
		PresentationAcceleration.X,
		0.0f,
		PresentationAcceleration.Z);
	const float MotionLagRatio = VisualMotionLagFullAcceleration > UE_SMALL_NUMBER
		? FMath::Clamp(
			PlaneAcceleration.Size() / VisualMotionLagFullAcceleration,
			0.0f,
			1.0f)
		: 0.0f;
	const FVector TargetMotionLag = PlaneAcceleration.IsNearlyZero()
		? FVector::ZeroVector
		: -PlaneAcceleration.GetSafeNormal() * MaxVisualMotionLag * MotionLagRatio;
	VisualMotionLagOffset = FMath::VInterpTo(
		VisualMotionLagOffset,
		TargetMotionLag,
		DeltaSeconds,
		VisualMotionLagResponseSpeed);
	const float HoverOffset = FMath::Sin(
		PresentationElapsedTime * HoverBobFrequency * 2.0f * UE_PI)
		* HoverBobAmplitude;
	const FVector VisualOffset = VisualMotionLagOffset + FVector(0.0f, 0.0f, HoverOffset);
	VisualMesh->SetRelativeLocation(VisualOffset);

	if (FMath::Abs(RouteDirection.X) > 0.05f)
	{
		VisualFacingDirection = FMath::Sign(RouteDirection.X);
	}
	RunningLight->SetRelativeLocation(FVector(
		VisualFacingDirection * 125.0f,
		55.0f,
		0.0f) + VisualOffset);
}

void AFlyingCabTrafficVehicle::ResetRoutePresentation()
{
	PreviousPresentationVelocity = FVector::ZeroVector;
	VisualMotionLagOffset = FVector::ZeroVector;
	bHasPreviousPresentationVelocity = false;
	VisualFacingDirection = RouteDirection.X < 0.0f ? -1.0f : 1.0f;
	PresentationElapsedTime = FMath::Fmod(FMath::Abs(RouteDistance) * 0.0013f, 10.0f);
	if (VisualMesh)
	{
		VisualMesh->SetRelativeLocation(FVector::ZeroVector);
		VisualMesh->SetRelativeRotation(FRotator::ZeroRotator);
	}
}

bool AFlyingCabTrafficVehicle::IsVehicleStopAction(EFlyingCabLivingRouteAction Action)
{
	return Action == EFlyingCabLivingRouteAction::Stop
		|| Action == EFlyingCabLivingRouteAction::Land
		|| Action == EFlyingCabLivingRouteAction::Park;
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
