#pragma once

#include "CoreMinimal.h"

namespace FlyingCabTrafficSignals
{
	// Four seconds of all-red clear the intersection before the other axis starts.
	inline bool IsGreen(FName Group, double WorldSeconds)
	{
		if (Group.IsNone()) return true;
		const double Phase = FMath::Fmod(FMath::Max(0.0, WorldSeconds), 40.0);
		return Group == TEXT("Horizontal") ? Phase < 16.0
			: Group == TEXT("Vertical") && Phase >= 20.0 && Phase < 36.0;
	}
}
