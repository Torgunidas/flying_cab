// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabDispatchComponent.h"

#include "FlyingCabCityData.h"
#include "FlyingCabDeliveryZone.h"
#include "FlyingCabPawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabDispatch, Log, All);

UFlyingCabDispatchComponent::UFlyingCabDispatchComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	for (const FFlyingCabDistrictDefinition& District : FlyingCabCityData::GetDistricts())
	{
		DeliveryStops.Add(District.StopLocation);
		DeliveryStopNames.Emplace(District.DisplayName);
	}
}

void UFlyingCabDispatchComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (DropoffZone)
	{
		DropoffZone->OnCabReady.RemoveAll(this);
	}
	for (FFlyingCabPassengerOfferState& Offer : PassengerOffers)
	{
		if (Offer.Zone)
		{
			Offer.Zone->OnCabReady.RemoveAll(this);
		}
	}
	OnPassengerPickedUp.Clear();
	OnFareCompleted.Clear();
	Super::EndPlay(EndPlayReason);
}

void UFlyingCabDispatchComponent::TickComponent(
	float DeltaSeconds,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);
	if (bMarketActive)
	{
		UpdatePassengerOffers(DeltaSeconds);
	}
	UpdateActiveFare();
}

bool UFlyingCabDispatchComponent::InitializeNetwork()
{
	if (DeliveryStops.Num() < 2)
	{
		UE_LOG(LogFlyingCabDispatch, Error, TEXT("At least two delivery stops are required."));
		return false;
	}
	if (DropoffZone)
	{
		return true;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = GetOwner();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	DropoffZone = GetWorld()->SpawnActor<AFlyingCabDeliveryZone>(
		AFlyingCabDeliveryZone::StaticClass(),
		DeliveryStops[1],
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!DropoffZone)
	{
		UE_LOG(LogFlyingCabDispatch, Error, TEXT("Could not spawn the delivery zone."));
		return false;
	}

	DropoffZone->Configure(
		EFlyingCabDeliveryZoneType::Dropoff,
		ArrivalMaxPlanarSpeed,
		DropoffExitDuration);
	DropoffZone->OnCabReady.AddUObject(this, &UFlyingCabDispatchComponent::HandleZoneReady);
	DropoffZone->SetZoneActive(false);
	PassengerOffers.Reset();
	PassengerSpawnCountdown = 0.0f;
	return true;
}

void UFlyingCabDispatchComponent::StartPassengerMarket(bool bUseFixedSeed)
{
	if (!InitializeNetwork())
	{
		UE_LOG(
			LogFlyingCabDispatch,
			Error,
			TEXT("Passenger market cannot start before the delivery network is ready."));
		return;
	}

	if (bUseFixedSeed)
	{
		DispatchRandom.Initialize(DispatchRandomSeed);
	}
	else
	{
		DispatchRandom.GenerateNewSeed();
	}

	for (FFlyingCabPassengerOfferState& Offer : PassengerOffers)
	{
		if (Offer.Zone)
		{
			Offer.Zone->OnCabReady.RemoveAll(this);
			Offer.Zone->Destroy();
		}
	}
	PassengerOffers.Reset();
	bMarketActive = true;

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
		LogFlyingCabDispatch,
		Display,
		TEXT("Passenger network initialized with %d stops and %d/%d waiting passengers (link %.2f s, exit %.2f s)."),
		DeliveryStops.Num(),
		PassengerOffers.Num(),
		MaxWaitingPassengers,
		PickupLinkDuration,
		DropoffExitDuration);
}

void UFlyingCabDispatchComponent::SetOfferAcceptanceAllowed(bool bAllowed)
{
	bOfferAcceptanceAllowed = bAllowed;
	RefreshOfferAcceptance();
}

void UFlyingCabDispatchComponent::AbortActiveRide()
{
	bPassengerOnBoard = false;
	ActiveFare = 0.0f;
	FareLastDistance = 0.0f;
	CurrentPickupIndex = INDEX_NONE;
	CurrentDropoffIndex = INDEX_NONE;
	if (DropoffZone)
	{
		DropoffZone->SetZoneActive(false);
	}
	RefreshOfferAcceptance();
}

bool UFlyingCabDispatchComponent::IsCurbsideLinkInProgress() const
{
	return PassengerOffers.ContainsByPredicate(
		[](const FFlyingCabPassengerOfferState& Offer)
		{
			return Offer.Zone && Offer.Zone->IsConfirmationInProgress();
		});
}

int32 UFlyingCabDispatchComponent::GetActiveFareCredits() const
{
	return bPassengerOnBoard ? FMath::Max(0, FMath::RoundToInt(ActiveFare)) : 0;
}

FString UFlyingCabDispatchComponent::GetStopName(int32 StopIndex) const
{
	return DeliveryStopNames.IsValidIndex(StopIndex)
		? DeliveryStopNames[StopIndex]
		: FString::Printf(TEXT("STOP %d"), StopIndex + 1);
}

const FFlyingCabPassengerOfferState* UFlyingCabDispatchComponent::FindNearestOffer(
	const FVector& WorldLocation,
	AFlyingCabDeliveryZone*& OutZone) const
{
	OutZone = nullptr;
	const FFlyingCabPassengerOfferState* NearestOffer = nullptr;
	float NearestDistanceSquared = TNumericLimits<float>::Max();
	for (const FFlyingCabPassengerOfferState& Offer : PassengerOffers)
	{
		if (!Offer.Zone || !Offer.Zone->IsZoneActive())
		{
			continue;
		}
		const FVector Delta = Offer.Zone->GetActorLocation() - WorldLocation;
		const float DistanceSquared = FVector2D(Delta.X, Delta.Z).SizeSquared();
		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			NearestOffer = &Offer;
			OutZone = Offer.Zone;
		}
	}
	return NearestOffer;
}

int32 UFlyingCabDispatchComponent::CalculateEstimatedFare(
	const FVector& PickupLocation,
	const FVector& DropoffLocation,
	float InBaseFare,
	float InFarePerMeterTowardTarget)
{
	const FVector Delta = DropoffLocation - PickupLocation;
	const float DirectDistanceMeters = FVector2D(Delta.X, Delta.Z).Size() / 100.0f;
	return FMath::Max(
		0,
		FMath::RoundToInt(
			FMath::Max(0.0f, InBaseFare)
				+ DirectDistanceMeters * FMath::Max(0.0f, InFarePerMeterTowardTarget)));
}

float UFlyingCabDispatchComponent::CalculateUpdatedFare(
	float CurrentFare,
	float PreviousDistanceCm,
	float CurrentDistanceCm,
	float InBaseFare,
	float InFarePerMeterTowardTarget,
	float InFareBacktrackPenaltyRatio)
{
	const float EffectiveBaseFare = FMath::Max(0.0f, InBaseFare);
	if (PreviousDistanceCm <= 0.0f)
	{
		return FMath::Max(EffectiveBaseFare, CurrentFare);
	}

	const float ProgressMeters = (PreviousDistanceCm - CurrentDistanceCm) / 100.0f;
	const float EffectiveRate = FMath::Max(0.0f, InFarePerMeterTowardTarget);
	const float FareDelta = ProgressMeters >= 0.0f
		? ProgressMeters * EffectiveRate
		: ProgressMeters * EffectiveRate * FMath::Clamp(InFareBacktrackPenaltyRatio, 0.0f, 1.0f);
	return FMath::Max(EffectiveBaseFare, CurrentFare + FareDelta);
}

void UFlyingCabDispatchComponent::UpdatePassengerOffers(float DeltaSeconds)
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

void UFlyingCabDispatchComponent::UpdateActiveFare()
{
	if (!bPassengerOnBoard || !TrackedPawn || TrackedPawn->IsDestroyed() || !DropoffZone)
	{
		return;
	}

	const FVector Delta = DropoffZone->GetActorLocation() - TrackedPawn->GetActorLocation();
	const float Distance = FVector2D(Delta.X, Delta.Z).Size();
	if (FareLastDistance <= 0.0f)
	{
		FareLastDistance = Distance;
		return;
	}

	const float ProgressMeters = (FareLastDistance - Distance) / 100.0f;
	if (!FMath::IsNearlyZero(ProgressMeters, 0.001f))
	{
		ActiveFare = CalculateUpdatedFare(
			ActiveFare,
			FareLastDistance,
			Distance,
			BaseFare,
			FarePerMeterTowardTarget,
			FareBacktrackPenaltyRatio);
		FareLastDistance = Distance;
	}
}

void UFlyingCabDispatchComponent::SpawnPassengerOffer()
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
	SpawnParameters.Owner = GetOwner();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AFlyingCabDeliveryZone* Zone = GetWorld()->SpawnActor<AFlyingCabDeliveryZone>(
		AFlyingCabDeliveryZone::StaticClass(),
		DeliveryStops[PickupIndex],
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!Zone)
	{
		UE_LOG(LogFlyingCabDispatch, Warning, TEXT("Could not spawn passenger offer."));
		return;
	}

	const FString DestinationName = GetStopName(DropoffIndex);
	const int32 EstimatedFare = CalculateEstimatedFare(PickupIndex, DropoffIndex);
	const float Lifetime = DispatchRandom.FRandRange(
		FMath::Min(PassengerLifetimeMin, PassengerLifetimeMax),
		FMath::Max(PassengerLifetimeMin, PassengerLifetimeMax));
	Zone->Configure(EFlyingCabDeliveryZoneType::Pickup, ArrivalMaxPlanarSpeed, PickupLinkDuration);
	Zone->ConfigurePassengerOffer(DestinationName, EstimatedFare);
	Zone->SetOfferRemainingSeconds(Lifetime);
	Zone->OnCabReady.AddUObject(this, &UFlyingCabDispatchComponent::HandleZoneReady);
	Zone->SetZoneActive(true);

	FFlyingCabPassengerOfferState& Offer = PassengerOffers.AddDefaulted_GetRef();
	Offer.Zone = Zone;
	Offer.PickupIndex = PickupIndex;
	Offer.DropoffIndex = DropoffIndex;
	Offer.EstimatedFareCredits = EstimatedFare;
	Offer.RemainingSeconds = Lifetime;
	RefreshOfferAcceptance();

	UE_LOG(
		LogFlyingCabDispatch,
		Display,
		TEXT("Passenger appeared at %s for %s (~%d credits, %.1f seconds)."),
		*GetStopName(PickupIndex),
		*DestinationName,
		EstimatedFare,
		Lifetime);
}

void UFlyingCabDispatchComponent::RemovePassengerOfferAt(
	int32 OfferIndex,
	const TCHAR* Reason)
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
		LogFlyingCabDispatch,
		Display,
		TEXT("Passenger offer at stop %d removed (%s); %d still waiting."),
		Offer.PickupIndex,
		Reason ? Reason : TEXT("unknown"),
		PassengerOffers.Num());
}

void UFlyingCabDispatchComponent::RefreshOfferAcceptance()
{
	const bool bEnabled = bOfferAcceptanceAllowed && !bPassengerOnBoard;
	for (FFlyingCabPassengerOfferState& Offer : PassengerOffers)
	{
		if (Offer.Zone)
		{
			Offer.Zone->SetAcceptanceEnabled(bEnabled);
		}
	}
}

int32 UFlyingCabDispatchComponent::FindPassengerOfferIndex(
	const AFlyingCabDeliveryZone* Zone) const
{
	return PassengerOffers.IndexOfByPredicate(
		[Zone](const FFlyingCabPassengerOfferState& Offer)
		{
			return Offer.Zone == Zone;
		});
}

int32 UFlyingCabDispatchComponent::CalculateEstimatedFare(
	int32 PickupIndex,
	int32 DropoffIndex) const
{
	if (!DeliveryStops.IsValidIndex(PickupIndex) || !DeliveryStops.IsValidIndex(DropoffIndex))
	{
		return FMath::RoundToInt(BaseFare);
	}
	return CalculateEstimatedFare(
		DeliveryStops[PickupIndex],
		DeliveryStops[DropoffIndex],
		BaseFare,
		FarePerMeterTowardTarget);
}

void UFlyingCabDispatchComponent::HandleZoneReady(AFlyingCabDeliveryZone* Zone)
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
		if (TrackedPawn)
		{
			const FVector FareDelta =
				DropoffZone->GetActorLocation() - TrackedPawn->GetActorLocation();
			FareLastDistance = FVector2D(FareDelta.X, FareDelta.Z).Size();
		}
		RemovePassengerOfferAt(OfferIndex, TEXT("boarded"));
		RefreshOfferAcceptance();
		DropoffZone->SetZoneActive(true);
		const FString DestinationName = GetStopName(CurrentDropoffIndex);
		UE_LOG(
			LogFlyingCabDispatch,
			Display,
			TEXT("Passenger picked up for %s; one-seat capacity is now occupied."),
			*DestinationName);
		OnPassengerPickedUp.Broadcast(DestinationName);
		return;
	}

	if (Zone->GetZoneType() == EFlyingCabDeliveryZoneType::Dropoff && bPassengerOnBoard)
	{
		const int32 FarePayout = GetActiveFareCredits();
		bPassengerOnBoard = false;
		++CompletedDeliveries;
		ActiveFare = 0.0f;
		FareLastDistance = 0.0f;
		CurrentPickupIndex = INDEX_NONE;
		CurrentDropoffIndex = INDEX_NONE;
		DropoffZone->SetZoneActive(false);
		RefreshOfferAcceptance();
		OnFareCompleted.Broadcast(FarePayout, CompletedDeliveries);
	}
}
