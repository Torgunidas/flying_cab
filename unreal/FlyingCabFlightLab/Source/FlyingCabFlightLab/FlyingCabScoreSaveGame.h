// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "FlyingCabScoreSaveGame.generated.h"

/** Small local leaderboard. Only completed Time Attack runs are persisted. */
UCLASS()
class FLYINGCABFLIGHTLAB_API UFlyingCabScoreSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** Increment before making an incompatible persistent-score format change. */
	UPROPERTY(SaveGame)
	int32 SaveVersion = 1;

	UPROPERTY(SaveGame)
	TArray<float> BestTimeAttackSeconds;
};
