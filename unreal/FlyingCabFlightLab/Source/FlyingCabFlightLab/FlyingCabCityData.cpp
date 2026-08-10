// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabCityData.h"

namespace
{
	const FFlyingCabDistrictDefinition Districts[] = {
		{TEXT("YELLOW PROJECTS"), TEXT("YP"), FVector(-900.0f, 0.0f, 1150.0f), nullptr, 0.0f, FLinearColor::Transparent, nullptr, nullptr},
		{TEXT("MIDTOWN EXCHANGE"), TEXT("ME"), FVector(850.0f, 0.0f, 2050.0f), nullptr, 0.0f, FLinearColor::Transparent, TEXT("MIDTOWN FUEL"), nullptr},
		{TEXT("SKYLINE TERRACES"), TEXT("ST"), FVector(-750.0f, 0.0f, 3150.0f), nullptr, 0.0f, FLinearColor::Transparent, nullptr, nullptr},
		{TEXT("ASHLINE MARKET"), TEXT("AM"), FVector(-3800.0f, 0.0f, 2500.0f), nullptr, 0.0f, FLinearColor::Transparent, TEXT("ASHLINE CHARGE"), nullptr},
		{TEXT("NEON DOCKS"), TEXT("ND"), FVector(3650.0f, 0.0f, 1150.0f), nullptr, 0.0f, FLinearColor::Transparent, nullptr, nullptr},
		{TEXT("ZENITH SPIRE"), TEXT("ZS"), FVector(3350.0f, 0.0f, 5200.0f), nullptr, 0.0f, FLinearColor::Transparent, nullptr, nullptr},
		{TEXT("GLASSWARD TRANSIT"), TEXT("GT"), FVector(6500.0f, 0.0f, 1150.0f), TEXT("Glassward"), 15.0f, FLinearColor(0.02f, 0.62f, 0.82f), nullptr, nullptr},
		{TEXT("RAINLINE BAZAAR"), TEXT("RB"), FVector(8650.0f, 0.0f, 2700.0f), TEXT("Rainline"), 16.0f, FLinearColor(0.92f, 0.32f, 0.04f), TEXT("RAINLINE ENERGY"), nullptr},
		{TEXT("COBALT HEIGHTS"), TEXT("CH"), FVector(11150.0f, 0.0f, 3950.0f), TEXT("Cobalt"), 16.0f, FLinearColor(0.72f, 0.06f, 0.70f), nullptr, nullptr},
		{TEXT("ORBITAL GARDENS"), TEXT("OG"), FVector(13250.0f, 0.0f, 5450.0f), TEXT("Orbital"), 17.0f, FLinearColor(0.08f, 0.62f, 0.32f), nullptr, TEXT("ORBITAL BODYWORKS")}};

	const FFlyingCabServiceDefinition StandaloneRepairStations[] = {
		{TEXT("NIGHTSHIFT REPAIR"), FVector(0.0f, 0.0f, 4200.0f)}};

	const FVector2D MinimapWorldMin(-5000.0f, 0.0f);
	const FVector2D MinimapWorldMax(15000.0f, 6500.0f);
}

TConstArrayView<FFlyingCabDistrictDefinition> FlyingCabCityData::GetDistricts()
{
	return MakeArrayView(Districts);
}

TArray<FFlyingCabServiceDefinition> FlyingCabCityData::GetFuelStations()
{
	TArray<FFlyingCabServiceDefinition> Stations;
	for (const FFlyingCabDistrictDefinition& District : Districts)
	{
		if (District.FuelStationName)
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
	Stations.Append(StandaloneRepairStations, UE_ARRAY_COUNT(StandaloneRepairStations));
	for (const FFlyingCabDistrictDefinition& District : Districts)
	{
		if (District.RepairStationName)
		{
			Stations.Add(FFlyingCabServiceDefinition{
				District.RepairStationName,
				District.StopLocation});
		}
	}
	return Stations;
}

FVector2D FlyingCabCityData::GetMinimapWorldMin()
{
	return MinimapWorldMin;
}

FVector2D FlyingCabCityData::GetMinimapWorldMax()
{
	return MinimapWorldMax;
}
