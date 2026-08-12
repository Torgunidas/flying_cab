// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/ArrayView.h"
#include "CoreMinimal.h"
#include "FlyingCabCityData.generated.h"

/** Canonical district definition shared by gameplay, city geometry and the minimap. */
USTRUCT(BlueprintType)
struct FFlyingCabDistrictDefinition
{
	GENERATED_BODY()

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

	FVector2D GetMapPosition() const
	{
		return FVector2D(StopLocation.X, StopLocation.Z);
	}

	bool BuildsRuntimeGeometry() const
	{
		return !RuntimeGeometryName.IsEmpty() && RuntimePlatformHalfWidth > 0.0f;
	}
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

namespace FlyingCabCityData
{
	TConstArrayView<FFlyingCabDistrictDefinition> GetDistricts();
	TArray<FFlyingCabServiceDefinition> GetFuelStations();
	TArray<FFlyingCabServiceDefinition> GetRepairStations();
	TConstArrayView<FFlyingCabTrafficRouteDefinition> GetTrafficRoutes();
	FVector2D GetMinimapWorldMin();
	FVector2D GetMinimapWorldMax();
	TConstArrayView<FFlyingCabDistrictDefinition> GetFallbackDistricts();
	TConstArrayView<FFlyingCabServiceDefinition> GetFallbackRepairStations();
	TConstArrayView<FFlyingCabTrafficRouteDefinition> GetFallbackTrafficRoutes();
}
