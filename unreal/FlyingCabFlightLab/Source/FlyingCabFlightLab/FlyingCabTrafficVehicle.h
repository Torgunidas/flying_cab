// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlyingCabTrafficVehicle.generated.h"

class UBoxComponent;
class UPointLightComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;
class AFlyingCabPawn;
class AFlyingCabTrafficVehicle;

DECLARE_MULTICAST_DELEGATE_TwoParams(
	FOnFlyingCabNearMiss,
	AFlyingCabTrafficVehicle*,
	AFlyingCabPawn*);

/** Lightweight kinematic traffic used to establish readable airborne lanes. */
UCLASS()
class FLYINGCABFLIGHTLAB_API AFlyingCabTrafficVehicle : public AActor
{
	GENERATED_BODY()

public:
	AFlyingCabTrafficVehicle();

	virtual void Tick(float DeltaSeconds) override;
	void Configure(
		const FVector& InRouteStart,
		const FVector& InRouteEnd,
		float InCruiseSpeed,
		float InitialRouteAlpha,
		const FLinearColor& VehicleColor);
	FVector GetTrafficVelocity() const { return RouteDirection * CruiseSpeed; }

	FOnFlyingCabNearMiss OnNearMiss;

private:
	void UpdateNearMissTracking(AFlyingCabPawn* Pawn);
	void ResetNearMissTracking();

	UFUNCTION()
	void HandleCollisionHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse,
		const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Traffic")
	TObjectPtr<UBoxComponent> CollisionBody;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Traffic")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Traffic")
	TObjectPtr<UPointLightComponent> RunningLight;

	/** Horizontal window in which a passing encounter is measured. */
	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Traffic|Near Miss", meta = (ClampMin = "1.0"))
	float NearMissDetectionHalfWidth = 420.0f;

	/** Leaves a small safety margin outside the combined physical hulls. */
	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Traffic|Near Miss", meta = (ClampMin = "0.0"))
	float NearMissMinimumVerticalSeparation = 90.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Traffic|Near Miss", meta = (ClampMin = "1.0"))
	float NearMissMaximumVerticalSeparation = 210.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Traffic|Near Miss", meta = (ClampMin = "0.0"))
	float NearMissMinimumRelativeSpeed = 300.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Traffic|Near Miss", meta = (ClampMin = "0.0"))
	float NearMissCooldown = 2.0f;

	FVector RouteStart = FVector::ZeroVector;
	FVector RouteEnd = FVector::ZeroVector;
	FVector RouteDirection = FVector::ForwardVector;
	float RouteLength = 0.0f;
	float RouteDistance = 0.0f;
	float CruiseSpeed = 450.0f;
	float NearMissCooldownRemaining = 0.0f;
	float PreviousEncounterHorizontalSeparation = 0.0f;
	float EncounterMinimumVerticalSeparation = TNumericLimits<float>::Max();
	float EncounterMaximumRelativeSpeed = 0.0f;
	bool bTrackingNearMiss = false;
	bool bEncounterInvalidated = false;
};
