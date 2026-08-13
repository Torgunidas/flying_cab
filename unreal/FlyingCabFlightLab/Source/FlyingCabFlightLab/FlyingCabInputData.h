// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UInputAction;
class UInputMappingContext;

/** Loaded Enhanced Input assets shared by the controller, cab and on-foot pawn. */
struct FFlyingCabInputAssets
{
	UInputMappingContext* MappingContext = nullptr;
	UInputAction* Horizontal = nullptr;
	UInputAction* Thrust = nullptr;
	UInputAction* Service = nullptr;
	UInputAction* Restart = nullptr;
	UInputAction* Telemetry = nullptr;
	UInputAction* Interact = nullptr;
	UInputAction* QuestJournal = nullptr;

	bool IsValid() const
	{
		return MappingContext && Horizontal && Thrust && Service
			&& Restart && Telemetry && Interact && QuestJournal;
	}
};

namespace FlyingCabInputData
{
	FLYINGCABFLIGHTLAB_API const FFlyingCabInputAssets& GetAssets();
}
