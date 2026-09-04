// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlyingCabLivingWorldTypes.h"
#include "GameFramework/Actor.h"
#include "FlyingCabLivingPedestrian.generated.h"

class AFlyingCabLivingRoute;
class AFlyingCabTrafficVehicle;
class UCapsuleComponent;
class UStaticMeshComponent;

class AFlyingCabLivingPedestrian;
DECLARE_MULTICAST_DELEGATE_TwoParams(
	FOnFlyingCabPedestrianWaiting,
	AFlyingCabLivingPedestrian*,
	FName);

/** Lightweight pedestrian following an authored loop and participating in ambient rides. */
UCLASS()
class FLYINGCABFLIGHTLAB_API AFlyingCabLivingPedestrian : public AActor
{
	GENERATED_BODY()

public:
	AFlyingCabLivingPedestrian();
	virtual void Tick(float DeltaSeconds) override;

	void Configure(
		AFlyingCabLivingRoute* InRoute,
		float InitialRouteAlpha,
		const FLinearColor& Color);
	bool BoardVehicle(AFlyingCabTrafficVehicle* Vehicle);
	bool CompleteRideAtStop(FName StopId);
	bool WantsToExitAt(FName StopId) const;
	EFlyingCabPedestrianState GetLivingState() const { return LivingState; }
	FName GetWaitingStopId() const { return WaitingStopId; }
	FName GetDestinationStopId() const { return DestinationStopId; }
	AFlyingCabTrafficVehicle* GetRidingVehicle() const { return RidingVehicle; }
	float GetCurrentWalkingSpeed() const { return CurrentSpeed; }

	FOnFlyingCabPedestrianWaiting OnWaitingForVehicle;

private:
	void ProcessRouteNode();
	void AdvanceRouteNode();
	void EnterBuilding(const FFlyingCabLivingRouteNode& Node);
	void LeaveBuilding();
	bool HasObstacle(float LookAheadDistance) const;
	void SetAgentVisible(bool bVisible);
	void BeginNodeWait(float Duration);

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Living World")
	TObjectPtr<UCapsuleComponent> CollisionBody;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Living World")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(Transient)
	TObjectPtr<AFlyingCabLivingRoute> Route;

	UPROPERTY(Transient)
	TObjectPtr<AFlyingCabTrafficVehicle> RidingVehicle;

	float RouteDistance = 0.0f;
	float CurrentSpeed = 0.0f;
	float WaitRemaining = 0.0f;
	float ExitDoorWait = 0.0f;
	int32 NextNodeIndex = INDEX_NONE;
	FName WaitingStopId = NAME_None;
	FName DestinationStopId = NAME_None;
	EFlyingCabPedestrianState LivingState = EFlyingCabPedestrianState::Walking;
};
