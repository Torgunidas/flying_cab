// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "FlyingCabFleetComponent.generated.h"

class AFlyingCabPawn;
class UFlyingCabEconomyAsset;

DECLARE_MULTICAST_DELEGATE_ThreeParams(
	FOnFlyingCabVehicleRecoveryStarted,
	AFlyingCabPawn*,
	bool,
	float);
DECLARE_MULTICAST_DELEGATE_TwoParams(
	FOnFlyingCabVehicleRecovered,
	AFlyingCabPawn*,
	bool);

/** Owns the player fleet registry, destruction classification and recovery timers. */
UCLASS(ClassGroup = "Flying Cab", meta = (BlueprintSpawnableComponent))
class FLYINGCABFLIGHTLAB_API UFlyingCabFleetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFlyingCabFleetComponent();
	void Configure(const UFlyingCabEconomyAsset* Config);

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void RegisterVehicle(AFlyingCabPawn* Pawn);
	void SetActiveVehicle(AFlyingCabPawn* Pawn);
	AFlyingCabPawn* GetActiveVehicle() const { return ActiveVehicle; }

	FOnFlyingCabVehicleRecoveryStarted OnVehicleRecoveryStarted;
	FOnFlyingCabVehicleRecovered OnVehicleRecovered;

private:
	void HandleVehicleDestroyed(AFlyingCabPawn* Pawn);
	void ScheduleVehicleRecovery(AFlyingCabPawn* Pawn);
	void RecoverVehicle(AFlyingCabPawn* Pawn);

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Fleet", meta = (ClampMin = "0.0"))
	float DestroyedRecoveryDelay = 2.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Fleet", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RecoveryFuelPercent = 0.25f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AFlyingCabPawn>> TrackedVehicles;

	UPROPERTY(Transient)
	TObjectPtr<AFlyingCabPawn> ActiveVehicle;

	UPROPERTY(Transient)
	TObjectPtr<AFlyingCabPawn> PendingActiveRecoveryVehicle;

	TMap<TWeakObjectPtr<AFlyingCabPawn>, FTimerHandle> RecoveryTimerHandles;
};
