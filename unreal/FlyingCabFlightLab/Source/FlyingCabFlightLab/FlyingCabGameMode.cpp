// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabGameMode.h"

#include "FlyingCabDispatchComponent.h"
#include "FlyingCabEconomyAsset.h"
#include "FlyingCabEconomyComponent.h"
#include "FlyingCabFleetComponent.h"
#include "FlyingCabHudPresenterComponent.h"
#include "FlyingCabPawn.h"
#include "FlyingCabPlayerController.h"
#include "FlyingCabProgressionSubsystem.h"
#include "FlyingCabQuestCatalog.h"
#include "FlyingCabQuestDefinition.h"
#include "FlyingCabQuestSubsystem.h"
#include "FlyingCabQuestTypes.h"
#include "FlyingCabRunComponent.h"
#include "FlyingCabTrafficAwarenessComponent.h"
#include "FlyingCabTrafficVehicle.h"
#include "FlyingCabWorldBootstrap.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabDelivery, Log, All);

AFlyingCabGameMode::AFlyingCabGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	Dispatch = CreateDefaultSubobject<UFlyingCabDispatchComponent>(TEXT("Dispatch"));
	Economy = CreateDefaultSubobject<UFlyingCabEconomyComponent>(TEXT("Economy"));
	Fleet = CreateDefaultSubobject<UFlyingCabFleetComponent>(TEXT("Fleet"));
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
	EconomyConfig = UFlyingCabEconomyAsset::LoadDefaultAsset();
	InitializeQuests();
	if (Economy)
	{
		Economy->Configure(EconomyConfig);
	}
	if (Dispatch)
	{
		Dispatch->Configure(EconomyConfig);
	}
	if (Fleet)
	{
		Fleet->Configure(EconomyConfig);
	}
	if (Run)
	{
		Run->Configure(EconomyConfig);
	}
	if (TrafficAwareness)
	{
		TrafficAwareness->OnNearMissDetected.AddUObject(
			this,
			&AFlyingCabGameMode::HandleTrafficNearMiss);
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
	if (Fleet)
	{
		Fleet->OnVehicleRecoveryStarted.AddUObject(
			this,
			&AFlyingCabGameMode::HandleVehicleRecoveryStarted);
		Fleet->OnVehicleRecovered.AddUObject(
			this,
			&AFlyingCabGameMode::HandleVehicleRecovered);
	}
	if (Economy)
	{
		Economy->OnServicePurchase.AddUObject(
			this,
			&AFlyingCabGameMode::HandleServicePurchase);
	}
	if (Run)
	{
		Run->OnTimeAttackCompleted.AddUObject(
			this,
			&AFlyingCabGameMode::HandleTimeAttackCompleted);
	}
	if (Economy)
	{
		Economy->ResetCredits();
	}
	if (HudPresenter)
	{
		HudPresenter->InitializePresenter(Dispatch, Run, TrafficAwareness, Economy);
	}
	InitializeWorldBootstrap();
	InitializeDispatch();
	EnsurePawnBinding();
}

void AFlyingCabGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (TrafficAwareness)
	{
		TrafficAwareness->OnNearMissDetected.RemoveAll(this);
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
	if (Fleet)
	{
		Fleet->OnVehicleRecoveryStarted.RemoveAll(this);
		Fleet->OnVehicleRecovered.RemoveAll(this);
	}
	if (Economy)
	{
		Economy->OnServicePurchase.RemoveAll(this);
	}
	if (QuestSystem)
	{
		QuestSystem->OnQuestCompleted.RemoveDynamic(this, &AFlyingCabGameMode::HandleQuestCompleted);
	}
	Super::EndPlay(EndPlayReason);
}

void AFlyingCabGameMode::StartRun(EFlyingCabRunMode Mode)
{
	if (!Run || !Run->StartRun(Mode))
	{
		return;
	}

	if (Economy)
	{
		Economy->ResetCredits();
	}
	if (Dispatch)
	{
		Dispatch->StartPassengerMarket(Mode == EFlyingCabRunMode::TimeAttack);
	}
	if (QuestSystem)
	{
		const bool bQuestGameplayEnabled = Mode == EFlyingCabRunMode::Freeroam;
		QuestSystem->SetGameplayEventsEnabled(bQuestGameplayEnabled);
		if (bQuestGameplayEnabled)
		{
			QuestSystem->StartAutoQuests();
		}
	}

	if (Mode == EFlyingCabRunMode::TimeAttack)
	{
		// Competitive runs always begin without session-granted vehicle access.
		// Freeroam intentionally keeps access across map reloads for this app session.
		if (WorldBootstrap)
		{
			WorldBootstrap->ResetCompetitiveServiceAccess();
		}
	}

	EnsurePawnBinding();
	if (HudPresenter)
	{
		HudPresenter->UpdateRunModeStatus(GetCredits());
	}
	UE_LOG(
		LogFlyingCabDelivery,
		Display,
		TEXT("Run started in %s mode with %d credits%s."),
		Mode == EFlyingCabRunMode::TimeAttack ? TEXT("Time Attack") : TEXT("Freeroam"),
		GetCredits(),
		Mode == EFlyingCabRunMode::TimeAttack
			? *FString::Printf(TEXT("; target %d"), Run->GetTimeAttackTargetCredits())
			: TEXT(""));
	Run->CheckTimeAttackGoal(GetCredits());
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

int32 AFlyingCabGameMode::GetCredits() const
{
	return Economy ? Economy->GetCredits() : 0;
}

AFlyingCabPawn* AFlyingCabGameMode::GetActiveVehicle() const
{
	return Fleet ? Fleet->GetActiveVehicle() : nullptr;
}

void AFlyingCabGameMode::InitializeQuests()
{
	QuestCatalog = UFlyingCabQuestCatalog::LoadDefaultAsset();
	UGameInstance* GameInstance = GetGameInstance();
	QuestSystem = GameInstance
		? GameInstance->GetSubsystem<UFlyingCabQuestSubsystem>()
		: nullptr;
	if (!QuestSystem || !QuestSystem->ConfigureCatalog(QuestCatalog))
	{
		UE_LOG(LogFlyingCabDelivery, Error, TEXT("Could not initialize the quest catalog."));
		return;
	}
	QuestSystem->OnQuestCompleted.AddDynamic(this, &AFlyingCabGameMode::HandleQuestCompleted);
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

	WorldBootstrap->Bootstrap(DefaultPawnClass, EconomyConfig);
	if (Fleet)
	{
		Fleet->RegisterVehicle(WorldBootstrap->GetServiceVehicle());
	}

	if (TrafficAwareness)
	{
		TrafficAwareness->InitializeTrafficVehicles(WorldBootstrap->GetTrafficVehicles());
	}

	UE_LOG(
		LogFlyingCabDelivery,
		Display,
		TEXT("Traffic initialized with %d/%d vehicles; clean near misses award %d credits."),
		WorldBootstrap->GetTrafficVehicles().Num(),
		AFlyingCabWorldBootstrap::GetExpectedTrafficVehicleCount(),
		Economy ? Economy->GetNearMissRewardCredits() : 0);
}

void AFlyingCabGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	EnsurePawnBinding();
	AFlyingCabPawn* ActiveVehicle = Fleet ? Fleet->GetActiveVehicle() : nullptr;
	if (HudPresenter)
	{
		HudPresenter->Refresh(DeltaSeconds, ActiveVehicle, GetCredits());
	}
	if (Run)
	{
		Run->CheckTimeAttackGoal(GetCredits());
	}
}

void AFlyingCabGameMode::InitializeDispatch()
{
	if (Dispatch && Dispatch->InitializeNetwork())
	{
		return;
	}
	UE_LOG(LogFlyingCabDelivery, Error, TEXT("Could not initialize passenger dispatch."));
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
	if (Fleet && Pawn == Fleet->GetActiveVehicle())
	{
		return;
	}

	if (Fleet)
	{
		Fleet->SetActiveVehicle(Pawn);
	}
	if (TrafficAwareness)
	{
		TrafficAwareness->SetTrackedPawn(Pawn);
	}
	if (Dispatch)
	{
		Dispatch->SetTrackedPawn(Pawn);
	}
}

bool AFlyingCabGameMode::CanPlayerExitVehicle(FText& OutFailureReason) const
{
	return !Dispatch || Dispatch->CanPlayerExitVehicle(OutFailureReason);
}

void AFlyingCabGameMode::HandlePassengerPickedUp(const FString& DestinationName)
{
	if (QuestSystem)
	{
		QuestSystem->RecordEvent(
			FlyingCabQuestEvents::PassengerPickedUp,
			FName(*DestinationName));
	}
	if (HudPresenter)
	{
		HudPresenter->ShowPassengerPickedUp();
	}
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
	const int32 AwardedFare = Economy ? Economy->AddCredits(FarePayout) : 0;
	if (Run)
	{
		Run->RecordDelivery(AwardedFare);
	}

	if (HudPresenter)
	{
		HudPresenter->ShowFareCompleted(AwardedFare, GetCredits(), TotalDeliveries);
	}
	if (QuestSystem)
	{
		QuestSystem->RecordEvent(FlyingCabQuestEvents::PassengerDelivered);
	}
	UE_LOG(
		LogFlyingCabDelivery,
		Display,
		TEXT("Delivery completed for %d credits. Balance %d, total deliveries %d."),
		AwardedFare,
		GetCredits(),
		TotalDeliveries);
	if (Run)
	{
		Run->CheckTimeAttackGoal(GetCredits());
	}
}

int32 AFlyingCabGameMode::TryPurchaseFuel(
	AFlyingCabPawn* Pawn,
	int32 RequestedUnits,
	int32 PricePerUnit)
{
	if (!Economy)
	{
		return 0;
	}

	return Economy->TryPurchaseFuel(
		Pawn,
		RequestedUnits,
		PricePerUnit).UnitsPurchased;
}

int32 AFlyingCabGameMode::TryPurchaseRepair(
	AFlyingCabPawn* Pawn,
	int32 RequestedUnits,
	int32 PricePerUnit)
{
	if (!Economy)
	{
		return 0;
	}

	return Economy->TryPurchaseRepair(
		Pawn,
		RequestedUnits,
		PricePerUnit).UnitsPurchased;
}

void AFlyingCabGameMode::HandleServicePurchase(
	const FFlyingCabServicePurchaseResult& Result)
{
	if (QuestSystem && Result.UnitsPurchased > 0)
	{
		QuestSystem->RecordEvent(
			Result.bRepairService
				? FlyingCabQuestEvents::RepairPurchased
				: FlyingCabQuestEvents::FuelPurchased,
			NAME_None,
			Result.UnitsPurchased);
	}
	if (Result.bInsufficientCredits && HudPresenter)
	{
		HudPresenter->ShowInsufficientServiceCredits(Result.bRepairService);
	}
	if (Result.CreditsSpent > 0 && Run)
	{
		Result.bRepairService
			? Run->RecordRepairPurchase(Result.CreditsSpent)
			: Run->RecordFuelPurchase(Result.CreditsSpent);
	}
}

void AFlyingCabGameMode::HandleVehicleRecoveryStarted(
	AFlyingCabPawn* Pawn,
	bool bAffectsActiveRun,
	float RecoveryDelay)
{
	if (!Pawn)
	{
		return;
	}

	if (!bAffectsActiveRun)
	{
		if (HudPresenter)
		{
			HudPresenter->ShowVehicleRecoveryStarted(
				Pawn->GetVehicleDisplayName(),
				false,
				0,
				RecoveryDelay);
		}
		return;
	}

	if (Dispatch)
	{
		Dispatch->AbortActiveRide();
	}
	const int32 ChargedTowFee = Economy ? Economy->ChargeTowFee() : 0;
	if (Run)
	{
		Run->RecordTow(ChargedTowFee);
	}
	if (Dispatch)
	{
		Dispatch->SetOfferAcceptanceAllowed(false);
	}
	if (HudPresenter)
	{
		HudPresenter->ClearMinimapTarget();
	}

	if (HudPresenter)
	{
		HudPresenter->ShowVehicleRecoveryStarted(
			Pawn->GetVehicleDisplayName(),
			true,
			ChargedTowFee,
			RecoveryDelay);
	}
	UE_LOG(
		LogFlyingCabDelivery,
		Warning,
		TEXT("Cab destroyed. Course aborted and %d credit tow fee charged."),
		ChargedTowFee);
}

void AFlyingCabGameMode::HandleVehicleRecovered(
	AFlyingCabPawn* Pawn,
	bool bRecoveredActiveVehicle)
{
	if (bRecoveredActiveVehicle && Dispatch)
	{
		Dispatch->SetOfferAcceptanceAllowed(true);
	}
}

void AFlyingCabGameMode::HandleTrafficNearMiss(
	AFlyingCabTrafficVehicle* Vehicle,
	AFlyingCabPawn* Pawn)
{
	if (!Vehicle || !Pawn || !Fleet || !Economy
		|| Pawn != Fleet->GetActiveVehicle() || Pawn->IsDestroyed())
	{
		return;
	}

	const int32 AwardedCredits = Economy->AwardNearMiss();
	if (AwardedCredits <= 0)
	{
		return;
	}
	if (Run)
	{
		Run->RecordNearMiss(AwardedCredits);
	}
	if (QuestSystem)
	{
		QuestSystem->RecordEvent(FlyingCabQuestEvents::NearMiss);
	}
	if (TrafficAwareness)
	{
		TrafficAwareness->ShowNearMissReward(AwardedCredits);
	}
	UE_LOG(
		LogFlyingCabDelivery,
		Display,
		TEXT("Clean traffic near miss awarded %d credits. Balance %d."),
		AwardedCredits,
		GetCredits());
	if (Run)
	{
		Run->CheckTimeAttackGoal(GetCredits());
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
		HudPresenter->UpdateRunModeStatus(GetCredits());
	}
	UE_LOG(
		LogFlyingCabDelivery,
		Display,
		TEXT("Time Attack complete in %.2f seconds with %d credits, %d deliveries and %d near misses."),
		Result.ElapsedSeconds,
		Result.FinalCredits,
		Result.CompletedDeliveries,
		Result.NearMissCount);

	if (AFlyingCabPlayerController* Controller = Cast<AFlyingCabPlayerController>(
		UGameplayStatics::GetPlayerController(this, 0)))
	{
		Controller->ShowTimeAttackResults(Result, GetBestTimeAttackTimes());
	}
}

void AFlyingCabGameMode::HandleQuestCompleted(UFlyingCabQuestDefinition* Quest)
{
	if (!Quest)
	{
		return;
	}
	const int32 RewardCredits = Economy ? Economy->AddCredits(Quest->Reward.Credits) : 0;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFlyingCabProgressionSubsystem* Progression =
			GameInstance->GetSubsystem<UFlyingCabProgressionSubsystem>())
		{
			for (const FName AccessId : Quest->Reward.GrantedAccessIds)
			{
				Progression->GrantAccess(AccessId);
			}
		}
	}
	if (HudPresenter)
	{
		HudPresenter->ShowEventMessage(
			FText::Format(
				NSLOCTEXT("FlyingCab", "QuestRewardMessage", "QUEST COMPLETE // {0} // +{1} CR"),
				Quest->Title,
				FText::AsNumber(RewardCredits)),
			FLinearColor::FromSRGBColor(FColor(80, 255, 155)),
			3.0f);
	}
}
