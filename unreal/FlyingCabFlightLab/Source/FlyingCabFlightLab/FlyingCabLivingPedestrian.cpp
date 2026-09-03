// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabLivingPedestrian.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "FlyingCabLivingRoute.h"
#include "FlyingCabTrafficVehicle.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AFlyingCabLivingPedestrian::AFlyingCabLivingPedestrian()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionBody = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionBody"));
	SetRootComponent(CollisionBody);
	CollisionBody->InitCapsuleSize(24.0f, 58.0f);
	CollisionBody->SetMobility(EComponentMobility::Movable);
	CollisionBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBody->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBody->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBody->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionBody->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionBody->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	CollisionBody->SetGenerateOverlapEvents(false);
	CollisionBody->SetCanEverAffectNavigation(false);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(CollisionBody);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetRelativeScale3D(FVector(0.34f, 0.34f, 1.05f));

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
}

void AFlyingCabLivingPedestrian::Configure(
	AFlyingCabLivingRoute* InRoute,
	float InitialRouteAlpha,
	const FLinearColor& Color)
{
	Route = InRoute;
	if (!Route)
	{
		SetActorTickEnabled(false);
		return;
	}
	const float RouteLength = Route->GetRouteLength();
	RouteDistance = Route->NormalizeDistance(
		FMath::Clamp(InitialRouteAlpha, 0.0f, 0.999f) * RouteLength);
	NextNodeIndex = Route->FindNodeAtOrAhead(RouteDistance, 2.0f);
	CurrentSpeed = 0.0f;
	LivingState = EFlyingCabPedestrianState::Walking;
	SetActorLocation(
		Route->GetWorldLocationAtDistance(RouteDistance),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	VisualMesh->SetVectorParameterValueOnMaterials(
		TEXT("Color"),
		FVector(Color.R, Color.G, Color.B));
}

void AFlyingCabLivingPedestrian::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Route || NextNodeIndex == INDEX_NONE)
	{
		CurrentSpeed = 0.0f;
		return;
	}
	if (LivingState == EFlyingCabPedestrianState::Riding)
	{
		if (RidingVehicle)
		{
			SetActorLocation(RidingVehicle->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
		}
		return;
	}
	if (LivingState == EFlyingCabPedestrianState::Waiting)
	{
		return;
	}
	if (LivingState == EFlyingCabPedestrianState::Inside)
	{
		WaitRemaining = FMath::Max(0.0f, WaitRemaining - DeltaSeconds);
		if (WaitRemaining <= 0.0f)
		{
			LeaveBuilding();
		}
		return;
	}
	if (WaitRemaining > 0.0f)
	{
		WaitRemaining = FMath::Max(0.0f, WaitRemaining - DeltaSeconds);
		CurrentSpeed = 0.0f;
		if (WaitRemaining <= 0.0f)
		{
			AdvanceRouteNode();
		}
		return;
	}

	const float DistanceToNode = Route->GetForwardDistanceToNode(RouteDistance, NextNodeIndex);
	const float SensorDistance = FMath::Min(
		DistanceToNode,
		FMath::Max(Route->GetMinimumSpacing(), CurrentSpeed * 0.45f + 70.0f));
	const bool bBlocked = HasObstacle(SensorDistance);
	const float DesiredSpeed = bBlocked ? 0.0f : Route->GetCruiseSpeed();
	const float ChangeRate = DesiredSpeed < CurrentSpeed
		? Route->GetDeceleration()
		: Route->GetAcceleration();
	CurrentSpeed = FMath::FInterpConstantTo(CurrentSpeed, DesiredSpeed, DeltaSeconds, ChangeRate);
	LivingState = bBlocked
		? EFlyingCabPedestrianState::WaitingForObstacle
		: EFlyingCabPedestrianState::Walking;

	const float RequestedAdvance = CurrentSpeed * DeltaSeconds;
	const float Advance = FMath::Min(RequestedAdvance, DistanceToNode);
	const float CandidateDistance = Route->NormalizeDistance(RouteDistance + Advance);
	FHitResult Hit;
	SetActorLocation(
		Route->GetWorldLocationAtDistance(CandidateDistance),
		true,
		&Hit,
		ETeleportType::None);
	if (Hit.IsValidBlockingHit())
	{
		CurrentSpeed = 0.0f;
		LivingState = EFlyingCabPedestrianState::WaitingForObstacle;
		return;
	}
	RouteDistance = CandidateDistance;
	if (DistanceToNode <= FMath::Max(3.0f, RequestedAdvance + 1.0f))
	{
		RouteDistance = Route->GetNodeDistance(NextNodeIndex);
		SetActorLocation(
			Route->GetWorldLocationAtDistance(RouteDistance),
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
		ProcessRouteNode();
	}
}

bool AFlyingCabLivingPedestrian::BoardVehicle(AFlyingCabTrafficVehicle* Vehicle)
{
	if (!Vehicle || LivingState != EFlyingCabPedestrianState::Waiting
		|| WaitingStopId.IsNone() || DestinationStopId.IsNone())
	{
		return false;
	}
	RidingVehicle = Vehicle;
	LivingState = EFlyingCabPedestrianState::Riding;
	WaitingStopId = NAME_None;
	CurrentSpeed = 0.0f;
	SetAgentVisible(false);
	AttachToActor(Vehicle, FAttachmentTransformRules::KeepWorldTransform);
	return true;
}

bool AFlyingCabLivingPedestrian::CompleteRideAtStop(FName StopId)
{
	if (!Route || LivingState != EFlyingCabPedestrianState::Riding
		|| DestinationStopId != StopId)
	{
		return false;
	}
	const int32 SearchStart = Route->FindNextNodeIndex(NextNodeIndex);
	const int32 ExitNodeIndex = Route->FindNextNodeWithAction(
		SearchStart == INDEX_NONE ? 0 : SearchStart,
		EFlyingCabLivingRouteAction::ExitVehicle,
		StopId);
	if (ExitNodeIndex == INDEX_NONE)
	{
		return false;
	}

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	RidingVehicle = nullptr;
	DestinationStopId = NAME_None;
	RouteDistance = Route->GetNodeDistance(ExitNodeIndex);
	SetActorLocation(
		Route->GetWorldLocationAtDistance(RouteDistance),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	SetAgentVisible(true);
	NextNodeIndex = Route->FindNextNodeIndex(ExitNodeIndex);
	LivingState = EFlyingCabPedestrianState::Walking;
	CurrentSpeed = 0.0f;
	return NextNodeIndex != INDEX_NONE;
}

bool AFlyingCabLivingPedestrian::WantsToExitAt(FName StopId) const
{
	return LivingState == EFlyingCabPedestrianState::Riding
		&& !StopId.IsNone()
		&& DestinationStopId == StopId;
}

void AFlyingCabLivingPedestrian::ProcessRouteNode()
{
	const FFlyingCabLivingRouteNode* Node = Route ? Route->GetNode(NextNodeIndex) : nullptr;
	if (!Node)
	{
		return;
	}
	switch (Node->Action)
	{
	case EFlyingCabLivingRouteAction::BoardVehicle:
	{
		const int32 SearchStart = Route->FindNextNodeIndex(NextNodeIndex);
		const int32 ExitNodeIndex = Route->FindNextNodeWithAction(
			SearchStart == INDEX_NONE ? 0 : SearchStart,
			EFlyingCabLivingRouteAction::ExitVehicle);
		const FFlyingCabLivingRouteNode* ExitNode = Route->GetNode(ExitNodeIndex);
		if (!ExitNode || Node->StopId.IsNone() || ExitNode->StopId.IsNone())
		{
			AdvanceRouteNode();
			return;
		}
		WaitingStopId = Node->StopId;
		DestinationStopId = ExitNode->StopId;
		CurrentSpeed = 0.0f;
		LivingState = EFlyingCabPedestrianState::Waiting;
		OnWaitingForVehicle.Broadcast(this, WaitingStopId);
		return;
	}
	case EFlyingCabLivingRouteAction::EnterBuilding:
		EnterBuilding(*Node);
		return;
	case EFlyingCabLivingRouteAction::ExitBuilding:
		SetAgentVisible(true);
		BeginNodeWait(Node->WaitDuration);
		return;
	case EFlyingCabLivingRouteAction::Stop:
	case EFlyingCabLivingRouteAction::Park:
		BeginNodeWait(Node->WaitDuration);
		return;
	default:
		AdvanceRouteNode();
		return;
	}
}

void AFlyingCabLivingPedestrian::AdvanceRouteNode()
{
	NextNodeIndex = Route ? Route->FindNextNodeIndex(NextNodeIndex) : INDEX_NONE;
	LivingState = EFlyingCabPedestrianState::Walking;
}

void AFlyingCabLivingPedestrian::EnterBuilding(const FFlyingCabLivingRouteNode& Node)
{
	const int32 SearchStart = Route->FindNextNodeIndex(NextNodeIndex);
	const int32 ExitNodeIndex = Route->FindNextNodeWithAction(
		SearchStart == INDEX_NONE ? 0 : SearchStart,
		EFlyingCabLivingRouteAction::ExitBuilding);
	if (ExitNodeIndex == INDEX_NONE)
	{
		AdvanceRouteNode();
		return;
	}
	const FFlyingCabLivingRouteNode* ExitNode = Route->GetNode(ExitNodeIndex);
	ExitDoorWait = ExitNode ? FMath::Max(0.0f, ExitNode->WaitDuration) : 0.0f;
	NextNodeIndex = ExitNodeIndex;
	RouteDistance = Route->GetNodeDistance(ExitNodeIndex);
	SetActorLocation(
		Route->GetWorldLocationAtDistance(RouteDistance),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	SetAgentVisible(false);
	CurrentSpeed = 0.0f;
	WaitRemaining = FMath::Max(0.05f, Node.WaitDuration);
	LivingState = EFlyingCabPedestrianState::Inside;
}

void AFlyingCabLivingPedestrian::LeaveBuilding()
{
	SetAgentVisible(true);
	LivingState = EFlyingCabPedestrianState::Walking;
	WaitRemaining = ExitDoorWait;
	ExitDoorWait = 0.0f;
	if (WaitRemaining <= 0.0f)
	{
		AdvanceRouteNode();
	}
}

bool AFlyingCabLivingPedestrian::HasObstacle(float LookAheadDistance) const
{
	if (!Route || !GetWorld() || LookAheadDistance <= 0.0f)
	{
		return false;
	}
	const FVector Direction = Route->GetWorldDirectionAtDistance(RouteDistance).GetSafeNormal();
	const FVector Start = GetActorLocation() + Direction * 30.0f;
	const FVector End = Route->GetWorldLocationAtDistance(RouteDistance + LookAheadDistance);
	FCollisionObjectQueryParams ObjectTypes;
	ObjectTypes.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectTypes.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectTypes.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FlyingCabLivingPedestrianSensor), false, this);
	TArray<FHitResult> Hits;
	if (!GetWorld()->SweepMultiByObjectType(
		Hits,
		Start,
		End,
		FQuat::Identity,
		ObjectTypes,
		FCollisionShape::MakeSphere(20.0f),
		QueryParams))
	{
		return false;
	}
	for (const FHitResult& Hit : Hits)
	{
		const AActor* HitActor = Hit.GetActor();
		const UPrimitiveComponent* HitComponent = Hit.GetComponent();
		if (Cast<APawn>(HitActor)
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

void AFlyingCabLivingPedestrian::SetAgentVisible(bool bVisible)
{
	SetActorHiddenInGame(!bVisible);
	CollisionBody->SetCollisionEnabled(
		bVisible ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
}

void AFlyingCabLivingPedestrian::BeginNodeWait(float Duration)
{
	CurrentSpeed = 0.0f;
	WaitRemaining = FMath::Max(0.0f, Duration);
	if (WaitRemaining <= 0.0f)
	{
		AdvanceRouteNode();
	}
}
