// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabCityLayoutAsset.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabCityLayoutAsset, Log, All);

UFlyingCabCityLayoutAsset::UFlyingCabCityLayoutAsset()
{
	for (const FFlyingCabDistrictDefinition& District : FlyingCabCityData::GetFallbackDistricts())
	{
		Districts.Add(District);
	}
	for (const FFlyingCabServiceDefinition& Station : FlyingCabCityData::GetFallbackRepairStations())
	{
		StandaloneRepairStations.Add(Station);
	}
	for (const FFlyingCabTrafficRouteDefinition& Route : FlyingCabCityData::GetFallbackTrafficRoutes())
	{
		TrafficRoutes.Add(Route);
	}
}

void UFlyingCabCityLayoutAsset::PostLoad()
{
	Super::PostLoad();
	FString ValidationError;
	if (!IsConfigurationValid(ValidationError))
	{
		UE_LOG(
			LogFlyingCabCityLayoutAsset,
			Error,
			TEXT("Invalid city layout asset %s: %s"),
			*GetPathName(),
			*ValidationError);
	}
}

bool UFlyingCabCityLayoutAsset::IsConfigurationValid(FString& OutError) const
{
	if (Districts.Num() < 2)
	{
		OutError = TEXT("At least two districts are required.");
		return false;
	}
	if (MinimapWorldMin.X >= MinimapWorldMax.X || MinimapWorldMin.Y >= MinimapWorldMax.Y)
	{
		OutError = TEXT("Minimap bounds must be ordered on both axes.");
		return false;
	}
	if (TrafficRoutes.IsEmpty())
	{
		OutError = TEXT("At least one traffic route is required.");
		return false;
	}

	TSet<FString> Names;
	TSet<FString> Codes;
	TSet<FString> ServiceNames;
	for (const FFlyingCabDistrictDefinition& District : Districts)
	{
		if (District.DisplayName.IsEmpty() || District.MinimapCode.Len() != 2)
		{
			OutError = TEXT("Every district needs a name and a two-character minimap code.");
			return false;
		}
		if (Names.Contains(District.DisplayName) || Codes.Contains(District.MinimapCode))
		{
			OutError = TEXT("District names and minimap codes must be unique.");
			return false;
		}
		const bool bHasRuntimeGeometryName = !District.RuntimeGeometryName.IsEmpty();
		const bool bHasRuntimePlatformWidth = District.RuntimePlatformHalfWidth > 0.0f;
		if (District.RuntimePlatformHalfWidth < 0.0f
			|| bHasRuntimeGeometryName != bHasRuntimePlatformWidth)
		{
			OutError = TEXT("Runtime city platforms need a name and positive half-width.");
			return false;
		}
		const FVector2D Position = District.GetMapPosition();
		if (Position.X < MinimapWorldMin.X || Position.X > MinimapWorldMax.X
			|| Position.Y < MinimapWorldMin.Y || Position.Y > MinimapWorldMax.Y)
		{
			OutError = FString::Printf(
				TEXT("District %s lies outside minimap bounds."),
				*District.DisplayName);
			return false;
		}
		Names.Add(District.DisplayName);
		Codes.Add(District.MinimapCode);
		for (const FString* ServiceName : {&District.FuelStationName, &District.RepairStationName})
		{
			if (!ServiceName->IsEmpty() && ServiceNames.Contains(*ServiceName))
			{
				OutError = TEXT("Service station names must be unique.");
				return false;
			}
			if (!ServiceName->IsEmpty())
			{
				ServiceNames.Add(*ServiceName);
			}
		}
	}
	for (const FFlyingCabServiceDefinition& Station : StandaloneRepairStations)
	{
		const FVector2D Position = Station.GetMapPosition();
		if (Station.DisplayName.IsEmpty() || ServiceNames.Contains(Station.DisplayName)
			|| Position.X < MinimapWorldMin.X || Position.X > MinimapWorldMax.X
			|| Position.Y < MinimapWorldMin.Y || Position.Y > MinimapWorldMax.Y)
		{
			OutError = TEXT("Standalone repair stations need unique names inside minimap bounds.");
			return false;
		}
		ServiceNames.Add(Station.DisplayName);
	}
	for (const FFlyingCabTrafficRouteDefinition& Route : TrafficRoutes)
	{
		const FVector2D Start(Route.Start.X, Route.Start.Z);
		const FVector2D End(Route.End.X, Route.End.Z);
		if (Route.Start.Equals(Route.End) || Route.Speed <= 0.0f
			|| Route.InitialAlpha < 0.0f || Route.InitialAlpha > 1.0f
			|| Start.X < MinimapWorldMin.X || Start.X > MinimapWorldMax.X
			|| Start.Y < MinimapWorldMin.Y || Start.Y > MinimapWorldMax.Y
			|| End.X < MinimapWorldMin.X || End.X > MinimapWorldMax.X
			|| End.Y < MinimapWorldMin.Y || End.Y > MinimapWorldMax.Y)
		{
			OutError = TEXT("Traffic routes need valid endpoints, speed, phase and minimap bounds.");
			return false;
		}
	}

	OutError.Reset();
	return true;
}
