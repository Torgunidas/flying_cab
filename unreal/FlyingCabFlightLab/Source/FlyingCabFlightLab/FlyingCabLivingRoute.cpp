// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabLivingRoute.h"

#include "Components/SplineComponent.h"

AFlyingCabLivingRoute::AFlyingCabLivingRoute()
{
	PrimaryActorTick.bCanEverTick = false;
	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("RouteSpline"));
	SetRootComponent(Spline);
	Spline->SetMobility(EComponentMobility::Movable);
	Spline->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RouteNodes = {
		FFlyingCabLivingRouteNode(),
		FFlyingCabLivingRouteNode()};
	RouteNodes[1].LocalLocation = FVector(1000.0f, 0.0f, 0.0f);
}

void AFlyingCabLivingRoute::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildSpline();
}

void AFlyingCabLivingRoute::Configure(const FFlyingCabLivingRouteDefinition& Definition)
{
	RouteId = Definition.RouteId;
	AgentKind = Definition.AgentKind;
	RouteClass = Definition.RouteClass;
	bClosedLoop = Definition.bClosedLoop;
	bSmoothVehicleCorners = Definition.bSmoothVehicleCorners;
	VehicleCornerSmoothingDistance = Definition.VehicleCornerSmoothingDistance;
	CruiseSpeed = Definition.CruiseSpeed;
	Acceleration = Definition.Acceleration;
	Deceleration = Definition.Deceleration;
	MinimumSpacing = Definition.MinimumSpacing;
	SpawnCount = Definition.SpawnCount;
	RouteNodes = Definition.Nodes;
	VehicleColors = Definition.VehicleColors;
	RebuildSpline();
}

bool AFlyingCabLivingRoute::IsRouteValid(FString& OutError) const
{
	return BuildDefinition().IsValid(OutError);
}

const FFlyingCabLivingRouteNode* AFlyingCabLivingRoute::GetNode(int32 NodeIndex) const
{
	return RouteNodes.IsValidIndex(NodeIndex) ? &RouteNodes[NodeIndex] : nullptr;
}

float AFlyingCabLivingRoute::GetRouteLength() const
{
	return Spline ? Spline->GetSplineLength() : 0.0f;
}

float AFlyingCabLivingRoute::GetNodeDistance(int32 NodeIndex) const
{
	return Spline && RouteNodes.IsValidIndex(NodeIndex)
		? Spline->GetDistanceAlongSplineAtSplinePoint(NodeIndex)
		: 0.0f;
}

float AFlyingCabLivingRoute::NormalizeDistance(float Distance) const
{
	const float Length = GetRouteLength();
	if (Length <= UE_SMALL_NUMBER)
	{
		return 0.0f;
	}
	if (bClosedLoop)
	{
		const float Wrapped = FMath::Fmod(Distance, Length);
		return Wrapped < 0.0f ? Wrapped + Length : Wrapped;
	}
	return FMath::Clamp(Distance, 0.0f, Length);
}

float AFlyingCabLivingRoute::GetForwardDistanceToNode(float Distance, int32 NodeIndex) const
{
	if (!RouteNodes.IsValidIndex(NodeIndex))
	{
		return TNumericLimits<float>::Max();
	}
	const float NormalizedDistance = NormalizeDistance(Distance);
	const float NodeDistance = GetNodeDistance(NodeIndex);
	if (NodeDistance >= NormalizedDistance)
	{
		return NodeDistance - NormalizedDistance;
	}
	return bClosedLoop
		? GetRouteLength() - NormalizedDistance + NodeDistance
		: TNumericLimits<float>::Max();
}

int32 AFlyingCabLivingRoute::FindNodeAtOrAhead(float Distance, float Tolerance) const
{
	if (RouteNodes.IsEmpty())
	{
		return INDEX_NONE;
	}
	const float NormalizedDistance = NormalizeDistance(Distance);
	for (int32 NodeIndex = 0; NodeIndex < RouteNodes.Num(); ++NodeIndex)
	{
		if (GetNodeDistance(NodeIndex) >= NormalizedDistance - FMath::Max(0.0f, Tolerance))
		{
			return NodeIndex;
		}
	}
	return bClosedLoop ? 0 : INDEX_NONE;
}

int32 AFlyingCabLivingRoute::FindNextNodeIndex(int32 NodeIndex) const
{
	if (RouteNodes.IsEmpty())
	{
		return INDEX_NONE;
	}
	if (NodeIndex + 1 < RouteNodes.Num())
	{
		return NodeIndex + 1;
	}
	return bClosedLoop ? 0 : INDEX_NONE;
}

int32 AFlyingCabLivingRoute::FindNextNodeWithAction(
	int32 StartNodeIndex,
	EFlyingCabLivingRouteAction Action,
	FName StopId) const
{
	if (RouteNodes.IsEmpty())
	{
		return INDEX_NONE;
	}
	int32 NodeIndex = FMath::Clamp(StartNodeIndex, 0, RouteNodes.Num() - 1);
	for (int32 Checked = 0; Checked < RouteNodes.Num(); ++Checked)
	{
		const FFlyingCabLivingRouteNode& Node = RouteNodes[NodeIndex];
		if (Node.Action == Action && (StopId.IsNone() || Node.StopId == StopId))
		{
			return NodeIndex;
		}
		NodeIndex = FindNextNodeIndex(NodeIndex);
		if (NodeIndex == INDEX_NONE)
		{
			break;
		}
	}
	return INDEX_NONE;
}

FVector AFlyingCabLivingRoute::GetWorldLocationAtDistance(float Distance) const
{
	return Spline
		? Spline->GetLocationAtDistanceAlongSpline(NormalizeDistance(Distance), ESplineCoordinateSpace::World)
		: GetActorLocation();
}

FVector AFlyingCabLivingRoute::GetWorldDirectionAtDistance(float Distance) const
{
	return Spline
		? Spline->GetDirectionAtDistanceAlongSpline(NormalizeDistance(Distance), ESplineCoordinateSpace::World)
		: FVector::ForwardVector;
}

void AFlyingCabLivingRoute::RebuildSpline()
{
	if (!Spline)
	{
		return;
	}
	Spline->ClearSplinePoints(false);
	for (int32 NodeIndex = 0; NodeIndex < RouteNodes.Num(); ++NodeIndex)
	{
		Spline->AddSplinePoint(RouteNodes[NodeIndex].LocalLocation, ESplineCoordinateSpace::Local, false);
		Spline->SetSplinePointType(NodeIndex, ESplinePointType::Linear, false);
	}
	Spline->SetClosedLoop(bClosedLoop, false);
	if (AgentKind == EFlyingCabLivingAgentKind::Vehicle
		&& bSmoothVehicleCorners
		&& VehicleCornerSmoothingDistance > UE_SMALL_NUMBER
		&& RouteNodes.Num() >= 3)
	{
		const int32 NodeCount = RouteNodes.Num();
		for (int32 NodeIndex = 0; NodeIndex < NodeCount; ++NodeIndex)
		{
			const bool bHasPrevious = NodeIndex > 0 || bClosedLoop;
			const bool bHasNext = NodeIndex + 1 < NodeCount || bClosedLoop;
			const int32 PreviousIndex = NodeIndex > 0 ? NodeIndex - 1 : NodeCount - 1;
			const int32 NextIndex = NodeIndex + 1 < NodeCount ? NodeIndex + 1 : 0;
			const FVector Current = RouteNodes[NodeIndex].LocalLocation;
			const FVector Incoming = bHasPrevious
				? Current - RouteNodes[PreviousIndex].LocalLocation
				: FVector::ZeroVector;
			const FVector Outgoing = bHasNext
				? RouteNodes[NextIndex].LocalLocation - Current
				: FVector::ZeroVector;
			const float IncomingLength = Incoming.Size();
			const float OutgoingLength = Outgoing.Size();
			FVector TangentDirection;
			if (bHasPrevious && bHasNext)
			{
				TangentDirection = (Incoming.GetSafeNormal() + Outgoing.GetSafeNormal()).GetSafeNormal();
			}
			else
			{
				TangentDirection = bHasNext
					? Outgoing.GetSafeNormal()
					: Incoming.GetSafeNormal();
			}
			if (TangentDirection.IsNearlyZero())
			{
				TangentDirection = bHasNext
					? Outgoing.GetSafeNormal()
					: Incoming.GetSafeNormal();
			}

			float TangentLength = VehicleCornerSmoothingDistance;
			if (bHasPrevious)
			{
				TangentLength = FMath::Min(TangentLength, IncomingLength * 0.35f);
			}
			if (bHasNext)
			{
				TangentLength = FMath::Min(TangentLength, OutgoingLength * 0.35f);
			}
			Spline->SetSplinePointType(NodeIndex, ESplinePointType::CurveCustomTangent, false);
			Spline->SetTangentsAtSplinePoint(
				NodeIndex,
				TangentDirection * TangentLength,
				TangentDirection * TangentLength,
				ESplineCoordinateSpace::Local,
				false);
		}
	}
	Spline->SetDrawDebug(bDrawRouteInGame);
	Spline->UpdateSpline();
}

FFlyingCabLivingRouteDefinition AFlyingCabLivingRoute::BuildDefinition() const
{
	FFlyingCabLivingRouteDefinition Definition;
	Definition.RouteId = RouteId;
	Definition.AgentKind = AgentKind;
	Definition.RouteClass = RouteClass;
	Definition.bClosedLoop = bClosedLoop;
	Definition.bSmoothVehicleCorners = bSmoothVehicleCorners;
	Definition.VehicleCornerSmoothingDistance = VehicleCornerSmoothingDistance;
	Definition.CruiseSpeed = CruiseSpeed;
	Definition.Acceleration = Acceleration;
	Definition.Deceleration = Deceleration;
	Definition.MinimumSpacing = MinimumSpacing;
	Definition.SpawnCount = SpawnCount;
	Definition.Nodes = RouteNodes;
	Definition.VehicleColors = VehicleColors;
	return Definition;
}
