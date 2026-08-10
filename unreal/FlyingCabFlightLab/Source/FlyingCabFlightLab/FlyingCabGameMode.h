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
	EFlyingCabRunMode GetCurrentRunMode() const;
	int32 GetTimeAttackTargetCredits() const;

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
	void HandleTimeAttackCompleted(const FFlyingCabTimeAttackResult& Result);
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

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Run")
	TObjectPtr<class UFlyingCabRunComponent> Run;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Traffic")
	TObjectPtr<class UFlyingCabTrafficAwarenessComponent> TrafficAwareness;

	UPROPERTY(Transient)
	TArray<TObjectPtr<class AFlyingCabPawn>> TrackedVehicles;

	UPROPERTY(Transient)
	TObjectPtr<class AFlyingCabPawn> BoundPawn;

	UPROPERTY(Transient)
	TObjectPtr<class AFlyingCabPawn> PendingRecoveryPawn;

	int32 Credits = 0;
	float HudRefreshElapsed = 0.0f;
	TMap<TWeakObjectPtr<class AFlyingCabPawn>, FTimerHandle> VehicleRecoveryTimerHandles;
};
