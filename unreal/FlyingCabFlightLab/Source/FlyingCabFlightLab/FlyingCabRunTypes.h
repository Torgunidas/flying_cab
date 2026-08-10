// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlyingCabRunTypes.generated.h"

UENUM(BlueprintType)
enum class EFlyingCabRunMode : uint8
{
	None,
	Freeroam,
	TimeAttack
};

USTRUCT(BlueprintType)
struct FLYINGCABFLIGHTLAB_API FFlyingCabTimeAttackResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	float ElapsedSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	int32 FinalCredits = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 TargetCredits = 1000;

	UPROPERTY(BlueprintReadOnly)
	int32 CompletedDeliveries = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 DeliveryCreditsEarned = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 NearMissCount = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 NearMissCreditsEarned = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 FuelCreditsSpent = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 RepairCreditsSpent = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 TowCount = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 TowCreditsSpent = 0;
};
