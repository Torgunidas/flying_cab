// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabQuestTypes.h"

namespace FlyingCabQuestEvents
{
	const FName VehicleEntered(TEXT("Vehicle.Entered"));
	const FName VehicleExited(TEXT("Vehicle.Exited"));
	const FName PassengerPickedUp(TEXT("Passenger.PickedUp"));
	const FName PassengerDelivered(TEXT("Passenger.Delivered"));
	const FName CreditsEarned(TEXT("Economy.CreditsEarned"));
	const FName FuelPurchased(TEXT("Service.FuelPurchased"));
	const FName RepairPurchased(TEXT("Service.RepairPurchased"));
	const FName NearMiss(TEXT("Traffic.NearMiss"));
	const FName InteractionCompleted(TEXT("Interaction.Completed"));
	const FName QuestGiverInteracted(TEXT("QuestGiver.Interacted"));
	const FName AccessGranted(TEXT("Progression.AccessGranted"));

	bool IsKnownEvent(FName EventId)
	{
		return EventId == VehicleEntered
			|| EventId == VehicleExited
			|| EventId == PassengerPickedUp
			|| EventId == PassengerDelivered
			|| EventId == CreditsEarned
			|| EventId == FuelPurchased
			|| EventId == RepairPurchased
			|| EventId == NearMiss
			|| EventId == InteractionCompleted
			|| EventId == QuestGiverInteracted
			|| EventId == AccessGranted;
	}
}
