// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlyingCabCityData.h"
#include "GameFramework/Actor.h"
#include "FlyingCabWorldBootstrap.generated.h"

class AFlyingCabAccessTerminal;
class AFlyingCabCityExpansion;
class AFlyingCabFuelStation;
class AFlyingCabNightshiftOffice;
class AFlyingCabOnFootPortal;
class AFlyingCabPawn;
class AFlyingCabRepairStation;
class AFlyingCabTrafficVehicle;
class UFlyingCabEconomyAsset;

/** Builds and owns the runtime-only infrastructure of the Flight Lab map. */
UCLASS()
class FLYINGCABFLIGHTLAB_API AFlyingCabWorldBootstrap : public AActor
{
	GENERATED_BODY()

public:
	AFlyingCabWorldBootstrap();

	bool Bootstrap(UClass* ServiceVehicleClass, UFlyingCabEconomyAsset* InEconomyConfig);
	void RefreshServiceAccess();
	void ResetCompetitiveServiceAccess();

	AFlyingCabPawn* GetServiceVehicle() const { return ServiceVehicle; }
	const TArray<TObjectPtr<AFlyingCabTrafficVehicle>>& GetTrafficVehicles() const
	{
		return TrafficVehicles;
	}
	int32 GetFuelStationCount() const { return FuelStations.Num(); }
	int32 GetRepairStationCount() const { return RepairStations.Num(); }
	static TConstArrayView<FFlyingCabTrafficRouteDefinition> GetTrafficRoutes();
	static int32 GetExpectedTrafficVehicleCount();

private:
	bool SpawnCityExpansion();
	bool SpawnServiceStations();
	bool SpawnOnFootSlice();
	bool SpawnServiceVehicle(UClass* ServiceVehicleClass);
	bool SpawnTraffic();
	FActorSpawnParameters MakeSpawnParameters(
		ESpawnActorCollisionHandlingMethod CollisionHandling);

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|On Foot")
	FVector NightshiftEntranceLocation = FVector(470.0f, 0.0f, 4120.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|On Foot")
	FVector NightshiftExteriorReturnLocation = FVector(310.0f, 0.0f, 4068.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|On Foot")
	FVector NightshiftOfficeLocation = FVector(23000.0f, 0.0f, 650.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|On Foot")
	FVector ServiceVehicleLocation = FVector(-600.0f, 0.0f, 4040.0f);

	UPROPERTY(Transient)
	TObjectPtr<AFlyingCabCityExpansion> CityExpansion;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AFlyingCabFuelStation>> FuelStations;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AFlyingCabRepairStation>> RepairStations;

	UPROPERTY(Transient)
	TObjectPtr<AFlyingCabNightshiftOffice> NightshiftOffice;

	UPROPERTY(Transient)
	TObjectPtr<AFlyingCabOnFootPortal> NightshiftEntrance;

	UPROPERTY(Transient)
	TObjectPtr<AFlyingCabOnFootPortal> NightshiftExit;

	UPROPERTY(Transient)
	TObjectPtr<AFlyingCabAccessTerminal> ServiceAccessTerminal;

	UPROPERTY(Transient)
	TObjectPtr<AFlyingCabPawn> ServiceVehicle;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AFlyingCabTrafficVehicle>> TrafficVehicles;

	UPROPERTY(Transient)
	TObjectPtr<UFlyingCabEconomyAsset> EconomyConfig;

	bool bBootstrapped = false;
	bool bBootstrapSucceeded = false;
};
