// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "FlyingCabDispatchComponent.generated.h"

class AFlyingCabDeliveryZone;
class AFlyingCabPawn;
class UFlyingCabEconomyAsset;

USTRUCT()
struct FFlyingCabPassengerOfferState
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<AFlyingCabDeliveryZone> Zone;

	int32 PickupIndex = INDEX_NONE;
	int32 DropoffIndex = INDEX_NONE;
	int32 EstimatedFareCredits = 0;
	float RemainingSeconds = 0.0f;
};

DECLARE_MULTICAST_DELEGATE_OneParam(
	FOnFlyingCabPassengerPickedUp,
	const FString&);
DECLARE_MULTICAST_DELEGATE_TwoParams(
	FOnFlyingCabFareCompleted,
	int32,
	int32);

/** Owns passenger offers, the active ride and distance-based fare calculation. */
UCLASS(ClassGroup = "Flying Cab", meta = (BlueprintSpawnableComponent))
class FLYINGCABFLIGHTLAB_API UFlyingCabDispatchComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFlyingCabDispatchComponent();
	void Configure(const UFlyingCabEconomyAsset* Config);

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(
		float DeltaSeconds,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	bool InitializeNetwork();
	void StartPassengerMarket(bool bUseFixedSeed);
	void SetMarketActive(bool bActive) { bMarketActive = bActive; }
	void SetTrackedPawn(AFlyingCabPawn* Pawn) { TrackedPawn = Pawn; }
	void SetOfferAcceptanceAllowed(bool bAllowed);
	void AbortActiveRide();
	bool CanPlayerExitVehicle(FText& OutFailureReason) const;

	bool HasPassengerOnBoard() const { return bPassengerOnBoard; }
	bool IsCurbsideLinkInProgress() const;
	int32 GetActiveFareCredits() const;
	int32 GetCompletedDeliveries() const { return CompletedDeliveries; }
	int32 GetCurrentDropoffIndex() const { return CurrentDropoffIndex; }
	int32 GetOfferCount() const { return PassengerOffers.Num(); }
	int32 GetStopCount() const { return DeliveryStops.Num(); }
	FString GetStopName(int32 StopIndex) const;
	AFlyingCabDeliveryZone* GetDropoffZone() const { return DropoffZone; }
	const TArray<FFlyingCabPassengerOfferState>& GetPassengerOffers() const
	{
		return PassengerOffers;
	}
	const FFlyingCabPassengerOfferState* FindNearestOffer(
		const FVector& WorldLocation,
		AFlyingCabDeliveryZone*& OutZone) const;

	static int32 CalculateEstimatedFare(
		const FVector& PickupLocation,
		const FVector& DropoffLocation,
		float BaseFare,
		float FarePerMeterTowardTarget);
	static float CalculateUpdatedFare(
		float CurrentFare,
		float PreviousDistanceCm,
		float CurrentDistanceCm,
		float BaseFare,
		float FarePerMeterTowardTarget,
		float FareBacktrackPenaltyRatio);

	FOnFlyingCabPassengerPickedUp OnPassengerPickedUp;
	FOnFlyingCabFareCompleted OnFareCompleted;

private:
	void UpdatePassengerOffers(float DeltaSeconds);
	void UpdateActiveFare();
	void SpawnPassengerOffer();
	void RemovePassengerOfferAt(int32 OfferIndex, const TCHAR* Reason);
	void RefreshOfferAcceptance();
	int32 FindPassengerOfferIndex(const AFlyingCabDeliveryZone* Zone) const;
	int32 CalculateEstimatedFare(int32 PickupIndex, int32 DropoffIndex) const;
	void HandleZoneReady(AFlyingCabDeliveryZone* Zone);

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

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Economy", meta = (ClampMin = "0.0"))
	float BaseFare = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Economy", meta = (ClampMin = "0.0"))
	float FarePerMeterTowardTarget = 1.10f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Economy", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FareBacktrackPenaltyRatio = 0.5f;

	UPROPERTY(Transient)
	TObjectPtr<AFlyingCabDeliveryZone> DropoffZone;

	UPROPERTY(Transient)
	TArray<FFlyingCabPassengerOfferState> PassengerOffers;

	UPROPERTY(Transient)
	TObjectPtr<AFlyingCabPawn> TrackedPawn;

	int32 CurrentPickupIndex = INDEX_NONE;
	int32 CurrentDropoffIndex = INDEX_NONE;
	int32 CompletedDeliveries = 0;
	float ActiveFare = 0.0f;
	float FareLastDistance = 0.0f;
	float PassengerSpawnCountdown = 0.0f;
	bool bPassengerOnBoard = false;
	bool bMarketActive = false;
	bool bOfferAcceptanceAllowed = true;
	FRandomStream DispatchRandom;
};
