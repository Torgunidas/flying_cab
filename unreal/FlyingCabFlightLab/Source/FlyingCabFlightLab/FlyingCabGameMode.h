// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FlyingCabGameMode.generated.h"

UCLASS()
class FLYINGCABFLIGHTLAB_API AFlyingCabGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFlyingCabGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	int32 TryPurchaseFuel(
		class AFlyingCabPawn* Pawn,
		int32 RequestedUnits,
		int32 PricePerUnit);
	int32 TryPurchaseRepair(
		class AFlyingCabPawn* Pawn,
		int32 RequestedUnits,
		int32 PricePerUnit);

private:
	void InitializeDeliveryLoop();
	void InitializeTraffic();
	void EnsurePawnBinding();
	void UpdateActiveFare();
	void HandleZoneReady(class AFlyingCabDeliveryZone* Zone);
	void HandleVehicleDestroyed(class AFlyingCabPawn* Pawn);
	void RecoverVehicleAfterTow();
	void ScheduleNextDispatch();
	void DispatchNextJob();
	void SetRoute(int32 PickupIndex, int32 DropoffIndex);
	void UpdateObjectiveStatus();
	void UpdateTrafficAwareness(float DeltaSeconds);
	void HandleTrafficNearMiss(
		class AFlyingCabTrafficVehicle* Vehicle,
		class AFlyingCabPawn* Pawn);

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Delivery")
	TArray<FVector> DeliveryStops;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Delivery")
	TArray<FString> DeliveryStopNames;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Delivery", meta = (ClampMin = "0.0"))
	float ArrivalMaxPlanarSpeed = 180.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Delivery", meta = (ClampMin = "0.0"))
	float PickupLinkDuration = 0.65f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Delivery", meta = (ClampMin = "0.0"))
	float DropoffExitDuration = 0.55f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Delivery", meta = (ClampMin = "0.0"))
	float DispatchDelay = 1.0f;

	/** Fixed seed keeps prototype routes repeatable while still exercising dynamic dispatch. */
	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Delivery")
	int32 DispatchRandomSeed = 1977;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Economy", meta = (ClampMin = "0"))
	int32 StartingCredits = 100;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Economy", meta = (ClampMin = "0.0"))
	float BaseFare = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Economy", meta = (ClampMin = "0.0"))
	float FarePerMeterTowardTarget = 1.10f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Economy", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FareBacktrackPenaltyRatio = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Economy", meta = (ClampMin = "0"))
	int32 TowFee = 35;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Economy", meta = (ClampMin = "0.0"))
	float DestroyedRecoveryDelay = 2.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Economy", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RecoveryFuelPercent = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Economy")
	TArray<FVector> FuelStationLocations;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Economy")
	TArray<FString> FuelStationNames;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Economy")
	FVector RepairStationLocation = FVector(0.0f, 0.0f, 4200.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Traffic", meta = (ClampMin = "0"))
	int32 NearMissRewardCredits = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Traffic", meta = (ClampMin = "0.1"))
	float TrafficWarningLookAhead = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Traffic", meta = (ClampMin = "0.0"))
	float TrafficWarningVerticalRange = 260.0f;

	UPROPERTY(Transient)
	TObjectPtr<class AFlyingCabDeliveryZone> PickupZone;

	UPROPERTY(Transient)
	TObjectPtr<class AFlyingCabDeliveryZone> DropoffZone;

	UPROPERTY(Transient)
	TArray<TObjectPtr<class AFlyingCabFuelStation>> FuelStations;

	UPROPERTY(Transient)
	TObjectPtr<class AFlyingCabRepairStation> RepairStation;

	UPROPERTY(Transient)
	TArray<TObjectPtr<class AFlyingCabTrafficVehicle>> TrafficVehicles;

	UPROPERTY(Transient)
	TObjectPtr<class AFlyingCabPawn> BoundPawn;

	UPROPERTY(Transient)
	TObjectPtr<class AFlyingCabPawn> PendingRecoveryPawn;

	int32 CurrentPickupIndex = 0;
	int32 CurrentDropoffIndex = 1;
	int32 LastCompletedPickupIndex = INDEX_NONE;
	int32 LastCompletedDropoffIndex = INDEX_NONE;
	int32 CompletedDeliveries = 0;
	int32 Credits = 0;
	float ActiveFare = 0.0f;
	float FareLastDistance = 0.0f;
	float NearMissMessageRemaining = 0.0f;
	bool bPassengerOnBoard = false;
	bool bDispatchPending = false;
	FRandomStream DispatchRandom;
	FTimerHandle RecoveryTimerHandle;
	FTimerHandle DispatchTimerHandle;
};
