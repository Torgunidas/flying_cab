// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlyingCabRunTypes.h"
#include "GameFramework/GameModeBase.h"
#include "FlyingCabGameMode.generated.h"

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
	void InitializeWorldBootstrap();
	void InitializeDispatch();
	void RegisterVehicle(class AFlyingCabPawn* Pawn);
	void EnsurePawnBinding();
	void HandlePassengerPickedUp(const FString& DestinationName);
	void HandleFareCompleted(int32 FarePayout, int32 TotalDeliveries);
	void HandleVehicleDestroyed(class AFlyingCabPawn* Pawn);
	void ScheduleVehicleRecovery(class AFlyingCabPawn* Pawn);
	void RecoverVehicleAfterTow(class AFlyingCabPawn* Pawn);
	void UpdateProximityGuidance();
	void UpdateObjectiveStatus();
	void UpdateRunModeStatus();
	void CheckTimeAttackGoal();
	void FinishTimeAttack();
	void SaveTimeAttackScore(float ElapsedSeconds);
	float GetRunElapsedSeconds() const;
	void HandleTrafficNearMiss(
		class AFlyingCabTrafficVehicle* Vehicle,
		class AFlyingCabPawn* Pawn);
	void HandleTrafficAlertChanged(const FText& Alert, const FLinearColor& Color);
	void ShowPlayerEventMessage(
		const FText& Message,
		const FLinearColor& Color,
		float DurationSeconds) const;
	class AFlyingCabPlayerController* GetFlyingCabPlayerController() const;
	void PushEconomyStatus() const;
	bool IsPlayerOnFoot() const;

	/** Exact in-world pointer appears only inside this range. */
	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Delivery", meta = (ClampMin = "0.0"))
	float ProximityGuidanceRange = 1800.0f;

	/** Text and minimap presentation do not need per-frame Slate invalidation. */
	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Interface", meta = (ClampMin = "0.05"))
	float HudRefreshInterval = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Economy", meta = (ClampMin = "0"))
	int32 StartingCredits = 100;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Time Attack", meta = (ClampMin = "1"))
	int32 TimeAttackTargetCredits = 1000;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Time Attack", meta = (ClampMin = "1", ClampMax = "20"))
	int32 TimeAttackLeaderboardSize = 5;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Economy", meta = (ClampMin = "0"))
	int32 TowFee = 35;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Economy", meta = (ClampMin = "0.0"))
	float DestroyedRecoveryDelay = 2.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Economy", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RecoveryFuelPercent = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Traffic", meta = (ClampMin = "0"))
	int32 NearMissRewardCredits = 3;

	UPROPERTY(Transient)
	TObjectPtr<class AFlyingCabWorldBootstrap> WorldBootstrap;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Dispatch")
	TObjectPtr<class UFlyingCabDispatchComponent> Dispatch;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Traffic")
	TObjectPtr<class UFlyingCabTrafficAwarenessComponent> TrafficAwareness;

	UPROPERTY(Transient)
	TArray<TObjectPtr<class AFlyingCabPawn>> TrackedVehicles;

	UPROPERTY(Transient)
	TObjectPtr<class AFlyingCabPawn> BoundPawn;

	UPROPERTY(Transient)
	TObjectPtr<class AFlyingCabPawn> PendingRecoveryPawn;

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
	float HudRefreshElapsed = 0.0f;
	TMap<TWeakObjectPtr<class AFlyingCabPawn>, FTimerHandle> VehicleRecoveryTimerHandles;
};
