// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabInputData.h"

#include "InputAction.h"
#include "InputMappingContext.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabInputData, Log, All);

namespace
{
	template <typename TAsset>
	TAsset* LoadInputAsset(const TCHAR* Path)
	{
		TAsset* Asset = LoadObject<TAsset>(nullptr, Path);
		if (Asset)
		{
			Asset->AddToRoot();
		}
		return Asset;
	}
}

const FFlyingCabInputAssets& FlyingCabInputData::GetAssets()
{
	static const FFlyingCabInputAssets Assets = []()
	{
		FFlyingCabInputAssets Loaded;
		Loaded.MappingContext = LoadInputAsset<UInputMappingContext>(
			TEXT("/Game/Input/IMC_FlyingCabGameplay.IMC_FlyingCabGameplay"));
		Loaded.Horizontal = LoadInputAsset<UInputAction>(
			TEXT("/Game/Input/IA_FlyingCabHorizontal.IA_FlyingCabHorizontal"));
		Loaded.Thrust = LoadInputAsset<UInputAction>(
			TEXT("/Game/Input/IA_FlyingCabThrust.IA_FlyingCabThrust"));
		Loaded.Service = LoadInputAsset<UInputAction>(
			TEXT("/Game/Input/IA_FlyingCabService.IA_FlyingCabService"));
		Loaded.Restart = LoadInputAsset<UInputAction>(
			TEXT("/Game/Input/IA_FlyingCabRestart.IA_FlyingCabRestart"));
		Loaded.Telemetry = LoadInputAsset<UInputAction>(
			TEXT("/Game/Input/IA_FlyingCabTelemetry.IA_FlyingCabTelemetry"));
		Loaded.Interact = LoadInputAsset<UInputAction>(
			TEXT("/Game/Input/IA_FlyingCabInteract.IA_FlyingCabInteract"));
		if (!Loaded.IsValid())
		{
			UE_LOG(
				LogFlyingCabInputData,
				Error,
				TEXT("Enhanced Input assets are incomplete; gameplay input cannot be bound."));
		}
		return Loaded;
	}();
	return Assets;
}
