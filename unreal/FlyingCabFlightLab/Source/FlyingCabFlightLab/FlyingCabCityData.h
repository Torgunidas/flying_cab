// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/ArrayView.h"
#include "CoreMinimal.h"

/** Canonical district definition shared by gameplay, city geometry and the minimap. */
struct FFlyingCabDistrictDefinition
{
	const TCHAR* DisplayName;
	const TCHAR* MinimapCode;
	FVector StopLocation;
	const TCHAR* RuntimeGeometryName;
	float RuntimePlatformHalfWidth;
	FLinearColor AccentColor;
	const TCHAR* FuelStationName;
	const TCHAR* RepairStationName;

	FVector2D GetMapPosition() const
	{
		return FVector2D(StopLocation.X, StopLocation.Z);
	}

	bool BuildsRuntimeGeometry() const
	{
		return RuntimeGeometryName != nullptr && RuntimePlatformHalfWidth > 0.0f;
	}
};

/** Canonical service location used by gameplay spawning and the minimap. */
struct FFlyingCabServiceDefinition
{
	const TCHAR* DisplayName;
	FVector Location;

	FVector2D GetMapPosition() const
	{
		return FVector2D(Location.X, Location.Z);
	}
};

namespace FlyingCabCityData
{
	TConstArrayView<FFlyingCabDistrictDefinition> GetDistricts();
	TArray<FFlyingCabServiceDefinition> GetFuelStations();
	TArray<FFlyingCabServiceDefinition> GetRepairStations();
	FVector2D GetMinimapWorldMin();
	FVector2D GetMinimapWorldMax();
}
