// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabHudPresenterComponent.h"

#include "FlyingCabCharacter.h"
#include "FlyingCabDeliveryZone.h"
#include "FlyingCabDispatchComponent.h"
#include "FlyingCabPawn.h"
#include "FlyingCabPlayerController.h"
#include "FlyingCabRunComponent.h"
#include "Kismet/GameplayStatics.h"

UFlyingCabHudPresenterComponent::UFlyingCabHudPresenterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFlyingCabHudPresenterComponent::InitializePresenter(
	UFlyingCabDispatchComponent* InDispatch,
	UFlyingCabRunComponent* InRun)
{
	Dispatch = InDispatch;
	Run = InRun;
	HudRefreshElapsed = 0.0f;
}

void UFlyingCabHudPresenterComponent::Refresh(
	float DeltaSeconds,
	AFlyingCabPawn* Pawn,
	int32 Credits,
	bool bForce)
{
	UpdateProximityGuidance(Pawn);
	HudRefreshElapsed += FMath::Max(0.0f, DeltaSeconds);
	const float EffectiveHudRefreshInterval = FMath::Max(0.05f, HudRefreshInterval);
	if (!bForce && HudRefreshElapsed < EffectiveHudRefreshInterval)
	{
		return;
	}

	HudRefreshElapsed = bForce
		? 0.0f
		: FMath::Fmod(HudRefreshElapsed, EffectiveHudRefreshInterval);
	UpdateObjectiveStatus(Pawn, Credits);
	UpdateRunModeStatus(Credits);
}

void UFlyingCabHudPresenterComponent::PushEconomyStatus(int32 Credits) const
{
	if (AFlyingCabPlayerController* PlayerController = GetPlayerController())
	{
		PlayerController->SetEconomyStatus(
			Credits,
			Dispatch ? Dispatch->GetActiveFareCredits() : 0);
	}
}

void UFlyingCabHudPresenterComponent::UpdateRunModeStatus(int32 Credits) const
{
	AFlyingCabPlayerController* PlayerController = GetPlayerController();
	if (!PlayerController || !Run)
	{
		return;
	}

	const bool bShowTimeAttack = Run->GetCurrentRunMode() == EFlyingCabRunMode::TimeAttack
		&& (Run->IsRunActive() || Run->IsRunCompleted());
	PlayerController->SetTimeAttackStatus(
		bShowTimeAttack,
		Run->GetElapsedSeconds(),
		Credits,
		Run->GetTimeAttackTargetCredits());
}

void UFlyingCabHudPresenterComponent::ShowEventMessage(
	const FText& Message,
	const FLinearColor& Color,
	float DurationSeconds) const
{
	if (const AFlyingCabPlayerController* PlayerController = GetPlayerController())
	{
		PlayerController->ShowEventMessage(Message, Color, DurationSeconds);
	}
}

void UFlyingCabHudPresenterComponent::SetTrafficAlert(
	const FText& Alert,
	const FLinearColor& Color) const
{
	if (AFlyingCabPlayerController* PlayerController = GetPlayerController())
	{
		PlayerController->SetTrafficAlert(Alert, Color);
	}
}

void UFlyingCabHudPresenterComponent::ClearMinimapTarget() const
{
	if (AFlyingCabPlayerController* PlayerController = GetPlayerController())
	{
		PlayerController->ClearMinimapTarget();
	}
}

AFlyingCabPlayerController* UFlyingCabHudPresenterComponent::GetPlayerController() const
{
	return Cast<AFlyingCabPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
}

bool UFlyingCabHudPresenterComponent::IsPlayerOnFoot() const
{
	const AFlyingCabPlayerController* PlayerController = GetPlayerController();
	return PlayerController
		&& PlayerController->GetPlayerMode() == EFlyingCabPlayerMode::OnFoot;
}

void UFlyingCabHudPresenterComponent::UpdateProximityGuidance(AFlyingCabPawn* Pawn) const
{
	if (!Pawn || IsPlayerOnFoot() || Pawn->IsDestroyed())
	{
		if (Pawn)
		{
			Pawn->SetProximityGuidance(false, FVector2D::ZeroVector, false);
		}
		return;
	}

	const bool bHasPassenger = Dispatch && Dispatch->HasPassengerOnBoard();
	AFlyingCabDeliveryZone* ActiveZone = bHasPassenger ? Dispatch->GetDropoffZone() : nullptr;
	if (!ActiveZone && Dispatch)
	{
		Dispatch->FindNearestOffer(Pawn->GetActorLocation(), ActiveZone);
	}

	if (!ActiveZone || !ActiveZone->IsZoneActive())
	{
		Pawn->SetProximityGuidance(false, FVector2D::ZeroVector, false);
		return;
	}

	const FVector ZoneLocation = ActiveZone->GetActorLocation();
	const FVector Delta = ZoneLocation - Pawn->GetActorLocation();
	const float Distance = FVector2D(Delta.X, Delta.Z).Size();
	Pawn->SetProximityGuidance(
		Distance <= ProximityGuidanceRange,
		FVector2D(ZoneLocation.X, ZoneLocation.Z),
		bHasPassenger);
}

void UFlyingCabHudPresenterComponent::UpdateObjectiveStatus(
	AFlyingCabPawn* Pawn,
	int32 Credits) const
{
	AFlyingCabPlayerController* PlayerController = GetPlayerController();
	if (!Pawn)
	{
		return;
	}
	const bool bHasPassenger = Dispatch && Dispatch->HasPassengerOnBoard();
	const int32 CompletedDeliveries = Dispatch ? Dispatch->GetCompletedDeliveries() : 0;

	TArray<FVector2D> OfferPositions;
	AFlyingCabDeliveryZone* NearestOfferZone = nullptr;
	const FFlyingCabPassengerOfferState* NearestOffer = nullptr;
	float NearestOfferDistanceSquared = TNumericLimits<float>::Max();
	if (Dispatch)
	{
		for (const FFlyingCabPassengerOfferState& Offer : Dispatch->GetPassengerOffers())
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
	}
	const FVector2D CabMapPosition(Pawn->GetActorLocation().X, Pawn->GetActorLocation().Z);
	if (PlayerController)
	{
		PlayerController->SetPassengerOfferMarkers(CabMapPosition, OfferPositions);
	}
	AFlyingCabDeliveryZone* ActiveZone = bHasPassenger && Dispatch
		? Dispatch->GetDropoffZone()
		: NearestOfferZone;

	PushEconomyStatus(Credits);
	if (IsPlayerOnFoot())
	{
		const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
		const AFlyingCabCharacter* OnFootCharacter = Cast<AFlyingCabCharacter>(PlayerPawn);
		const FVector PlayerLocation = PlayerPawn
			? PlayerPawn->GetActorLocation()
			: Pawn->GetActorLocation();
		const FVector CabDelta = Pawn->GetActorLocation() - PlayerLocation;
		const float CabDistanceMeters = FVector2D(CabDelta.X, CabDelta.Z).Size() / 100.0f;
		if (PlayerController)
		{
			PlayerController->SetTrafficAlert(FText::GetEmpty(), FLinearColor::Transparent);
		}
		if (OnFootCharacter && OnFootCharacter->IsDead())
		{
			if (PlayerController)
			{
				PlayerController->SetObjectiveStatus(FText::FromString(
					TEXT("DRIVER DOWN\nRELOADING LEVEL")));
			}
		}
		else
		{
			const float HealthPercent = OnFootCharacter
				? OnFootCharacter->GetHealthPercent() * 100.0f
				: 0.0f;
			const FString ContextPrompt = PlayerController
				? PlayerController->GetContextPrompt().ToString()
				: FString(TEXT("EXPLORE ON FOOT"));
			if (PlayerController)
			{
				PlayerController->SetObjectiveStatus(FText::FromString(FString::Printf(
					TEXT("ON FOOT // HEALTH %.0f%% // CAB %.1f M\n%s"),
					HealthPercent,
					CabDistanceMeters,
					*ContextPrompt)));
			}
		}
		if (PlayerController && bHasPassenger && ActiveZone && ActiveZone->IsZoneActive())
		{
			PlayerController->SetMinimapState(
				CabMapPosition,
				FVector2D(ActiveZone->GetActorLocation().X, ActiveZone->GetActorLocation().Z),
				true);
		}
		else if (PlayerController)
		{
			PlayerController->ClearMinimapTarget();
		}
		return;
	}
	if (Pawn->IsDestroyed())
	{
		if (PlayerController)
		{
			PlayerController->SetObjectiveStatus(FText::FromString(
				TEXT("CAB DESTROYED\nRECOVERY CREW INBOUND")));
		}
		return;
	}
	if (!ActiveZone || !ActiveZone->IsZoneActive())
	{
		if (PlayerController)
		{
			PlayerController->SetObjectiveStatus(FText::FromString(
				TEXT("NO CURBSIDE CALLS\nPASSENGER NETWORK SEARCHING")));
			PlayerController->ClearMinimapTarget();
		}
		return;
	}

	const FVector Delta = ActiveZone->GetActorLocation() - Pawn->GetActorLocation();
	const float Distance = FVector2D(Delta.X, Delta.Z).Size();
	const float DistanceMeters = Distance / 100.0f;
	const FVector Velocity = Pawn->GetVelocity();
	const float PlanarSpeed = FVector2D(Velocity.X, Velocity.Z).Size();
	const bool bInsideTooFast = ActiveZone->IsPawnInside(Pawn)
		&& PlanarSpeed > ActiveZone->GetArrivalMaxPlanarSpeed();
	const int32 TargetIndex = bHasPassenger && Dispatch
		? Dispatch->GetCurrentDropoffIndex()
		: (NearestOffer ? NearestOffer->PickupIndex : INDEX_NONE);
	const FString TargetName = Dispatch
		? Dispatch->GetStopName(TargetIndex)
		: FString::Printf(TEXT("STOP %d"), TargetIndex + 1);
	const FString DestinationName = !bHasPassenger && NearestOffer && Dispatch
		? Dispatch->GetStopName(NearestOffer->DropoffIndex)
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
	else if (bHasPassenger)
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
			Dispatch ? Dispatch->GetOfferCount() : 0);
	}
	if (PlayerController)
	{
		PlayerController->SetObjectiveStatus(FText::FromString(Status));
		if (bHasPassenger)
		{
			PlayerController->SetMinimapState(
				CabMapPosition,
				FVector2D(ActiveZone->GetActorLocation().X, ActiveZone->GetActorLocation().Z),
				true);
		}
		else
		{
			PlayerController->ClearMinimapTarget();
		}
	}
}
