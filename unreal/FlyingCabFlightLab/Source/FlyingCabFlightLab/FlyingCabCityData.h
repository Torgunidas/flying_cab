// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/ArrayView.h"
#include "Containers/StaticArray.h"
#include "CoreMinimal.h"
#include "FlyingCabCityData.generated.h"

/** Canonical district definition shared by gameplay, city geometry and the minimap. */
USTRUCT(BlueprintType)
struct FFlyingCabDistrictDefinition
{
	GENERATED_BODY()

	/** Stable gameplay key used by quests, saves and future dialogue conditions. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|City")
	FName DistrictId = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|City")
	FString DisplayName;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|City")
	FString MinimapCode;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|City")
	FVector StopLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|City")
	FString RuntimeGeometryName;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|City", meta = (ClampMin = "0.0"))
	float RuntimePlatformHalfWidth = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|City")
	FLinearColor AccentColor = FLinearColor::Transparent;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|City")
	FString FuelStationName;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|City")
	FString RepairStationName;

	/** Several taxi stops belong to one neighborhood; stable IDs keep quest targets intact. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|City")
	FName NeighborhoodId = NAME_None;

	/** Solid residential block above the platform; services land on its flat roof. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|City", meta = (ClampMin = "400.0", ClampMax = "1200.0"))
	float ResidentialTowerHeight = 800.0f;

	FVector2D GetMapPosition() const
	{
		return FVector2D(StopLocation.X, StopLocation.Z);
	}

	bool BuildsRuntimeGeometry() const
	{
		return !RuntimeGeometryName.IsEmpty() && RuntimePlatformHalfWidth > 0.0f;
	}
};

/** An estate between the express corridors, with its own ambient transit terrace. */
USTRUCT(BlueprintType)
struct FFlyingCabNeighborhoodDefinition
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, Category = "Flying Cab|City")
	FName NeighborhoodId = NAME_None;
	UPROPERTY(EditAnywhere, Category = "Flying Cab|City")
	FString DisplayName;
	UPROPERTY(EditAnywhere, Category = "Flying Cab|City")
	FVector Center = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, Category = "Flying Cab|City")
	FLinearColor Color = FLinearColor::White;
};

/** Canonical service location used by gameplay spawning and the minimap. */
USTRUCT(BlueprintType)
struct FFlyingCabServiceDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Flying Cab|City")
	FString DisplayName;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|City")
	FVector Location = FVector::ZeroVector;

	FVector2D GetMapPosition() const
	{
		return FVector2D(Location.X, Location.Z);
	}
};

/** Runtime traffic route stored alongside the rest of the city topology. */
USTRUCT(BlueprintType)
struct FFlyingCabTrafficRouteDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Traffic")
	FVector Start = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Traffic")
	FVector End = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Traffic", meta = (ClampMin = "0.0"))
	float Speed = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Traffic", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float InitialAlpha = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Traffic")
	FLinearColor Color = FLinearColor::White;
};

/** The visible highway strip and its gameplay area share this exact X/Z rectangle. */
struct FFlyingCabHighwayStrip
{
	const TCHAR* Name = TEXT("");
	FVector2D Center = FVector2D::ZeroVector;
	FVector2D HalfSize = FVector2D::ZeroVector;
};

namespace FlyingCabCityData
{
	TStaticArray<FFlyingCabHighwayStrip, 6> GetHighwayStrips();
	bool IsOnHighway(const FVector& WorldLocation);
	TConstArrayView<FFlyingCabDistrictDefinition> GetDistricts();
	TConstArrayView<FFlyingCabNeighborhoodDefinition> GetNeighborhoods();
	TConstArrayView<FFlyingCabNeighborhoodDefinition> GetFallbackNeighborhoods();
	FVector GetPassengerPickupLocation(const FVector& DistrictStopLocation);
	FVector GetPassengerDropoffLocation(const FVector& DistrictStopLocation);
	float GetCurbsidePlatformScaleX();
	FVector GetDistrictServiceLocation(const FFlyingCabDistrictDefinition& District);
	FVector GetResidentialEntranceLocation(const FFlyingCabDistrictDefinition& District, bool bRightSide);
	TArray<FFlyingCabServiceDefinition> GetFuelStations();
	TArray<FFlyingCabServiceDefinition> GetRepairStations();
	TConstArrayView<FFlyingCabTrafficRouteDefinition> GetTrafficRoutes();
	FVector2D GetMinimapWorldMin();
	FVector2D GetMinimapWorldMax();
	TConstArrayView<FFlyingCabDistrictDefinition> GetFallbackDistricts();
	TConstArrayView<FFlyingCabServiceDefinition> GetFallbackRepairStations();
	TConstArrayView<FFlyingCabTrafficRouteDefinition> GetFallbackTrafficRoutes();
}
