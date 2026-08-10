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
	float VehicleArmLength = 1600.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Camera", meta = (ClampMin = "0.0"))
	float OnFootArmLength = 1050.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Camera")
	FVector OnFootTargetOffset = FVector(0.0f, 0.0f, 80.0f);

	/** Large portal teleports snap like a level transition instead of showing the space between locations. */
	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Camera", meta = (ClampMin = "0.0"))
	float TeleportSnapDistance = 2000.0f;

	UPROPERTY(Transient)
	TObjectPtr<AActor> FollowTarget;
};
