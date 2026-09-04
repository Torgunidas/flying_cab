// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "FlyingCabCityData.h"
#include "FlyingCabCityLayoutAsset.generated.h"

/** Editable city topology consumed by dispatch, runtime geometry and the minimap. */
UCLASS(BlueprintType)
class FLYINGCABFLIGHTLAB_API UFlyingCabCityLayoutAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UFlyingCabCityLayoutAsset();
	virtual void PostLoad() override;

	bool IsConfigurationValid(FString& OutError) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Cab|City")
	TArray<FFlyingCabDistrictDefinition> Districts;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Cab|City")
	TArray<FFlyingCabServiceDefinition> StandaloneRepairStations;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Cab|Traffic")
	TArray<FFlyingCabTrafficRouteDefinition> TrafficRoutes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Cab|Minimap")
	FVector2D MinimapWorldMin = FVector2D(-5000.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Cab|Minimap")
	FVector2D MinimapWorldMax = FVector2D(15000.0f, 6500.0f);
};
