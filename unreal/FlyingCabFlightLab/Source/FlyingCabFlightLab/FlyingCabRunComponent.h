// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "FlyingCabRunTypes.h"
#include "FlyingCabRunComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(
	FOnFlyingCabTimeAttackCompleted,
	const FFlyingCabTimeAttackResult&);

/** Owns run lifecycle, run statistics and the local Time Attack leaderboard. */
UCLASS(ClassGroup = "Flying Cab", meta = (BlueprintSpawnableComponent))
class FLYINGCABFLIGHTLAB_API UFlyingCabRunComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFlyingCabRunComponent();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool StartRun(EFlyingCabRunMode Mode);
	bool CheckTimeAttackGoal(int32 CurrentCredits);

	void RecordDelivery(int32 CreditsEarned);
	void RecordNearMiss(int32 CreditsEarned);
	void RecordFuelPurchase(int32 CreditsSpent);
	void RecordRepairPurchase(int32 CreditsSpent);
	void RecordTow(int32 CreditsSpent);

	EFlyingCabRunMode GetCurrentRunMode() const { return CurrentRunMode; }
	bool IsRunActive() const { return bRunActive; }
	bool IsRunCompleted() const { return bRunCompleted; }
	float GetElapsedSeconds() const;
	int32 GetTimeAttackTargetCredits() const { return TimeAttackTargetCredits; }
	TArray<float> GetBestTimeAttackTimes() const;

	static TArray<float> NormalizeLeaderboard(TArray<float> Times, int32 MaxEntries);

	FOnFlyingCabTimeAttackCompleted OnTimeAttackCompleted;

private:
	FFlyingCabTimeAttackResult FinishTimeAttack(int32 FinalCredits);
	void SaveTimeAttackScore(float ElapsedSeconds);

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Time Attack", meta = (ClampMin = "1"))
	int32 TimeAttackTargetCredits = 1000;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Time Attack", meta = (ClampMin = "1", ClampMax = "20"))
	int32 TimeAttackLeaderboardSize = 5;

	EFlyingCabRunMode CurrentRunMode = EFlyingCabRunMode::None;
	float RunStartWorldTime = 0.0f;
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
};
