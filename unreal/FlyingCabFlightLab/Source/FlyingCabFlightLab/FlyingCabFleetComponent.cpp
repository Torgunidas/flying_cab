// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabFleetComponent.h"

#include "FlyingCabPawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabFleet, Log, All);

UFlyingCabFleetComponent::UFlyingCabFleetComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFlyingCabFleetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		for (TPair<TWeakObjectPtr<AFlyingCabPawn>, FTimerHandle>& Entry : RecoveryTimerHandles)
		{
			World->GetTimerManager().ClearTimer(Entry.Value);
		}
	}
	RecoveryTimerHandles.Empty();

	for (AFlyingCabPawn* Vehicle : TrackedVehicles)
	{
		if (Vehicle)
		{
			Vehicle->OnVehicleDestroyed.RemoveAll(this);
		}
	}
	TrackedVehicles.Empty();
	ActiveVehicle = nullptr;
	PendingActiveRecoveryVehicle = nullptr;
	OnVehicleRecoveryStarted.Clear();
	OnVehicleRecovered.Clear();
	Super::EndPlay(EndPlayReason);
}

void UFlyingCabFleetComponent::RegisterVehicle(AFlyingCabPawn* Pawn)
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
	Pawn->OnVehicleDestroyed.AddUObject(this, &UFlyingCabFleetComponent::HandleVehicleDestroyed);
	UE_LOG(
		LogFlyingCabFleet,
		Verbose,
		TEXT("Vehicle registered for destruction recovery: %s."),
		*Pawn->GetName());
}

void UFlyingCabFleetComponent::SetActiveVehicle(AFlyingCabPawn* Pawn)
{
	if (!Pawn)
	{
		return;
	}
	RegisterVehicle(Pawn);
	ActiveVehicle = Pawn;
}

void UFlyingCabFleetComponent::HandleVehicleDestroyed(AFlyingCabPawn* Pawn)
{
	if (!Pawn)
	{
		return;
	}
	RegisterVehicle(Pawn);

	const APawn* ControlledPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const bool bAffectsActiveRun = ControlledPawn == Pawn || ActiveVehicle == Pawn;
	if (bAffectsActiveRun)
	{
		PendingActiveRecoveryVehicle = Pawn;
	}

	UE_LOG(
		LogFlyingCabFleet,
		Warning,
		TEXT("%s vehicle %s destroyed; scheduling recovery in %.2f seconds."),
		bAffectsActiveRun ? TEXT("Active") : TEXT("Parked"),
		*Pawn->GetName(),
		DestroyedRecoveryDelay);
	OnVehicleRecoveryStarted.Broadcast(
		Pawn,
		bAffectsActiveRun,
		DestroyedRecoveryDelay);
	ScheduleVehicleRecovery(Pawn);
}

void UFlyingCabFleetComponent::ScheduleVehicleRecovery(AFlyingCabPawn* Pawn)
{
	if (!Pawn)
	{
		return;
	}

	const TWeakObjectPtr<AFlyingCabPawn> VehicleKey(Pawn);
	if (FTimerHandle* ExistingHandle = RecoveryTimerHandles.Find(VehicleKey))
	{
		GetWorld()->GetTimerManager().ClearTimer(*ExistingHandle);
	}

	FTimerDelegate RecoveryDelegate;
	RecoveryDelegate.BindUObject(this, &UFlyingCabFleetComponent::RecoverVehicle, Pawn);
	FTimerHandle& RecoveryHandle = RecoveryTimerHandles.FindOrAdd(VehicleKey);
	GetWorld()->GetTimerManager().SetTimer(
		RecoveryHandle,
		RecoveryDelegate,
		DestroyedRecoveryDelay,
		false);
}

void UFlyingCabFleetComponent::RecoverVehicle(AFlyingCabPawn* Pawn)
{
	RecoveryTimerHandles.Remove(TWeakObjectPtr<AFlyingCabPawn>(Pawn));
	const bool bRecoveredActiveVehicle = PendingActiveRecoveryVehicle == Pawn;
	if (Pawn)
	{
		Pawn->RecoverVehicle(RecoveryFuelPercent);
		UE_LOG(
			LogFlyingCabFleet,
			Display,
			TEXT("Vehicle %s recovered after tow."),
			*Pawn->GetName());
	}
	if (bRecoveredActiveVehicle)
	{
		PendingActiveRecoveryVehicle = nullptr;
	}
	OnVehicleRecovered.Broadcast(Pawn, bRecoveredActiveVehicle);
}
