// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabCityData.h"

#include "FlyingCabCityLayoutAsset.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabCityData, Log, All);

namespace
{
	const TArray<FFlyingCabNeighborhoodDefinition> Neighborhoods = {
		{TEXT("Neighborhood.NW"), TEXT("ASHLINE / NORTH WEST"), FVector(-4500, 0, 9500), FLinearColor(0.92f, 0.36f, 0.04f)},
		{TEXT("Neighborhood.NE"), TEXT("ORBITAL / NORTH EAST"), FVector(14500, 0, 9500), FLinearColor(0.08f, 0.65f, 0.45f)},
		{TEXT("Neighborhood.SW"), TEXT("YELLOW / SOUTH WEST"), FVector(-4500, 0, 3500), FLinearColor(0.04f, 0.52f, 0.85f)},
		{TEXT("Neighborhood.SE"), TEXT("FOUNDRY / SOUTH EAST"), FVector(14500, 0, 3500), FLinearColor(0.65f, 0.10f, 0.72f)}};
	const TArray<FFlyingCabDistrictDefinition> Districts = {
		{TEXT("District.YellowProjects"), TEXT("YELLOW PROJECTS"), TEXT("YP"), FVector(-8500.0f, 0.0f, 2400.0f), TEXT("YellowProjects"), 22.0f, FLinearColor(0.04f, 0.52f, 0.85f), TEXT(""), TEXT(""), TEXT("Neighborhood.SW")},
		{TEXT("District.MidtownExchange"), TEXT("MIDTOWN EXCHANGE"), TEXT("ME"), FVector(-500.0f, 0.0f, 3500.0f), TEXT("MidtownExchange"), 22.0f, FLinearColor(0.04f, 0.52f, 0.85f), TEXT("MIDTOWN FUEL"), TEXT(""), TEXT("Neighborhood.SW")},
		{TEXT("District.SkylineTerraces"), TEXT("SKYLINE TERRACES"), TEXT("ST"), FVector(-5500.0f, 0.0f, 4400.0f), TEXT("SkylineTerraces"), 22.0f, FLinearColor(0.04f, 0.52f, 0.85f), TEXT(""), TEXT(""), TEXT("Neighborhood.SW")},
		{TEXT("District.AshlineMarket"), TEXT("ASHLINE MARKET"), TEXT("AM"), FVector(-8500.0f, 0.0f, 8400.0f), TEXT("AshlineMarket"), 22.0f, FLinearColor(0.92f, 0.36f, 0.04f), TEXT("ASHLINE CHARGE"), TEXT(""), TEXT("Neighborhood.NW")},
		{TEXT("District.NeonDocks"), TEXT("NEON DOCKS"), TEXT("ND"), FVector(10500.0f, 0.0f, 2400.0f), TEXT("NeonDocks"), 22.0f, FLinearColor(0.65f, 0.10f, 0.72f), TEXT(""), TEXT(""), TEXT("Neighborhood.SE")},
		{TEXT("District.ZenithSpire"), TEXT("ZENITH SPIRE"), TEXT("ZS"), FVector(-5500.0f, 0.0f, 10400.0f), TEXT("ZenithSpire"), 22.0f, FLinearColor(0.92f, 0.36f, 0.04f), TEXT(""), TEXT(""), TEXT("Neighborhood.NW")},
		{TEXT("District.GlasswardTransit"), TEXT("GLASSWARD TRANSIT"), TEXT("GT"), FVector(10500.0f, 0.0f, 8400.0f), TEXT("GlasswardTransit"), 22.0f, FLinearColor(0.08f, 0.65f, 0.45f), TEXT("GLASSWARD FUEL"), TEXT(""), TEXT("Neighborhood.NE")},
		{TEXT("District.RainlineBazaar"), TEXT("RAINLINE BAZAAR"), TEXT("RB"), FVector(18500.0f, 0.0f, 3500.0f), TEXT("RainlineBazaar"), 22.0f, FLinearColor(0.65f, 0.10f, 0.72f), TEXT("RAINLINE ENERGY"), TEXT(""), TEXT("Neighborhood.SE")},
		{TEXT("District.CobaltHeights"), TEXT("COBALT HEIGHTS"), TEXT("CH"), FVector(18500.0f, 0.0f, 9500.0f), TEXT("CobaltHeights"), 22.0f, FLinearColor(0.08f, 0.65f, 0.45f), TEXT(""), TEXT(""), TEXT("Neighborhood.NE")},
		{TEXT("District.OrbitalGardens"), TEXT("ORBITAL GARDENS"), TEXT("OG"), FVector(13500.0f, 0.0f, 10400.0f), TEXT("OrbitalGardens"), 22.0f, FLinearColor(0.08f, 0.65f, 0.45f), TEXT(""), TEXT(""), TEXT("Neighborhood.NE")},
		{TEXT("District.NightshiftSquare"), TEXT("NIGHTSHIFT SQUARE"), TEXT("NS"), FVector(-500.0f, 0.0f, 9500.0f), TEXT("NightshiftSquare"), 22.0f, FLinearColor(0.92f, 0.36f, 0.04f), TEXT(""), TEXT("NIGHTSHIFT REPAIR"), TEXT("Neighborhood.NW")},
		{TEXT("District.FoundryYards"), TEXT("FOUNDRY YARDS"), TEXT("FY"), FVector(13500.0f, 0.0f, 4400.0f), TEXT("FoundryYards"), 22.0f, FLinearColor(0.65f, 0.10f, 0.72f), TEXT(""), TEXT("FOUNDRY BODYWORKS"), TEXT("Neighborhood.SE")},
		// Append only: the original stop IDs and ordering remain stable for existing quests.
		{TEXT("District.YellowSteps"), TEXT("YELLOW STEPS"), TEXT("YS"), FVector(-10600,0,4100), TEXT("YellowSteps"),22,FLinearColor(.04f,.52f,.85f),TEXT(""),TEXT(""),TEXT("Neighborhood.SW"),650},
		{TEXT("District.LowerStacks"), TEXT("LOWER STACKS"), TEXT("LS"), FVector(-4500,0,2100), TEXT("LowerStacks"),22,FLinearColor(.04f,.52f,.85f),TEXT(""),TEXT(""),TEXT("Neighborhood.SW"),900},
		{TEXT("District.EastTenements"), TEXT("EAST TENEMENTS"), TEXT("ET"), FVector(1500,0,2100), TEXT("EastTenements"),22,FLinearColor(.04f,.52f,.85f),TEXT(""),TEXT(""),TEXT("Neighborhood.SW"),750},
		{TEXT("District.AshlineCourt"), TEXT("ASHLINE COURT"), TEXT("AC"), FVector(-10600,0,7900), TEXT("AshlineCourt"),22,FLinearColor(.92f,.36f,.04f),TEXT(""),TEXT(""),TEXT("Neighborhood.NW"),650},
		{TEXT("District.FurnaceHomes"), TEXT("FURNACE HOMES"), TEXT("FH"), FVector(-4500,0,8100), TEXT("FurnaceHomes"),22,FLinearColor(.92f,.36f,.04f),TEXT(""),TEXT(""),TEXT("Neighborhood.NW"),900},
		{TEXT("District.LanternHeights"), TEXT("LANTERN HEIGHTS"), TEXT("LH"), FVector(1750,0,10100), TEXT("LanternHeights"),22,FLinearColor(.92f,.36f,.04f),TEXT(""),TEXT(""),TEXT("Neighborhood.NW"),750},
		{TEXT("District.SilicaCourt"), TEXT("SILICA COURT"), TEXT("SC"), FVector(8400,0,10100), TEXT("SilicaCourt"),22,FLinearColor(.08f,.65f,.45f),TEXT(""),TEXT(""),TEXT("Neighborhood.NE"),650},
		{TEXT("District.AuroraStacks"), TEXT("AURORA STACKS"), TEXT("AS"), FVector(14500,0,8100), TEXT("AuroraStacks"),22,FLinearColor(.08f,.65f,.45f),TEXT(""),TEXT(""),TEXT("Neighborhood.NE"),900},
		{TEXT("District.RelayHomes"), TEXT("RELAY HOMES"), TEXT("RH"), FVector(20500,0,8100), TEXT("RelayHomes"),22,FLinearColor(.08f,.65f,.45f),TEXT(""),TEXT(""),TEXT("Neighborhood.NE"),750},
		{TEXT("District.CopperSteps"), TEXT("COPPER STEPS"), TEXT("CS"), FVector(8400,0,4100), TEXT("CopperSteps"),22,FLinearColor(.65f,.10f,.72f),TEXT(""),TEXT(""),TEXT("Neighborhood.SE"),650},
		{TEXT("District.BoilerCourt"), TEXT("BOILER COURT"), TEXT("BC"), FVector(14500,0,2100), TEXT("BoilerCourt"),22,FLinearColor(.65f,.10f,.72f),TEXT(""),TEXT(""),TEXT("Neighborhood.SE"),900},
		{TEXT("District.FoundryEast"), TEXT("FOUNDRY EAST"), TEXT("FE"), FVector(20500,0,2100), TEXT("FoundryEast"),22,FLinearColor(.65f,.10f,.72f),TEXT(""),TEXT(""),TEXT("Neighborhood.SE"),750}};

	const TArray<FFlyingCabServiceDefinition> StandaloneRepairStations;

	const TArray<FFlyingCabTrafficRouteDefinition> TrafficRoutes = {
		{FVector(-4700.0f, 0.0f, 1500.0f), FVector(4700.0f, 0.0f, 1500.0f), 480.0f, 0.08f, FLinearColor(0.05f, 0.85f, 1.0f)},
		{FVector(-4700.0f, 0.0f, 1500.0f), FVector(4700.0f, 0.0f, 1500.0f), 430.0f, 0.58f, FLinearColor(1.0f, 0.52f, 0.05f)},
		{FVector(4700.0f, 0.0f, 2850.0f), FVector(-4700.0f, 0.0f, 2850.0f), 400.0f, 0.28f, FLinearColor(0.95f, 0.12f, 0.65f)},
		{FVector(-4700.0f, 0.0f, 4550.0f), FVector(4700.0f, 0.0f, 4550.0f), 560.0f, 0.72f, FLinearColor(0.30f, 1.0f, 0.35f)},
		{FVector(5250.0f, 0.0f, 1650.0f), FVector(14700.0f, 0.0f, 1650.0f), 520.0f, 0.18f, FLinearColor(0.12f, 0.82f, 1.0f)},
		{FVector(14700.0f, 0.0f, 3150.0f), FVector(5250.0f, 0.0f, 3150.0f), 470.0f, 0.52f, FLinearColor(1.0f, 0.30f, 0.08f)},
		{FVector(5250.0f, 0.0f, 4450.0f), FVector(14700.0f, 0.0f, 4450.0f), 590.0f, 0.76f, FLinearColor(0.90f, 0.08f, 0.72f)},
		{FVector(14700.0f, 0.0f, 5550.0f), FVector(5250.0f, 0.0f, 5550.0f), 430.0f, 0.34f, FLinearColor(0.22f, 1.0f, 0.42f)}};

	const FVector2D MinimapWorldMin(-15000.0f, 0.0f);
	const FVector2D MinimapWorldMax(25000.0f, 13000.0f);
	constexpr float PassengerZoneHorizontalOffset = 820.0f;
	constexpr float CurbsidePlatformScaleX = 22.0f;
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

TConstArrayView<FFlyingCabNeighborhoodDefinition> FlyingCabCityData::GetNeighborhoods()
{
	const UFlyingCabCityLayoutAsset* Asset = GetCityLayoutAsset();
	return Asset ? MakeArrayView(Asset->Neighborhoods) : MakeArrayView(Neighborhoods);
}

TConstArrayView<FFlyingCabNeighborhoodDefinition> FlyingCabCityData::GetFallbackNeighborhoods()
{
	return MakeArrayView(Neighborhoods);
}

FVector FlyingCabCityData::GetPassengerPickupLocation(
	const FVector& DistrictStopLocation)
{
	return DistrictStopLocation - FVector(PassengerZoneHorizontalOffset, 0.0f, 0.0f);
}

FVector FlyingCabCityData::GetPassengerDropoffLocation(
	const FVector& DistrictStopLocation)
{
	return DistrictStopLocation + FVector(PassengerZoneHorizontalOffset, 0.0f, 0.0f);
}

float FlyingCabCityData::GetCurbsidePlatformScaleX()
{
	return CurbsidePlatformScaleX;
}

FVector FlyingCabCityData::GetDistrictServiceLocation(const FFlyingCabDistrictDefinition& District)
{
	return District.StopLocation + FVector(0,0,District.ResidentialTowerHeight);
}

FVector FlyingCabCityData::GetResidentialEntranceLocation(const FFlyingCabDistrictDefinition& District, bool bRightSide)
{
	// Future on-foot portal anchors just outside the solid facade, standing on the platform.
	return District.StopLocation + FVector(bRightSide ? 520 : -520,0,-84); // Platform top -150 + character half-height 66.
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
				GetDistrictServiceLocation(District)});
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
				GetDistrictServiceLocation(District)});
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

TStaticArray<FFlyingCabHighwayStrip, 6> FlyingCabCityData::GetHighwayStrips()
{
	const FVector2D Min = GetMinimapWorldMin();
	const FVector2D Max = GetMinimapWorldMax();
	const FVector2D Mid = (Min + Max) * .5;
	TStaticArray<FFlyingCabHighwayStrip, 6> Strips;
	int32 Index = 0;
	auto Add = [&](const TCHAR* Name, FVector2D A, FVector2D B, double Width)
	{
		Strips[Index++] = {Name, (A+B)*.5,
			FVector2D(FMath::Max(FMath::Abs(A.X-B.X), Width),
				FMath::Max(FMath::Abs(A.Y-B.Y), Width))*.5};
	};
	Add(TEXT("HighwayEW"), FVector2D(Min.X+1500,Mid.Y), FVector2D(Max.X-1500,Mid.Y),1100);
	Add(TEXT("HighwayNS"), FVector2D(Mid.X,Min.Y+1500), FVector2D(Mid.X,Max.Y-1500),1100);
	Add(TEXT("RingWest"), FVector2D(Min.X+1250,Min.Y+1250), FVector2D(Min.X+1250,Max.Y-1250),900);
	Add(TEXT("RingEast"), FVector2D(Max.X-1250,Min.Y+1250), FVector2D(Max.X-1250,Max.Y-1250),900);
	Add(TEXT("RingSouth"), FVector2D(Min.X+1250,Min.Y+1250), FVector2D(Max.X-1250,Min.Y+1250),900);
	Add(TEXT("RingNorth"), FVector2D(Min.X+1250,Max.Y-1250), FVector2D(Max.X-1250,Max.Y-1250),900);
	return Strips;
}

bool FlyingCabCityData::IsOnHighway(const FVector& WorldLocation)
{
	// The corridors serve the flight plane, not background scenery or separate interiors.
	if (FMath::Abs(WorldLocation.Y) > 200.0) return false;
	for (const FFlyingCabHighwayStrip& Strip : GetHighwayStrips())
	{
		if (FMath::Abs(WorldLocation.X-Strip.Center.X) <= Strip.HalfSize.X
			&& FMath::Abs(WorldLocation.Z-Strip.Center.Y) <= Strip.HalfSize.Y) return true;
	}
	return false;
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
