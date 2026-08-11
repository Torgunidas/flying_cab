// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "FlyingCabVehicleVitalsComponent.generated.h"

/** Tunable values remain on the pawn so existing Blueprint class defaults stay authoritative. */
struct FFlyingCabVehicleVitalsConfig
{
	float MaxFuel = 100.0f;
	float StartingFuel = 65.0f;
	float VerticalFuelPerSecond = 1.8f;
	float HorizontalFuelPerSecond = 0.9f;
	float DescentRegenerationPerSecond = 0.12f;
	float RegenerationFullSpeed = 900.0f;
	float MaxHull = 100.0f;
	float DamageImpactSpeedThreshold = 700.0f;
	float DamageFullHullSpeed = 1400.0f;
	float CollisionDamageExponent = 2.0f;
	float CollisionDamageCooldown = 0.15f;
};

struct FFlyingCabImpactResult
{
	float Damage = 0.0f;
	bool bDestroyedNow = false;
};

/** Owns a vehicle's fuel, hull, damage cooldown and destroyed state. */
UCLASS(ClassGroup = "Flying Cab", meta = (BlueprintSpawnableComponent))
class FLYINGCABFLIGHTLAB_API UFlyingCabVehicleVitalsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFlyingCabVehicleVitalsComponent();

	void InitializeVitals(const FFlyingCabVehicleVitalsConfig& Config);
	void ResetResources();
	bool Advance(
		float DeltaSeconds,
		float HorizontalInput,
		float ThrustInput,
		float VerticalVelocity);
	FFlyingCabImpactResult ApplyImpact(float NormalSpeedChange);
	float AddFuel(float Units);
	float AddHull(float Units);
	void Recover(float RecoveryFuelPercent);

	bool CanUseThrusters() const;
	bool IsDestroyed() const { return bDestroyed; }
	bool IsDamageFlashActive() const { return DamageFlashRemaining > 0.0f; }
	float GetFuel() const { return CurrentFuel; }
	float GetMaxFuel() const { return MaxFuel; }
	float GetHull() const { return CurrentHull; }
	float GetMaxHull() const { return MaxHull; }
	float GetFuelPercent() const;
	float GetHullPercent() const;
	float GetFuelNeeded() const;
	float GetHullNeeded() const;

private:
	float MaxFuel = 100.0f;
	float StartingFuel = 65.0f;
	float VerticalFuelPerSecond = 1.8f;
	float HorizontalFuelPerSecond = 0.9f;
	float DescentRegenerationPerSecond = 0.12f;
	float RegenerationFullSpeed = 900.0f;
	float MaxHull = 100.0f;
	float DamageImpactSpeedThreshold = 700.0f;
	float DamageFullHullSpeed = 1400.0f;
	float CollisionDamageExponent = 2.0f;
	float CollisionDamageCooldown = 0.15f;

	float CurrentFuel = 0.0f;
	float CurrentHull = 0.0f;
	float DamageCooldownRemaining = 0.0f;
	float DamageFlashRemaining = 0.0f;
	bool bDestroyed = false;
	bool bFuelEmptyWarningShown = false;
};
