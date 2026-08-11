// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "FlyingCabEconomyComponent.generated.h"

class AFlyingCabPawn;

struct FFlyingCabServicePurchaseResult
{
	int32 UnitsPurchased = 0;
	int32 CreditsSpent = 0;
	bool bInsufficientCredits = false;
	bool bRepairService = false;
};

DECLARE_MULTICAST_DELEGATE_OneParam(
	FOnFlyingCabServicePurchase,
	const FFlyingCabServicePurchaseResult&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnFlyingCabCreditsChanged, int32);

/** Owns the current balance and the arithmetic of rewards, fees and vehicle service. */
UCLASS(ClassGroup = "Flying Cab", meta = (BlueprintSpawnableComponent))
class FLYINGCABFLIGHTLAB_API UFlyingCabEconomyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFlyingCabEconomyComponent();

	void ResetCredits();
	int32 GetCredits() const { return Credits; }
	int32 GetNearMissRewardCredits() const { return NearMissRewardCredits; }
	int32 AddCredits(int32 RequestedCredits);
	int32 ChargeTowFee();
	int32 AwardNearMiss();
	FFlyingCabServicePurchaseResult TryPurchaseFuel(
		AFlyingCabPawn* Pawn,
		int32 RequestedUnits,
		int32 PricePerUnit);
	FFlyingCabServicePurchaseResult TryPurchaseRepair(
		AFlyingCabPawn* Pawn,
		int32 RequestedUnits,
		int32 PricePerUnit);

	static int32 CalculateServicePurchaseUnits(
		int32 RequestedUnits,
		float NeededUnits,
		int32 AvailableCredits,
		int32 PricePerUnit);

	FOnFlyingCabServicePurchase OnServicePurchase;
	FOnFlyingCabCreditsChanged OnCreditsChanged;

private:
	FFlyingCabServicePurchaseResult TryPurchaseService(
		AFlyingCabPawn* Pawn,
		int32 RequestedUnits,
		int32 PricePerUnit,
		bool bRepairService);

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Economy", meta = (ClampMin = "0"))
	int32 StartingCredits = 100;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Economy", meta = (ClampMin = "0"))
	int32 TowFee = 35;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Economy", meta = (ClampMin = "0"))
	int32 NearMissRewardCredits = 3;

	int32 Credits = 0;
};
