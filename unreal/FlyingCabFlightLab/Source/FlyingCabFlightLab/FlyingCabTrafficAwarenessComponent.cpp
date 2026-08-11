// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabTrafficAwarenessComponent.h"

#include "FlyingCabPawn.h"
#include "FlyingCabTrafficVehicle.h"

UFlyingCabTrafficAwarenessComponent::UFlyingCabTrafficAwarenessComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UFlyingCabTrafficAwarenessComponent::BeginPlay()
{
	Super::BeginPlay();
	PrimaryComponentTick.TickInterval = FMath::Max(0.05f, RefreshInterval);
}

void UFlyingCabTrafficAwarenessComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	ResetTrafficVehicles();
	OnTrafficAlertChanged.Clear();
	OnNearMissDetected.Clear();
	Super::EndPlay(EndPlayReason);
}

void UFlyingCabTrafficAwarenessComponent::TickComponent(
	float DeltaSeconds,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);
	UpdateTrafficAlert(DeltaSeconds);
}

void UFlyingCabTrafficAwarenessComponent::SetTrackedPawn(AFlyingCabPawn* Pawn)
{
	if (TrackedPawn == Pawn)
	{
		return;
	}

	TrackedPawn = Pawn;
	NearMissMessageRemaining = 0.0f;
	NearMissRewardCredits = 0;
}

void UFlyingCabTrafficAwarenessComponent::ResetTrafficVehicles()
{
	for (AFlyingCabTrafficVehicle* Vehicle : TrafficVehicles)
	{
		if (Vehicle)
		{
			Vehicle->OnNearMiss.RemoveAll(this);
		}
	}
	TrafficVehicles.Reset();
}

void UFlyingCabTrafficAwarenessComponent::RegisterTrafficVehicle(
	AFlyingCabTrafficVehicle* Vehicle)
{
	if (IsValid(Vehicle) && !TrafficVehicles.Contains(Vehicle))
	{
		TrafficVehicles.Add(Vehicle);
		Vehicle->OnNearMiss.AddUObject(
			this,
			&UFlyingCabTrafficAwarenessComponent::HandleNearMiss);
	}
}

void UFlyingCabTrafficAwarenessComponent::InitializeTrafficVehicles(
	const TArray<TObjectPtr<AFlyingCabTrafficVehicle>>& Vehicles)
{
	ResetTrafficVehicles();
	for (AFlyingCabTrafficVehicle* Vehicle : Vehicles)
	{
		RegisterTrafficVehicle(Vehicle);
	}
}

void UFlyingCabTrafficAwarenessComponent::HandleNearMiss(
	AFlyingCabTrafficVehicle* Vehicle,
	AFlyingCabPawn* Pawn)
{
	OnNearMissDetected.Broadcast(Vehicle, Pawn);
}

void UFlyingCabTrafficAwarenessComponent::ShowNearMissReward(int32 RewardCredits)
{
	NearMissRewardCredits = FMath::Max(0, RewardCredits);
	NearMissMessageRemaining = NearMissRewardCredits > 0
		? FMath::Max(0.0f, NearMissMessageDuration)
		: 0.0f;
}

FFlyingCabTrafficThreat UFlyingCabTrafficAwarenessComponent::FindClosestThreat(
	const FVector& PawnLocation,
	const FVector& PawnVelocity,
	TConstArrayView<FFlyingCabTrafficSample> TrafficSamples,
	float WarningLookAhead,
	float WarningVerticalRange)
{
	FFlyingCabTrafficThreat Result;
	const float EffectiveLookAhead = FMath::Max(0.0f, WarningLookAhead);
	const float EffectiveVerticalRange = FMath::Max(0.0f, WarningVerticalRange);
	float ClosestImpactTime = EffectiveLookAhead + 1.0f;

	for (const FFlyingCabTrafficSample& Sample : TrafficSamples)
	{
		const FVector RelativeLocation = Sample.Location - PawnLocation;
		const FVector RelativeVelocity = Sample.Velocity - PawnVelocity;
		if (FMath::Abs(RelativeVelocity.X) <= UE_SMALL_NUMBER)
		{
			continue;
		}

		const float ImpactTime = -RelativeLocation.X / RelativeVelocity.X;
		if (ImpactTime <= 0.0f || ImpactTime > EffectiveLookAhead
			|| ImpactTime >= ClosestImpactTime)
		{
			continue;
		}

		const float PredictedVerticalSeparation = FMath::Abs(
			RelativeLocation.Z + RelativeVelocity.Z * ImpactTime);
		if (PredictedVerticalSeparation > EffectiveVerticalRange)
		{
			continue;
		}

		Result.bFound = true;
		Result.bFromLeft = Sample.Location.X < PawnLocation.X;
		Result.ImpactTime = ImpactTime;
		ClosestImpactTime = ImpactTime;
	}

	return Result;
}

void UFlyingCabTrafficAwarenessComponent::UpdateTrafficAlert(float DeltaSeconds)
{
	NearMissMessageRemaining = FMath::Max(
		0.0f,
		NearMissMessageRemaining - DeltaSeconds);
	if (!IsValid(TrackedPawn) || !TrackedPawn->IsPlayerControlled()
		|| TrackedPawn->IsDestroyed())
	{
		SetTrafficAlert(FText::GetEmpty(), FLinearColor::Transparent);
		return;
	}

	if (NearMissMessageRemaining > 0.0f && NearMissRewardCredits > 0)
	{
		SetTrafficAlert(
			FText::FromString(FString::Printf(
				TEXT("CLEAN NEAR MISS // +%d CR"),
				NearMissRewardCredits)),
			FLinearColor(0.15f, 1.0f, 0.45f));
		return;
	}

	TArray<FFlyingCabTrafficSample, TInlineAllocator<16>> TrafficSamples;
	TrafficSamples.Reserve(TrafficVehicles.Num());
	for (AFlyingCabTrafficVehicle* Vehicle : TrafficVehicles)
	{
		if (IsValid(Vehicle))
		{
			TrafficSamples.Add({Vehicle->GetActorLocation(), Vehicle->GetTrafficVelocity()});
		}
	}

	const FFlyingCabTrafficThreat Threat = FindClosestThreat(
		TrackedPawn->GetActorLocation(),
		TrackedPawn->GetVelocity(),
		TrafficSamples,
		WarningLookAhead,
		WarningVerticalRange);
	if (!Threat.bFound)
	{
		SetTrafficAlert(FText::GetEmpty(), FLinearColor::Transparent);
		return;
	}

	SetTrafficAlert(
		FText::FromString(FString::Printf(
			TEXT("TRAFFIC // %.1f SEC\nINBOUND FROM %s"),
			Threat.ImpactTime,
			Threat.bFromLeft ? TEXT("LEFT") : TEXT("RIGHT"))),
		Threat.ImpactTime <= CriticalWarningTime
			? FLinearColor(1.0f, 0.12f, 0.03f)
			: FLinearColor(1.0f, 0.66f, 0.05f));
}

void UFlyingCabTrafficAwarenessComponent::SetTrafficAlert(
	const FText& Text,
	const FLinearColor& Color)
{
	if (CurrentAlertText.EqualTo(Text) && CurrentAlertColor.Equals(Color))
	{
		return;
	}

	CurrentAlertText = Text;
	CurrentAlertColor = Color;
	OnTrafficAlertChanged.Broadcast(CurrentAlertText, CurrentAlertColor);
}
