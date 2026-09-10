#pragma once
#include "CoreMinimal.h"

namespace FlyingCabSupercarData
{
	inline float GetBayOffsetX(FName DistrictId)
	{
		// The west side of Ashline Market is the vertical approach to Ashline Court.
		return DistrictId == TEXT("District.AshlineMarket") ? 1330.f : -1330.f;
	}
	inline TConstArrayView<FName> GetDistrictIds()
	{
		static const FName Ids[] = {TEXT("District.YellowProjects"), TEXT("District.AshlineMarket"),
			TEXT("District.NeonDocks"), TEXT("District.GlasswardTransit")};
		return MakeArrayView(Ids);
	}
}
