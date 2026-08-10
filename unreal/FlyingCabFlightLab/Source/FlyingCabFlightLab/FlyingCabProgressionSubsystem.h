// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FlyingCabProgressionSubsystem.generated.h"

/** Session-persistent access rights, deliberately independent from vehicle possession. */
UCLASS()
class FLYINGCABFLIGHTLAB_API UFlyingCabProgressionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Flying Cab|Progression")
	bool HasAccess(FName AccessId) const;

	/** Returns true only when a new permission was added. */
	UFUNCTION(BlueprintCallable, Category = "Flying Cab|Progression")
	bool GrantAccess(FName AccessId);

	/** Used to give every competitive run the same starting permissions. */
	void ResetAccess();

private:
	UPROPERTY(Transient)
	TSet<FName> GrantedAccess;
};
