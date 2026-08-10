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
	UPROPERTY(SaveGame)
	TArray<float> BestTimeAttackSeconds;
};
