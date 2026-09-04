// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "FlyingCabEconomyAsset.generated.h"

/** Editable economy and recovery tuning shared by all gameplay systems. */
UCLASS(BlueprintType)
class FLYINGCABFLIGHTLAB_API UFlyingCabEconomyAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual void PostLoad() override;

	bool IsConfigurationValid(FString& OutError) const;
	static UFlyingCabEconomyAsset* LoadDefaultAsset();
	static const TCHAR* GetDefaultAssetPath();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Cab|Economy", meta = (ClampMin = "0"))
	int32 StartingCredits = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Cab|Economy", meta = (ClampMin = "0"))
	int32 TowFee = 35;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Cab|Economy", meta = (ClampMin = "0"))
	int32 NearMissRewardCredits = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Cab|Fares", meta = (ClampMin = "0.0"))
	float BaseFare = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Cab|Fares", meta = (ClampMin = "0.0"))
	float FarePerMeterTowardTarget = 1.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Cab|Fares", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FareBacktrackPenaltyRatio = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Cab|Time Attack", meta = (ClampMin = "1"))
	int32 TimeAttackTargetCredits = 1000;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Cab|Services", meta = (ClampMin = "1"))
	int32 FuelPricePerUnit = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Cab|Services", meta = (ClampMin = "1"))
	int32 RepairPricePerHullUnit = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Cab|Recovery", meta = (ClampMin = "0.0"))
	float DestroyedRecoveryDelay = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Cab|Recovery", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RecoveryFuelPercent = 0.25f;
};
