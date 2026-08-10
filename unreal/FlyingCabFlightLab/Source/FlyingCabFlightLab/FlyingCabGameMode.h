// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlyingCabRunTypes.h"
#include "GameFramework/GameModeBase.h"
#include "FlyingCabGameMode.generated.h"

USTRUCT()
struct FFlyingCabPassengerOfferState
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<class AFlyingCabDeliveryZone> Zone;

	int32 PickupIndex = INDEX_NONE;
	int32 DropoffIndex = INDEX_NONE;
	int32 EstimatedFareCredits = 0;
	float RemainingSeconds = 0.0f;
};

UCLASS()
class FLYINGCABFLIGHTLAB_API AFlyingCabGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFlyingCabGameMode();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	void StartRun(EFlyingCabRunMode Mode);
	TArray<float> GetBestTimeAttackTimes() const;
	EFlyingCabRunMode GetCurrentRunMode() const { return CurrentRunMode; }
	int32 GetTimeAttackTargetCredits() const { return TimeAttackTargetCredits; }

	int32 TryPurchaseFuel(
		class AFlyingCabPawn* Pawn,
		int32 RequestedUnits,
		int32 PricePerUnit);
	int32 TryPurchaseRepair(
		class AFlyingCabPawn* Pawn,
		int32 RequestedUnits,
		int32 PricePerUnit);
	bool CanPlayerExitVehicle(FText& OutFailureReason) const;

private:
	void InitializeCityExpansion();
	void InitializeDeliveryLoop();
	void InitializePassengerMarket();
	void InitializeTraffic();
	void InitializeOnFootSlice();
	void InitializeServiceVehicle();
	void RegisterVehicle(class AFlyingCabPawn* Pawn);
	void EnsurePawnBinding();
	void UpdatePassengerOffers(float DeltaSeconds);
	void SpawnPassengerOffer();
	void RemovePassengerOfferAt(int32 OfferIndex, const TCHAR* Reason);
	void SetPassengerOfferAcceptance(bool bEnabled);
	int32 FindPassengerOfferIndex(const class AFlyingCabDeliveryZone* Zone) const;
	int32 CalculateEstimatedFare(int32 PickupIndex, int32 DropoffIndex) const;
	void UpdateActiveFare();
	void HandleZoneReady(class AFlyingCabDeliveryZone* Zone);
	void HandleVehicleDestroyed(class AFlyingCabPawn* Pawn);
	void ScheduleVehicleRecovery(class AFlyingCabPawn* Pawn);
	void RecoverVehicleAfterTow(class AFlyingCabPawn* Pawn);
	void UpdateObjectiveStatus();
	void UpdateTrafficAwareness(float DeltaSeconds);
	void UpdateRunModeStatus();
	void CheckTimeAttackGoal();
	void FinishTimeAttack();
	void SaveTimeAttackScore(float ElapsedSeconds);
	float GetRunElapsedSeconds() const;
	void HandleTrafficNearMiss(
		class AFlyingCabTrafficVehicle* Vehicle,
		class AFlyingCabPawn* Pawn);
	void ShowPlayerEventMessage(
		const FText& Message,
		const FLinearColor& Color,
		float DurationSeconds) const;
	bool IsPlayerOnFoot() const;

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

	/** Fixed seed keeps Time Attack passenger availability repeatable. */
	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Delivery")
	int32 DispatchRandomSeed = 1977;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Passengers", meta = (ClampMin = "1", ClampMax = "8"))
	int32 MaxWaitingPassengers = 4;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Passengers", meta = (ClampMin = "1", ClampMax = "8"))
	int32 InitialWaitingPassengers = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Passengers", meta = (ClampMin = "1.0"))
	float PassengerLifetimeMin = 32.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Passengers", meta = (ClampMin = "1.0"))
	float PassengerLifetimeMax = 52.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Passengers", meta = (ClampMin = "0.1"))
	float PassengerSpawnIntervalMin = 3.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Passengers", meta = (ClampMin = "0.1"))
	float PassengerSpawnIntervalMax = 7.0f;

	/** Exact in-world pointer appears only inside this range. */
	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Delivery", meta = (ClampMin = "0.0"))
	float ProximityGuidanceRange = 1800.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Economy", meta = (ClampMin = "0"))
	int32 StartingCredits = 100;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Time Attack", meta = (ClampMin = "1"))
	int32 TimeAttackTargetCredits = 1000;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Time Attack", meta = (ClampMin = "1", ClampMax = "20"))
	int32 TimeAttackLeaderboardSize = 5;

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
	TArray<FVector> RepairStationLocations;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Economy")
	TArray<FString> RepairStationNames;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|On Foot")
	FVector NightshiftEntranceLocation = FVector(470.0f, 0.0f, 4120.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|On Foot")
	FVector NightshiftExteriorReturnLocation = FVector(310.0f, 0.0f, 4068.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|On Foot")
	FVector NightshiftOfficeLocation = FVector(23000.0f, 0.0f, 650.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|On Foot")
	FVector ServiceVehicleLocation = FVector(-600.0f, 0.0f, 4040.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Traffic", meta = (ClampMin = "0"))
	int32 NearMissRewardCredits = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Traffic", meta = (ClampMin = "0.1"))
	float TrafficWarningLookAhead = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Traffic", meta = (ClampMin = "0.0"))
	float TrafficWarningVerticalRange = 260.0f;

	UPROPERTY(Transient)
	TObjectPtr<class AFlyingCabDeliveryZone> DropoffZone;

	UPROPERTY(Transient)
	TArray<FFlyingCabPassengerOfferState> PassengerOffers;

	UPROPERTY(Transient)
	TArray<TObjectPtr<class AFlyingCabFuelStation>> FuelStations;

	UPROPERTY(Transient)
	TArray<TObjectPtr<class AFlyingCabRepairStation>> RepairStations;

	UPROPERTY(Transient)
	TObjectPtr<class AFlyingCabCityExpansion> CityExpansion;

	UPROPERTY(Transient)
	TObjectPtr<class AFlyingCabNightshiftOffice> NightshiftOffice;

	UPROPERTY(Transient)
	TObjectPtr<class AFlyingCabOnFootPortal> NightshiftEntrance;

	UPROPERTY(Transient)
	TObjectPtr<class AFlyingCabOnFootPortal> NightshiftExit;

	UPROPERTY(Transient)
	TObjectPtr<class AFlyingCabAccessTerminal> ServiceAccessTerminal;

	UPROPERTY(Transient)
	TObjectPtr<class AFlyingCabPawn> ServiceVehicle;

	UPROPERTY(Transient)
	TArray<TObjectPtr<class AFlyingCabTrafficVehicle>> TrafficVehicles;

	UPROPERTY(Transient)
	TArray<TObjectPtr<class AFlyingCabPawn>> TrackedVehicles;

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
	EFlyingCabRunMode CurrentRunMode = EFlyingCabRunMode::None;
	float RunStartWorldTime = 0.0f;
	int32 RunStartingCredits = 0;
	int32 RunCompletedDeliveries = 0;
	int32 RunDeliveryCreditsEarned = 0;
	int32 RunNearMissCount = 0;
	int32 RunNearMissCreditsEarned = 0;
	int32 RunFuelCreditsSpent = 0;
	int32 RunRepairCreditsSpent = 0;
	int32 RunTowCount = 0;
	int32 RunTowCreditsSpent = 0;
	bool bRunActive = false;
	bool bRunCompleted = false;
	float ActiveFare = 0.0f;
	float FareLastDistance = 0.0f;
	float NearMissMessageRemaining = 0.0f;
	float PassengerSpawnCountdown = 0.0f;
	bool bPassengerOnBoard = false;
	FRandomStream DispatchRandom;
	TMap<TWeakObjectPtr<class AFlyingCabPawn>, FTimerHandle> VehicleRecoveryTimerHandles;
};
