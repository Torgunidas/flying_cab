// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabEconomyComponent.h"

#include "FlyingCabEconomyAsset.h"
#include "FlyingCabPawn.h"

UFlyingCabEconomyComponent::UFlyingCabEconomyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFlyingCabEconomyComponent::Configure(const UFlyingCabEconomyAsset* Config)
{
	if (!Config)
	{
		return;
	}
	StartingCredits = FMath::Max(0, Config->StartingCredits);
	TowFee = FMath::Max(0, Config->TowFee);
	NearMissRewardCredits = FMath::Max(0, Config->NearMissRewardCredits);
}

void UFlyingCabEconomyComponent::ResetCredits()
{
	Credits = FMath::Max(0, StartingCredits);
	OnCreditsChanged.Broadcast(Credits);
}

int32 UFlyingCabEconomyComponent::AddCredits(int32 RequestedCredits)
{
	const int32 AwardedCredits = FMath::Max(0, RequestedCredits);
	Credits += AwardedCredits;
	if (AwardedCredits > 0)
	{
		OnCreditsChanged.Broadcast(Credits);
		OnCreditsEarned.Broadcast(AwardedCredits);
	}
	return AwardedCredits;
}

int32 UFlyingCabEconomyComponent::ChargeTowFee()
{
	const int32 ChargedCredits = FMath::Min(Credits, FMath::Max(0, TowFee));
	Credits -= ChargedCredits;
	if (ChargedCredits > 0)
	{
		OnCreditsChanged.Broadcast(Credits);
	}
	return ChargedCredits;
}

int32 UFlyingCabEconomyComponent::AwardNearMiss()
{
	return AddCredits(NearMissRewardCredits);
}

FFlyingCabServicePurchaseResult UFlyingCabEconomyComponent::TryPurchaseFuel(
	AFlyingCabPawn* Pawn,
	int32 RequestedUnits,
	int32 PricePerUnit)
{
	FFlyingCabServicePurchaseResult Result = TryPurchaseService(
		Pawn,
		RequestedUnits,
		PricePerUnit,
		false);
	if (Result.UnitsPurchased > 0 || Result.bInsufficientCredits)
	{
		OnServicePurchase.Broadcast(Result);
	}
	return Result;
}

FFlyingCabServicePurchaseResult UFlyingCabEconomyComponent::TryPurchaseRepair(
	AFlyingCabPawn* Pawn,
	int32 RequestedUnits,
	int32 PricePerUnit)
{
	FFlyingCabServicePurchaseResult Result = TryPurchaseService(
		Pawn,
		RequestedUnits,
		PricePerUnit,
		true);
	Result.bRepairService = true;
	if (Result.UnitsPurchased > 0 || Result.bInsufficientCredits)
	{
		OnServicePurchase.Broadcast(Result);
	}
	return Result;
}

int32 UFlyingCabEconomyComponent::CalculateServicePurchaseUnits(
	int32 RequestedUnits,
	float NeededUnits,
	int32 AvailableCredits,
	int32 PricePerUnit)
{
	if (RequestedUnits <= 0 || NeededUnits <= 0.0f
		|| AvailableCredits <= 0 || PricePerUnit <= 0)
	{
		return 0;
	}

	const int32 RoundedNeededUnits = FMath::Max(0, FMath::CeilToInt(NeededUnits));
	const int32 AffordableUnits = AvailableCredits / PricePerUnit;
	return FMath::Min3(RequestedUnits, RoundedNeededUnits, AffordableUnits);
}

FFlyingCabServicePurchaseResult UFlyingCabEconomyComponent::TryPurchaseService(
	AFlyingCabPawn* Pawn,
	int32 RequestedUnits,
	int32 PricePerUnit,
	bool bRepairService)
{
	FFlyingCabServicePurchaseResult Result;
	if (!Pawn || Pawn->IsDestroyed() || RequestedUnits <= 0 || PricePerUnit <= 0)
	{
		return Result;
	}

	const float NeededUnits = bRepairService ? Pawn->GetHullNeeded() : Pawn->GetFuelNeeded();
	const int32 AffordableUnits = Credits / PricePerUnit;
	const int32 UnitsToPurchase = CalculateServicePurchaseUnits(
		RequestedUnits,
		NeededUnits,
		Credits,
		PricePerUnit);
	if (UnitsToPurchase <= 0)
	{
		Result.bInsufficientCredits = AffordableUnits <= 0 && NeededUnits > 0.0f;
		return Result;
	}

	const float UnitsAdded = bRepairService
		? Pawn->AddHull(static_cast<float>(UnitsToPurchase))
		: Pawn->AddFuel(static_cast<float>(UnitsToPurchase));
	if (UnitsAdded <= UE_SMALL_NUMBER)
	{
		return Result;
	}

	Result.UnitsPurchased = FMath::CeilToInt(UnitsAdded);
	Result.CreditsSpent = Result.UnitsPurchased * PricePerUnit;
	Credits = FMath::Max(0, Credits - Result.CreditsSpent);
	OnCreditsChanged.Broadcast(Credits);
	return Result;
}
