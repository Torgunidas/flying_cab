// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabGameMode.h"

#include "FlyingCabPawn.h"

AFlyingCabGameMode::AFlyingCabGameMode()
{
	DefaultPawnClass = AFlyingCabPawn::StaticClass();
}
