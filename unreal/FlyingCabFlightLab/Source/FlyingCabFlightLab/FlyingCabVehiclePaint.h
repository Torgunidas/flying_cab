#pragma once

#include "CoreMinimal.h"

namespace FlyingCabVehiclePaint
{
	/** Shared body-only finishes; glass and lamps retain their own materials. */
	inline FLinearColor Get(int32 Index)
	{
		static const FColor Colors[] = {
			FColor(229, 184, 73),  // Mustard
			FColor(216, 100, 76),  // Coral
			FColor(66, 150, 154),  // Teal
			FColor(83, 108, 157),  // Blue
			FColor(145, 75, 105),  // Cherry
			FColor(216, 211, 191), // Cream
		};
		return FLinearColor(Colors[FMath::Max(0, Index) % UE_ARRAY_COUNT(Colors)]);
	}
}
