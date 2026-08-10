// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabGameMode.h"

#include "Engine/GameInstance.h"
#include "FlyingCabAccessTerminal.h"
#include "FlyingCabCharacter.h"
#include "FlyingCabCityExpansion.h"
#include "FlyingCabDeliveryZone.h"
#include "FlyingCabFuelStation.h"
#include "FlyingCabNightshiftOffice.h"
#include "FlyingCabOnFootPortal.h"
#include "FlyingCabPawn.h"
#include "FlyingCabPlayerController.h"
#include "FlyingCabProgressionSubsystem.h"
#include "FlyingCabRepairStation.h"
#include "FlyingCabScoreSaveGame.h"
#include "FlyingCabTrafficVehicle.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabDelivery, Log, All);

namespace
{
	const FString TimeAttackSaveSlot = TEXT("FlyingCabTimeAttackScores");
	constexpr int32 TimeAttackSaveUserIndex = 0;
}

AFlyingCabGameMode::AFlyingCabGameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FClassFinder<AFlyingCabPawn> TunablePawnClass(
		TEXT("/Game/Blueprints/BP_FlyingCabPawn"));

	DefaultPawnClass = AFlyingCabPawn::StaticClass();
	PlayerControllerClass = AFlyingCabPlayerController::StaticClass();
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
		FVector(3350.0f, 0.0f, 5200.0f),
		FVector(6500.0f, 0.0f, 1150.0f),
		FVector(8650.0f, 0.0f, 2700.0f),
		FVector(11150.0f, 0.0f, 3950.0f),
		FVector(13250.0f, 0.0f, 5450.0f)};
	DeliveryStopNames = {
		TEXT("YELLOW PROJECTS"),
		TEXT("MIDTOWN EXCHANGE"),
		TEXT("SKYLINE TERRACES"),
		TEXT("ASHLINE MARKET"),
		TEXT("NEON DOCKS"),
		TEXT("ZENITH SPIRE"),
		TEXT("GLASSWARD TRANSIT"),
		TEXT("RAINLINE BAZAAR"),
		TEXT("COBALT HEIGHTS"),
		TEXT("ORBITAL GARDENS")};
	FuelStationLocations = {
		FVector(850.0f, 0.0f, 2050.0f),
		FVector(-3800.0f, 0.0f, 2500.0f),
		FVector(8650.0f, 0.0f, 2700.0f)};
	FuelStationNames = {
		TEXT("MIDTOWN FUEL"),
		TEXT("ASHLINE CHARGE"),
		TEXT("RAINLINE ENERGY")};
	RepairStationLocations = {
		FVector(0.0f, 0.0f, 4200.0f),
		FVector(13250.0f, 0.0f, 5450.0f)};
	RepairStationNames = {
		TEXT("NIGHTSHIFT REPAIR"),
		TEXT("ORBITAL BODYWORKS")};
}

void AFlyingCabGameMode::BeginPlay()
{
	Super::BeginPlay();
	Credits = FMath::Max(0, StartingCredits);
	DispatchRandom.Initialize(DispatchRandomSeed);
	InitializeCityExpansion();
	InitializeDeliveryLoop();
	InitializeOnFootSlice();
	InitializeServiceVehicle();
	InitializeTraffic();
	EnsurePawnBinding();
}

void AFlyingCabGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (TPair<TWeakObjectPtr<AFlyingCabPawn>, FTimerHandle>& Entry :
		VehicleRecoveryTimerHandles)
	{
		GetWorldTimerManager().ClearTimer(Entry.Value);
	}
	VehicleRecoveryTimerHandles.Empty();
	for (AFlyingCabPawn* Vehicle : TrackedVehicles)
	{
		if (Vehicle)
		{
			Vehicle->OnVehicleDestroyed.RemoveAll(this);
		}
	}
	TrackedVehicles.Empty();
	Super::EndPlay(EndPlayReason);
}

void AFlyingCabGameMode::StartRun(EFlyingCabRunMode Mode)
{
	if (Mode == EFlyingCabRunMode::None || bRunActive)
	{
		return;
	}

	CurrentRunMode = Mode;
	bRunActive = true;
	bRunCompleted = false;
	Credits = FMath::Max(0, StartingCredits);
	RunStartingCredits = Credits;
	RunStartWorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	RunCompletedDeliveries = 0;
	RunDeliveryCreditsEarned = 0;
	RunNearMissCount = 0;
	RunNearMissCreditsEarned = 0;
	RunFuelCreditsSpent = 0;
	RunRepairCreditsSpent = 0;
	RunTowCount = 0;
	RunTowCreditsSpent = 0;

	if (Mode == EFlyingCabRunMode::TimeAttack)
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UFlyingCabProgressionSubsystem* Progression =
				GameInstance->GetSubsystem<UFlyingCabProgressionSubsystem>())
			{
				Progression->ResetAccess();
			}
		}
		if (ServiceAccessTerminal)
		{
			ServiceAccessTerminal->Configure(TEXT("Vehicle.Service"), TEXT("SERVICE VEHICLES"));
		}
	}

	EnsurePawnBinding();
	if (BoundPawn)
	{
		BoundPawn->SetEconomyStatus(Credits, 0);
	}
	UpdateRunModeStatus();
	UE_LOG(
		LogFlyingCabDelivery,
		Display,
		TEXT("Run started in %s mode with %d credits%s."),
		Mode == EFlyingCabRunMode::TimeAttack ? TEXT("Time Attack") : TEXT("Freeroam"),
		Credits,
		Mode == EFlyingCabRunMode::TimeAttack
			? *FString::Printf(TEXT("; target %d"), TimeAttackTargetCredits)
			: TEXT(""));
	CheckTimeAttackGoal();
}

TArray<float> AFlyingCabGameMode::GetBestTimeAttackTimes() const
{
	if (!UGameplayStatics::DoesSaveGameExist(TimeAttackSaveSlot, TimeAttackSaveUserIndex))
	{
		return {};
	}

	const UFlyingCabScoreSaveGame* SaveGame = Cast<UFlyingCabScoreSaveGame>(
		UGameplayStatics::LoadGameFromSlot(TimeAttackSaveSlot, TimeAttackSaveUserIndex));
	if (!SaveGame)
	{
		return {};
	}

	TArray<float> Times = SaveGame->BestTimeAttackSeconds;
	Times.RemoveAll([](float Seconds) { return Seconds <= 0.0f; });
	Times.Sort();
	if (Times.Num() > TimeAttackLeaderboardSize)
	{
		Times.SetNum(TimeAttackLeaderboardSize);
	}
	return Times;
}

void AFlyingCabGameMode::InitializeCityExpansion()
{
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	CityExpansion = GetWorld()->SpawnActor<AFlyingCabCityExpansion>(
		AFlyingCabCityExpansion::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!CityExpansion)
	{
		UE_LOG(LogFlyingCabDelivery, Error, TEXT("Could not create east city extension."));
	}
}

void AFlyingCabGameMode::InitializeOnFootSlice()
{
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

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
		UE_LOG(LogFlyingCabDelivery, Error, TEXT("Could not create Nightshift Office entrance."));
		return;
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
		UE_LOG(LogFlyingCabDelivery, Error, TEXT("Could not complete Nightshift Office interactables."));
		return;
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
	ServiceAccessTerminal->Configure(TEXT("Vehicle.Service"), TEXT("SERVICE VEHICLES"));

	UE_LOG(
		LogFlyingCabDelivery,
		Display,
		TEXT("Nightshift Office initialized at %s with foot-only entrance at %s."),
		*NightshiftOfficeLocation.ToCompactString(),
		*NightshiftEntranceLocation.ToCompactString());
}

void AFlyingCabGameMode::InitializeServiceVehicle()
{
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

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	ServiceVehicle = GetWorld()->SpawnActor<AFlyingCabPawn>(
		DefaultPawnClass,
		GroundedLocation,
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!ServiceVehicle)
	{
		UE_LOG(LogFlyingCabDelivery, Error, TEXT("Could not spawn Nightshift service vehicle."));
		return;
	}

	ServiceVehicle->ConfigureVehicleIdentity(
		TEXT("Vehicle.Service.01"),
		TEXT("NIGHTSHIFT SERVICE CAB"),
		TEXT("Vehicle.Service"),
		FLinearColor(0.06f, 0.78f, 0.92f));
	RegisterVehicle(ServiceVehicle);
	UE_LOG(
		LogFlyingCabDelivery,
		Display,
		TEXT("Nightshift service vehicle spawned at %s; access requires Vehicle.Service."),
		*GroundedLocation.ToCompactString());
}

void AFlyingCabGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	EnsurePawnBinding();
	UpdatePassengerOffers(DeltaSeconds);
	UpdateActiveFare();
	UpdateObjectiveStatus();
	UpdateTrafficAwareness(DeltaSeconds);
	UpdateRunModeStatus();
	CheckTimeAttackGoal();
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
	RepairStations.Reset();
	for (int32 StationIndex = 0; StationIndex < RepairStationLocations.Num(); ++StationIndex)
	{
		AFlyingCabRepairStation* Station = GetWorld()->SpawnActor<AFlyingCabRepairStation>(
			AFlyingCabRepairStation::StaticClass(),
			RepairStationLocations[StationIndex],
			FRotator::ZeroRotator,
			SpawnParameters);
		if (Station)
		{
			const FString StationName = RepairStationNames.IsValidIndex(StationIndex)
				? RepairStationNames[StationIndex]
				: FString::Printf(TEXT("REPAIR SHOP %d"), StationIndex + 1);
			Station->Configure(StationName);
			RepairStations.Add(Station);
		}
	}

	if (!DropoffZone
		|| FuelStations.Num() != FuelStationLocations.Num()
		|| RepairStations.Num() != RepairStationLocations.Num())
	{
		UE_LOG(LogFlyingCabDelivery, Error, TEXT("Could not spawn delivery zone or all service stations."));
		return;
	}

	DropoffZone->Configure(
		EFlyingCabDeliveryZoneType::Dropoff,
		ArrivalMaxPlanarSpeed,
		DropoffExitDuration);
	DropoffZone->OnCabReady.AddUObject(this, &AFlyingCabGameMode::HandleZoneReady);
	DropoffZone->SetZoneActive(false);

	PassengerOffers.Reset();
	const int32 InitialOfferCount = FMath::Clamp(
		InitialWaitingPassengers,
		1,
		FMath::Min(MaxWaitingPassengers, DeliveryStops.Num()));
	for (int32 Index = 0; Index < InitialOfferCount; ++Index)
	{
		SpawnPassengerOffer();
	}
	PassengerSpawnCountdown = DispatchRandom.FRandRange(
		PassengerSpawnIntervalMin,
		PassengerSpawnIntervalMax);

	UE_LOG(
		LogFlyingCabDelivery,
		Display,
		TEXT("Passenger network initialized with %d stops and %d/%d waiting passengers (link %.2f s, exit %.2f s)."),
		DeliveryStops.Num(),
		PassengerOffers.Num(),
		MaxWaitingPassengers,
		PickupLinkDuration,
		DropoffExitDuration);
	UE_LOG(
		LogFlyingCabDelivery,
		Display,
		TEXT("Economy initialized with %d credits, %d fuel stations and %d repair shops."),
		Credits,
		FuelStations.Num(),
		RepairStations.Num());
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
		{FVector(-4700.0f, 0.0f, 4550.0f), FVector(4700.0f, 0.0f, 4550.0f), 560.0f, 0.72f, FLinearColor(0.30f, 1.0f, 0.35f)},
		{FVector(5250.0f, 0.0f, 1650.0f), FVector(14700.0f, 0.0f, 1650.0f), 520.0f, 0.18f, FLinearColor(0.12f, 0.82f, 1.0f)},
		{FVector(14700.0f, 0.0f, 3150.0f), FVector(5250.0f, 0.0f, 3150.0f), 470.0f, 0.52f, FLinearColor(1.0f, 0.30f, 0.08f)},
		{FVector(5250.0f, 0.0f, 4450.0f), FVector(14700.0f, 0.0f, 4450.0f), 590.0f, 0.76f, FLinearColor(0.90f, 0.08f, 0.72f)},
		{FVector(14700.0f, 0.0f, 5550.0f), FVector(5250.0f, 0.0f, 5550.0f), 430.0f, 0.34f, FLinearColor(0.22f, 1.0f, 0.42f)}};

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
	if (!Pawn)
	{
		// The cab remains the active world vehicle while the controller possesses
		// the on-foot character. Do not discard its delivery/economy binding.
		return;
	}
	if (Pawn == BoundPawn)
	{
		return;
	}

	RegisterVehicle(Pawn);
	BoundPawn = Pawn;
	if (BoundPawn)
	{
		BoundPawn->SetEconomyStatus(Credits, FMath::RoundToInt(ActiveFare));
	}
}

void AFlyingCabGameMode::RegisterVehicle(AFlyingCabPawn* Pawn)
{
	TrackedVehicles.RemoveAll([](const TObjectPtr<AFlyingCabPawn>& Vehicle)
	{
		return !IsValid(Vehicle);
	});
	if (!Pawn || TrackedVehicles.Contains(Pawn))
	{
		return;
	}

	TrackedVehicles.Add(Pawn);
	Pawn->OnVehicleDestroyed.AddUObject(this, &AFlyingCabGameMode::HandleVehicleDestroyed);
	UE_LOG(
		LogFlyingCabDelivery,
		Verbose,
		TEXT("Vehicle registered for destruction recovery: %s."),
		*Pawn->GetName());
}

void AFlyingCabGameMode::UpdatePassengerOffers(float DeltaSeconds)
{
	for (int32 OfferIndex = PassengerOffers.Num() - 1; OfferIndex >= 0; --OfferIndex)
	{
		FFlyingCabPassengerOfferState& Offer = PassengerOffers[OfferIndex];
		if (!Offer.Zone)
		{
			PassengerOffers.RemoveAt(OfferIndex);
			continue;
		}
		if (!Offer.Zone->IsConfirmationInProgress())
		{
			Offer.RemainingSeconds = FMath::Max(0.0f, Offer.RemainingSeconds - DeltaSeconds);
			Offer.Zone->SetOfferRemainingSeconds(Offer.RemainingSeconds);
		}
		if (Offer.RemainingSeconds <= 0.0f)
		{
			RemovePassengerOfferAt(OfferIndex, TEXT("expired"));
		}
	}

	if (PassengerOffers.Num() >= MaxWaitingPassengers)
	{
		return;
	}

	PassengerSpawnCountdown -= DeltaSeconds;
	if (PassengerSpawnCountdown <= 0.0f)
	{
		SpawnPassengerOffer();
		PassengerSpawnCountdown = DispatchRandom.FRandRange(
			PassengerSpawnIntervalMin,
			PassengerSpawnIntervalMax);
	}
}

void AFlyingCabGameMode::SpawnPassengerOffer()
{
	if (DeliveryStops.Num() < 2 || PassengerOffers.Num() >= MaxWaitingPassengers)
	{
		return;
	}

	TArray<int32> AvailablePickupIndices;
	for (int32 StopIndex = 0; StopIndex < DeliveryStops.Num(); ++StopIndex)
	{
		const bool bAlreadyOccupied = PassengerOffers.ContainsByPredicate(
			[StopIndex](const FFlyingCabPassengerOfferState& Offer)
			{
				return Offer.PickupIndex == StopIndex;
			});
		const bool bActiveDropoff = bPassengerOnBoard && StopIndex == CurrentDropoffIndex;
		if (!bAlreadyOccupied && !bActiveDropoff)
		{
			AvailablePickupIndices.Add(StopIndex);
		}
	}
	if (AvailablePickupIndices.IsEmpty())
	{
		return;
	}

	const int32 PickupIndex = AvailablePickupIndices[
		DispatchRandom.RandRange(0, AvailablePickupIndices.Num() - 1)];
	TArray<int32> DestinationIndices;
	for (int32 StopIndex = 0; StopIndex < DeliveryStops.Num(); ++StopIndex)
	{
		if (StopIndex != PickupIndex)
		{
			DestinationIndices.Add(StopIndex);
		}
	}
	const int32 DropoffIndex = DestinationIndices[
		DispatchRandom.RandRange(0, DestinationIndices.Num() - 1)];

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AFlyingCabDeliveryZone* Zone = GetWorld()->SpawnActor<AFlyingCabDeliveryZone>(
		AFlyingCabDeliveryZone::StaticClass(),
		DeliveryStops[PickupIndex],
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!Zone)
	{
		UE_LOG(LogFlyingCabDelivery, Warning, TEXT("Could not spawn passenger offer."));
		return;
	}

	const FString DestinationName = DeliveryStopNames.IsValidIndex(DropoffIndex)
		? DeliveryStopNames[DropoffIndex]
		: FString::Printf(TEXT("STOP %d"), DropoffIndex + 1);
	const int32 EstimatedFare = CalculateEstimatedFare(PickupIndex, DropoffIndex);
	const float Lifetime = DispatchRandom.FRandRange(
		FMath::Min(PassengerLifetimeMin, PassengerLifetimeMax),
		FMath::Max(PassengerLifetimeMin, PassengerLifetimeMax));
	Zone->Configure(EFlyingCabDeliveryZoneType::Pickup, ArrivalMaxPlanarSpeed, PickupLinkDuration);
	Zone->ConfigurePassengerOffer(DestinationName, EstimatedFare);
	Zone->SetOfferRemainingSeconds(Lifetime);
	Zone->SetAcceptanceEnabled(!bPassengerOnBoard && !PendingRecoveryPawn);
	Zone->OnCabReady.AddUObject(this, &AFlyingCabGameMode::HandleZoneReady);
	Zone->SetZoneActive(true);

	FFlyingCabPassengerOfferState& Offer = PassengerOffers.AddDefaulted_GetRef();
	Offer.Zone = Zone;
	Offer.PickupIndex = PickupIndex;
	Offer.DropoffIndex = DropoffIndex;
	Offer.EstimatedFareCredits = EstimatedFare;
	Offer.RemainingSeconds = Lifetime;

	const FString PickupName = DeliveryStopNames.IsValidIndex(PickupIndex)
		? DeliveryStopNames[PickupIndex]
		: FString::Printf(TEXT("STOP %d"), PickupIndex + 1);
	UE_LOG(
		LogFlyingCabDelivery,
		Display,
		TEXT("Passenger appeared at %s for %s (~%d credits, %.1f seconds)."),
		*PickupName,
		*DestinationName,
		EstimatedFare,
		Lifetime);
}

void AFlyingCabGameMode::RemovePassengerOfferAt(int32 OfferIndex, const TCHAR* Reason)
{
	if (!PassengerOffers.IsValidIndex(OfferIndex))
	{
		return;
	}

	const FFlyingCabPassengerOfferState Offer = PassengerOffers[OfferIndex];
	PassengerOffers.RemoveAt(OfferIndex);
	if (Offer.Zone)
	{
		Offer.Zone->OnCabReady.RemoveAll(this);
		Offer.Zone->SetZoneActive(false);
		Offer.Zone->Destroy();
	}
	PassengerSpawnCountdown = FMath::Min(
		PassengerSpawnCountdown,
		DispatchRandom.FRandRange(PassengerSpawnIntervalMin, PassengerSpawnIntervalMax));

	UE_LOG(
		LogFlyingCabDelivery,
		Display,
		TEXT("Passenger offer at stop %d removed (%s); %d still waiting."),
		Offer.PickupIndex,
		Reason ? Reason : TEXT("unknown"),
		PassengerOffers.Num());
}

void AFlyingCabGameMode::SetPassengerOfferAcceptance(bool bEnabled)
{
	for (FFlyingCabPassengerOfferState& Offer : PassengerOffers)
	{
		if (Offer.Zone)
		{
			Offer.Zone->SetAcceptanceEnabled(bEnabled);
		}
	}
}

int32 AFlyingCabGameMode::FindPassengerOfferIndex(const AFlyingCabDeliveryZone* Zone) const
{
	return PassengerOffers.IndexOfByPredicate(
		[Zone](const FFlyingCabPassengerOfferState& Offer)
		{
			return Offer.Zone == Zone;
		});
}

int32 AFlyingCabGameMode::CalculateEstimatedFare(int32 PickupIndex, int32 DropoffIndex) const
{
	if (!DeliveryStops.IsValidIndex(PickupIndex) || !DeliveryStops.IsValidIndex(DropoffIndex))
	{
		return FMath::RoundToInt(BaseFare);
	}
	const FVector Delta = DeliveryStops[DropoffIndex] - DeliveryStops[PickupIndex];
	const float DirectDistanceMeters = FVector2D(Delta.X, Delta.Z).Size() / 100.0f;
	return FMath::Max(
		0,
		FMath::RoundToInt(BaseFare + DirectDistanceMeters * FarePerMeterTowardTarget));
}

bool AFlyingCabGameMode::CanPlayerExitVehicle(FText& OutFailureReason) const
{
	if (bPassengerOnBoard)
	{
		OutFailureReason = FText::FromString(
			TEXT("PASSENGER ON BOARD // COMPLETE FARE BEFORE EXITING"));
		return false;
	}
	const bool bLinkInProgress = PassengerOffers.ContainsByPredicate(
		[](const FFlyingCabPassengerOfferState& Offer)
		{
			return Offer.Zone && Offer.Zone->IsConfirmationInProgress();
		});
	if (bLinkInProgress)
	{
		OutFailureReason = FText::FromString(
			TEXT("CURBSIDE LINK IN PROGRESS // HOLD POSITION"));
		return false;
	}

	OutFailureReason = FText::GetEmpty();
	return true;
}

void AFlyingCabGameMode::ShowPlayerEventMessage(
	const FText& Message,
	const FLinearColor& Color,
	float DurationSeconds) const
{
	if (const AFlyingCabPlayerController* PlayerController =
		Cast<AFlyingCabPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PlayerController->ShowEventMessage(Message, Color, DurationSeconds);
	}
}

bool AFlyingCabGameMode::IsPlayerOnFoot() const
{
	return BoundPawn && UGameplayStatics::GetPlayerPawn(this, 0) != BoundPawn;
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
	if (!Zone || !Zone->IsZoneActive())
	{
		return;
	}

	if (Zone->GetZoneType() == EFlyingCabDeliveryZoneType::Pickup && !bPassengerOnBoard)
	{
		const int32 OfferIndex = FindPassengerOfferIndex(Zone);
		if (!PassengerOffers.IsValidIndex(OfferIndex))
		{
			return;
		}
		const FFlyingCabPassengerOfferState SelectedOffer = PassengerOffers[OfferIndex];
		CurrentPickupIndex = SelectedOffer.PickupIndex;
		CurrentDropoffIndex = SelectedOffer.DropoffIndex;
		DropoffZone->SetActorLocation(DeliveryStops[CurrentDropoffIndex]);
		bPassengerOnBoard = true;
		ActiveFare = BaseFare;
		if (BoundPawn && DropoffZone)
		{
			const FVector FareDelta = DropoffZone->GetActorLocation() - BoundPawn->GetActorLocation();
			FareLastDistance = FVector2D(FareDelta.X, FareDelta.Z).Size();
		}
		SetPassengerOfferAcceptance(false);
		RemovePassengerOfferAt(OfferIndex, TEXT("boarded"));
		DropoffZone->SetZoneActive(true);
		ShowPlayerEventMessage(
			FText::FromString(TEXT("CURBSIDE LINK // PASSENGER SECURED")),
			FLinearColor::FromSRGBColor(FColor(60, 235, 255)),
			1.5f);
		const FString DestinationName = DeliveryStopNames.IsValidIndex(CurrentDropoffIndex)
			? DeliveryStopNames[CurrentDropoffIndex]
			: FString::Printf(TEXT("STOP %d"), CurrentDropoffIndex + 1);
		UE_LOG(
			LogFlyingCabDelivery,
			Display,
			TEXT("Passenger picked up for %s; one-seat capacity is now occupied."),
			*DestinationName);
		return;
	}

	if (Zone->GetZoneType() == EFlyingCabDeliveryZoneType::Dropoff && bPassengerOnBoard)
	{
		const int32 FarePayout = FMath::Max(0, FMath::RoundToInt(ActiveFare));
		Credits += FarePayout;
		if (bRunActive)
		{
			++RunCompletedDeliveries;
			RunDeliveryCreditsEarned += FarePayout;
		}
		bPassengerOnBoard = false;
		++CompletedDeliveries;
		LastCompletedPickupIndex = CurrentPickupIndex;
		LastCompletedDropoffIndex = CurrentDropoffIndex;
		ActiveFare = 0.0f;
		FareLastDistance = 0.0f;
		DropoffZone->SetZoneActive(false);
		SetPassengerOfferAcceptance(!PendingRecoveryPawn);

		ShowPlayerEventMessage(
			FText::FromString(FString::Printf(
					TEXT("PASSENGER CLEAR // +%d CR  |  BALANCE: %d  |  TOTAL: %d"),
					FarePayout,
					Credits,
					CompletedDeliveries)),
			FLinearColor::FromSRGBColor(FColor(70, 255, 150)),
			2.5f);
		UE_LOG(
			LogFlyingCabDelivery,
			Display,
			TEXT("Delivery completed for %d credits. Balance %d, total deliveries %d."),
			FarePayout,
			Credits,
			CompletedDeliveries);
		CheckTimeAttackGoal();
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
		if (AffordableUnits <= 0)
		{
			ShowPlayerEventMessage(
				FText::FromString(TEXT("FUEL SERVICE // INSUFFICIENT CREDITS")),
				FLinearColor::FromSRGBColor(FColor(255, 90, 30)),
				1.0f);
		}
		return 0;
	}

	const float FuelAdded = Pawn->AddFuel(static_cast<float>(UnitsToPurchase));
	if (FuelAdded <= UE_SMALL_NUMBER)
	{
		return 0;
	}

	const int32 ChargedUnits = FMath::CeilToInt(FuelAdded);
	const int32 FuelCost = ChargedUnits * PricePerUnit;
	Credits = FMath::Max(0, Credits - FuelCost);
	if (bRunActive)
	{
		RunFuelCreditsSpent += FuelCost;
	}
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
		if (AffordableUnits <= 0)
		{
			ShowPlayerEventMessage(
				FText::FromString(TEXT("NIGHTSHIFT REPAIR // INSUFFICIENT CREDITS")),
				FLinearColor::FromSRGBColor(FColor(255, 90, 30)),
				1.0f);
		}
		return 0;
	}

	const float HullAdded = Pawn->AddHull(static_cast<float>(UnitsToPurchase));
	if (HullAdded <= UE_SMALL_NUMBER)
	{
		return 0;
	}

	const int32 ChargedUnits = FMath::CeilToInt(HullAdded);
	const int32 RepairCost = ChargedUnits * PricePerUnit;
	Credits = FMath::Max(0, Credits - RepairCost);
	if (bRunActive)
	{
		RunRepairCreditsSpent += RepairCost;
	}
	Pawn->SetEconomyStatus(Credits, FMath::RoundToInt(ActiveFare));
	return ChargedUnits;
}

void AFlyingCabGameMode::HandleVehicleDestroyed(AFlyingCabPawn* Pawn)
{
	if (!Pawn)
	{
		return;
	}
	RegisterVehicle(Pawn);

	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const bool bAffectsActiveRun = PlayerPawn == Pawn
		|| (Pawn == BoundPawn && IsPlayerOnFoot());
	if (!bAffectsActiveRun)
	{
		Pawn->SetEconomyStatus(Credits, 0);
		ShowPlayerEventMessage(
			FText::FromString(FString::Printf(
				TEXT("%s DAMAGED // REMOTE RECOVERY INBOUND"),
				*Pawn->GetVehicleDisplayName())),
			FLinearColor::FromSRGBColor(FColor(255, 140, 35)),
			DestroyedRecoveryDelay);
		UE_LOG(
			LogFlyingCabDelivery,
			Warning,
			TEXT("Parked vehicle %s destroyed; scheduling recovery without tow charge."),
			*Pawn->GetName());
		ScheduleVehicleRecovery(Pawn);
		return;
	}

	bPassengerOnBoard = false;
	ActiveFare = 0.0f;
	FareLastDistance = 0.0f;
	const int32 ChargedTowFee = FMath::Min(Credits, FMath::Max(0, TowFee));
	Credits -= ChargedTowFee;
	if (bRunActive)
	{
		++RunTowCount;
		RunTowCreditsSpent += ChargedTowFee;
	}
	PendingRecoveryPawn = Pawn;
	SetPassengerOfferAcceptance(false);
	if (DropoffZone)
	{
		DropoffZone->SetZoneActive(false);
	}
	Pawn->SetEconomyStatus(Credits, 0);
	Pawn->ClearMinimapTarget();

	ShowPlayerEventMessage(
		FText::FromString(FString::Printf(
				TEXT("CAB DESTROYED // TOW CHARGE: %d CR // RECOVERY INBOUND"),
				ChargedTowFee)),
		FLinearColor::FromSRGBColor(FColor(255, 40, 20)),
		DestroyedRecoveryDelay);
	UE_LOG(
		LogFlyingCabDelivery,
		Warning,
		TEXT("Cab destroyed. Course aborted and %d credit tow fee charged."),
		ChargedTowFee);

	ScheduleVehicleRecovery(Pawn);
}

void AFlyingCabGameMode::ScheduleVehicleRecovery(AFlyingCabPawn* Pawn)
{
	if (!Pawn)
	{
		return;
	}

	const TWeakObjectPtr<AFlyingCabPawn> VehicleKey(Pawn);
	if (FTimerHandle* ExistingHandle = VehicleRecoveryTimerHandles.Find(VehicleKey))
	{
		GetWorldTimerManager().ClearTimer(*ExistingHandle);
	}

	FTimerDelegate RecoveryDelegate;
	RecoveryDelegate.BindUObject(
		this,
		&AFlyingCabGameMode::RecoverVehicleAfterTow,
		Pawn);
	FTimerHandle& RecoveryHandle = VehicleRecoveryTimerHandles.FindOrAdd(VehicleKey);
	GetWorldTimerManager().SetTimer(
		RecoveryHandle,
		RecoveryDelegate,
		DestroyedRecoveryDelay,
		false);
}

void AFlyingCabGameMode::RecoverVehicleAfterTow(AFlyingCabPawn* Pawn)
{
	VehicleRecoveryTimerHandles.Remove(TWeakObjectPtr<AFlyingCabPawn>(Pawn));
	if (Pawn)
	{
		Pawn->RecoverVehicle(RecoveryFuelPercent);
		Pawn->SetEconomyStatus(Credits, 0);
		UE_LOG(
			LogFlyingCabDelivery,
			Display,
			TEXT("Vehicle %s recovered after tow."),
			*Pawn->GetName());
	}
	if (PendingRecoveryPawn == Pawn)
	{
		PendingRecoveryPawn = nullptr;
		SetPassengerOfferAcceptance(true);
	}
}

void AFlyingCabGameMode::UpdateObjectiveStatus()
{
	AFlyingCabPawn* Pawn = BoundPawn;
	if (!Pawn)
	{
		return;
	}

	TArray<FVector2D> OfferPositions;
	AFlyingCabDeliveryZone* NearestOfferZone = nullptr;
	const FFlyingCabPassengerOfferState* NearestOffer = nullptr;
	float NearestOfferDistanceSquared = TNumericLimits<float>::Max();
	for (const FFlyingCabPassengerOfferState& Offer : PassengerOffers)
	{
		if (!Offer.Zone || !Offer.Zone->IsZoneActive())
		{
			continue;
		}
		const FVector OfferLocation = Offer.Zone->GetActorLocation();
		OfferPositions.Emplace(OfferLocation.X, OfferLocation.Z);
		const FVector OfferDelta = OfferLocation - Pawn->GetActorLocation();
		const float DistanceSquared = FVector2D(OfferDelta.X, OfferDelta.Z).SizeSquared();
		if (DistanceSquared < NearestOfferDistanceSquared)
		{
			NearestOfferDistanceSquared = DistanceSquared;
			NearestOfferZone = Offer.Zone;
			NearestOffer = &Offer;
		}
	}
	const FVector2D CabMapPosition(Pawn->GetActorLocation().X, Pawn->GetActorLocation().Z);
	Pawn->SetPassengerOfferMarkers(CabMapPosition, OfferPositions);
	AFlyingCabDeliveryZone* ActiveZone = bPassengerOnBoard ? DropoffZone.Get() : NearestOfferZone;

	Pawn->SetEconomyStatus(Credits, bPassengerOnBoard ? FMath::RoundToInt(ActiveFare) : 0);
	if (IsPlayerOnFoot())
	{
		const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
		const AFlyingCabCharacter* OnFootCharacter = Cast<AFlyingCabCharacter>(PlayerPawn);
		const FVector PlayerLocation = PlayerPawn ? PlayerPawn->GetActorLocation() : Pawn->GetActorLocation();
		const FVector CabDelta = Pawn->GetActorLocation() - PlayerLocation;
		const float CabDistanceMeters = FVector2D(CabDelta.X, CabDelta.Z).Size() / 100.0f;
		Pawn->SetTrafficAlert(FText::GetEmpty(), FLinearColor::Transparent);
		if (OnFootCharacter && OnFootCharacter->IsDead())
		{
			Pawn->SetObjectiveStatus(FText::FromString(
				TEXT("DRIVER DOWN\nRELOADING LEVEL")));
		}
		else
		{
			const float HealthPercent = OnFootCharacter
				? OnFootCharacter->GetHealthPercent() * 100.0f
				: 0.0f;
			AFlyingCabPlayerController* FlyingCabController = Cast<AFlyingCabPlayerController>(
				UGameplayStatics::GetPlayerController(this, 0));
			const FString ContextPrompt = FlyingCabController
				? FlyingCabController->GetContextPrompt().ToString()
				: FString(TEXT("EXPLORE ON FOOT"));
			Pawn->SetObjectiveStatus(FText::FromString(FString::Printf(
				TEXT("ON FOOT // HEALTH %.0f%% // CAB %.1f M\n%s"),
				HealthPercent,
				CabDistanceMeters,
				*ContextPrompt)));
		}
		Pawn->SetProximityGuidance(false, FVector2D::ZeroVector, false);
		if (bPassengerOnBoard && ActiveZone && ActiveZone->IsZoneActive())
		{
			Pawn->SetMinimapState(
				CabMapPosition,
				FVector2D(ActiveZone->GetActorLocation().X, ActiveZone->GetActorLocation().Z),
				true);
		}
		else
		{
			Pawn->ClearMinimapTarget();
		}
		return;
	}
	if (Pawn->IsDestroyed())
	{
		Pawn->SetObjectiveStatus(FText::FromString(TEXT("CAB DESTROYED\nRECOVERY CREW INBOUND")));
		Pawn->SetProximityGuidance(false, FVector2D::ZeroVector, false);
		return;
	}
	if (!ActiveZone || !ActiveZone->IsZoneActive())
	{
		Pawn->SetObjectiveStatus(FText::FromString(
			TEXT("NO CURBSIDE CALLS\nPASSENGER NETWORK SEARCHING")));
		Pawn->ClearMinimapTarget();
		Pawn->SetProximityGuidance(false, FVector2D::ZeroVector, false);
		return;
	}

	const FVector Delta = ActiveZone->GetActorLocation() - Pawn->GetActorLocation();
	const float Distance = FVector2D(Delta.X, Delta.Z).Size();
	const float DistanceMeters = Distance / 100.0f;
	const FVector Velocity = Pawn->GetVelocity();
	const float PlanarSpeed = FVector2D(Velocity.X, Velocity.Z).Size();
	const bool bInsideTooFast = ActiveZone->IsPawnInside(Pawn)
		&& PlanarSpeed > ActiveZone->GetArrivalMaxPlanarSpeed();
	const int32 TargetIndex = bPassengerOnBoard
		? CurrentDropoffIndex
		: (NearestOffer ? NearestOffer->PickupIndex : INDEX_NONE);
	const FString TargetName = DeliveryStopNames.IsValidIndex(TargetIndex)
		? DeliveryStopNames[TargetIndex]
		: FString::Printf(TEXT("STOP %d"), TargetIndex + 1);
	const FString DestinationName = !bPassengerOnBoard && NearestOffer
		&& DeliveryStopNames.IsValidIndex(NearestOffer->DropoffIndex)
		? DeliveryStopNames[NearestOffer->DropoffIndex]
		: FString();

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
		if (bPassengerOnBoard)
		{
			Status = FString::Printf(
				TEXT("DROPOFF: %s  |  %.0f m\nSTOP IN THE GATE  |  DELIVERIES: %d"),
				*TargetName,
				DistanceMeters,
				CompletedDeliveries);
		}
		else
		{
			Status = FString::Printf(
				TEXT("NEAREST: %s -> %s  |  %.0f m\n~%d CR  |  %d CALLS ACTIVE // CHOOSE ON MAP"),
				*TargetName,
				*DestinationName,
				DistanceMeters,
				NearestOffer ? NearestOffer->EstimatedFareCredits : 0,
				PassengerOffers.Num());
		}
	}
	Pawn->SetObjectiveStatus(FText::FromString(Status));
	if (bPassengerOnBoard)
	{
		Pawn->SetMinimapState(
			CabMapPosition,
			FVector2D(ActiveZone->GetActorLocation().X, ActiveZone->GetActorLocation().Z),
			true);
	}
	else
	{
		Pawn->ClearMinimapTarget();
	}
	Pawn->SetProximityGuidance(
		Distance <= ProximityGuidanceRange,
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
	if (IsPlayerOnFoot())
	{
		BoundPawn->SetTrafficAlert(FText::GetEmpty(), FLinearColor::Transparent);
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
	if (bRunActive)
	{
		++RunNearMissCount;
		RunNearMissCreditsEarned += NearMissRewardCredits;
	}
	NearMissMessageRemaining = 1.25f;
	Pawn->SetEconomyStatus(Credits, bPassengerOnBoard ? FMath::RoundToInt(ActiveFare) : 0);
	UE_LOG(
		LogFlyingCabDelivery,
		Display,
		TEXT("Clean traffic near miss awarded %d credits. Balance %d."),
		NearMissRewardCredits,
		Credits);
	CheckTimeAttackGoal();
}

void AFlyingCabGameMode::UpdateRunModeStatus()
{
	if (!BoundPawn)
	{
		return;
	}

	const bool bShowTimeAttack = CurrentRunMode == EFlyingCabRunMode::TimeAttack
		&& (bRunActive || bRunCompleted);
	BoundPawn->SetTimeAttackStatus(
		bShowTimeAttack,
		GetRunElapsedSeconds(),
		Credits,
		TimeAttackTargetCredits);
}

void AFlyingCabGameMode::CheckTimeAttackGoal()
{
	if (!bRunActive || CurrentRunMode != EFlyingCabRunMode::TimeAttack
		|| Credits < TimeAttackTargetCredits)
	{
		return;
	}
	FinishTimeAttack();
}

void AFlyingCabGameMode::FinishTimeAttack()
{
	if (!bRunActive || bRunCompleted)
	{
		return;
	}

	const float ElapsedSeconds = GetRunElapsedSeconds();
	bRunActive = false;
	bRunCompleted = true;

	FFlyingCabTimeAttackResult Result;
	Result.ElapsedSeconds = ElapsedSeconds;
	Result.FinalCredits = Credits;
	Result.TargetCredits = TimeAttackTargetCredits;
	Result.CompletedDeliveries = RunCompletedDeliveries;
	Result.DeliveryCreditsEarned = RunDeliveryCreditsEarned;
	Result.NearMissCount = RunNearMissCount;
	Result.NearMissCreditsEarned = RunNearMissCreditsEarned;
	Result.FuelCreditsSpent = RunFuelCreditsSpent;
	Result.RepairCreditsSpent = RunRepairCreditsSpent;
	Result.TowCount = RunTowCount;
	Result.TowCreditsSpent = RunTowCreditsSpent;

	SaveTimeAttackScore(ElapsedSeconds);
	UpdateRunModeStatus();
	UE_LOG(
		LogFlyingCabDelivery,
		Display,
		TEXT("Time Attack complete in %.2f seconds with %d credits, %d deliveries and %d near misses."),
		ElapsedSeconds,
		Credits,
		RunCompletedDeliveries,
		RunNearMissCount);

	if (AFlyingCabPlayerController* Controller = Cast<AFlyingCabPlayerController>(
		UGameplayStatics::GetPlayerController(this, 0)))
	{
		Controller->ShowTimeAttackResults(Result, GetBestTimeAttackTimes());
	}
}

void AFlyingCabGameMode::SaveTimeAttackScore(float ElapsedSeconds)
{
	if (ElapsedSeconds <= 0.0f)
	{
		return;
	}

	UFlyingCabScoreSaveGame* SaveGame = nullptr;
	if (UGameplayStatics::DoesSaveGameExist(TimeAttackSaveSlot, TimeAttackSaveUserIndex))
	{
		SaveGame = Cast<UFlyingCabScoreSaveGame>(
			UGameplayStatics::LoadGameFromSlot(TimeAttackSaveSlot, TimeAttackSaveUserIndex));
	}
	if (!SaveGame)
	{
		SaveGame = Cast<UFlyingCabScoreSaveGame>(
			UGameplayStatics::CreateSaveGameObject(UFlyingCabScoreSaveGame::StaticClass()));
	}
	if (!SaveGame)
	{
		UE_LOG(LogFlyingCabDelivery, Warning, TEXT("Could not create Time Attack score save."));
		return;
	}

	SaveGame->BestTimeAttackSeconds.Add(ElapsedSeconds);
	SaveGame->BestTimeAttackSeconds.RemoveAll([](float Seconds) { return Seconds <= 0.0f; });
	SaveGame->BestTimeAttackSeconds.Sort();
	if (SaveGame->BestTimeAttackSeconds.Num() > TimeAttackLeaderboardSize)
	{
		SaveGame->BestTimeAttackSeconds.SetNum(TimeAttackLeaderboardSize);
	}
	if (!UGameplayStatics::SaveGameToSlot(
		SaveGame,
		TimeAttackSaveSlot,
		TimeAttackSaveUserIndex))
	{
		UE_LOG(LogFlyingCabDelivery, Warning, TEXT("Could not persist Time Attack leaderboard."));
	}
}

float AFlyingCabGameMode::GetRunElapsedSeconds() const
{
	if (CurrentRunMode != EFlyingCabRunMode::TimeAttack || !GetWorld())
	{
		return 0.0f;
	}
	return FMath::Max(0.0f, GetWorld()->GetTimeSeconds() - RunStartWorldTime);
}
