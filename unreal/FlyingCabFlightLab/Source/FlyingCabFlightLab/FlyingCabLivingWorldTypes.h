// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlyingCabLivingWorldTypes.generated.h"

UENUM(BlueprintType)
enum class EFlyingCabLivingAgentKind : uint8
{
	Vehicle,
	Pedestrian
};

UENUM(BlueprintType)
enum class EFlyingCabLivingRouteClass : uint8
{
	Local,
	Express,
	Vertical,
	LandingApproach,
	Pedestrian
};

UENUM(BlueprintType)
enum class EFlyingCabLivingRouteAction : uint8
{
	PassThrough,
	Stop,
	TakeOff,
	Land,
	BoardVehicle,
	ExitVehicle,
	EnterBuilding,
	ExitBuilding,
	Park
};

UENUM(BlueprintType)
enum class EFlyingCabTrafficMovementState : uint8
{
	Cruising,
	ApproachingStop,
	Dwelling,
	WaitingForObstacle
};

UENUM(BlueprintType)
enum class EFlyingCabPedestrianState : uint8
{
	Walking,
	Waiting,
	Inside,
	Riding,
	WaitingForObstacle
};

/** One authored point and its semantic action on a living-world route. */
USTRUCT(BlueprintType)
struct FFlyingCabLivingRouteNode
{
	GENERATED_BODY()

	/** Position relative to the route actor. It can be moved directly in the editor viewport. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Living World", meta = (MakeEditWidget))
	FVector LocalLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Living World")
	EFlyingCabLivingRouteAction Action = EFlyingCabLivingRouteAction::PassThrough;

	/** Time spent at this node. EnterBuilding uses it as time spent indoors. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Living World", meta = (ClampMin = "0.0"))
	float WaitDuration = 0.0f;

	/** Shared key used to match vehicle stops with boarding and exiting pedestrians. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Living World")
	FName StopId = NAME_None;
};

/** Data-asset representation of an authored or generated living-world route. */
USTRUCT(BlueprintType)
struct FFlyingCabLivingRouteDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Living World")
	FName RouteId = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Living World")
	EFlyingCabLivingAgentKind AgentKind = EFlyingCabLivingAgentKind::Vehicle;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Living World")
	EFlyingCabLivingRouteClass RouteClass = EFlyingCabLivingRouteClass::Local;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Living World")
	bool bClosedLoop = true;

	/** Rounds vehicle corners without changing pedestrian paths or authored stop positions. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Living World|Vehicle")
	bool bSmoothVehicleCorners = true;

	/** Maximum custom spline tangent used around a vehicle-route corner. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Living World|Vehicle", meta = (ClampMin = "0.0"))
	float VehicleCornerSmoothingDistance = 260.0f;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Living World", meta = (ClampMin = "1.0"))
	float CruiseSpeed = 450.0f;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Living World", meta = (ClampMin = "1.0"))
	float Acceleration = 400.0f;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Living World", meta = (ClampMin = "1.0"))
	float Deceleration = 650.0f;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Living World", meta = (ClampMin = "0.0"))
	float MinimumSpacing = 340.0f;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Living World", meta = (ClampMin = "0", ClampMax = "32"))
	int32 SpawnCount = 1;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Living World")
	TArray<FFlyingCabLivingRouteNode> Nodes;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Living World|Vehicle")
	TArray<FLinearColor> VehicleColors;

	bool IsValid(FString& OutError) const;
};
