// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabLivingWorldProfile.h"

#include "UObject/UObjectGlobals.h"

namespace
{
	FFlyingCabLivingRouteNode MakeNode(
		const FVector& Location,
		EFlyingCabLivingRouteAction Action = EFlyingCabLivingRouteAction::PassThrough,
		float WaitDuration = 0.0f,
		FName StopId = NAME_None)
	{
		FFlyingCabLivingRouteNode Node;
		Node.LocalLocation = Location;
		Node.Action = Action;
		Node.WaitDuration = WaitDuration;
		Node.StopId = StopId;
		return Node;
	}
}

bool FFlyingCabLivingRouteDefinition::IsValid(FString& OutError) const
{
	if (RouteId.IsNone())
	{
		OutError = TEXT("A living-world route needs a non-empty RouteId.");
		return false;
	}
	if (Nodes.Num() < 2)
	{
		OutError = FString::Printf(TEXT("Route %s needs at least two nodes."), *RouteId.ToString());
		return false;
	}
	if (CruiseSpeed <= 0.0f || Acceleration <= 0.0f || Deceleration <= 0.0f
		|| MinimumSpacing < 0.0f || SpawnCount < 0)
	{
		OutError = FString::Printf(TEXT("Route %s has invalid movement or population settings."), *RouteId.ToString());
		return false;
	}
	for (int32 NodeIndex = 0; NodeIndex < Nodes.Num(); ++NodeIndex)
	{
		const FFlyingCabLivingRouteNode& Node = Nodes[NodeIndex];
		if (Node.LocalLocation.ContainsNaN() || Node.WaitDuration < 0.0f)
		{
			OutError = FString::Printf(TEXT("Route %s has an invalid node at index %d."), *RouteId.ToString(), NodeIndex);
			return false;
		}
		const bool bNeedsStopId = Node.Action == EFlyingCabLivingRouteAction::BoardVehicle
			|| Node.Action == EFlyingCabLivingRouteAction::ExitVehicle;
		if (bNeedsStopId && Node.StopId.IsNone())
		{
			OutError = FString::Printf(TEXT("Route %s has a passenger action without StopId."), *RouteId.ToString());
			return false;
		}
	}
	OutError.Reset();
	return true;
}

bool UFlyingCabLivingWorldProfile::IsConfigurationValid(FString& OutError) const
{
	if (Routes.IsEmpty())
	{
		OutError = TEXT("A living-world profile needs at least one route.");
		return false;
	}

	TSet<FName> RouteIds;
	TSet<FName> VehicleStopIds;
	TSet<FName> PassengerStopIds;
	for (const FFlyingCabLivingRouteDefinition& Route : Routes)
	{
		if (!Route.IsValid(OutError))
		{
			return false;
		}
		if (RouteIds.Contains(Route.RouteId))
		{
			OutError = FString::Printf(TEXT("Living-world RouteId %s is duplicated."), *Route.RouteId.ToString());
			return false;
		}
		RouteIds.Add(Route.RouteId);
		for (const FFlyingCabLivingRouteNode& Node : Route.Nodes)
		{
			if (Node.StopId.IsNone())
			{
				continue;
			}
			if (Route.AgentKind == EFlyingCabLivingAgentKind::Vehicle)
			{
				VehicleStopIds.Add(Node.StopId);
			}
			else if (Node.Action == EFlyingCabLivingRouteAction::BoardVehicle
				|| Node.Action == EFlyingCabLivingRouteAction::ExitVehicle)
			{
				PassengerStopIds.Add(Node.StopId);
			}
		}
	}

	for (const FName StopId : PassengerStopIds)
	{
		if (!VehicleStopIds.Contains(StopId))
		{
			OutError = FString::Printf(
				TEXT("Passenger StopId %s is not served by a vehicle route."),
				*StopId.ToString());
			return false;
		}
	}
	OutError.Reset();
	return true;
}

UFlyingCabLivingWorldProfile* UFlyingCabLivingWorldProfile::LoadDefaultAsset()
{
	return LoadObject<UFlyingCabLivingWorldProfile>(
		nullptr,
		TEXT("/Game/Data/DA_FlyingCabLivingWorldProfile.DA_FlyingCabLivingWorldProfile"),
		nullptr,
		LOAD_NoWarn);
}

TArray<FFlyingCabLivingRouteDefinition> UFlyingCabLivingWorldProfile::BuildPrototypeRoutes()
{
	const FName GlasswardStop(TEXT("LivingStop.Glassward"));
	const FName RainlineStop(TEXT("LivingStop.Rainline"));

	FFlyingCabLivingRouteDefinition Shuttle;
	Shuttle.RouteId = TEXT("LivingRoute.GlasswardRainlineShuttle");
	Shuttle.AgentKind = EFlyingCabLivingAgentKind::Vehicle;
	Shuttle.RouteClass = EFlyingCabLivingRouteClass::LandingApproach;
	Shuttle.CruiseSpeed = 520.0f;
	Shuttle.Acceleration = 330.0f;
	Shuttle.Deceleration = 520.0f;
	Shuttle.MinimumSpacing = 420.0f;
	Shuttle.SpawnCount = 1;
	Shuttle.VehicleColors = {FLinearColor(0.05f, 0.85f, 1.0f)};
	Shuttle.Nodes = {
		MakeNode(FVector(6500.0f, 0.0f, 1150.0f), EFlyingCabLivingRouteAction::Land, 3.0f, GlasswardStop),
		MakeNode(FVector(6300.0f, 0.0f, 2000.0f), EFlyingCabLivingRouteAction::TakeOff),
		MakeNode(FVector(6300.0f, 0.0f, 3800.0f)),
		MakeNode(FVector(8300.0f, 0.0f, 3800.0f)),
		MakeNode(FVector(8650.0f, 0.0f, 2700.0f), EFlyingCabLivingRouteAction::Land, 3.0f, RainlineStop),
		MakeNode(FVector(9000.0f, 0.0f, 4000.0f), EFlyingCabLivingRouteAction::TakeOff),
		MakeNode(FVector(8500.0f, 0.0f, 4300.0f)),
		MakeNode(FVector(6300.0f, 0.0f, 4300.0f)),
		MakeNode(FVector(6300.0f, 0.0f, 2000.0f))};

	FFlyingCabLivingRouteDefinition Express;
	Express.RouteId = TEXT("LivingRoute.EastExpressLoop");
	Express.AgentKind = EFlyingCabLivingAgentKind::Vehicle;
	Express.RouteClass = EFlyingCabLivingRouteClass::Express;
	Express.CruiseSpeed = 900.0f;
	Express.Acceleration = 520.0f;
	Express.Deceleration = 820.0f;
	Express.MinimumSpacing = 520.0f;
	Express.SpawnCount = 2;
	Express.VehicleColors = {
		FLinearColor(1.0f, 0.30f, 0.08f),
		FLinearColor(0.90f, 0.08f, 0.72f)};
	Express.Nodes = {
		MakeNode(FVector(5300.0f, 0.0f, 5000.0f)),
		MakeNode(FVector(14500.0f, 0.0f, 5000.0f)),
		MakeNode(FVector(14500.0f, 0.0f, 5140.0f)),
		MakeNode(FVector(5300.0f, 0.0f, 5140.0f))};

	FFlyingCabLivingRouteDefinition Pedestrians;
	Pedestrians.RouteId = TEXT("LivingRoute.GlasswardRainlinePedestrians");
	Pedestrians.AgentKind = EFlyingCabLivingAgentKind::Pedestrian;
	Pedestrians.RouteClass = EFlyingCabLivingRouteClass::Pedestrian;
	Pedestrians.CruiseSpeed = 145.0f;
	Pedestrians.Acceleration = 300.0f;
	Pedestrians.Deceleration = 500.0f;
	Pedestrians.MinimumSpacing = 90.0f;
	Pedestrians.SpawnCount = 2;
	Pedestrians.Nodes = {
		MakeNode(FVector(6150.0f, 0.0f, 1080.0f), EFlyingCabLivingRouteAction::ExitBuilding, 0.5f),
		MakeNode(FVector(6220.0f, 0.0f, 1080.0f)),
		MakeNode(FVector(6300.0f, 0.0f, 1080.0f), EFlyingCabLivingRouteAction::BoardVehicle, 0.0f, GlasswardStop),
		MakeNode(FVector(8850.0f, 0.0f, 2620.0f), EFlyingCabLivingRouteAction::ExitVehicle, 0.0f, RainlineStop),
		MakeNode(FVector(8920.0f, 0.0f, 2620.0f)),
		MakeNode(FVector(9000.0f, 0.0f, 2620.0f), EFlyingCabLivingRouteAction::EnterBuilding, 5.0f),
		MakeNode(FVector(9000.0f, 0.0f, 2620.0f), EFlyingCabLivingRouteAction::ExitBuilding, 0.5f),
		MakeNode(FVector(8920.0f, 0.0f, 2620.0f)),
		MakeNode(FVector(8850.0f, 0.0f, 2620.0f), EFlyingCabLivingRouteAction::BoardVehicle, 0.0f, RainlineStop),
		MakeNode(FVector(6300.0f, 0.0f, 1080.0f), EFlyingCabLivingRouteAction::ExitVehicle, 0.0f, GlasswardStop),
		MakeNode(FVector(6220.0f, 0.0f, 1080.0f)),
		MakeNode(FVector(6150.0f, 0.0f, 1080.0f), EFlyingCabLivingRouteAction::EnterBuilding, 5.0f)};

	return {Shuttle, Express, Pedestrians};
}

int32 UFlyingCabLivingWorldProfile::CountAgents(
	TConstArrayView<FFlyingCabLivingRouteDefinition> Definitions,
	EFlyingCabLivingAgentKind AgentKind)
{
	int32 Count = 0;
	for (const FFlyingCabLivingRouteDefinition& Definition : Definitions)
	{
		if (Definition.AgentKind == AgentKind)
		{
			Count += FMath::Max(0, Definition.SpawnCount);
		}
	}
	return Count;
}
