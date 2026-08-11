// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "FlyingCabTrafficAwarenessComponent.generated.h"

class AFlyingCabPawn;
class AFlyingCabTrafficVehicle;

struct FFlyingCabTrafficSample
{
	FVector Location = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
};

struct FFlyingCabTrafficThreat
{
	float ImpactTime = 0.0f;
	bool bFound = false;
	bool bFromLeft = false;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(
	FOnFlyingCabTrafficAlertChanged,
	const FText&,
	const FLinearColor&);
DECLARE_MULTICAST_DELEGATE_TwoParams(
	FOnFlyingCabNearMissDetected,
	AFlyingCabTrafficVehicle*,
	AFlyingCabPawn*);

/** Predicts short-range traffic conflicts and owns their HUD presentation state. */
UCLASS(ClassGroup = "Flying Cab", meta = (BlueprintSpawnableComponent))
class FLYINGCABFLIGHTLAB_API UFlyingCabTrafficAwarenessComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFlyingCabTrafficAwarenessComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(
		float DeltaSeconds,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	void SetTrackedPawn(AFlyingCabPawn* Pawn);
	void ResetTrafficVehicles();
	void RegisterTrafficVehicle(AFlyingCabTrafficVehicle* Vehicle);
	void InitializeTrafficVehicles(
		const TArray<TObjectPtr<AFlyingCabTrafficVehicle>>& Vehicles);
	void ShowNearMissReward(int32 RewardCredits);

	static FFlyingCabTrafficThreat FindClosestThreat(
		const FVector& PawnLocation,
		const FVector& PawnVelocity,
		TConstArrayView<FFlyingCabTrafficSample> TrafficSamples,
		float WarningLookAhead,
		float WarningVerticalRange);

	FOnFlyingCabTrafficAlertChanged OnTrafficAlertChanged;
	FOnFlyingCabNearMissDetected OnNearMissDetected;

private:
	void HandleNearMiss(AFlyingCabTrafficVehicle* Vehicle, AFlyingCabPawn* Pawn);
	void UpdateTrafficAlert(float DeltaSeconds);
	void SetTrafficAlert(const FText& Text, const FLinearColor& Color);

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Traffic", meta = (ClampMin = "0.05"))
	float RefreshInterval = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Traffic", meta = (ClampMin = "0.1"))
	float WarningLookAhead = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Traffic", meta = (ClampMin = "0.0"))
	float WarningVerticalRange = 260.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Traffic", meta = (ClampMin = "0.0"))
	float CriticalWarningTime = 0.65f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Traffic", meta = (ClampMin = "0.0"))
	float NearMissMessageDuration = 1.25f;

	UPROPERTY(Transient)
	TObjectPtr<AFlyingCabPawn> TrackedPawn;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AFlyingCabTrafficVehicle>> TrafficVehicles;

	FText CurrentAlertText;
	FLinearColor CurrentAlertColor = FLinearColor::Transparent;
	int32 NearMissRewardCredits = 0;
	float NearMissMessageRemaining = 0.0f;
};
