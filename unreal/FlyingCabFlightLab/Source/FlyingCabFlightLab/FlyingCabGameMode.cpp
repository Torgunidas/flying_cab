// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabGameMode.h"

#include "Engine/GameInstance.h"
#include "FlyingCabDispatchComponent.h"
#include "FlyingCabHudPresenterComponent.h"
#include "FlyingCabPawn.h"
#include "FlyingCabPlayerController.h"
#include "FlyingCabProgressionSubsystem.h"
#include "FlyingCabRunComponent.h"
#include "FlyingCabTrafficAwarenessComponent.h"
#include "FlyingCabTrafficVehicle.h"
#include "FlyingCabWorldBootstrap.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabDelivery, Log, All);

AFlyingCabGameMode::AFlyingCabGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	Dispatch = CreateDefaultSubobject<UFlyingCabDispatchComponent>(TEXT("Dispatch"));
	Run = CreateDefaultSubobject<UFlyingCabRunComponent>(TEXT("Run"));
	HudPresenter = CreateDefaultSubobject<UFlyingCabHudPresenterComponent>(TEXT("HudPresenter"));
	TrafficAwareness = CreateDefaultSubobject<UFlyingCabTrafficAwarenessComponent>(
		TEXT("TrafficAwareness"));

	static ConstructorHelpers::FClassFinder<AFlyingCabPawn> TunablePawnClass(
		TEXT("/Game/Blueprints/BP_FlyingCabPawn"));

	DefaultPawnClass = AFlyingCabPawn::StaticClass();
	PlayerControllerClass = AFlyingCabPlayerController::StaticClass();
	if (TunablePawnClass.Succeeded())
	{
		DefaultPawnClass = TunablePawnClass.Class;
	}
}

void AFlyingCabGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (TrafficAwareness)
	{
		TrafficAwareness->OnTrafficAlertChanged.AddUObject(
			this,
			&AFlyingCabGameMode::HandleTrafficAlertChanged);
	}
	if (Dispatch)
	{
		Dispatch->OnPassengerPickedUp.AddUObject(
			this,
			&AFlyingCabGameMode::HandlePassengerPickedUp);
		Dispatch->OnFareCompleted.AddUObject(
			this,
			&AFlyingCabGameMode::HandleFareCompleted);
	}
	if (Run)
	{
		Run->OnTimeAttackCompleted.AddUObject(
			this,
			&AFlyingCabGameMode::HandleTimeAttackCompleted);
	}
	Credits = FMath::Max(0, StartingCredits);
	if (HudPresenter)
	{
		HudPresenter->InitializePresenter(Dispatch, Run);
	}
	InitializeWorldBootstrap();
	InitializeDispatch();
	EnsurePawnBinding();
}

void AFlyingCabGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (TrafficAwareness)
	{
		TrafficAwareness->OnTrafficAlertChanged.RemoveAll(this);
	}
	if (Dispatch)
	{
		Dispatch->OnPassengerPickedUp.RemoveAll(this);
		Dispatch->OnFareCompleted.RemoveAll(this);
	}
	if (Run)
	{
		Run->OnTimeAttackCompleted.RemoveAll(this);
	}
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
	if (!Run || !Run->StartRun(Mode))
	{
		return;
	}

	Credits = FMath::Max(0, StartingCredits);
	if (Dispatch)
	{
		Dispatch->StartPassengerMarket(Mode == EFlyingCabRunMode::TimeAttack);
	}

	if (Mode == EFlyingCabRunMode::TimeAttack)
	{
		// Competitive runs always begin without session-granted vehicle access.
		// Freeroam intentionally keeps access across map reloads for this app session.
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UFlyingCabProgressionSubsystem* Progression =
				GameInstance->GetSubsystem<UFlyingCabProgressionSubsystem>())
			{
				Progression->ResetAccess();
			}
		}
		if (WorldBootstrap)
		{
			WorldBootstrap->RefreshServiceAccess();
		}
	}

	EnsurePawnBinding();
	PushEconomyStatus();
	if (HudPresenter)
	{
		HudPresenter->UpdateRunModeStatus(Credits);
	}
	UE_LOG(
		LogFlyingCabDelivery,
		Display,
		TEXT("Run started in %s mode with %d credits%s."),
		Mode == EFlyingCabRunMode::TimeAttack ? TEXT("Time Attack") : TEXT("Freeroam"),
		Credits,
		Mode == EFlyingCabRunMode::TimeAttack
			? *FString::Printf(TEXT("; target %d"), Run->GetTimeAttackTargetCredits())
			: TEXT(""));
	Run->CheckTimeAttackGoal(Credits);
}

TArray<float> AFlyingCabGameMode::GetBestTimeAttackTimes() const
{
	return Run ? Run->GetBestTimeAttackTimes() : TArray<float>();
}

EFlyingCabRunMode AFlyingCabGameMode::GetCurrentRunMode() const
{
	return Run ? Run->GetCurrentRunMode() : EFlyingCabRunMode::None;
}

int32 AFlyingCabGameMode::GetTimeAttackTargetCredits() const
{
	return Run ? Run->GetTimeAttackTargetCredits() : 1000;
}

void AFlyingCabGameMode::InitializeWorldBootstrap()
{
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	WorldBootstrap = GetWorld()->SpawnActor<AFlyingCabWorldBootstrap>(
		AFlyingCabWorldBootstrap::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!WorldBootstrap)
	{
		UE_LOG(LogFlyingCabDelivery, Error, TEXT("Could not create the world bootstrap actor."));
		return;
	}

	WorldBootstrap->Bootstrap(DefaultPawnClass);
	if (AFlyingCabPawn* ServiceVehicle = WorldBootstrap->GetServiceVehicle())
	{
		RegisterVehicle(ServiceVehicle);
	}

	if (TrafficAwareness)
	{
		TrafficAwareness->ResetTrafficVehicles();
	}
	for (AFlyingCabTrafficVehicle* Vehicle : WorldBootstrap->GetTrafficVehicles())
	{
		if (!Vehicle)
		{
			continue;
		}
		Vehicle->OnNearMiss.AddUObject(this, &AFlyingCabGameMode::HandleTrafficNearMiss);
		if (TrafficAwareness)
		{
			TrafficAwareness->RegisterTrafficVehicle(Vehicle);
		}
	}

	UE_LOG(
		LogFlyingCabDelivery,
		Display,
		TEXT("Traffic initialized with %d/%d vehicles; clean near misses award %d credits."),
		WorldBootstrap->GetTrafficVehicles().Num(),
		AFlyingCabWorldBootstrap::GetExpectedTrafficVehicleCount(),
		NearMissRewardCredits);
}

void AFlyingCabGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	EnsurePawnBinding();
	if (HudPresenter)
	{
		HudPresenter->Refresh(DeltaSeconds, BoundPawn, Credits);
	}
	if (Run)
	{
		Run->CheckTimeAttackGoal(Credits);
	}
}

void AFlyingCabGameMode::InitializeDispatch()
{
	if (!Dispatch || !Dispatch->InitializeNetwork())
	{
		UE_LOG(LogFlyingCabDelivery, Error, TEXT("Could not initialize passenger dispatch."));
		return;
	}

	UE_LOG(
		LogFlyingCabDelivery,
		Display,
		TEXT("Delivery network initialized with %d stops, %d fuel stations and %d repair shops."),
		Dispatch->GetStopCount(),
		WorldBootstrap ? WorldBootstrap->GetFuelStationCount() : 0,
		WorldBootstrap ? WorldBootstrap->GetRepairStationCount() : 0);
	UE_LOG(
		LogFlyingCabDelivery,
		Display,
		TEXT("Economy initialized with %d credits."),
		Credits);
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
	if (TrafficAwareness)
	{
		TrafficAwareness->SetTrackedPawn(BoundPawn);
	}
	if (Dispatch)
	{
		Dispatch->SetTrackedPawn(BoundPawn);
	}
	PushEconomyStatus();
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

bool AFlyingCabGameMode::CanPlayerExitVehicle(FText& OutFailureReason) const
{
	if (Dispatch && Dispatch->HasPassengerOnBoard())
	{
		OutFailureReason = FText::FromString(
			TEXT("PASSENGER ON BOARD // COMPLETE FARE BEFORE EXITING"));
		return false;
	}
	if (Dispatch && Dispatch->IsCurbsideLinkInProgress())
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
	if (HudPresenter)
	{
		HudPresenter->ShowEventMessage(Message, Color, DurationSeconds);
	}
}

AFlyingCabPlayerController* AFlyingCabGameMode::GetFlyingCabPlayerController() const
{
	return Cast<AFlyingCabPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
}

void AFlyingCabGameMode::PushEconomyStatus() const
{
	if (HudPresenter)
	{
		HudPresenter->PushEconomyStatus(Credits);
	}
}

bool AFlyingCabGameMode::IsPlayerOnFoot() const
{
	const AFlyingCabPlayerController* PlayerController = GetFlyingCabPlayerController();
	return PlayerController
		&& PlayerController->GetPlayerMode() == EFlyingCabPlayerMode::OnFoot;
}

void AFlyingCabGameMode::HandlePassengerPickedUp(const FString& DestinationName)
{
	ShowPlayerEventMessage(
		FText::FromString(TEXT("CURBSIDE LINK // PASSENGER SECURED")),
		FLinearColor::FromSRGBColor(FColor(60, 235, 255)),
		1.5f);
	UE_LOG(
		LogFlyingCabDelivery,
		Display,
		TEXT("Passenger picked up for %s; dispatch now owns the active ride."),
		*DestinationName);
}

void AFlyingCabGameMode::HandleFareCompleted(
	int32 FarePayout,
	int32 TotalDeliveries)
{
	const int32 EffectiveFarePayout = FMath::Max(0, FarePayout);
	Credits += EffectiveFarePayout;
	if (Run)
	{
		Run->RecordDelivery(EffectiveFarePayout);
	}

	ShowPlayerEventMessage(
		FText::FromString(FString::Printf(
				TEXT("PASSENGER CLEAR // +%d CR  |  BALANCE: %d  |  TOTAL: %d"),
				EffectiveFarePayout,
				Credits,
				TotalDeliveries)),
		FLinearColor::FromSRGBColor(FColor(70, 255, 150)),
		2.5f);
	UE_LOG(
		LogFlyingCabDelivery,
		Display,
		TEXT("Delivery completed for %d credits. Balance %d, total deliveries %d."),
		EffectiveFarePayout,
		Credits,
		TotalDeliveries);
	if (Run)
	{
		Run->CheckTimeAttackGoal(Credits);
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
	if (Run)
	{
		Run->RecordFuelPurchase(FuelCost);
	}
	PushEconomyStatus();
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
	if (Run)
	{
		Run->RecordRepairPurchase(RepairCost);
	}
	PushEconomyStatus();
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
		PushEconomyStatus();
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

	if (Dispatch)
	{
		Dispatch->AbortActiveRide();
	}
	const int32 ChargedTowFee = FMath::Min(Credits, FMath::Max(0, TowFee));
	Credits -= ChargedTowFee;
	if (Run)
	{
		Run->RecordTow(ChargedTowFee);
	}
	PendingRecoveryPawn = Pawn;
	if (Dispatch)
	{
		Dispatch->SetOfferAcceptanceAllowed(false);
	}
	PushEconomyStatus();
	if (HudPresenter)
	{
		HudPresenter->ClearMinimapTarget();
	}

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
		PushEconomyStatus();
		UE_LOG(
			LogFlyingCabDelivery,
			Display,
			TEXT("Vehicle %s recovered after tow."),
			*Pawn->GetName());
	}
	if (PendingRecoveryPawn == Pawn)
	{
		PendingRecoveryPawn = nullptr;
		if (Dispatch)
		{
			Dispatch->SetOfferAcceptanceAllowed(true);
		}
	}
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
	if (Run)
	{
		Run->RecordNearMiss(NearMissRewardCredits);
	}
	if (TrafficAwareness)
	{
		TrafficAwareness->ShowNearMissReward(NearMissRewardCredits);
	}
	PushEconomyStatus();
	UE_LOG(
		LogFlyingCabDelivery,
		Display,
		TEXT("Clean traffic near miss awarded %d credits. Balance %d."),
		NearMissRewardCredits,
		Credits);
	if (Run)
	{
		Run->CheckTimeAttackGoal(Credits);
	}
}

void AFlyingCabGameMode::HandleTrafficAlertChanged(
	const FText& Alert,
	const FLinearColor& Color)
{
	if (HudPresenter)
	{
		HudPresenter->SetTrafficAlert(Alert, Color);
	}
}

void AFlyingCabGameMode::HandleTimeAttackCompleted(
	const FFlyingCabTimeAttackResult& Result)
{
	if (Dispatch)
	{
		Dispatch->SetMarketActive(false);
	}
	if (HudPresenter)
	{
		HudPresenter->UpdateRunModeStatus(Credits);
	}
	UE_LOG(
		LogFlyingCabDelivery,
		Display,
		TEXT("Time Attack complete in %.2f seconds with %d credits, %d deliveries and %d near misses."),
		Result.ElapsedSeconds,
		Result.FinalCredits,
		Result.CompletedDeliveries,
		Result.NearMissCount);

	if (AFlyingCabPlayerController* Controller = GetFlyingCabPlayerController())
	{
		Controller->ShowTimeAttackResults(Result, GetBestTimeAttackTimes());
	}
}
