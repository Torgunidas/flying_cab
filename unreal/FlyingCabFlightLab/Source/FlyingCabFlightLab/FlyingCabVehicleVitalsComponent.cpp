// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabVehicleVitalsComponent.h"

UFlyingCabVehicleVitalsComponent::UFlyingCabVehicleVitalsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFlyingCabVehicleVitalsComponent::InitializeVitals(
	const FFlyingCabVehicleVitalsConfig& Config)
{
	MaxFuel = FMath::Max(1.0f, Config.MaxFuel);
	StartingFuel = FMath::Clamp(Config.StartingFuel, 0.0f, MaxFuel);
	VerticalFuelPerSecond = FMath::Max(0.0f, Config.VerticalFuelPerSecond);
	HorizontalFuelPerSecond = FMath::Max(0.0f, Config.HorizontalFuelPerSecond);
	DescentRegenerationPerSecond = FMath::Max(0.0f, Config.DescentRegenerationPerSecond);
	RegenerationFullSpeed = FMath::Max(0.0f, Config.RegenerationFullSpeed);
	MaxHull = FMath::Max(1.0f, Config.MaxHull);
	DamageImpactSpeedThreshold = FMath::Max(0.0f, Config.DamageImpactSpeedThreshold);
	DamageFullHullSpeed = FMath::Max(1.0f, Config.DamageFullHullSpeed);
	CollisionDamageExponent = FMath::Max(1.0f, Config.CollisionDamageExponent);
	CollisionDamageCooldown = FMath::Max(0.0f, Config.CollisionDamageCooldown);
	ResetResources();
}

void UFlyingCabVehicleVitalsComponent::ResetResources()
{
	CurrentFuel = StartingFuel;
	CurrentHull = MaxHull;
	DamageCooldownRemaining = 0.0f;
	DamageFlashRemaining = 0.0f;
	bDestroyed = false;
	bFuelEmptyWarningShown = false;
}

bool UFlyingCabVehicleVitalsComponent::Advance(
	float DeltaSeconds,
	float HorizontalInput,
	float ThrustInput,
	float VerticalVelocity)
{
	const float EffectiveDeltaSeconds = FMath::Max(0.0f, DeltaSeconds);
	DamageCooldownRemaining = FMath::Max(
		0.0f,
		DamageCooldownRemaining - EffectiveDeltaSeconds);
	DamageFlashRemaining = FMath::Max(
		0.0f,
		DamageFlashRemaining - EffectiveDeltaSeconds);
	if (bDestroyed)
	{
		return false;
	}

	const float EffectiveHorizontalInput = FMath::Clamp(HorizontalInput, -1.0f, 1.0f);
	const float EffectiveThrustInput = FMath::Clamp(ThrustInput, 0.0f, 1.0f);
	const bool bUsingThrusters = CurrentFuel > UE_SMALL_NUMBER
		&& (!FMath::IsNearlyZero(EffectiveHorizontalInput)
			|| EffectiveThrustInput > UE_SMALL_NUMBER);
	if (bUsingThrusters)
	{
		const float FuelUsed = (
			FMath::Abs(EffectiveHorizontalInput) * HorizontalFuelPerSecond
			+ EffectiveThrustInput * VerticalFuelPerSecond) * EffectiveDeltaSeconds;
		CurrentFuel = FMath::Max(0.0f, CurrentFuel - FuelUsed);
	}
	else if (FMath::IsNearlyZero(EffectiveHorizontalInput)
		&& EffectiveThrustInput <= UE_SMALL_NUMBER
		&& VerticalVelocity < 0.0f)
	{
		const float RegenerationRatio = RegenerationFullSpeed > UE_SMALL_NUMBER
			? FMath::Clamp(-VerticalVelocity / RegenerationFullSpeed, 0.0f, 1.0f)
			: 1.0f;
		CurrentFuel = FMath::Min(
			MaxFuel,
			CurrentFuel
				+ DescentRegenerationPerSecond * RegenerationRatio * EffectiveDeltaSeconds);
	}

	if (CurrentFuel <= UE_SMALL_NUMBER && !bFuelEmptyWarningShown)
	{
		bFuelEmptyWarningShown = true;
		return true;
	}
	return false;
}

FFlyingCabImpactResult UFlyingCabVehicleVitalsComponent::ApplyImpact(
	float NormalSpeedChange)
{
	FFlyingCabImpactResult Result;
	if (NormalSpeedChange <= DamageImpactSpeedThreshold
		|| bDestroyed
		|| DamageCooldownRemaining > 0.0f)
	{
		return Result;
	}

	const float DamageRange = FMath::Max(
		DamageFullHullSpeed - DamageImpactSpeedThreshold,
		1.0f);
	const float ImpactAlpha = FMath::Clamp(
		(NormalSpeedChange - DamageImpactSpeedThreshold) / DamageRange,
		0.0f,
		1.0f);
	Result.Damage = MaxHull * FMath::Pow(ImpactAlpha, CollisionDamageExponent);
	if (Result.Damage <= UE_SMALL_NUMBER)
	{
		Result.Damage = 0.0f;
		return Result;
	}

	CurrentHull = FMath::Max(0.0f, CurrentHull - Result.Damage);
	DamageCooldownRemaining = CollisionDamageCooldown;
	DamageFlashRemaining = 0.12f;
	if (CurrentHull <= UE_SMALL_NUMBER)
	{
		bDestroyed = true;
		Result.bDestroyedNow = true;
	}
	return Result;
}

float UFlyingCabVehicleVitalsComponent::AddFuel(float Units)
{
	if (Units <= 0.0f)
	{
		return 0.0f;
	}

	const float PreviousFuel = CurrentFuel;
	CurrentFuel = FMath::Clamp(CurrentFuel + Units, 0.0f, MaxFuel);
	if (CurrentFuel > 1.0f)
	{
		bFuelEmptyWarningShown = false;
	}
	return CurrentFuel - PreviousFuel;
}

float UFlyingCabVehicleVitalsComponent::AddHull(float Units)
{
	if (Units <= 0.0f || bDestroyed)
	{
		return 0.0f;
	}

	const float PreviousHull = CurrentHull;
	CurrentHull = FMath::Clamp(CurrentHull + Units, 0.0f, MaxHull);
	return CurrentHull - PreviousHull;
}

void UFlyingCabVehicleVitalsComponent::Recover(float RecoveryFuelPercent)
{
	bDestroyed = false;
	CurrentHull = MaxHull;
	CurrentFuel = FMath::Max(
		CurrentFuel,
		MaxFuel * FMath::Clamp(RecoveryFuelPercent, 0.0f, 1.0f));
	DamageCooldownRemaining = 0.0f;
	DamageFlashRemaining = 0.0f;
	bFuelEmptyWarningShown = false;
}

bool UFlyingCabVehicleVitalsComponent::CanUseThrusters() const
{
	return !bDestroyed && CurrentFuel > UE_SMALL_NUMBER;
}

float UFlyingCabVehicleVitalsComponent::GetFuelPercent() const
{
	return MaxFuel > UE_SMALL_NUMBER ? CurrentFuel / MaxFuel : 0.0f;
}

float UFlyingCabVehicleVitalsComponent::GetHullPercent() const
{
	return MaxHull > UE_SMALL_NUMBER ? CurrentHull / MaxHull : 0.0f;
}

float UFlyingCabVehicleVitalsComponent::GetFuelNeeded() const
{
	return FMath::Max(0.0f, MaxFuel - CurrentFuel);
}

float UFlyingCabVehicleVitalsComponent::GetHullNeeded() const
{
	return FMath::Max(0.0f, MaxHull - CurrentHull);
}
