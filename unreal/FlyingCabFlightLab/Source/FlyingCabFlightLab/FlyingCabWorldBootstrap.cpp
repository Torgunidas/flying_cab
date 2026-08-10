// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabWorldBootstrap.h"

#include "FlyingCabAccessTerminal.h"
#include "FlyingCabCityData.h"
#include "FlyingCabCityExpansion.h"
#include "FlyingCabFuelStation.h"
#include "FlyingCabNightshiftOffice.h"
#include "FlyingCabOnFootPortal.h"
#include "FlyingCabPawn.h"
#include "FlyingCabRepairStation.h"
#include "FlyingCabTrafficVehicle.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabWorldBootstrap, Log, All);

namespace
{
	const FFlyingCabTrafficRouteDefinition TrafficRoutes[] = {
		{FVector(-4700.0f, 0.0f, 1500.0f), FVector(4700.0f, 0.0f, 1500.0f), 480.0f, 0.08f, FLinearColor(0.05f, 0.85f, 1.0f)},
		{FVector(-4700.0f, 0.0f, 1500.0f), FVector(4700.0f, 0.0f, 1500.0f), 430.0f, 0.58f, FLinearColor(1.0f, 0.52f, 0.05f)},
		{FVector(4700.0f, 0.0f, 2850.0f), FVector(-4700.0f, 0.0f, 2850.0f), 400.0f, 0.28f, FLinearColor(0.95f, 0.12f, 0.65f)},
		{FVector(-4700.0f, 0.0f, 4550.0f), FVector(4700.0f, 0.0f, 4550.0f), 560.0f, 0.72f, FLinearColor(0.30f, 1.0f, 0.35f)},
		{FVector(5250.0f, 0.0f, 1650.0f), FVector(14700.0f, 0.0f, 1650.0f), 520.0f, 0.18f, FLinearColor(0.12f, 0.82f, 1.0f)},
		{FVector(14700.0f, 0.0f, 3150.0f), FVector(5250.0f, 0.0f, 3150.0f), 470.0f, 0.52f, FLinearColor(1.0f, 0.30f, 0.08f)},
		{FVector(5250.0f, 0.0f, 4450.0f), FVector(14700.0f, 0.0f, 4450.0f), 590.0f, 0.76f, FLinearColor(0.90f, 0.08f, 0.72f)},
		{FVector(14700.0f, 0.0f, 5550.0f), FVector(5250.0f, 0.0f, 5550.0f), 430.0f, 0.34f, FLinearColor(0.22f, 1.0f, 0.42f)}};
}

AFlyingCabWorldBootstrap::AFlyingCabWorldBootstrap()
{
	PrimaryActorTick.bCanEverTick = false;
}

bool AFlyingCabWorldBootstrap::Bootstrap(
	UClass* ServiceVehicleClass)
{
	if (bBootstrapped)
	{
		return bBootstrapSucceeded;
	}
	bBootstrapped = true;

	const bool bCityReady = SpawnCityExpansion();
	const bool bStationsReady = SpawnServiceStations();
	const bool bOnFootReady = SpawnOnFootSlice();
	const bool bServiceVehicleReady = SpawnServiceVehicle(ServiceVehicleClass);
	const bool bTrafficReady = SpawnTraffic();
	bBootstrapSucceeded = bCityReady && bStationsReady && bOnFootReady
		&& bServiceVehicleReady && bTrafficReady;

	UE_LOG(
		LogFlyingCabWorldBootstrap,
		Display,
		TEXT("World bootstrap %s: %d fuel stations, %d repair shops and %d/%d traffic vehicles."),
		bBootstrapSucceeded ? TEXT("completed") : TEXT("completed with missing actors"),
		FuelStations.Num(),
		RepairStations.Num(),
		TrafficVehicles.Num(),
		GetExpectedTrafficVehicleCount());
	return bBootstrapSucceeded;
}

int32 AFlyingCabWorldBootstrap::GetExpectedTrafficVehicleCount()
{
	return GetTrafficRoutes().Num();
}

TConstArrayView<FFlyingCabTrafficRouteDefinition>
AFlyingCabWorldBootstrap::GetTrafficRoutes()
{
	return MakeArrayView(TrafficRoutes);
}

void AFlyingCabWorldBootstrap::RefreshServiceAccess()
{
	if (ServiceAccessTerminal)
	{
		ServiceAccessTerminal->Configure(TEXT("Vehicle.Service"), TEXT("SERVICE VEHICLES"));
	}
}

bool AFlyingCabWorldBootstrap::SpawnCityExpansion()
{
	CityExpansion = GetWorld()->SpawnActor<AFlyingCabCityExpansion>(
		AFlyingCabCityExpansion::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		MakeSpawnParameters(ESpawnActorCollisionHandlingMethod::AlwaysSpawn));
	if (!CityExpansion)
	{
		UE_LOG(LogFlyingCabWorldBootstrap, Error, TEXT("Could not create east city extension."));
	}
	return CityExpansion != nullptr;
}

bool AFlyingCabWorldBootstrap::SpawnServiceStations()
{
	const TArray<FFlyingCabServiceDefinition> FuelDefinitions =
		FlyingCabCityData::GetFuelStations();
	FuelStations.Reset();
	for (const FFlyingCabServiceDefinition& Definition : FuelDefinitions)
	{
		AFlyingCabFuelStation* Station = GetWorld()->SpawnActor<AFlyingCabFuelStation>(
			AFlyingCabFuelStation::StaticClass(),
			Definition.Location,
			FRotator::ZeroRotator,
			MakeSpawnParameters(ESpawnActorCollisionHandlingMethod::AlwaysSpawn));
		if (Station)
		{
			Station->Configure(Definition.DisplayName);
			FuelStations.Add(Station);
		}
	}

	const TArray<FFlyingCabServiceDefinition> RepairDefinitions =
		FlyingCabCityData::GetRepairStations();
	RepairStations.Reset();
	for (const FFlyingCabServiceDefinition& Definition : RepairDefinitions)
	{
		AFlyingCabRepairStation* Station = GetWorld()->SpawnActor<AFlyingCabRepairStation>(
			AFlyingCabRepairStation::StaticClass(),
			Definition.Location,
			FRotator::ZeroRotator,
			MakeSpawnParameters(ESpawnActorCollisionHandlingMethod::AlwaysSpawn));
		if (Station)
		{
			Station->Configure(Definition.DisplayName);
			RepairStations.Add(Station);
		}
	}

	const bool bSucceeded = FuelStations.Num() == FuelDefinitions.Num()
		&& RepairStations.Num() == RepairDefinitions.Num();
	if (!bSucceeded)
	{
		UE_LOG(LogFlyingCabWorldBootstrap, Error, TEXT("Could not spawn all service stations."));
	}
	return bSucceeded;
}

bool AFlyingCabWorldBootstrap::SpawnOnFootSlice()
{
	const FActorSpawnParameters SpawnParameters =
		MakeSpawnParameters(ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	NightshiftOffice = GetWorld()->SpawnActor<AFlyingCabNightshiftOffice>(
		AFlyingCabNightshiftOffice::StaticClass(),
		NightshiftOfficeLocation,
		FRotator::ZeroRotator,
		SpawnParameters);
	NightshiftEntrance = GetWorld()->SpawnActor<AFlyingCabOnFootPortal>(
		AFlyingCabOnFootPortal::StaticClass(),
		NightshiftEntranceLocation,
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!NightshiftOffice || !NightshiftEntrance)
	{
		UE_LOG(LogFlyingCabWorldBootstrap, Error, TEXT("Could not create Nightshift Office entrance."));
		return false;
	}

	NightshiftExit = GetWorld()->SpawnActor<AFlyingCabOnFootPortal>(
		AFlyingCabOnFootPortal::StaticClass(),
		NightshiftOffice->GetExitPortalLocation(),
		FRotator::ZeroRotator,
		SpawnParameters);
	ServiceAccessTerminal = GetWorld()->SpawnActor<AFlyingCabAccessTerminal>(
		AFlyingCabAccessTerminal::StaticClass(),
		NightshiftOffice->GetTerminalLocation(),
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!NightshiftExit || !ServiceAccessTerminal)
	{
		UE_LOG(LogFlyingCabWorldBootstrap, Error, TEXT("Could not complete Nightshift Office interactables."));
		return false;
	}

	NightshiftEntrance->Configure(
		TEXT("NIGHTSHIFT OFFICE"),
		FText::FromString(TEXT("Q // ENTER NIGHTSHIFT OFFICE")),
		NightshiftOffice->GetEntryLocation(),
		FLinearColor(0.80f, 0.08f, 1.0f));
	NightshiftExit->Configure(
		TEXT("CITY PLATFORM"),
		FText::FromString(TEXT("Q // RETURN TO CITY")),
		NightshiftExteriorReturnLocation,
		FLinearColor(0.05f, 0.78f, 1.0f));
	RefreshServiceAccess();

	UE_LOG(
		LogFlyingCabWorldBootstrap,
		Display,
		TEXT("Nightshift Office initialized at %s with foot-only entrance at %s."),
		*NightshiftOfficeLocation.ToCompactString(),
		*NightshiftEntranceLocation.ToCompactString());
	return true;
}

bool AFlyingCabWorldBootstrap::SpawnServiceVehicle(
	UClass* ServiceVehicleClass)
{
	if (!ServiceVehicleClass || !ServiceVehicleClass->IsChildOf(AFlyingCabPawn::StaticClass()))
	{
		UE_LOG(LogFlyingCabWorldBootstrap, Error, TEXT("Invalid class supplied for the service vehicle."));
		return false;
	}

	FVector GroundedLocation = ServiceVehicleLocation;
	FHitResult GroundHit;
	const FVector TraceStart = ServiceVehicleLocation + FVector(0.0f, 0.0f, 600.0f);
	const FVector TraceEnd = ServiceVehicleLocation - FVector(0.0f, 0.0f, 900.0f);
	const FCollisionObjectQueryParams WorldStaticObjects(ECC_WorldStatic);
	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ServiceVehicleGround), false);
	if (GetWorld()->LineTraceSingleByObjectType(
		GroundHit,
		TraceStart,
		TraceEnd,
		WorldStaticObjects,
		QueryParams)
		&& GroundHit.ImpactNormal.Z >= 0.65f)
	{
		GroundedLocation.Z = GroundHit.ImpactPoint.Z + 39.0f;
	}

	ServiceVehicle = GetWorld()->SpawnActor<AFlyingCabPawn>(
		ServiceVehicleClass,
		GroundedLocation,
		FRotator::ZeroRotator,
		MakeSpawnParameters(ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn));
	if (!ServiceVehicle)
	{
		UE_LOG(LogFlyingCabWorldBootstrap, Error, TEXT("Could not spawn Nightshift service vehicle."));
		return false;
	}

	ServiceVehicle->ConfigureVehicleIdentity(
		TEXT("Vehicle.Service.01"),
		TEXT("NIGHTSHIFT SERVICE CAB"),
		TEXT("Vehicle.Service"),
		FLinearColor(0.06f, 0.78f, 0.92f));
	UE_LOG(
		LogFlyingCabWorldBootstrap,
		Display,
		TEXT("Nightshift service vehicle spawned at %s; access requires Vehicle.Service."),
		*GroundedLocation.ToCompactString());
	return true;
}

bool AFlyingCabWorldBootstrap::SpawnTraffic()
{
	TrafficVehicles.Reset();
	const FActorSpawnParameters SpawnParameters =
		MakeSpawnParameters(ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	for (const FFlyingCabTrafficRouteDefinition& Spec : GetTrafficRoutes())
	{
		AFlyingCabTrafficVehicle* Vehicle = GetWorld()->SpawnActor<AFlyingCabTrafficVehicle>(
			AFlyingCabTrafficVehicle::StaticClass(),
			Spec.Start,
			FRotator::ZeroRotator,
			SpawnParameters);
		if (Vehicle)
		{
			Vehicle->Configure(Spec.Start, Spec.End, Spec.Speed, Spec.InitialAlpha, Spec.Color);
			TrafficVehicles.Add(Vehicle);
		}
	}

	if (TrafficVehicles.Num() != GetExpectedTrafficVehicleCount())
	{
		UE_LOG(LogFlyingCabWorldBootstrap, Warning, TEXT("One or more traffic vehicles failed to spawn."));
		return false;
	}
	return true;
}

FActorSpawnParameters AFlyingCabWorldBootstrap::MakeSpawnParameters(
	ESpawnActorCollisionHandlingMethod CollisionHandling)
{
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = CollisionHandling;
	return SpawnParameters;
}
