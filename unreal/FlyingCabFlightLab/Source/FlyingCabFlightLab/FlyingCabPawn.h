// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "FlyingCabPawn.generated.h"

class UBoxComponent;
class UCameraComponent;
class UStaticMeshComponent;
class USpringArmComponent;

/**
 * Minimal, force-driven 2.5D vehicle used to tune the flight feel.
 * Keyboard and touch commands meet here so both platforms use one physics model.
 */
UCLASS()
class FLYINGCABFLIGHTLAB_API AFlyingCabPawn : public APawn
{
	GENERATED_BODY()

public:
	AFlyingCabPawn();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** Horizontal input for the future mobile UI: -1 is left, +1 is right. */
	UFUNCTION(BlueprintCallable, Category = "Flying Cab|Input")
	void SetTouchHorizontalInput(float Value);

	/** Press/release input for the future mobile thrust button. */
	UFUNCTION(BlueprintCallable, Category = "Flying Cab|Input")
	void SetTouchThrustPressed(bool bPressed);

	UFUNCTION(BlueprintCallable, Category = "Flying Cab|Flight")
	void ResetVehicle();

protected:
	virtual void BeginPlay() override;

private:
	void SetKeyboardHorizontalInput(float Value);
	void SetKeyboardThrustInput(float Value);

	float GetHorizontalInput() const;
	float GetThrustInput() const;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Components")
	TObjectPtr<UBoxComponent> CollisionBody;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Components")
	TObjectPtr<UCameraComponent> Camera;

	/** Acceleration produced by full vertical thrust, in cm/s^2. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Flight", meta = (ClampMin = "0.0"))
	float VerticalThrustAcceleration = 2800.0f;

	/** Horizontal acceleration produced by either side thruster, in cm/s^2. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Flight", meta = (ClampMin = "0.0"))
	float HorizontalThrustAcceleration = 1800.0f;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Flight", meta = (ClampMin = "0.0"))
	float MaxClimbSpeed = 1500.0f;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Flight", meta = (ClampMin = "0.0"))
	float MaxFallSpeed = 1800.0f;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Flight", meta = (ClampMin = "0.0"))
	float MaxHorizontalSpeed = 1200.0f;

	/** How quickly horizontal drift fades after releasing A/D. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Flight", meta = (ClampMin = "0.0"))
	float HorizontalCoastDamping = 1.7f;

	/** Reproduces the old game's soft braking of upward motion after releasing thrust. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Flight", meta = (ClampMin = "0.0"))
	float UpwardCoastDamping = 1.5f;

	FTransform SpawnTransform;

	float KeyboardHorizontalInput = 0.0f;
	float KeyboardThrustInput = 0.0f;
	float TouchHorizontalInput = 0.0f;
	float TouchThrustInput = 0.0f;
};
