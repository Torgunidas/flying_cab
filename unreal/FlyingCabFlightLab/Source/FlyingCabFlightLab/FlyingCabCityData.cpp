// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabCityData.h"

#include "FlyingCabCityLayoutAsset.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabCityData, Log, All);

namespace
{
	const TArray<FFlyingCabDistrictDefinition> Districts = {
		{TEXT("District.YellowProjects"), TEXT("YELLOW PROJECTS"), TEXT("YP"), FVector(-900.0f, 0.0f, 1150.0f), TEXT(""), 0.0f, FLinearColor::Transparent, TEXT(""), TEXT("")},
		{TEXT("District.MidtownExchange"), TEXT("MIDTOWN EXCHANGE"), TEXT("ME"), FVector(850.0f, 0.0f, 2050.0f), TEXT(""), 0.0f, FLinearColor::Transparent, TEXT("MIDTOWN FUEL"), TEXT("")},
		{TEXT("District.SkylineTerraces"), TEXT("SKYLINE TERRACES"), TEXT("ST"), FVector(-750.0f, 0.0f, 3150.0f), TEXT(""), 0.0f, FLinearColor::Transparent, TEXT(""), TEXT("")},
		{TEXT("District.AshlineMarket"), TEXT("ASHLINE MARKET"), TEXT("AM"), FVector(-3800.0f, 0.0f, 2500.0f), TEXT(""), 0.0f, FLinearColor::Transparent, TEXT("ASHLINE CHARGE"), TEXT("")},
		{TEXT("District.NeonDocks"), TEXT("NEON DOCKS"), TEXT("ND"), FVector(3650.0f, 0.0f, 1150.0f), TEXT(""), 0.0f, FLinearColor::Transparent, TEXT(""), TEXT("")},
		{TEXT("District.ZenithSpire"), TEXT("ZENITH SPIRE"), TEXT("ZS"), FVector(3350.0f, 0.0f, 5200.0f), TEXT(""), 0.0f, FLinearColor::Transparent, TEXT(""), TEXT("")},
		{TEXT("District.GlasswardTransit"), TEXT("GLASSWARD TRANSIT"), TEXT("GT"), FVector(6500.0f, 0.0f, 1150.0f), TEXT("Glassward"), 15.0f, FLinearColor(0.02f, 0.62f, 0.82f), TEXT(""), TEXT("")},
		{TEXT("District.RainlineBazaar"), TEXT("RAINLINE BAZAAR"), TEXT("RB"), FVector(8650.0f, 0.0f, 2700.0f), TEXT("Rainline"), 16.0f, FLinearColor(0.92f, 0.32f, 0.04f), TEXT("RAINLINE ENERGY"), TEXT("")},
		{TEXT("District.CobaltHeights"), TEXT("COBALT HEIGHTS"), TEXT("CH"), FVector(11150.0f, 0.0f, 3950.0f), TEXT("Cobalt"), 16.0f, FLinearColor(0.72f, 0.06f, 0.70f), TEXT(""), TEXT("")},
		{TEXT("District.OrbitalGardens"), TEXT("ORBITAL GARDENS"), TEXT("OG"), FVector(13250.0f, 0.0f, 5450.0f), TEXT("Orbital"), 17.0f, FLinearColor(0.08f, 0.62f, 0.32f), TEXT(""), TEXT("ORBITAL BODYWORKS")}};

	const TArray<FFlyingCabServiceDefinition> StandaloneRepairStations = {
		{TEXT("NIGHTSHIFT REPAIR"), FVector(0.0f, 0.0f, 4200.0f)}};

	const TArray<FFlyingCabTrafficRouteDefinition> TrafficRoutes = {
		{FVector(-4700.0f, 0.0f, 1500.0f), FVector(4700.0f, 0.0f, 1500.0f), 480.0f, 0.08f, FLinearColor(0.05f, 0.85f, 1.0f)},
		{FVector(-4700.0f, 0.0f, 1500.0f), FVector(4700.0f, 0.0f, 1500.0f), 430.0f, 0.58f, FLinearColor(1.0f, 0.52f, 0.05f)},
		{FVector(4700.0f, 0.0f, 2850.0f), FVector(-4700.0f, 0.0f, 2850.0f), 400.0f, 0.28f, FLinearColor(0.95f, 0.12f, 0.65f)},
		{FVector(-4700.0f, 0.0f, 4550.0f), FVector(4700.0f, 0.0f, 4550.0f), 560.0f, 0.72f, FLinearColor(0.30f, 1.0f, 0.35f)},
		{FVector(5250.0f, 0.0f, 1650.0f), FVector(14700.0f, 0.0f, 1650.0f), 520.0f, 0.18f, FLinearColor(0.12f, 0.82f, 1.0f)},
		{FVector(14700.0f, 0.0f, 3150.0f), FVector(5250.0f, 0.0f, 3150.0f), 470.0f, 0.52f, FLinearColor(1.0f, 0.30f, 0.08f)},
		{FVector(5250.0f, 0.0f, 4450.0f), FVector(14700.0f, 0.0f, 4450.0f), 590.0f, 0.76f, FLinearColor(0.90f, 0.08f, 0.72f)},
		{FVector(14700.0f, 0.0f, 5550.0f), FVector(5250.0f, 0.0f, 5550.0f), 430.0f, 0.34f, FLinearColor(0.22f, 1.0f, 0.42f)}};

	const FVector2D MinimapWorldMin(-5000.0f, 0.0f);
	const FVector2D MinimapWorldMax(15000.0f, 6500.0f);
	const TCHAR* CityLayoutAssetPath =
		TEXT("/Game/Data/DA_FlyingCabCityLayout.DA_FlyingCabCityLayout");

	const UFlyingCabCityLayoutAsset* GetCityLayoutAsset()
	{
		static const UFlyingCabCityLayoutAsset* Asset = []()
		{
			UFlyingCabCityLayoutAsset* Loaded = LoadObject<UFlyingCabCityLayoutAsset>(
				nullptr,
				CityLayoutAssetPath);
			FString Error;
			if (Loaded && Loaded->IsConfigurationValid(Error))
			{
				Loaded->AddToRoot();
				return Loaded;
			}
			UE_LOG(
				LogFlyingCabCityData,
				Warning,
				TEXT("Using built-in city defaults because %s is missing or invalid%s%s."),
				CityLayoutAssetPath,
				Error.IsEmpty() ? TEXT("") : TEXT(": "),
				*Error);
			return static_cast<UFlyingCabCityLayoutAsset*>(nullptr);
		}();
		return Asset;
	}
}

TConstArrayView<FFlyingCabDistrictDefinition> FlyingCabCityData::GetDistricts()
{
	const UFlyingCabCityLayoutAsset* Asset = GetCityLayoutAsset();
	return Asset ? MakeArrayView(Asset->Districts) : MakeArrayView(Districts);
}

TArray<FFlyingCabServiceDefinition> FlyingCabCityData::GetFuelStations()
{
	TArray<FFlyingCabServiceDefinition> Stations;
	for (const FFlyingCabDistrictDefinition& District : GetDistricts())
	{
		if (!District.FuelStationName.IsEmpty())
		{
			Stations.Add(FFlyingCabServiceDefinition{
				District.FuelStationName,
				District.StopLocation});
		}
	}
	return Stations;
}

TArray<FFlyingCabServiceDefinition> FlyingCabCityData::GetRepairStations()
{
	TArray<FFlyingCabServiceDefinition> Stations;
	const UFlyingCabCityLayoutAsset* Asset = GetCityLayoutAsset();
	Stations.Append(Asset ? Asset->StandaloneRepairStations : StandaloneRepairStations);
	for (const FFlyingCabDistrictDefinition& District : GetDistricts())
	{
		if (!District.RepairStationName.IsEmpty())
		{
			Stations.Add(FFlyingCabServiceDefinition{
				District.RepairStationName,
				District.StopLocation});
		}
	}
	return Stations;
}

TConstArrayView<FFlyingCabTrafficRouteDefinition> FlyingCabCityData::GetTrafficRoutes()
{
	const UFlyingCabCityLayoutAsset* Asset = GetCityLayoutAsset();
	return Asset ? MakeArrayView(Asset->TrafficRoutes) : MakeArrayView(TrafficRoutes);
}

FVector2D FlyingCabCityData::GetMinimapWorldMin()
{
	const UFlyingCabCityLayoutAsset* Asset = GetCityLayoutAsset();
	return Asset ? Asset->MinimapWorldMin : MinimapWorldMin;
}

FVector2D FlyingCabCityData::GetMinimapWorldMax()
{
	const UFlyingCabCityLayoutAsset* Asset = GetCityLayoutAsset();
	return Asset ? Asset->MinimapWorldMax : MinimapWorldMax;
}

TConstArrayView<FFlyingCabDistrictDefinition> FlyingCabCityData::GetFallbackDistricts()
{
	return MakeArrayView(Districts);
}

TConstArrayView<FFlyingCabServiceDefinition>
FlyingCabCityData::GetFallbackRepairStations()
{
	return MakeArrayView(StandaloneRepairStations);
}

TConstArrayView<FFlyingCabTrafficRouteDefinition>
FlyingCabCityData::GetFallbackTrafficRoutes()
{
	return MakeArrayView(TrafficRoutes);
}
