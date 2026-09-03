// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabLivingWorldManager.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "FlyingCabLivingPedestrian.h"
#include "FlyingCabLivingRoute.h"
#include "FlyingCabLivingWorldProfile.h"
#include "FlyingCabTrafficVehicle.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabLivingWorld, Log, All);

AFlyingCabLivingWorldManager::AFlyingCabLivingWorldManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AFlyingCabLivingWorldManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (AFlyingCabTrafficVehicle* Vehicle : TrafficVehicles)
	{
		if (IsValid(Vehicle))
		{
			Vehicle->OnLivingStopReached.RemoveAll(this);
		}
	}
	for (AFlyingCabLivingPedestrian* Pedestrian : Pedestrians)
	{
		if (IsValid(Pedestrian))
		{
			Pedestrian->OnWaitingForVehicle.RemoveAll(this);
		}
	}
	WaitingPedestrians.Reset();
	Super::EndPlay(EndPlayReason);
}

bool AFlyingCabLivingWorldManager::Initialize(UFlyingCabLivingWorldProfile* InProfile)
{
	if (bInitialized)
	{
		return !Routes.IsEmpty();
	}
	bInitialized = true;
	if (!DiscoverOrGenerateRoutes(InProfile) || !SpawnPopulation())
	{
		UE_LOG(LogFlyingCabLivingWorld, Error, TEXT("Living-world prototype failed to initialize."));
		return false;
	}

	UE_LOG(
		LogFlyingCabLivingWorld,
		Display,
		TEXT("Living world initialized with %d routes, %d vehicles and %d pedestrians."),
		Routes.Num(),
		TrafficVehicles.Num(),
		Pedestrians.Num());
	return true;
}

int32 AFlyingCabLivingWorldManager::GetPrototypeVehicleCount()
{
	const TArray<FFlyingCabLivingRouteDefinition> Definitions =
		UFlyingCabLivingWorldProfile::BuildPrototypeRoutes();
	return UFlyingCabLivingWorldProfile::CountAgents(
		Definitions,
		EFlyingCabLivingAgentKind::Vehicle);
}

int32 AFlyingCabLivingWorldManager::GetPrototypePedestrianCount()
{
	const TArray<FFlyingCabLivingRouteDefinition> Definitions =
		UFlyingCabLivingWorldProfile::BuildPrototypeRoutes();
	return UFlyingCabLivingWorldProfile::CountAgents(
		Definitions,
		EFlyingCabLivingAgentKind::Pedestrian);
}

bool AFlyingCabLivingWorldManager::DiscoverOrGenerateRoutes(UFlyingCabLivingWorldProfile* InProfile)
{
	Routes.Reset();
	for (TActorIterator<AFlyingCabLivingRoute> It(GetWorld()); It; ++It)
	{
		AFlyingCabLivingRoute* Route = *It;
		FString ValidationError;
		if (IsValid(Route) && Route->IsRouteValid(ValidationError))
		{
			Routes.Add(Route);
		}
		else if (IsValid(Route))
		{
			UE_LOG(
				LogFlyingCabLivingWorld,
				Warning,
				TEXT("Ignoring invalid authored route %s: %s"),
				*Route->GetName(),
				*ValidationError);
		}
	}
	if (!Routes.IsEmpty())
	{
		UE_LOG(
			LogFlyingCabLivingWorld,
			Display,
			TEXT("Using %d living-world routes authored directly in the level."),
			Routes.Num());
		return true;
	}

	if (InProfile)
	{
		FString ValidationError;
		if (!InProfile->IsConfigurationValid(ValidationError))
		{
			UE_LOG(
				LogFlyingCabLivingWorld,
				Error,
				TEXT("Living-world profile is invalid: %s"),
				*ValidationError);
			return false;
		}
		return GenerateRoutes(InProfile->Routes);
	}

	const TArray<FFlyingCabLivingRouteDefinition> PrototypeDefinitions =
		UFlyingCabLivingWorldProfile::BuildPrototypeRoutes();
	return GenerateRoutes(PrototypeDefinitions);
}

bool AFlyingCabLivingWorldManager::GenerateRoutes(
	TConstArrayView<FFlyingCabLivingRouteDefinition> Definitions)
{
	for (const FFlyingCabLivingRouteDefinition& Definition : Definitions)
	{
		FString ValidationError;
		if (!Definition.IsValid(ValidationError))
		{
			UE_LOG(LogFlyingCabLivingWorld, Error, TEXT("%s"), *ValidationError);
			return false;
		}
		AFlyingCabLivingRoute* Route = GetWorld()->SpawnActor<AFlyingCabLivingRoute>(
			AFlyingCabLivingRoute::StaticClass(),
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			MakeSpawnParameters(this));
		if (!Route)
		{
			return false;
		}
		Route->Configure(Definition);
		Routes.Add(Route);
	}
	return !Routes.IsEmpty();
}

bool AFlyingCabLivingWorldManager::SpawnPopulation()
{
	TrafficVehicles.Reset();
	Pedestrians.Reset();
	const FLinearColor PedestrianColors[] = {
		FLinearColor(1.0f, 0.78f, 0.12f),
		FLinearColor(0.38f, 1.0f, 0.62f),
		FLinearColor(0.86f, 0.28f, 1.0f)};

	for (AFlyingCabLivingRoute* Route : Routes)
	{
		if (!IsValid(Route))
		{
			continue;
		}
		for (int32 AgentIndex = 0; AgentIndex < Route->GetSpawnCount(); ++AgentIndex)
		{
			float EvenAlpha = Route->GetSpawnCount() > 0
				? static_cast<float>(AgentIndex) / static_cast<float>(Route->GetSpawnCount())
				: 0.0f;
			if (Route->GetAgentKind() == EFlyingCabLivingAgentKind::Pedestrian)
			{
				int32 ExitNodeIndex = 0;
				int32 SearchStart = 0;
				for (int32 ExitIndex = 0; ExitIndex <= AgentIndex; ++ExitIndex)
				{
					ExitNodeIndex = Route->FindNextNodeWithAction(
						SearchStart,
						EFlyingCabLivingRouteAction::ExitBuilding);
					if (ExitNodeIndex == INDEX_NONE)
					{
						break;
					}
					SearchStart = Route->FindNextNodeIndex(ExitNodeIndex);
				}
				if (ExitNodeIndex != INDEX_NONE && Route->GetRouteLength() > UE_SMALL_NUMBER)
				{
					EvenAlpha = Route->GetNodeDistance(ExitNodeIndex) / Route->GetRouteLength();
				}
			}
			if (Route->GetAgentKind() == EFlyingCabLivingAgentKind::Vehicle)
			{
				const float InitialAlpha = Route->GetRouteClass() == EFlyingCabLivingRouteClass::LandingApproach
					&& AgentIndex == 0
					? 0.0f
					: FMath::Fmod(EvenAlpha + 0.08f, 1.0f);
				AFlyingCabTrafficVehicle* Vehicle = GetWorld()->SpawnActor<AFlyingCabTrafficVehicle>(
					AFlyingCabTrafficVehicle::StaticClass(),
					Route->GetWorldLocationAtDistance(Route->GetRouteLength() * InitialAlpha),
					FRotator::ZeroRotator,
					MakeSpawnParameters(this));
				if (!Vehicle)
				{
					return false;
				}
				const TArray<FLinearColor>& Colors = Route->GetVehicleColors();
				const FLinearColor Color = Colors.IsEmpty()
					? FLinearColor(0.08f, 0.80f, 1.0f)
					: Colors[AgentIndex % Colors.Num()];
				Vehicle->ConfigureLivingRoute(Route, InitialAlpha, Color);
				Vehicle->OnLivingStopReached.AddUObject(
					this,
					&AFlyingCabLivingWorldManager::HandleVehicleStop);
				TrafficVehicles.Add(Vehicle);
			}
			else
			{
				AFlyingCabLivingPedestrian* Pedestrian =
					GetWorld()->SpawnActor<AFlyingCabLivingPedestrian>(
						AFlyingCabLivingPedestrian::StaticClass(),
						Route->GetWorldLocationAtDistance(Route->GetRouteLength() * EvenAlpha),
						FRotator::ZeroRotator,
						MakeSpawnParameters(this));
				if (!Pedestrian)
				{
					return false;
				}
				Pedestrian->Configure(
					Route,
					EvenAlpha,
					PedestrianColors[AgentIndex % UE_ARRAY_COUNT(PedestrianColors)]);
				Pedestrian->OnWaitingForVehicle.AddUObject(
					this,
					&AFlyingCabLivingWorldManager::RegisterPedestrianWaiting);
				Pedestrians.Add(Pedestrian);
			}
		}
	}
	return true;
}

void AFlyingCabLivingWorldManager::RegisterPedestrianWaiting(
	AFlyingCabLivingPedestrian* Pedestrian,
	FName StopId)
{
	if (!IsValid(Pedestrian) || StopId.IsNone())
	{
		return;
	}
	for (AFlyingCabTrafficVehicle* Vehicle : TrafficVehicles)
	{
		if (!IsValid(Vehicle)
			|| Vehicle->GetMovementState() != EFlyingCabTrafficMovementState::Dwelling
			|| Vehicle->GetCurrentLivingStopId() != StopId)
		{
			continue;
		}
		int32 Occupancy = 0;
		for (const AFlyingCabLivingPedestrian* Candidate : Pedestrians)
		{
			Occupancy += IsValid(Candidate) && Candidate->GetRidingVehicle() == Vehicle ? 1 : 0;
		}
		if (Occupancy < 2 && Pedestrian->BoardVehicle(Vehicle))
		{
			++TotalBoardings;
			return;
		}
	}
	TArray<TWeakObjectPtr<AFlyingCabLivingPedestrian>>& AtStop =
		WaitingPedestrians.FindOrAdd(StopId);
	if (!AtStop.Contains(Pedestrian))
	{
		AtStop.Add(Pedestrian);
	}
}

void AFlyingCabLivingWorldManager::HandleVehicleStop(
	AFlyingCabTrafficVehicle* Vehicle,
	FName StopId,
	EFlyingCabLivingRouteAction Action)
{
	if (!IsValid(Vehicle) || StopId.IsNone())
	{
		return;
	}

	int32 ExitedCount = 0;
	for (AFlyingCabLivingPedestrian* Pedestrian : Pedestrians)
	{
		if (IsValid(Pedestrian)
			&& Pedestrian->GetRidingVehicle() == Vehicle
			&& Pedestrian->WantsToExitAt(StopId)
			&& Pedestrian->CompleteRideAtStop(StopId))
		{
			++ExitedCount;
			++TotalPassengerExits;
		}
	}

	int32 BoardedCount = 0;
	if (TArray<TWeakObjectPtr<AFlyingCabLivingPedestrian>>* AtStop =
		WaitingPedestrians.Find(StopId))
	{
		for (int32 Index = AtStop->Num() - 1; Index >= 0; --Index)
		{
			AFlyingCabLivingPedestrian* Pedestrian = (*AtStop)[Index].Get();
			if (!Pedestrian)
			{
				AtStop->RemoveAtSwap(Index);
				continue;
			}
			if (BoardedCount < 2 && Pedestrian->BoardVehicle(Vehicle))
			{
					AtStop->RemoveAtSwap(Index);
					++BoardedCount;
					++TotalBoardings;
			}
		}
		if (AtStop->IsEmpty())
		{
			WaitingPedestrians.Remove(StopId);
		}
	}

	UE_LOG(
		LogFlyingCabLivingWorld,
		Verbose,
		TEXT("Vehicle %s served %s: %d exited, %d boarded."),
		*Vehicle->GetName(),
		*StopId.ToString(),
		ExitedCount,
		BoardedCount);
}

FActorSpawnParameters AFlyingCabLivingWorldManager::MakeSpawnParameters(AActor* SpawnOwner) const
{
	FActorSpawnParameters Parameters;
	Parameters.Owner = SpawnOwner;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return Parameters;
}
