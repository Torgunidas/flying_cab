// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlyingCabLivingWorldTypes.h"
#include "GameFramework/Actor.h"
#include "FlyingCabLivingRoute.generated.h"

class USplineComponent;

/** Viewport-editable route shared by living-world vehicles and pedestrians. */
UCLASS(BlueprintType)
class FLYINGCABFLIGHTLAB_API AFlyingCabLivingRoute : public AActor
{
	GENERATED_BODY()

public:
	AFlyingCabLivingRoute();
	virtual void OnConstruction(const FTransform& Transform) override;

	void Configure(const FFlyingCabLivingRouteDefinition& Definition);
	bool IsRouteValid(FString& OutError) const;

	FName GetRouteId() const { return RouteId; }
	EFlyingCabLivingAgentKind GetAgentKind() const { return AgentKind; }
	EFlyingCabLivingRouteClass GetRouteClass() const { return RouteClass; }
	bool IsClosedLoop() const { return bClosedLoop; }
	bool SmoothsVehicleCorners() const { return bSmoothVehicleCorners; }
	float GetVehicleCornerSmoothingDistance() const { return VehicleCornerSmoothingDistance; }
	float GetCruiseSpeed() const { return CruiseSpeed; }
	float GetAcceleration() const { return Acceleration; }
	float GetDeceleration() const { return Deceleration; }
	float GetMinimumSpacing() const { return MinimumSpacing; }
	int32 GetSpawnCount() const { return SpawnCount; }
	const TArray<FLinearColor>& GetVehicleColors() const { return VehicleColors; }
	int32 GetNodeCount() const { return RouteNodes.Num(); }
	const FFlyingCabLivingRouteNode* GetNode(int32 NodeIndex) const;
	float GetRouteLength() const;
	float GetNodeDistance(int32 NodeIndex) const;
	float NormalizeDistance(float Distance) const;
	float GetForwardDistanceToNode(float Distance, int32 NodeIndex) const;
	int32 FindNodeAtOrAhead(float Distance, float Tolerance = 1.0f) const;
	int32 FindNextNodeIndex(int32 NodeIndex) const;
	int32 FindNextNodeWithAction(
		int32 StartNodeIndex,
		EFlyingCabLivingRouteAction Action,
		FName StopId = NAME_None) const;
	FVector GetWorldLocationAtDistance(float Distance) const;
	FVector GetWorldDirectionAtDistance(float Distance) const;

private:
	void RebuildSpline();
	FFlyingCabLivingRouteDefinition BuildDefinition() const;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Living World")
	TObjectPtr<USplineComponent> Spline;

	UPROPERTY(EditInstanceOnly, Category = "Flying Cab|Living World")
	FName RouteId = TEXT("LivingRoute.New");

	UPROPERTY(EditInstanceOnly, Category = "Flying Cab|Living World")
	EFlyingCabLivingAgentKind AgentKind = EFlyingCabLivingAgentKind::Vehicle;

	UPROPERTY(EditInstanceOnly, Category = "Flying Cab|Living World")
	EFlyingCabLivingRouteClass RouteClass = EFlyingCabLivingRouteClass::Local;

	UPROPERTY(EditInstanceOnly, Category = "Flying Cab|Living World")
	bool bClosedLoop = true;

	UPROPERTY(EditInstanceOnly, Category = "Flying Cab|Living World|Vehicle")
	bool bSmoothVehicleCorners = true;

	UPROPERTY(EditInstanceOnly, Category = "Flying Cab|Living World|Vehicle", meta = (ClampMin = "0.0"))
	float VehicleCornerSmoothingDistance = 260.0f;

	UPROPERTY(EditInstanceOnly, Category = "Flying Cab|Living World", meta = (ClampMin = "1.0"))
	float CruiseSpeed = 450.0f;

	UPROPERTY(EditInstanceOnly, Category = "Flying Cab|Living World", meta = (ClampMin = "1.0"))
	float Acceleration = 400.0f;

	UPROPERTY(EditInstanceOnly, Category = "Flying Cab|Living World", meta = (ClampMin = "1.0"))
	float Deceleration = 650.0f;

	UPROPERTY(EditInstanceOnly, Category = "Flying Cab|Living World", meta = (ClampMin = "0.0"))
	float MinimumSpacing = 340.0f;

	UPROPERTY(EditInstanceOnly, Category = "Flying Cab|Living World", meta = (ClampMin = "0", ClampMax = "32"))
	int32 SpawnCount = 1;

	UPROPERTY(EditInstanceOnly, Category = "Flying Cab|Living World")
	TArray<FFlyingCabLivingRouteNode> RouteNodes;

	UPROPERTY(EditInstanceOnly, Category = "Flying Cab|Living World|Vehicle")
	TArray<FLinearColor> VehicleColors;

	UPROPERTY(EditInstanceOnly, Category = "Flying Cab|Living World|Debug")
	bool bDrawRouteInGame = false;
};
