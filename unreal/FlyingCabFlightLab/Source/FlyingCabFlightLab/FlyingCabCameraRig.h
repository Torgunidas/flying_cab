// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlyingCabCameraRig.generated.h"

class UCameraComponent;
class USceneComponent;
class USpringArmComponent;

/**
 * One persistent side-view camera, matching the Godot CameraController model:
 * switch only the follow target, never blend between independently rotated cameras.
 */
UCLASS()
class FLYINGCABFLIGHTLAB_API AFlyingCabCameraRig : public AActor
{
	GENERATED_BODY()

public:
	AFlyingCabCameraRig();

	virtual void Tick(float DeltaSeconds) override;
	void SetFollowTarget(AActor* NewTarget, bool bSnapToTarget);
	void SetDeveloperObserverEnabled(bool bEnabled);
	void MoveDeveloperObserver(const FVector2D& PanInput, bool bFast, float DeltaSeconds);
	void AdjustDeveloperObserverZoom(float ZoomInput, float DeltaSeconds);
	void RecenterDeveloperObserver();
	bool IsDeveloperObserverEnabled() const { return bDeveloperObserverEnabled; }
	float GetCurrentArmLength() const;

private:
	FVector GetDesiredRigLocation() const;
	void ApplyTargetFraming();

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Camera")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Camera")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Camera", meta = (ClampMin = "0.0"))
	float FollowSpeed = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Camera", meta = (ClampMin = "0.0"))
	float VehicleArmLength = 3200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Camera", meta = (ClampMin = "0.0"))
	float OnFootArmLength = 1050.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Camera")
	FVector OnFootTargetOffset = FVector(0.0f, 0.0f, 80.0f);

	/** Large portal teleports snap like a level transition instead of showing the space between locations. */
	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Camera", meta = (ClampMin = "0.0"))
	float TeleportSnapDistance = 2000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Camera|Developer", meta = (ClampMin = "0.0"))
	float DeveloperObserverPanSpeed = 3200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Camera|Developer", meta = (ClampMin = "1.0"))
	float DeveloperObserverFastMultiplier = 3.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Camera|Developer", meta = (ClampMin = "0.0"))
	float DeveloperObserverZoomSpeed = 6000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Camera|Developer", meta = (ClampMin = "100.0"))
	float DeveloperObserverInitialArmLength = 6200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Camera|Developer", meta = (ClampMin = "100.0"))
	float DeveloperObserverMinArmLength = 900.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Camera|Developer", meta = (ClampMin = "100.0"))
	float DeveloperObserverMaxArmLength = 18000.0f;

	UPROPERTY(Transient)
	TObjectPtr<AActor> FollowTarget;

	bool bDeveloperObserverEnabled = false;
};
