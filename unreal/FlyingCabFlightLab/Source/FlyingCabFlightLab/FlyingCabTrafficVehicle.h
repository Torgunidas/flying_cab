// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlyingCabLivingWorldTypes.h"
#include "GameFramework/Actor.h"
#include "FlyingCabTrafficVehicle.generated.h"

class UBoxComponent;
class UPointLightComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;
class AFlyingCabLivingRoute;
class AFlyingCabPawn;
class AFlyingCabTrafficVehicle;

DECLARE_MULTICAST_DELEGATE_TwoParams(
	FOnFlyingCabNearMiss,
	AFlyingCabTrafficVehicle*,
	AFlyingCabPawn*);

DECLARE_MULTICAST_DELEGATE_ThreeParams(
	FOnFlyingCabTrafficStopReached,
	AFlyingCabTrafficVehicle*,
	FName,
	EFlyingCabLivingRouteAction);

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
	void ConfigureLivingRoute(
		AFlyingCabLivingRoute* InRoute,
		float InitialRouteAlpha,
		const FLinearColor& VehicleColor);
	void SetTrackedPawn(AFlyingCabPawn* Pawn) { TrackedPawn = Pawn; }
	FVector GetTrafficVelocity() const { return RouteDirection * CurrentSpeed; }
	EFlyingCabTrafficMovementState GetMovementState() const { return MovementState; }
	FName GetCurrentLivingStopId() const { return CurrentLivingStopId; }
	float GetCurrentTrafficSpeed() const { return CurrentSpeed; }
	float GetVisualPitchDegrees() const;
	FName GetLivingRouteId() const;
	bool UsesLivingRoute() const { return LivingRoute != nullptr; }

	FOnFlyingCabNearMiss OnNearMiss;
	FOnFlyingCabTrafficStopReached OnLivingStopReached;

private:
	void TickLegacyRoute(float DeltaSeconds);
	void TickLivingRoute(float DeltaSeconds);
	bool HasLivingRouteObstacle(float LookAheadDistance) const;
	void ProcessLivingRouteNode();
	void AdvanceLivingRouteNode();
	void UpdateRoutePresentation(float DeltaSeconds);
	void ResetRoutePresentation();
	static bool IsVehicleStopAction(EFlyingCabLivingRouteAction Action);
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

	/** Matches the player cab: vertical travel never rotates the body onto its nose. */
	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Traffic|Presentation", meta = (ClampMin = "0.0", ClampMax = "45.0"))
	float MaxVisualPitchDegrees = 12.0f;

	/** Horizontal acceleration required to reach the full presentation pitch. */
	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Traffic|Presentation", meta = (ClampMin = "0.0"))
	float VisualPitchFullAcceleration = 1400.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Traffic|Presentation", meta = (ClampMin = "0.0"))
	float VisualPitchResponseSpeed = 7.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Traffic|Presentation", meta = (ClampMin = "0.0"))
	float VisualPitchReturnSpeed = 2.5f;

	/** Maximum distance the visual body can lag behind its kinematic collision body. */
	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Traffic|Presentation", meta = (ClampMin = "0.0"))
	float MaxVisualMotionLag = 24.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Traffic|Presentation", meta = (ClampMin = "1.0"))
	float VisualMotionLagFullAcceleration = 1400.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Traffic|Presentation", meta = (ClampMin = "0.0"))
	float VisualMotionLagResponseSpeed = 4.0f;

	/** Small asynchronous hover keeps stopped and cruising traffic from looking pinned to a rail. */
	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Traffic|Presentation", meta = (ClampMin = "0.0"))
	float HoverBobAmplitude = 7.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Traffic|Presentation", meta = (ClampMin = "0.0"))
	float HoverBobFrequency = 0.55f;

	FVector RouteStart = FVector::ZeroVector;
	FVector RouteEnd = FVector::ZeroVector;
	FVector RouteDirection = FVector::ForwardVector;
	float RouteLength = 0.0f;
	float RouteDistance = 0.0f;
	float CruiseSpeed = 450.0f;
	float CurrentSpeed = 0.0f;
	float DwellRemaining = 0.0f;
	int32 NextLivingNodeIndex = INDEX_NONE;
	FName CurrentLivingStopId = NAME_None;
	EFlyingCabTrafficMovementState MovementState = EFlyingCabTrafficMovementState::Cruising;
	float NearMissCooldownRemaining = 0.0f;
	FVector PreviousPresentationVelocity = FVector::ZeroVector;
	FVector VisualMotionLagOffset = FVector::ZeroVector;
	float VisualFacingDirection = 1.0f;
	float PresentationElapsedTime = 0.0f;
	bool bHasPreviousPresentationVelocity = false;
	float PreviousEncounterHorizontalSeparation = 0.0f;
	float EncounterMinimumVerticalSeparation = TNumericLimits<float>::Max();
	float EncounterMaximumRelativeSpeed = 0.0f;
	bool bTrackingNearMiss = false;
	bool bEncounterInvalidated = false;

	UPROPERTY(Transient)
	TObjectPtr<AFlyingCabLivingRoute> LivingRoute;

	UPROPERTY(Transient)
	TObjectPtr<AFlyingCabPawn> TrackedPawn;
};
