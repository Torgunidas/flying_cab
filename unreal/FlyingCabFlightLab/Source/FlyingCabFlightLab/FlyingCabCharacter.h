// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FlyingCabCharacter.generated.h"

class UCameraComponent;
class UPointLightComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;
class USpringArmComponent;

/**
 * Lightweight 2.5D on-foot prototype. Cab and driver share the same Enhanced Input
 * actions and mapping context, so possession changes do not create a second keyboard state.
 */
UCLASS()
class FLYINGCABFLIGHTLAB_API AFlyingCabCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFlyingCabCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

	void SetTouchHorizontalInput(float Value);
	void SetTouchJumpPressed(bool bPressed);

	/** Immediately clears stored keyboard values when the controller flushes pressed keys. */
	void ReleaseKeyboardInputState();
	float GetHealthPercent() const;
	bool IsDead() const { return bDead; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void UnPossessed() override;

private:
	void SetKeyboardHorizontalInput(float Value);
	void SetKeyboardThrustInput(float Value);
	void RefreshKeyboardInputState();
	void ClearInputState();
	void ApplyCharacterDamage(float DamageAmount, const TCHAR* DamageSource);
	void EnterDeathState();
	void RestartCurrentLevel();
	void UpdateDamageAppearance();

	UFUNCTION()
	void HandleCapsuleHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse,
		const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|On Foot")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|On Foot")
	TObjectPtr<UStaticMeshComponent> HeadMesh;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|On Foot")
	TObjectPtr<UPointLightComponent> RunningLight;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|On Foot")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|On Foot")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|On Foot|Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	/** A normal jump lands below this speed and remains harmless. */
	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|On Foot|Health", meta = (ClampMin = "0.0"))
	float FallDamageSpeedThreshold = 650.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|On Foot|Health", meta = (ClampMin = "1.0"))
	float FatalFallSpeed = 1500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|On Foot|Health", meta = (ClampMin = "1.0"))
	float FallDamageExponent = 1.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|On Foot|Health", meta = (ClampMin = "0.0"))
	float CollisionDamageSpeedThreshold = 550.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|On Foot|Health", meta = (ClampMin = "1.0"))
	float FatalCollisionSpeed = 1200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|On Foot|Health", meta = (ClampMin = "0.0"))
	float CollisionDamageCooldown = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|On Foot|Health", meta = (ClampMin = "0.0"))
	float DeathRestartDelay = 1.4f;

	float TouchHorizontalInput = 0.0f;
	float KeyboardHorizontalInput = 0.0f;
	float KeyboardThrustInput = 0.0f;
	float CurrentHealth = 100.0f;
	float MaximumTrackedDownwardSpeed = 0.0f;
	float DamageCooldownRemaining = 0.0f;
	float DamageFlashRemaining = 0.0f;
	bool bTouchJumpPressed = false;
	bool bPreviousJumpPressed = false;
	bool bDead = false;
	FTimerHandle RestartLevelTimerHandle;
};
