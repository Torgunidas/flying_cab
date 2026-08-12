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
	int32 GetCredits() const;
	class AFlyingCabPawn* GetActiveVehicle() const;

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
	void EnsurePawnBinding();
	void HandlePassengerPickedUp(const FString& DestinationName);
	void HandleFareCompleted(int32 FarePayout, int32 TotalDeliveries);
	void HandleVehicleRecoveryStarted(
		class AFlyingCabPawn* Pawn,
		bool bAffectsActiveRun,
		float RecoveryDelay);
	void HandleVehicleRecovered(
		class AFlyingCabPawn* Pawn,
		bool bRecoveredActiveVehicle);
	void HandleServicePurchase(const struct FFlyingCabServicePurchaseResult& Result);
	void HandleTimeAttackCompleted(const FFlyingCabTimeAttackResult& Result);
	void HandleTrafficNearMiss(
		class AFlyingCabTrafficVehicle* Vehicle,
		class AFlyingCabPawn* Pawn);

	UPROPERTY(Transient)
	TObjectPtr<class AFlyingCabWorldBootstrap> WorldBootstrap;

	UPROPERTY(Transient)
	TObjectPtr<class UFlyingCabEconomyAsset> EconomyConfig;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Economy")
	TObjectPtr<class UFlyingCabEconomyComponent> Economy;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Dispatch")
	TObjectPtr<class UFlyingCabDispatchComponent> Dispatch;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Run")
	TObjectPtr<class UFlyingCabRunComponent> Run;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Interface")
	TObjectPtr<class UFlyingCabHudPresenterComponent> HudPresenter;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Traffic")
	TObjectPtr<class UFlyingCabTrafficAwarenessComponent> TrafficAwareness;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Fleet")
	TObjectPtr<class UFlyingCabFleetComponent> Fleet;

};
