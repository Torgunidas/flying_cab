// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabGameMode.h"

#include "Engine/Engine.h"
#include "FlyingCabDeliveryZone.h"
#include "FlyingCabFuelStation.h"
#include "FlyingCabPawn.h"
#include "FlyingCabRepairStation.h"
#include "FlyingCabTrafficVehicle.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabDelivery, Log, All);

namespace
{
	constexpr uint64 DeliveryMessageKey = 0xFCAB0002ULL;
	constexpr uint64 FuelMessageKey = 0xFCAB0003ULL;
	constexpr uint64 RepairMessageKey = 0xFCAB0004ULL;
}

AFlyingCabGameMode::AFlyingCabGameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FClassFinder<AFlyingCabPawn> TunablePawnClass(
		TEXT("/Game/Blueprints/BP_FlyingCabPawn"));

	DefaultPawnClass = AFlyingCabPawn::StaticClass();
	if (TunablePawnClass.Succeeded())
	{
		DefaultPawnClass = TunablePawnClass.Class;
	}

	DeliveryStops = {
		FVector(-900.0f, 0.0f, 1150.0f),
		FVector(850.0f, 0.0f, 2050.0f),
		FVector(-750.0f, 0.0f, 3150.0f),
		FVector(-3800.0f, 0.0f, 2500.0f),
		FVector(3650.0f, 0.0f, 1150.0f),
		FVector(3350.0f, 0.0f, 5200.0f)};
	DeliveryStopNames = {
		TEXT("YELLOW PROJECTS"),
		TEXT("MIDTOWN EXCHANGE"),
		TEXT("SKYLINE TERRACES"),
		TEXT("ASHLINE MARKET"),
		TEXT("NEON DOCKS"),
		TEXT("ZENITH SPIRE")};
	FuelStationLocations = {
		FVector(850.0f, 0.0f, 2050.0f),
		FVector(-3800.0f, 0.0f, 2500.0f)};
	FuelStationNames = {
		TEXT("MIDTOWN FUEL"),
		TEXT("ASHLINE CHARGE")};
}

void AFlyingCabGameMode::BeginPlay()
{
	Super::BeginPlay();
	Credits = FMath::Max(0, StartingCredits);
	DispatchRandom.Initialize(DispatchRandomSeed);
	InitializeDeliveryLoop();
	InitializeTraffic();
	EnsurePawnBinding();
}

void AFlyingCabGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	EnsurePawnBinding();
	UpdateActiveFare();
	UpdateObjectiveStatus();
	UpdateTrafficAwareness(DeltaSeconds);
}

void AFlyingCabGameMode::InitializeDeliveryLoop()
{
	if (DeliveryStops.Num() < 2)
	{
		UE_LOG(LogFlyingCabDelivery, Error, TEXT("At least two delivery stops are required."));
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	PickupZone = GetWorld()->SpawnActor<AFlyingCabDeliveryZone>(
		AFlyingCabDeliveryZone::StaticClass(),
		DeliveryStops[0],
		FRotator::ZeroRotator,
		SpawnParameters);
	DropoffZone = GetWorld()->SpawnActor<AFlyingCabDeliveryZone>(
		AFlyingCabDeliveryZone::StaticClass(),
		DeliveryStops[1],
		FRotator::ZeroRotator,
		SpawnParameters);
	FuelStations.Reset();
	for (int32 StationIndex = 0; StationIndex < FuelStationLocations.Num(); ++StationIndex)
	{
		AFlyingCabFuelStation* Station = GetWorld()->SpawnActor<AFlyingCabFuelStation>(
			AFlyingCabFuelStation::StaticClass(),
			FuelStationLocations[StationIndex],
			FRotator::ZeroRotator,
			SpawnParameters);
		if (Station)
		{
			const FString StationName = FuelStationNames.IsValidIndex(StationIndex)
				? FuelStationNames[StationIndex]
				: FString::Printf(TEXT("FUEL STATION %d"), StationIndex + 1);
			Station->Configure(StationName);
			FuelStations.Add(Station);
		}
	}
	RepairStation = GetWorld()->SpawnActor<AFlyingCabRepairStation>(
		AFlyingCabRepairStation::StaticClass(),
		RepairStationLocation,
		FRotator::ZeroRotator,
		SpawnParameters);

	if (!PickupZone || !DropoffZone || !RepairStation
		|| FuelStations.Num() != FuelStationLocations.Num())
	{
		UE_LOG(LogFlyingCabDelivery, Error, TEXT("Could not spawn delivery zones or all service stations."));
		return;
	}

	PickupZone->Configure(
		EFlyingCabDeliveryZoneType::Pickup,
		ArrivalMaxPlanarSpeed,
		PickupLinkDuration);
	DropoffZone->Configure(
		EFlyingCabDeliveryZoneType::Dropoff,
		ArrivalMaxPlanarSpeed,
		DropoffExitDuration);
	PickupZone->OnCabReady.AddUObject(this, &AFlyingCabGameMode::HandleZoneReady);
	DropoffZone->OnCabReady.AddUObject(this, &AFlyingCabGameMode::HandleZoneReady);
	DispatchNextJob();

	UE_LOG(
		LogFlyingCabDelivery,
		Display,
		TEXT("Delivery dispatcher initialized with %d stops (link %.2f s, exit %.2f s, next call %.2f s)."),
		DeliveryStops.Num(),
		PickupLinkDuration,
		DropoffExitDuration,
		DispatchDelay);
	UE_LOG(
		LogFlyingCabDelivery,
		Display,
		TEXT("Economy initialized with %d credits, %d fuel stations and repair at %s."),
		Credits,
		FuelStations.Num(),
		*RepairStationLocation.ToCompactString());
}

void AFlyingCabGameMode::InitializeTraffic()
{
	struct FTrafficSpec
	{
		FVector Start;
		FVector End;
		float Speed;
		float InitialAlpha;
		FLinearColor Color;
	};

	const FTrafficSpec TrafficSpecs[] = {
		{FVector(-4700.0f, 0.0f, 1500.0f), FVector(4700.0f, 0.0f, 1500.0f), 480.0f, 0.08f, FLinearColor(0.05f, 0.85f, 1.0f)},
		{FVector(-4700.0f, 0.0f, 1500.0f), FVector(4700.0f, 0.0f, 1500.0f), 430.0f, 0.58f, FLinearColor(1.0f, 0.52f, 0.05f)},
		{FVector(4700.0f, 0.0f, 2850.0f), FVector(-4700.0f, 0.0f, 2850.0f), 400.0f, 0.28f, FLinearColor(0.95f, 0.12f, 0.65f)},
		{FVector(-4700.0f, 0.0f, 4550.0f), FVector(4700.0f, 0.0f, 4550.0f), 560.0f, 0.72f, FLinearColor(0.30f, 1.0f, 0.35f)}};

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	TrafficVehicles.Reset();
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(TrafficSpecs); ++Index)
	{
		const FTrafficSpec& Spec = TrafficSpecs[Index];
		AFlyingCabTrafficVehicle* Vehicle = GetWorld()->SpawnActor<AFlyingCabTrafficVehicle>(
			AFlyingCabTrafficVehicle::StaticClass(),
			Spec.Start,
			FRotator::ZeroRotator,
			SpawnParameters);
		if (Vehicle)
		{
			Vehicle->Configure(Spec.Start, Spec.End, Spec.Speed, Spec.InitialAlpha, Spec.Color);
			Vehicle->OnNearMiss.AddUObject(this, &AFlyingCabGameMode::HandleTrafficNearMiss);
			TrafficVehicles.Add(Vehicle);
		}
	}

	UE_LOG(
		LogFlyingCabDelivery,
		Display,
		TEXT("Traffic initialized with %d/%d vehicles; clean near misses award %d credits."),
		TrafficVehicles.Num(),
		UE_ARRAY_COUNT(TrafficSpecs),
		NearMissRewardCredits);
	if (TrafficVehicles.Num() != UE_ARRAY_COUNT(TrafficSpecs))
	{
		UE_LOG(LogFlyingCabDelivery, Warning, TEXT("One or more traffic vehicles failed to spawn."));
	}
}

void AFlyingCabGameMode::EnsurePawnBinding()
{
	AFlyingCabPawn* Pawn = Cast<AFlyingCabPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (Pawn == BoundPawn)
	{
		return;
	}

	if (BoundPawn)
	{
		BoundPawn->OnVehicleDestroyed.RemoveAll(this);
	}
	BoundPawn = Pawn;
	if (BoundPawn)
	{
		BoundPawn->OnVehicleDestroyed.AddUObject(this, &AFlyingCabGameMode::HandleVehicleDestroyed);
		BoundPawn->SetEconomyStatus(Credits, FMath::RoundToInt(ActiveFare));
	}
}

void AFlyingCabGameMode::UpdateActiveFare()
{
	if (!bPassengerOnBoard || !BoundPawn || BoundPawn->IsDestroyed() || !DropoffZone)
	{
		return;
	}

	const FVector Delta = DropoffZone->GetActorLocation() - BoundPawn->GetActorLocation();
	const float Distance = FVector2D(Delta.X, Delta.Z).Size();
	if (FareLastDistance <= 0.0f)
	{
		FareLastDistance = Distance;
		return;
	}

	const float ProgressMeters = (FareLastDistance - Distance) / 100.0f;
	if (!FMath::IsNearlyZero(ProgressMeters, 0.001f))
	{
		const float FareDelta = ProgressMeters >= 0.0f
			? ProgressMeters * FarePerMeterTowardTarget
			: ProgressMeters * FarePerMeterTowardTarget * FareBacktrackPenaltyRatio;
		ActiveFare = FMath::Max(BaseFare, ActiveFare + FareDelta);
		FareLastDistance = Distance;
	}
}

void AFlyingCabGameMode::HandleZoneReady(AFlyingCabDeliveryZone* Zone)
{
	if (!Zone || !Zone->IsZoneActive() || bDispatchPending)
	{
		return;
	}

	if (Zone->GetZoneType() == EFlyingCabDeliveryZoneType::Pickup && !bPassengerOnBoard)
	{
		bPassengerOnBoard = true;
		ActiveFare = BaseFare;
		if (BoundPawn && DropoffZone)
		{
			const FVector FareDelta = DropoffZone->GetActorLocation() - BoundPawn->GetActorLocation();
			FareLastDistance = FVector2D(FareDelta.X, FareDelta.Z).Size();
		}
		PickupZone->SetZoneActive(false);
		DropoffZone->SetZoneActive(true);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				DeliveryMessageKey,
				1.5f,
				FColor(60, 235, 255),
				TEXT("CURBSIDE LINK // PASSENGER SECURED"));
		}
		UE_LOG(LogFlyingCabDelivery, Display, TEXT("Passenger picked up."));
		return;
	}

	if (Zone->GetZoneType() == EFlyingCabDeliveryZoneType::Dropoff && bPassengerOnBoard)
	{
		const int32 FarePayout = FMath::Max(0, FMath::RoundToInt(ActiveFare));
		Credits += FarePayout;
		bPassengerOnBoard = false;
		++CompletedDeliveries;
		LastCompletedPickupIndex = CurrentPickupIndex;
		LastCompletedDropoffIndex = CurrentDropoffIndex;
		ActiveFare = 0.0f;
		FareLastDistance = 0.0f;
		DropoffZone->SetZoneActive(false);

		ScheduleNextDispatch();

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				DeliveryMessageKey,
				2.5f,
				FColor(70, 255, 150),
				FString::Printf(
					TEXT("PASSENGER CLEAR // +%d CR  |  BALANCE: %d  |  TOTAL: %d"),
					FarePayout,
					Credits,
					CompletedDeliveries));
		}
		UE_LOG(
			LogFlyingCabDelivery,
			Display,
			TEXT("Delivery completed for %d credits. Balance %d, total deliveries %d."),
			FarePayout,
			Credits,
			CompletedDeliveries);
	}
}

int32 AFlyingCabGameMode::TryPurchaseFuel(
	AFlyingCabPawn* Pawn,
	int32 RequestedUnits,
	int32 PricePerUnit)
{
	if (!Pawn || Pawn->IsDestroyed() || RequestedUnits <= 0 || PricePerUnit <= 0)
	{
		return 0;
	}

	const int32 NeededUnits = FMath::CeilToInt(Pawn->GetFuelNeeded());
	const int32 AffordableUnits = Credits / PricePerUnit;
	const int32 UnitsToPurchase = FMath::Min3(RequestedUnits, NeededUnits, AffordableUnits);
	if (UnitsToPurchase <= 0)
	{
		if (GEngine && AffordableUnits <= 0)
		{
			GEngine->AddOnScreenDebugMessage(
				FuelMessageKey,
				1.0f,
				FColor(255, 90, 30),
				TEXT("FUEL SERVICE // INSUFFICIENT CREDITS"));
		}
		return 0;
	}

	const float FuelAdded = Pawn->AddFuel(static_cast<float>(UnitsToPurchase));
	if (FuelAdded <= UE_SMALL_NUMBER)
	{
		return 0;
	}

	const int32 ChargedUnits = FMath::CeilToInt(FuelAdded);
	Credits = FMath::Max(0, Credits - ChargedUnits * PricePerUnit);
	Pawn->SetEconomyStatus(Credits, FMath::RoundToInt(ActiveFare));
	return ChargedUnits;
}

int32 AFlyingCabGameMode::TryPurchaseRepair(
	AFlyingCabPawn* Pawn,
	int32 RequestedUnits,
	int32 PricePerUnit)
{
	if (!Pawn || Pawn->IsDestroyed() || RequestedUnits <= 0 || PricePerUnit <= 0)
	{
		return 0;
	}

	const int32 NeededUnits = FMath::CeilToInt(Pawn->GetHullNeeded());
	const int32 AffordableUnits = Credits / PricePerUnit;
	const int32 UnitsToPurchase = FMath::Min3(RequestedUnits, NeededUnits, AffordableUnits);
	if (UnitsToPurchase <= 0)
	{
		if (GEngine && AffordableUnits <= 0)
		{
			GEngine->AddOnScreenDebugMessage(
				RepairMessageKey,
				1.0f,
				FColor(255, 90, 30),
				TEXT("NIGHTSHIFT REPAIR // INSUFFICIENT CREDITS"));
		}
		return 0;
	}

	const float HullAdded = Pawn->AddHull(static_cast<float>(UnitsToPurchase));
	if (HullAdded <= UE_SMALL_NUMBER)
	{
		return 0;
	}

	const int32 ChargedUnits = FMath::CeilToInt(HullAdded);
	Credits = FMath::Max(0, Credits - ChargedUnits * PricePerUnit);
	Pawn->SetEconomyStatus(Credits, FMath::RoundToInt(ActiveFare));
	return ChargedUnits;
}

void AFlyingCabGameMode::HandleVehicleDestroyed(AFlyingCabPawn* Pawn)
{
	if (!Pawn || Pawn != BoundPawn)
	{
		return;
	}

	bPassengerOnBoard = false;
	ActiveFare = 0.0f;
	FareLastDistance = 0.0f;
	const int32 ChargedTowFee = FMath::Min(Credits, FMath::Max(0, TowFee));
	Credits -= ChargedTowFee;
	PendingRecoveryPawn = Pawn;
	bDispatchPending = false;
	GetWorldTimerManager().ClearTimer(DispatchTimerHandle);
	if (PickupZone)
	{
		PickupZone->SetZoneActive(false);
	}
	if (DropoffZone)
	{
		DropoffZone->SetZoneActive(false);
	}
	Pawn->SetEconomyStatus(Credits, 0);
	Pawn->ClearMinimapTarget();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			DeliveryMessageKey,
			DestroyedRecoveryDelay,
			FColor(255, 40, 20),
			FString::Printf(
				TEXT("CAB DESTROYED // TOW CHARGE: %d CR // RECOVERY INBOUND"),
				ChargedTowFee));
	}
	UE_LOG(
		LogFlyingCabDelivery,
		Warning,
		TEXT("Cab destroyed. Course aborted and %d credit tow fee charged."),
		ChargedTowFee);

	GetWorldTimerManager().ClearTimer(RecoveryTimerHandle);
	GetWorldTimerManager().SetTimer(
		RecoveryTimerHandle,
		this,
		&AFlyingCabGameMode::RecoverVehicleAfterTow,
		DestroyedRecoveryDelay,
		false);
}

void AFlyingCabGameMode::RecoverVehicleAfterTow()
{
	if (PendingRecoveryPawn)
	{
		PendingRecoveryPawn->RecoverVehicle(RecoveryFuelPercent);
		PendingRecoveryPawn->SetEconomyStatus(Credits, 0);
		UE_LOG(LogFlyingCabDelivery, Display, TEXT("Cab recovered after tow."));
	}
	PendingRecoveryPawn = nullptr;
	DispatchNextJob();
}

void AFlyingCabGameMode::ScheduleNextDispatch()
{
	bDispatchPending = true;
	if (PickupZone)
	{
		PickupZone->SetZoneActive(false);
	}
	if (DropoffZone)
	{
		DropoffZone->SetZoneActive(false);
	}
	if (BoundPawn)
	{
		BoundPawn->ClearMinimapTarget();
	}

	GetWorldTimerManager().ClearTimer(DispatchTimerHandle);
	GetWorldTimerManager().SetTimer(
		DispatchTimerHandle,
		this,
		&AFlyingCabGameMode::DispatchNextJob,
		DispatchDelay,
		false);
}

void AFlyingCabGameMode::DispatchNextJob()
{
	if (!PickupZone || !DropoffZone || DeliveryStops.Num() < 2)
	{
		return;
	}

	TArray<TPair<int32, int32>> RouteCandidates;
	for (int32 PickupIndex = 0; PickupIndex < DeliveryStops.Num(); ++PickupIndex)
	{
		for (int32 DropoffIndex = 0; DropoffIndex < DeliveryStops.Num(); ++DropoffIndex)
		{
			const bool bSameStop = PickupIndex == DropoffIndex;
			const bool bRepeatsLastRoute = PickupIndex == LastCompletedPickupIndex
				&& DropoffIndex == LastCompletedDropoffIndex;
			const bool bWouldSpawnAtLastDropoff = DeliveryStops.Num() > 2
				&& PickupIndex == LastCompletedDropoffIndex;
			if (!bSameStop && !bRepeatsLastRoute && !bWouldSpawnAtLastDropoff)
			{
				RouteCandidates.Emplace(PickupIndex, DropoffIndex);
			}
		}
	}
	if (RouteCandidates.IsEmpty())
	{
		for (int32 PickupIndex = 0; PickupIndex < DeliveryStops.Num(); ++PickupIndex)
		{
			for (int32 DropoffIndex = 0; DropoffIndex < DeliveryStops.Num(); ++DropoffIndex)
			{
				if (PickupIndex != DropoffIndex)
				{
					RouteCandidates.Emplace(PickupIndex, DropoffIndex);
				}
			}
		}
	}

	const TPair<int32, int32>& Route = RouteCandidates[
		DispatchRandom.RandRange(0, RouteCandidates.Num() - 1)];
	const int32 PickupIndex = Route.Key;
	const int32 DropoffIndex = Route.Value;
	bDispatchPending = false;
	SetRoute(PickupIndex, DropoffIndex);

	const FString PickupName = DeliveryStopNames.IsValidIndex(PickupIndex)
		? DeliveryStopNames[PickupIndex]
		: FString::Printf(TEXT("STOP %d"), PickupIndex + 1);
	const FString DropoffName = DeliveryStopNames.IsValidIndex(DropoffIndex)
		? DeliveryStopNames[DropoffIndex]
		: FString::Printf(TEXT("STOP %d"), DropoffIndex + 1);
	UE_LOG(
		LogFlyingCabDelivery,
		Display,
		TEXT("Dispatch assigned route %s -> %s."),
		*PickupName,
		*DropoffName);
	if (GEngine && CompletedDeliveries > 0)
	{
		GEngine->AddOnScreenDebugMessage(
			DeliveryMessageKey,
			1.5f,
			FColor(60, 235, 255),
			FString::Printf(TEXT("NEW CURBSIDE CALL // %s"), *PickupName));
	}
}

void AFlyingCabGameMode::SetRoute(int32 PickupIndex, int32 DropoffIndex)
{
	if (!PickupZone || !DropoffZone || !DeliveryStops.IsValidIndex(PickupIndex)
		|| !DeliveryStops.IsValidIndex(DropoffIndex))
	{
		return;
	}

	CurrentPickupIndex = PickupIndex;
	CurrentDropoffIndex = DropoffIndex;
	PickupZone->SetActorLocation(DeliveryStops[CurrentPickupIndex]);
	DropoffZone->SetActorLocation(DeliveryStops[CurrentDropoffIndex]);
	DropoffZone->SetZoneActive(false);
	PickupZone->SetZoneActive(true);
}

void AFlyingCabGameMode::UpdateObjectiveStatus()
{
	AFlyingCabPawn* Pawn = Cast<AFlyingCabPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	AFlyingCabDeliveryZone* ActiveZone = bPassengerOnBoard ? DropoffZone : PickupZone;
	if (!Pawn)
	{
		return;
	}

	Pawn->SetEconomyStatus(Credits, bPassengerOnBoard ? FMath::RoundToInt(ActiveFare) : 0);
	if (Pawn->IsDestroyed())
	{
		Pawn->SetObjectiveStatus(FText::FromString(TEXT("CAB DESTROYED\nRECOVERY CREW INBOUND")));
		return;
	}
	if (bDispatchPending)
	{
		Pawn->SetObjectiveStatus(FText::FromString(TEXT("DISPATCH // SCANNING CURBSIDE CALLS")));
		Pawn->ClearMinimapTarget();
		return;
	}
	if (!ActiveZone || !ActiveZone->IsZoneActive())
	{
		return;
	}

	const FVector Delta = ActiveZone->GetActorLocation() - Pawn->GetActorLocation();
	const float DistanceMeters = FVector2D(Delta.X, Delta.Z).Size() / 100.0f;
	const FVector Velocity = Pawn->GetVelocity();
	const float PlanarSpeed = FVector2D(Velocity.X, Velocity.Z).Size();
	const bool bInsideTooFast = ActiveZone->IsPawnInside(Pawn)
		&& PlanarSpeed > ActiveZone->GetArrivalMaxPlanarSpeed();
	const int32 TargetIndex = bPassengerOnBoard ? CurrentDropoffIndex : CurrentPickupIndex;
	const FString TargetName = DeliveryStopNames.IsValidIndex(TargetIndex)
		? DeliveryStopNames[TargetIndex]
		: FString::Printf(TEXT("STOP %d"), TargetIndex + 1);
	const FString Action = bPassengerOnBoard
		? FString::Printf(TEXT("DROPOFF: %s"), *TargetName)
		: FString::Printf(TEXT("PASSENGER IN %s"), *TargetName);

	FString Status;
	if (ActiveZone->IsConfirmationInProgress())
	{
		const TCHAR* SequenceName = ActiveZone->GetZoneType() == EFlyingCabDeliveryZoneType::Pickup
			? TEXT("CURBSIDE LINK")
			: TEXT("CURBSIDE EXIT");
		Status = FString::Printf(
			TEXT("%s  |  %d%%\nHOLD POSITION // %s"),
			SequenceName,
			FMath::RoundToInt(ActiveZone->GetConfirmationAlpha() * 100.0f),
			*TargetName);
	}
	else if (bInsideTooFast)
	{
		Status = FString::Printf(
			TEXT("SLOW DOWN IN %s  |  SPEED %.0f / %.0f\nDELIVERIES: %d"),
			*TargetName,
			PlanarSpeed,
			ActiveZone->GetArrivalMaxPlanarSpeed(),
			CompletedDeliveries);
	}
	else
	{
		Status = FString::Printf(
			TEXT("%s  |  %.0f m\nSTOP IN THE GATE  |  DELIVERIES: %d"),
			*Action,
			DistanceMeters,
			CompletedDeliveries);
	}
	Pawn->SetObjectiveStatus(FText::FromString(Status));
	Pawn->SetMinimapState(
		FVector2D(Pawn->GetActorLocation().X, Pawn->GetActorLocation().Z),
		FVector2D(ActiveZone->GetActorLocation().X, ActiveZone->GetActorLocation().Z),
		bPassengerOnBoard);
}

void AFlyingCabGameMode::UpdateTrafficAwareness(float DeltaSeconds)
{
	NearMissMessageRemaining = FMath::Max(0.0f, NearMissMessageRemaining - DeltaSeconds);
	if (!BoundPawn)
	{
		return;
	}
	if (BoundPawn->IsDestroyed())
	{
		BoundPawn->SetTrafficAlert(FText::GetEmpty(), FLinearColor::Transparent);
		return;
	}
	if (NearMissMessageRemaining > 0.0f)
	{
		BoundPawn->SetTrafficAlert(
			FText::FromString(FString::Printf(TEXT("CLEAN NEAR MISS // +%d CR"), NearMissRewardCredits)),
			FLinearColor(0.15f, 1.0f, 0.45f));
		return;
	}

	AFlyingCabTrafficVehicle* ClosestThreat = nullptr;
	float ClosestImpactTime = TrafficWarningLookAhead + 1.0f;
	const FVector PawnLocation = BoundPawn->GetActorLocation();
	const FVector PawnVelocity = BoundPawn->GetVelocity();
	for (AFlyingCabTrafficVehicle* Vehicle : TrafficVehicles)
	{
		if (!Vehicle)
		{
			continue;
		}

		const FVector RelativeLocation = Vehicle->GetActorLocation() - PawnLocation;
		const FVector RelativeVelocity = Vehicle->GetTrafficVelocity() - PawnVelocity;
		if (FMath::Abs(RelativeVelocity.X) <= UE_SMALL_NUMBER)
		{
			continue;
		}

		const float ImpactTime = -RelativeLocation.X / RelativeVelocity.X;
		if (ImpactTime <= 0.0f || ImpactTime > TrafficWarningLookAhead
			|| ImpactTime >= ClosestImpactTime)
		{
			continue;
		}

		const float PredictedVerticalSeparation = FMath::Abs(
			RelativeLocation.Z + RelativeVelocity.Z * ImpactTime);
		if (PredictedVerticalSeparation <= TrafficWarningVerticalRange)
		{
			ClosestThreat = Vehicle;
			ClosestImpactTime = ImpactTime;
		}
	}

	if (!ClosestThreat)
	{
		BoundPawn->SetTrafficAlert(FText::GetEmpty(), FLinearColor::Transparent);
		return;
	}

	const bool bFromLeft = ClosestThreat->GetActorLocation().X < PawnLocation.X;
	const FLinearColor AlertColor = ClosestImpactTime <= 0.65f
		? FLinearColor(1.0f, 0.12f, 0.03f)
		: FLinearColor(1.0f, 0.66f, 0.05f);
	BoundPawn->SetTrafficAlert(
		FText::FromString(FString::Printf(
			TEXT("TRAFFIC // %.1f SEC\nINBOUND FROM %s"),
			ClosestImpactTime,
			bFromLeft ? TEXT("LEFT") : TEXT("RIGHT"))),
		AlertColor);
}

void AFlyingCabGameMode::HandleTrafficNearMiss(
	AFlyingCabTrafficVehicle* Vehicle,
	AFlyingCabPawn* Pawn)
{
	if (!Vehicle || !Pawn || Pawn != BoundPawn || Pawn->IsDestroyed()
		|| NearMissRewardCredits <= 0)
	{
		return;
	}

	Credits += NearMissRewardCredits;
	NearMissMessageRemaining = 1.25f;
	Pawn->SetEconomyStatus(Credits, bPassengerOnBoard ? FMath::RoundToInt(ActiveFare) : 0);
	UE_LOG(
		LogFlyingCabDelivery,
		Display,
		TEXT("Clean traffic near miss awarded %d credits. Balance %d."),
		NearMissRewardCredits,
		Credits);
}
