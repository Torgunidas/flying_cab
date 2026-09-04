// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabEconomyAsset.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabEconomyAsset, Log, All);

namespace
{
	const TCHAR* EconomyAssetPath =
		TEXT("/Game/Data/DA_FlyingCabEconomy.DA_FlyingCabEconomy");
}

void UFlyingCabEconomyAsset::PostLoad()
{
	Super::PostLoad();
	FString ValidationError;
	if (!IsConfigurationValid(ValidationError))
	{
		UE_LOG(
			LogFlyingCabEconomyAsset,
			Error,
			TEXT("Invalid economy asset %s: %s"),
			*GetPathName(),
			*ValidationError);
	}
}

bool UFlyingCabEconomyAsset::IsConfigurationValid(FString& OutError) const
{
	if (StartingCredits < 0 || TowFee < 0 || NearMissRewardCredits < 0
		|| BaseFare < 0.0f || FarePerMeterTowardTarget < 0.0f)
	{
		OutError = TEXT("Credits, fees, rewards and fare rates cannot be negative.");
		return false;
	}
	if (FareBacktrackPenaltyRatio < 0.0f || FareBacktrackPenaltyRatio > 1.0f)
	{
		OutError = TEXT("Fare backtrack penalty ratio must be in [0, 1].");
		return false;
	}
	if (TimeAttackTargetCredits <= 0 || FuelPricePerUnit <= 0
		|| RepairPricePerHullUnit <= 0)
	{
		OutError = TEXT("Time Attack target and service prices must be positive.");
		return false;
	}
	if (DestroyedRecoveryDelay < 0.0f
		|| RecoveryFuelPercent < 0.0f || RecoveryFuelPercent > 1.0f)
	{
		OutError = TEXT("Recovery delay must be non-negative and fuel percent must be in [0, 1].");
		return false;
	}

	OutError.Reset();
	return true;
}

UFlyingCabEconomyAsset* UFlyingCabEconomyAsset::LoadDefaultAsset()
{
	static UFlyingCabEconomyAsset* Asset = []()
	{
		UFlyingCabEconomyAsset* Loaded = LoadObject<UFlyingCabEconomyAsset>(
			nullptr,
			EconomyAssetPath);
		FString ValidationError;
		if (Loaded && Loaded->IsConfigurationValid(ValidationError))
		{
			Loaded->AddToRoot();
			return Loaded;
		}

		UFlyingCabEconomyAsset* Fallback = NewObject<UFlyingCabEconomyAsset>(
			GetTransientPackage(),
			TEXT("FlyingCabFallbackEconomy"));
		Fallback->AddToRoot();
		UE_LOG(
			LogFlyingCabEconomyAsset,
			Warning,
			TEXT("Using built-in economy defaults because %s is missing or invalid."),
			EconomyAssetPath);
		return Fallback;
	}();
	return Asset;
}

const TCHAR* UFlyingCabEconomyAsset::GetDefaultAssetPath()
{
	return EconomyAssetPath;
}
