// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "EnhancedPlayerInput.h"
#include "EnhancedInputSubsystems.h"
#include "Components/PrimitiveComponent.h"
#include "EngineUtils.h"
#include "FlyingCabAccessTerminal.h"
#include "FlyingCabCameraRig.h"
#include "FlyingCabCharacter.h"
#include "FlyingCabQuestGiver.h"
#include "FlyingCabQuestJournalWidget.h"
#include "FlyingCabCityExpansion.h"
#include "FlyingCabDeliveryZone.h"
#include "FlyingCabDispatchComponent.h"
#include "FlyingCabFuelStation.h"
#include "FlyingCabGameMode.h"
#include "FlyingCabInputData.h"
#include "FlyingCabLivingPedestrian.h"
#include "FlyingCabLivingRoute.h"
#include "FlyingCabLivingWorldManager.h"
#include "FlyingCabNightshiftOffice.h"
#include "FlyingCabOnFootPortal.h"
#include "FlyingCabPawn.h"
#include "FlyingCabPlayerController.h"
#include "FlyingCabQuestCatalog.h"
#include "FlyingCabQuestDefinition.h"
#include "FlyingCabQuestSubsystem.h"
#include "FlyingCabRepairStation.h"
#include "FlyingCabTrafficVehicle.h"
#include "FlyingCabVehicleVitalsComponent.h"
#include "FlyingCabWorldBootstrap.h"
#include "Misc/AutomationTest.h"
#include "InputKeyEventArgs.h"
#include "Tests/AutomationCommon.h"

namespace
{
	constexpr const TCHAR* FlightLabMap = TEXT("/Game/Maps/FlightLab");
	constexpr double FunctionalTestTimeoutSeconds = 10.0;

	struct FFlyingCabPIEState
	{
		UWorld* World = nullptr;
		AFlyingCabGameMode* GameMode = nullptr;
		AFlyingCabPlayerController* PlayerController = nullptr;
		AFlyingCabPawn* Pawn = nullptr;
		UFlyingCabDispatchComponent* Dispatch = nullptr;
	};

	bool ResolvePIEState(FFlyingCabPIEState& OutState)
	{
		OutState = {};
		OutState.World = AutomationCommon::GetAnyGameWorld();
		if (!OutState.World)
		{
			return false;
		}

		OutState.GameMode = OutState.World->GetAuthGameMode<AFlyingCabGameMode>();
		OutState.PlayerController = Cast<AFlyingCabPlayerController>(
			OutState.World->GetFirstPlayerController());
		OutState.Pawn = OutState.PlayerController
			? Cast<AFlyingCabPawn>(OutState.PlayerController->GetPawn())
			: nullptr;
		OutState.Dispatch = OutState.GameMode
			? OutState.GameMode->FindComponentByClass<UFlyingCabDispatchComponent>()
			: nullptr;
		return OutState.GameMode && OutState.PlayerController
			&& OutState.Pawn && OutState.Dispatch;
	}

	UEnhancedPlayerInput* GetEnhancedPlayerInput(
		const AFlyingCabPlayerController* PlayerController)
	{
		return PlayerController
			? Cast<UEnhancedPlayerInput>(PlayerController->PlayerInput)
			: nullptr;
	}

	template <typename TActor>
	int32 CountActors(UWorld* World)
	{
		int32 Count = 0;
		for (TActorIterator<TActor> It(World); It; ++It)
		{
			Count += IsValid(*It) ? 1 : 0;
		}
		return Count;
	}

	AFlyingCabWorldBootstrap* FindWorldBootstrap(UWorld* World)
	{
		for (TActorIterator<AFlyingCabWorldBootstrap> It(World); It; ++It)
		{
			if (IsValid(*It))
			{
				return *It;
			}
		}
		return nullptr;
	}

	void HoldPawnAt(AFlyingCabPawn* Pawn, const FVector& Location)
	{
		if (!Pawn)
		{
			return;
		}

		Pawn->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
		if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(Pawn->GetRootComponent()))
		{
			RootPrimitive->SetPhysicsLinearVelocity(FVector::ZeroVector);
			RootPrimitive->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		}
	}

	TArray<FIntPoint> GetOfferSignature(const UFlyingCabDispatchComponent* Dispatch)
	{
		TArray<FIntPoint> Signature;
		if (!Dispatch)
		{
			return Signature;
		}
		for (const FFlyingCabPassengerOfferState& Offer : Dispatch->GetPassengerOffers())
		{
			Signature.Emplace(Offer.PickupIndex, Offer.DropoffIndex);
		}
		return Signature;
	}

	bool AreOffersAccepting(const UFlyingCabDispatchComponent* Dispatch, bool bExpected)
	{
		if (!Dispatch || Dispatch->GetPassengerOffers().IsEmpty())
		{
			return false;
		}
		return Dispatch->GetPassengerOffers().ContainsByPredicate(
			[bExpected](const FFlyingCabPassengerOfferState& Offer)
			{
				return !Offer.Zone || Offer.Zone->IsAcceptanceEnabled() != bExpected;
			}) == false;
	}

	class FFlyingCabVerifyWorldStartupCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FFlyingCabVerifyWorldStartupCommand(FAutomationTestBase* InTest)
			: Test(InTest)
			, Deadline(FPlatformTime::Seconds() + FunctionalTestTimeoutSeconds)
		{
		}

		virtual bool Update() override
		{
			FFlyingCabPIEState State;
			AFlyingCabWorldBootstrap* Bootstrap = nullptr;
			if (!ResolvePIEState(State)
				|| !(Bootstrap = FindWorldBootstrap(State.World)))
			{
				if (FPlatformTime::Seconds() < Deadline)
				{
					return false;
				}
				Test->AddError(TEXT("FlightLab PIE world did not finish bootstrapping in time."));
				return true;
			}

			Test->TestEqual(
				TEXT("Dispatch exposes all passenger stops"),
				State.Dispatch->GetStopCount(),
				10);
			Test->TestNotNull(TEXT("The inactive dropoff zone is created"), State.Dispatch->GetDropoffZone());
			Test->TestFalse(
				TEXT("Legacy point-to-point traffic is disabled by default"),
				Bootstrap->IsLegacyTrafficEnabled());
			Test->TestEqual(
				TEXT("Only the built-in Living World vehicles are registered"),
				Bootstrap->GetTrafficVehicles().Num(),
				AFlyingCabWorldBootstrap::GetExpectedTrafficVehicleCount());
			Test->TestEqual(
				TEXT("All traffic actors exist in the PIE world"),
				CountActors<AFlyingCabTrafficVehicle>(State.World),
				Bootstrap->GetConfiguredTrafficVehicleCount());
			Test->TestEqual(
				TEXT("One manager coordinates the living world"),
				CountActors<AFlyingCabLivingWorldManager>(State.World),
				1);
			AFlyingCabLivingWorldManager* LivingWorld = Bootstrap->GetLivingWorldManager();
			Test->TestNotNull(TEXT("Bootstrap exposes the living-world manager"), LivingWorld);
			Test->TestTrue(
				TEXT("At least one living-world route is active"),
				LivingWorld && !LivingWorld->GetRoutes().IsEmpty());
			Test->TestEqual(
				TEXT("All configured living-world routes exist in PIE"),
				CountActors<AFlyingCabLivingRoute>(State.World),
				LivingWorld ? LivingWorld->GetRoutes().Num() : 0);
			Test->TestEqual(
				TEXT("All configured ambient pedestrians exist in PIE"),
				CountActors<AFlyingCabLivingPedestrian>(State.World),
				LivingWorld ? LivingWorld->GetPedestrians().Num() : 0);
			Test->TestEqual(
				TEXT("Bootstrap registers all living pedestrians"),
				Bootstrap->GetLivingPedestrianCount(),
				LivingWorld ? LivingWorld->GetPedestrians().Num() : 0);
			Test->TestEqual(TEXT("Three fuel stations are present"), Bootstrap->GetFuelStationCount(), 3);
			Test->TestEqual(TEXT("Two repair stations are present"), Bootstrap->GetRepairStationCount(), 2);
			Test->TestEqual(TEXT("One city expansion actor is present"), CountActors<AFlyingCabCityExpansion>(State.World), 1);
			Test->TestEqual(TEXT("Two on-foot portals are present"), CountActors<AFlyingCabOnFootPortal>(State.World), 2);
			Test->TestEqual(TEXT("One access terminal is present"), CountActors<AFlyingCabAccessTerminal>(State.World), 1);
			Test->TestEqual(TEXT("Mike and Jack quest givers are present"), CountActors<AFlyingCabQuestGiver>(State.World), 2);
			Test->TestEqual(TEXT("Bootstrap registers both quest hubs"), Bootstrap->GetQuestGiverCount(), 2);
			Test->TestEqual(TEXT("One nightshift office is present"), CountActors<AFlyingCabNightshiftOffice>(State.World), 1);
			Test->TestTrue(
				TEXT("The player cab and parked service cab both exist"),
				CountActors<AFlyingCabPawn>(State.World) >= 2);
			Test->TestNotNull(TEXT("The parked service cab is registered by bootstrap"), Bootstrap->GetServiceVehicle());
			const FFlyingCabInputAssets& InputAssets = FlyingCabInputData::GetAssets();
			UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
				State.PlayerController->GetLocalPlayer()
					? State.PlayerController->GetLocalPlayer()->GetSubsystem<
						UEnhancedInputLocalPlayerSubsystem>()
					: nullptr;
			Test->TestNotNull(TEXT("The local player owns an Enhanced Input subsystem"), InputSubsystem);
			Test->TestTrue(
				TEXT("The Flying Cab mapping context is active in PIE"),
				InputSubsystem && InputSubsystem->HasMappingContext(InputAssets.MappingContext));
			return true;
		}

	private:
		FAutomationTestBase* Test = nullptr;
		double Deadline = 0.0;
	};

	class FFlyingCabVerifyDeveloperObserverCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FFlyingCabVerifyDeveloperObserverCommand(FAutomationTestBase* InTest)
			: Test(InTest)
			, Deadline(FPlatformTime::Seconds() + FunctionalTestTimeoutSeconds)
		{
		}

		virtual bool Update() override
		{
			FFlyingCabPIEState State;
			if (!ResolvePIEState(State))
			{
				if (FPlatformTime::Seconds() < Deadline)
				{
					return false;
				}
				Test->AddError(TEXT("Developer observer test could not resolve the PIE player."));
				return true;
			}

			if (!bRunStarted)
			{
				State.PlayerController->StartRunMode(EFlyingCabRunMode::Freeroam);
				bRunStarted = true;
				return false;
			}

			AFlyingCabCameraRig* CameraRig = State.PlayerController->GetCameraRig();
			Test->TestNotNull(TEXT("The persistent camera rig exists"), CameraRig);
			if (!CameraRig)
			{
				return true;
			}

			const FFlyingCabInputAssets& InputAssets = FlyingCabInputData::GetAssets();
			UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
				State.PlayerController->GetLocalPlayer()
					? State.PlayerController->GetLocalPlayer()->GetSubsystem<
						UEnhancedInputLocalPlayerSubsystem>()
					: nullptr;
			const FVector StartLocation = CameraRig->GetActorLocation();
			const float StartArmLength = CameraRig->GetCurrentArmLength();
			UPrimitiveComponent* VehicleBody =
				Cast<UPrimitiveComponent>(State.Pawn->GetRootComponent());
			Test->TestTrue(
				TEXT("The gameplay camera provides a wide planning frame"),
				StartArmLength >= 3100.0f);
			Test->TestTrue(
				TEXT("The controlled cab keeps its visual focus marker"),
				State.Pawn->IsTestPlayerFocusVisible());

			Test->TestTrue(
				TEXT("O reaches the player controller"),
				State.PlayerController->InputKey(
					FInputKeyEventArgs::CreateSimulated(EKeys::O, IE_Pressed, 1.0f)));
			State.PlayerController->InputKey(
				FInputKeyEventArgs::CreateSimulated(EKeys::O, IE_Released, 0.0f));
			Test->TestTrue(
				TEXT("Developer observer mode activates"),
				State.PlayerController->IsDeveloperObserverMode());
			Test->TestTrue(
				TEXT("The camera rig stops following while observing"),
				CameraRig->IsDeveloperObserverEnabled());
			Test->TestTrue(
				TEXT("Gameplay input is suppressed while observing"),
				State.PlayerController->IsGameplayInputSuppressed());
			Test->TestFalse(
				TEXT("The controlled cab is frozen while ambient actors keep simulating"),
				VehicleBody && VehicleBody->IsSimulatingPhysics());
			Test->TestTrue(
				TEXT("The gameplay mapping context remains stable while observing"),
				InputSubsystem && InputSubsystem->HasMappingContext(InputAssets.MappingContext));

			CameraRig->MoveDeveloperObserver(FVector2D(1.0f, 0.0f), false, 0.25f);
			CameraRig->AdjustDeveloperObserverZoom(1.0f, 0.25f);
			Test->TestTrue(
				TEXT("Observer panning changes the camera rig position"),
				!CameraRig->GetActorLocation().Equals(StartLocation));
			Test->TestTrue(
				TEXT("Observer mode starts wider and can zoom farther out"),
				CameraRig->GetCurrentArmLength() > StartArmLength);

			Test->TestTrue(
				TEXT("O exits the developer observer"),
				State.PlayerController->InputKey(
					FInputKeyEventArgs::CreateSimulated(EKeys::O, IE_Pressed, 1.0f)));
			State.PlayerController->InputKey(
				FInputKeyEventArgs::CreateSimulated(EKeys::O, IE_Released, 0.0f));
			Test->TestFalse(
				TEXT("Developer observer mode deactivates"),
				State.PlayerController->IsDeveloperObserverMode());
			Test->TestFalse(
				TEXT("The camera resumes follow mode"),
				CameraRig->IsDeveloperObserverEnabled());
			Test->TestTrue(
				TEXT("The gameplay mapping context is restored"),
				InputSubsystem && InputSubsystem->HasMappingContext(InputAssets.MappingContext));
			Test->TestTrue(
				TEXT("Cab physics is restored after leaving the observer"),
				VehicleBody && VehicleBody->IsSimulatingPhysics());
			return true;
		}

	private:
		FAutomationTestBase* Test = nullptr;
		double Deadline = 0.0;
		bool bRunStarted = false;
	};

	class FFlyingCabCompleteCourseCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FFlyingCabCompleteCourseCommand(FAutomationTestBase* InTest)
			: Test(InTest)
			, Deadline(FPlatformTime::Seconds() + FunctionalTestTimeoutSeconds)
		{
		}

		virtual bool Update() override
		{
			FFlyingCabPIEState State;
			if (!ResolvePIEState(State))
			{
				return WaitOrFail(TEXT("FlightLab course test could not resolve its PIE actors."));
			}

			if (Stage == EStage::Initialize)
			{
				State.PlayerController->StartRunMode(EFlyingCabRunMode::Freeroam);
				State.Dispatch->StartPassengerMarket(true);
				const TArray<FIntPoint> FirstSignature = GetOfferSignature(State.Dispatch);
				State.Dispatch->StartPassengerMarket(true);
				const TArray<FIntPoint> SecondSignature = GetOfferSignature(State.Dispatch);
				Test->TestTrue(
					TEXT("Fixed seed 1977 reproduces the initial passenger offers"),
					FirstSignature.Num() > 0 && FirstSignature == SecondSignature);

				if (State.Dispatch->GetPassengerOffers().IsEmpty()
					|| !State.Dispatch->GetPassengerOffers()[0].Zone)
				{
					Test->AddError(TEXT("Time Attack did not create a usable passenger offer."));
					return true;
				}

				const FFlyingCabPassengerOfferState& Offer =
					State.Dispatch->GetPassengerOffers()[0];
				PickupZone = Offer.Zone;
				ExpectedDropoffIndex = Offer.DropoffIndex;
				ExpectedDestinationId = State.Dispatch->GetStopId(ExpectedDropoffIndex);
				Test->TestFalse(
					TEXT("The selected passenger destination has a stable district ID"),
					ExpectedDestinationId.IsNone());

				UFlyingCabQuestSubsystem* QuestSystem = State.World->GetGameInstance()
					? State.World->GetGameInstance()->GetSubsystem<UFlyingCabQuestSubsystem>()
					: nullptr;
				if (!QuestSystem)
				{
					Test->AddError(TEXT("The course test could not resolve the quest subsystem."));
					return true;
				}
				UFlyingCabQuestCatalog* Catalog = NewObject<UFlyingCabQuestCatalog>(QuestSystem);
				UFlyingCabQuestDefinition* Quest = NewObject<UFlyingCabQuestDefinition>(Catalog);
				Quest->QuestId = CourseQuestId;
				Quest->Title = FText::FromString(TEXT("DISTRICT TARGET TEST"));
				Quest->Description = FText::FromString(TEXT("Verify passenger district events."));
				FFlyingCabQuestObjectiveDefinition PickupObjective;
				PickupObjective.ObjectiveId = TEXT("PickupForDistrict");
				PickupObjective.Description = FText::FromString(TEXT("Pick up a passenger"));
				PickupObjective.EventId = FlyingCabQuestEvents::PassengerPickedUp;
				PickupObjective.TargetId = ExpectedDestinationId;
				FFlyingCabQuestObjectiveDefinition DeliveryObjective;
				DeliveryObjective.ObjectiveId = TEXT("DeliverToDistrict");
				DeliveryObjective.Description = FText::FromString(TEXT("Complete the fare"));
				DeliveryObjective.EventId = FlyingCabQuestEvents::PassengerDelivered;
				DeliveryObjective.TargetId = ExpectedDestinationId;
				Quest->Objectives = {PickupObjective, DeliveryObjective};
				Catalog->Quests = {Quest};
				Test->TestTrue(
					TEXT("The district-target test quest catalog is accepted"),
					QuestSystem->ConfigureCatalog(Catalog));
				QuestSystem->SetGameplayEventsEnabled(true);
				Test->TestTrue(
					TEXT("The district-target test quest starts"),
					QuestSystem->StartQuest(CourseQuestId));
				InitialCredits = State.GameMode->GetCredits();
				Stage = EStage::Pickup;
			}

			if (Stage == EStage::Pickup)
			{
				if (State.Dispatch->HasPassengerOnBoard())
				{
					Test->TestEqual(
						TEXT("The accepted offer selects its configured destination"),
						State.Dispatch->GetCurrentDropoffIndex(),
						ExpectedDropoffIndex);
					Test->TestTrue(
						TEXT("Boarding starts a positive fare"),
						State.Dispatch->GetActiveFareCredits() > 0);
					Test->TestNotNull(
						TEXT("Boarding activates the shared dropoff zone"),
						State.Dispatch->GetDropoffZone());
					const UFlyingCabQuestSubsystem* QuestSystem = State.World->GetGameInstance()
						? State.World->GetGameInstance()->GetSubsystem<UFlyingCabQuestSubsystem>()
						: nullptr;
					const FFlyingCabQuestRuntimeState* QuestState = QuestSystem
						? QuestSystem->FindState(CourseQuestId)
						: nullptr;
					Test->TestTrue(
						TEXT("Pickup advances the quest using the stable destination ID"),
						QuestState && QuestState->Status == EFlyingCabQuestStatus::Active
							&& QuestState->ActiveObjectiveIndex == 1);
					Stage = EStage::Dropoff;
				}
				else if (PickupZone.IsValid())
				{
					HoldPawnAt(State.Pawn, PickupZone->GetActorLocation());
				}
				else
				{
					Test->AddError(TEXT("Passenger pickup zone disappeared before boarding."));
					return true;
				}
			}

			if (Stage == EStage::Dropoff)
			{
				AFlyingCabDeliveryZone* DropoffZone = State.Dispatch->GetDropoffZone();
				if (!DropoffZone)
				{
					Test->AddError(TEXT("Dropoff zone disappeared during the active fare."));
					return true;
				}
				HoldPawnAt(State.Pawn, DropoffZone->GetActorLocation());
				if (State.Dispatch->HasPassengerOnBoard())
				{
					LastObservedFare = State.Dispatch->GetActiveFareCredits();
				}
				else if (State.Dispatch->GetCompletedDeliveries() == 1)
				{
					Test->TestTrue(TEXT("The completed ride produced a payout"), LastObservedFare > 0);
					Test->TestEqual(
						TEXT("The fare payout is credited exactly once"),
						State.GameMode->GetCredits(),
						InitialCredits + LastObservedFare);
					Test->TestFalse(
						TEXT("The passenger seat is free after curbside exit"),
						State.Dispatch->HasPassengerOnBoard());
					const UFlyingCabQuestSubsystem* QuestSystem = State.World->GetGameInstance()
						? State.World->GetGameInstance()->GetSubsystem<UFlyingCabQuestSubsystem>()
						: nullptr;
					Test->TestTrue(
						TEXT("Delivery completes the quest using the same stable destination ID"),
						QuestSystem
							&& QuestSystem->GetQuestStatus(CourseQuestId)
								== EFlyingCabQuestStatus::Completed);
					return true;
				}
			}

			return WaitOrFail(TEXT("The deterministic pickup-to-payout flow timed out."));
		}

	private:
		enum class EStage : uint8
		{
			Initialize,
			Pickup,
			Dropoff
		};

		bool WaitOrFail(const TCHAR* Message)
		{
			if (FPlatformTime::Seconds() < Deadline)
			{
				return false;
			}
			Test->AddError(Message);
			return true;
		}

		FAutomationTestBase* Test = nullptr;
		TWeakObjectPtr<AFlyingCabDeliveryZone> PickupZone;
		const FName CourseQuestId = TEXT("Quest.FunctionalDistrictTarget");
		FName ExpectedDestinationId = NAME_None;
		EStage Stage = EStage::Initialize;
		double Deadline = 0.0;
		int32 ExpectedDropoffIndex = INDEX_NONE;
		int32 InitialCredits = 0;
		int32 LastObservedFare = 0;
	};

	class FFlyingCabVerifyActiveTowCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FFlyingCabVerifyActiveTowCommand(FAutomationTestBase* InTest)
			: Test(InTest)
			, Deadline(FPlatformTime::Seconds() + FunctionalTestTimeoutSeconds)
		{
		}

		virtual bool Update() override
		{
			FFlyingCabPIEState State;
			if (!ResolvePIEState(State))
			{
				return WaitOrFail(TEXT("FlightLab tow test could not resolve its PIE actors."));
			}

			if (!bTowTriggered)
			{
				State.PlayerController->StartRunMode(EFlyingCabRunMode::TimeAttack);
				Test->TestTrue(
					TEXT("Passenger offers accept cabs before destruction"),
					AreOffersAccepting(State.Dispatch, true));

				UFlyingCabVehicleVitalsComponent* Vitals =
					State.Pawn->FindComponentByClass<UFlyingCabVehicleVitalsComponent>();
				if (!Vitals)
				{
					Test->AddError(TEXT("The player cab has no vitals component."));
					return true;
				}

				InitialCredits = State.GameMode->GetCredits();
				UPrimitiveComponent* Body = Cast<UPrimitiveComponent>(State.Pawn->GetRootComponent());
				if (!Body)
				{
					Test->AddError(TEXT("The active cab has no collision body."));
					return true;
				}
				Body->OnComponentHit.Broadcast(Body, nullptr, nullptr,
					FVector(0.0f, 0.0f, Body->GetMass() * 1000000.0f), FHitResult());
				Test->TestTrue(TEXT("The collision path destroys the active cab"), State.Pawn->IsDestroyed());

				CreditsAfterTow = State.GameMode->GetCredits();
				Test->TestEqual(
					TEXT("Active-cab destruction charges the configured 35-credit tow fee"),
					CreditsAfterTow,
					FMath::Max(0, InitialCredits - 35));
				Test->TestTrue(
					TEXT("Tow recovery blocks every waiting passenger offer"),
					AreOffersAccepting(State.Dispatch, false));
				bTowTriggered = true;
			}

			if (!State.Pawn->IsDestroyed())
			{
				Test->TestEqual(
					TEXT("Recovery does not charge the tow fee twice"),
					State.GameMode->GetCredits(),
					CreditsAfterTow);
				Test->TestTrue(
					TEXT("Recovered cab has a full hull"),
					FMath::IsNearlyEqual(State.Pawn->GetHullPercent(), 1.0f));
				Test->TestTrue(
					TEXT("Recovered cab has the guaranteed fuel reserve"),
					State.Pawn->GetFuelPercent() >= 0.25f);
				Test->TestTrue(
					TEXT("Passenger offers reopen after recovery"),
					AreOffersAccepting(State.Dispatch, true));
				return true;
			}

			return WaitOrFail(TEXT("The active-cab tow recovery flow timed out."));
		}

	private:
		bool WaitOrFail(const TCHAR* Message)
		{
			if (FPlatformTime::Seconds() < Deadline)
			{
				return false;
			}
			Test->AddError(Message);
			return true;
		}

		FAutomationTestBase* Test = nullptr;
		double Deadline = 0.0;
		int32 InitialCredits = 0;
		int32 CreditsAfterTow = 0;
		bool bTowTriggered = false;
	};

	class FFlyingCabVerifyEnhancedInputReleaseCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FFlyingCabVerifyEnhancedInputReleaseCommand(FAutomationTestBase* InTest)
			: Test(InTest)
			, Deadline(FPlatformTime::Seconds() + FunctionalTestTimeoutSeconds)
		{
		}

		virtual bool Update() override
		{
			FFlyingCabPIEState State;
			if (!ResolvePIEState(State))
			{
				return WaitOrFail(TEXT("Enhanced Input release test could not resolve the PIE cab."));
			}

			switch (Phase)
			{
			case 0:
				State.PlayerController->StartRunMode(EFlyingCabRunMode::Freeroam);
				Test->TestTrue(
					TEXT("Simulated D press reaches the player controller"),
					State.PlayerController->InputKey(
						FInputKeyEventArgs::CreateSimulated(EKeys::D, IE_Pressed, 1.0f)));
				Phase = 1;
				return false;

			case 1:
				if (!FMath::IsNearlyEqual(State.Pawn->GetTestKeyboardHorizontalInput(), 1.0f))
				{
					return WaitOrFail(TEXT("Enhanced Input did not apply right thrust."));
				}
				Test->TestTrue(
					TEXT("Simulated W press reaches the player controller"),
					State.PlayerController->InputKey(
						FInputKeyEventArgs::CreateSimulated(EKeys::W, IE_Pressed, 1.0f)));
				Phase = 2;
				return false;

			case 2:
				if (!FMath::IsNearlyEqual(State.Pawn->GetTestKeyboardThrustInput(), 1.0f))
				{
					return WaitOrFail(TEXT("Enhanced Input did not apply vertical thrust."));
				}
				State.PlayerController->InputKey(
					FInputKeyEventArgs::CreateSimulated(EKeys::D, IE_Released, 0.0f));
				State.PlayerController->InputKey(
					FInputKeyEventArgs::CreateSimulated(EKeys::W, IE_Released, 0.0f));
				Phase = 3;
				return false;

			default:
				if (!FMath::IsNearlyZero(State.Pawn->GetTestKeyboardHorizontalInput())
					|| !FMath::IsNearlyZero(State.Pawn->GetTestKeyboardThrustInput()))
				{
					return WaitOrFail(TEXT("Released Enhanced Input remained latched on the cab."));
				}
				Test->TestTrue(
					TEXT("Releasing D clears right thrust"),
					FMath::IsNearlyZero(State.Pawn->GetTestKeyboardHorizontalInput()));
				Test->TestTrue(
					TEXT("Releasing W clears vertical thrust"),
					FMath::IsNearlyZero(State.Pawn->GetTestKeyboardThrustInput()));
				return true;
			}
		}

	private:
		bool WaitOrFail(const TCHAR* Message)
		{
			if (FPlatformTime::Seconds() < Deadline)
			{
				return false;
			}
			Test->AddError(Message);
			return true;
		}

		FAutomationTestBase* Test = nullptr;
		double Deadline = 0.0;
		int32 Phase = 0;
	};

	class FFlyingCabVerifyEnhancedInputFocusFlushCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FFlyingCabVerifyEnhancedInputFocusFlushCommand(FAutomationTestBase* InTest)
			: Test(InTest)
			, Deadline(FPlatformTime::Seconds() + FunctionalTestTimeoutSeconds)
		{
		}

		virtual bool Update() override
		{
			FFlyingCabPIEState State;
			if (!ResolvePIEState(State))
			{
				return WaitOrFail(TEXT("Enhanced Input focus-flush test could not resolve the PIE cab."));
			}

			switch (Phase)
			{
			case 0:
				State.PlayerController->StartRunMode(EFlyingCabRunMode::Freeroam);
				Test->TestTrue(
					TEXT("Simulated W press reaches the player controller before focus flush"),
					State.PlayerController->InputKey(
						FInputKeyEventArgs::CreateSimulated(EKeys::W, IE_Pressed, 1.0f)));
				Phase = 1;
				return false;

			case 1:
				if (!FMath::IsNearlyEqual(State.Pawn->GetTestKeyboardThrustInput(), 1.0f))
				{
					return WaitOrFail(TEXT("Enhanced Input did not apply W before focus flush."));
				}
				State.PlayerController->FlushPressedKeys();
				Test->TestTrue(
					TEXT("Focus flush clears stored vertical thrust synchronously"),
					FMath::IsNearlyZero(State.Pawn->GetTestKeyboardThrustInput()));
				Phase = 2;
				return false;

			default:
				if (!FMath::IsNearlyZero(State.Pawn->GetTestKeyboardThrustInput()))
				{
					return WaitOrFail(TEXT("Vertical thrust returned after the focus flush."));
				}
				Test->TestTrue(
					TEXT("Vertical thrust remains clear after input processing resumes"),
					FMath::IsNearlyZero(State.Pawn->GetTestKeyboardThrustInput()));
				return true;
			}
		}

	private:
		bool WaitOrFail(const TCHAR* Message)
		{
			if (FPlatformTime::Seconds() < Deadline)
			{
				return false;
			}
			Test->AddError(Message);
			return true;
		}

		FAutomationTestBase* Test = nullptr;
		double Deadline = 0.0;
		int32 Phase = 0;
	};

	class FFlyingCabVerifyQuestJournalInputCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FFlyingCabVerifyQuestJournalInputCommand(FAutomationTestBase* InTest)
			: Test(InTest)
			, Deadline(FPlatformTime::Seconds() + FunctionalTestTimeoutSeconds)
		{
		}

		virtual bool Update() override
		{
			FFlyingCabPIEState State;
			if (!ResolvePIEState(State))
			{
				return WaitOrFail(TEXT("Quest journal test could not resolve the PIE cab."));
			}

			switch (Phase)
			{
			case 0:
				State.PlayerController->StartRunMode(EFlyingCabRunMode::Freeroam);
				Test->TestTrue(
					TEXT("Simulated W press reaches the cab before opening the journal"),
					State.PlayerController->InputKey(
						FInputKeyEventArgs::CreateSimulated(EKeys::W, IE_Pressed, 1.0f)));
				Phase = 1;
				return false;

			case 1:
				if (!FMath::IsNearlyEqual(State.Pawn->GetTestKeyboardThrustInput(), 1.0f))
				{
					return WaitOrFail(TEXT("Enhanced Input did not apply W before opening the journal."));
				}
				Test->TestTrue(
					TEXT("J reaches the player controller"),
					State.PlayerController->InputKey(
						FInputKeyEventArgs::CreateSimulated(EKeys::J, IE_Pressed, 1.0f)));
				Phase = 2;
				return false;

			case 2:
				if (!State.PlayerController->IsQuestJournalOpen())
				{
					return WaitOrFail(TEXT("J did not open the quest journal."));
				}
				Test->TestTrue(
					TEXT("Opening the journal flushes held thrust synchronously"),
					FMath::IsNearlyZero(State.Pawn->GetTestKeyboardThrustInput()));
				Test->TestTrue(TEXT("Quest journal pauses gameplay"), State.World->IsPaused());
				if (UFlyingCabQuestSubsystem* Quests =
					State.World->GetGameInstance()->GetSubsystem<UFlyingCabQuestSubsystem>())
				{
					Quests->StartQuest(TEXT("Get_Money"));
					Quests->ClearTrackedQuest();
					UFlyingCabQuestJournalWidget* Journal =
						State.PlayerController->GetQuestJournalWidget();
					Test->TestNotNull(TEXT("The open journal widget exists"), Journal);
					if (Journal)
					{
						Test->TestTrue(TEXT("Right selects the side-quest tab"), Journal->HandleNavigationKey(EKeys::Right));
						Test->TestEqual(
							TEXT("Keyboard navigation selects Side Quest"),
							Journal->GetSelectedCategory(),
							EFlyingCabQuestCategory::Side);
						Test->TestEqual(
							TEXT("The side tab selects Jack's accepted quest"),
							Journal->GetSelectedQuestId(),
							FName(TEXT("Get_Money")));
						Test->TestTrue(TEXT("Enter toggles quest tracking"), Journal->HandleNavigationKey(EKeys::Enter));
						Test->TestEqual(
							TEXT("Enter tracks the selected side quest"),
							Quests->GetTrackedQuestId(),
							FName(TEXT("Get_Money")));
						Test->TestTrue(TEXT("Left returns to the main-quest tab"), Journal->HandleNavigationKey(EKeys::Left));
						Test->TestEqual(
							TEXT("Keyboard navigation selects Main Quest"),
							Journal->GetSelectedCategory(),
							EFlyingCabQuestCategory::Main);
						Test->TestTrue(TEXT("J closes the journal from UI navigation"), Journal->HandleNavigationKey(EKeys::J));
					}
				}
				else
				{
					Test->AddError(TEXT("Quest subsystem was unavailable during journal input test."));
					State.PlayerController->CloseQuestJournal();
				}
				Phase = 3;
				return false;

			default:
				Test->TestFalse(
					TEXT("Closing the journal restores its closed state"),
					State.PlayerController->IsQuestJournalOpen());
				Test->TestFalse(TEXT("Closing the journal resumes gameplay"), State.World->IsPaused());
				Test->TestTrue(
					TEXT("Held thrust remains clear after the journal closes"),
					FMath::IsNearlyZero(State.Pawn->GetTestKeyboardThrustInput()));
				return true;
			}
		}

	private:
		bool WaitOrFail(const TCHAR* Message)
		{
			if (FPlatformTime::Seconds() < Deadline)
			{
				return false;
			}
			Test->AddError(Message);
			return true;
		}

		FAutomationTestBase* Test = nullptr;
		double Deadline = 0.0;
		int32 Phase = 0;
	};

	class FFlyingCabVerifyRecoveryInputCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FFlyingCabVerifyRecoveryInputCommand(FAutomationTestBase* InTest)
			: Test(InTest), Deadline(FPlatformTime::Seconds() + 30.0) {}

		virtual bool Update() override
		{
			FFlyingCabPIEState State;
			if (FPlatformTime::Seconds() > Deadline)
			{
				Test->AddError(FString::Printf(TEXT("Recovery input timeout: cycle=%d phase=%d"), Cycle, Phase));
				return true;
			}
			if (!ResolvePIEState(State))
			{
				return false;
			}
			UPrimitiveComponent* Body = Cast<UPrimitiveComponent>(State.Pawn->GetRootComponent());
			UEnhancedPlayerInput* EnhancedInput = GetEnhancedPlayerInput(State.PlayerController);
			if (!Body || !EnhancedInput)
			{
				Test->AddError(TEXT("Recovery input test has no physics or Enhanced Input."));
				return true;
			}
			// Keep the real pawn ticking, but exclude incidental city collisions from
			// this input test. Destruction still goes through the actual hit delegate.
			Body->SetEnableGravity(false);
			Body->SetCollisionResponseToAllChannels(ECR_Ignore);
			HoldPawnAt(State.Pawn, FVector(-30000.0f, 0.0f, 20000.0f));
			const FKey ThrustKeys[] = {EKeys::W, EKeys::SpaceBar, EKeys::Up};
			const FKey DirectionKeys[] = {EKeys::A, EKeys::D, EKeys::Left};
			const FKey ThrustKey = ThrustKeys[Cycle];
			const FKey DirectionKey = DirectionKeys[Cycle];
			const float Direction = Cycle == 1 ? 1.0f : -1.0f;
			auto SendKeys = [&](EInputEvent Event)
			{
				const float Amount = Event == IE_Released ? 0.0f : 1.0f;
				State.PlayerController->InputKey(FInputKeyEventArgs::CreateSimulated(ThrustKey, Event, Amount));
				State.PlayerController->InputKey(FInputKeyEventArgs::CreateSimulated(DirectionKey, Event, Amount));
			};
			const FFlyingCabInputAssets& Assets = FlyingCabInputData::GetAssets();
			auto CheckNeutral = [&]()
			{
				const bool bNeutral = !State.PlayerController->IsInputKeyDown(ThrustKey)
					&& !State.PlayerController->IsInputKeyDown(DirectionKey)
					&& !EnhancedInput->GetActionValue(Assets.Thrust).Get<bool>()
					&& FMath::IsNearlyZero(EnhancedInput->GetActionValue(Assets.Horizontal).Get<float>())
					&& FMath::IsNearlyZero(State.PlayerController->GetControlFrame().Thrust)
					&& FMath::IsNearlyZero(State.PlayerController->GetControlFrame().Horizontal)
					&& FMath::IsNearlyZero(State.Pawn->GetTestKeyboardThrustInput())
					&& FMath::IsNearlyZero(State.Pawn->GetTestKeyboardHorizontalInput());
				if (!bNeutral)
				{
					Test->AddError(FString::Printf(
						TEXT("Recovery latch cycle=%d phase=%d rawT=%d rawH=%d actT=%d actH=%.2f frameT=%.2f pawnT=%.2f fuel=%.2f"),
						Cycle, Phase, State.PlayerController->IsInputKeyDown(ThrustKey),
						State.PlayerController->IsInputKeyDown(DirectionKey),
						EnhancedInput->GetActionValue(Assets.Thrust).Get<bool>(),
						EnhancedInput->GetActionValue(Assets.Horizontal).Get<float>(),
						State.PlayerController->GetControlFrame().Thrust,
						State.Pawn->GetTestKeyboardThrustInput(), State.Pawn->GetFuel()));
				}
				return bNeutral;
			};

			switch (Phase)
			{
			case 0:
				State.PlayerController->StartRunMode(EFlyingCabRunMode::Freeroam);
				Phase = 1;
				return false;
			case 1:
				SendKeys(IE_Pressed);
				Phase = 2;
				return false;
			case 2:
				if (State.Pawn->GetTestKeyboardThrustInput() < 0.5f
					|| !FMath::IsNearlyEqual(State.Pawn->GetTestKeyboardHorizontalInput(), Direction))
				{
					return false;
				}
				Body->OnComponentHit.Broadcast(Body, nullptr, nullptr,
					FVector(0.0f, 0.0f, Body->GetMass() * 1000000.0f), FHitResult());
				Test->TestTrue(TEXT("A real collision callback destroys the cab with held inputs"), State.Pawn->IsDestroyed());
				if (Cycle == 0)
				{
					SendKeys(IE_Released);
				}
				Frames = 0;
				Phase = 3;
				return false;
			case 3:
				if (State.Pawn->IsDestroyed())
				{
					if (Cycle > 0 && ++Frames % 2 == 0)
					{
						SendKeys(IE_Repeat);
					}
					return false;
				}
				Test->TestTrue(TEXT("Real timer recovery restores fuel and physics"),
					State.Pawn->GetFuel() > 0.0f && Body->IsSimulatingPhysics());
				if (Cycle > 0)
				{
					SendKeys(IE_Repeat);
				}
				Frames = 0;
				Phase = 4;
				return false;
			case 4:
				if (++Frames < 2)
				{
					return false;
				}
				SendKeys(IE_Released);
				Frames = 0;
				Phase = 5;
				return false;
			case 5:
				if (++Frames < 3)
				{
					return false;
				}
				if (!CheckNeutral()) { return true; }
				if (Frames < 13) { return false; }
				SendKeys(IE_Pressed);
				Phase = 6;
				return false;
			case 6:
				if (State.Pawn->GetTestKeyboardThrustInput() < 0.5f
					|| !FMath::IsNearlyEqual(State.Pawn->GetTestKeyboardHorizontalInput(), Direction))
				{
					return false;
				}
				SendKeys(IE_Released);
				Frames = 0;
				Phase = 7;
				return false;
			default:
				if (++Frames < 3) { return false; }
				if (!CheckNeutral()) { return true; }
				if (++Cycle == 3) { return true; }
				Phase = 1;
				return false;
			}
		}

	private:
		FAutomationTestBase* Test;
		double Deadline;
		int32 Phase = 0;
		int32 Cycle = 0;
		int32 Frames = 0;
	};

	class FFlyingCabVerifyInputTransitionChainCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FFlyingCabVerifyInputTransitionChainCommand(FAutomationTestBase* InTest)
			: Test(InTest)
			, Deadline(FPlatformTime::Seconds() + 20.0)
		{
		}

		virtual bool Update() override
		{
			FFlyingCabPIEState State;
			if (!ResolvePIEState(State))
			{
				return WaitOrFail(TEXT("Input transition-chain test could not resolve the PIE cab."));
			}

			const FFlyingCabInputAssets& InputAssets = FlyingCabInputData::GetAssets();
			UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
				State.PlayerController->GetLocalPlayer()
					? State.PlayerController->GetLocalPlayer()->GetSubsystem<
						UEnhancedInputLocalPlayerSubsystem>()
					: nullptr;

			switch (Phase)
			{
			case 0:
				State.PlayerController->StartRunMode(EFlyingCabRunMode::Freeroam);
				State.PlayerController->ToggleDeveloperObserverMode();
				Test->TestTrue(
					TEXT("Observer suppresses gameplay during the transition chain"),
					State.PlayerController->IsGameplayInputSuppressed());
				Test->TestTrue(
					TEXT("Observer does not rebuild the gameplay mapping context"),
					InputSubsystem && InputSubsystem->HasMappingContext(InputAssets.MappingContext));
				Phase = 1;
				return false;

			case 1:
				State.PlayerController->ToggleDeveloperObserverMode();
				Test->TestFalse(
					TEXT("Returning from observer restores gameplay"),
					State.PlayerController->IsGameplayInputSuppressed());
				State.Dispatch->StartPassengerMarket(true);
				Phase = 2;
				return false;

			case 2:
				if (State.Dispatch->HasPassengerOnBoard())
				{
					FVector AirborneLocation = State.Pawn->GetActorLocation();
					AirborneLocation.Z += 700.0f;
					HoldPawnAt(State.Pawn, AirborneLocation);
					Test->TestTrue(
						TEXT("A reaches the occupied airborne cab"),
						State.PlayerController->InputKey(
							FInputKeyEventArgs::CreateSimulated(EKeys::A, IE_Pressed, 1.0f)));
					Phase = 3;
					return false;
				}
				if (State.Dispatch->GetPassengerOffers().IsEmpty()
					|| !State.Dispatch->GetPassengerOffers()[0].Zone)
				{
					return WaitOrFail(TEXT("Transition-chain test did not receive a passenger offer."));
				}
				HoldPawnAt(
					State.Pawn,
					State.Dispatch->GetPassengerOffers()[0].Zone->GetActorLocation());
				return false;

			case 3:
				if (!FMath::IsNearlyEqual(State.Pawn->GetTestKeyboardHorizontalInput(), -1.0f))
				{
					return WaitOrFail(TEXT("Left input did not reach the cab before opening the journal."));
				}
				Test->TestTrue(
					TEXT("J opens the journal after observer and passenger pickup"),
					State.PlayerController->InputKey(
						FInputKeyEventArgs::CreateSimulated(EKeys::J, IE_Pressed, 1.0f)));
				Phase = 4;
				return false;

			case 4:
			{
				if (!State.PlayerController->IsQuestJournalOpen())
				{
					return WaitOrFail(TEXT("Journal did not open during the transition chain."));
				}
				Test->TestTrue(
					TEXT("Journal keeps the mapping context installed"),
					InputSubsystem && InputSubsystem->HasMappingContext(InputAssets.MappingContext));
				Test->TestTrue(
					TEXT("Journal clears the held left command"),
					FMath::IsNearlyZero(State.Pawn->GetTestKeyboardHorizontalInput()));
				if (++JournalOpenFrames < 5)
				{
					return false;
				}
				UFlyingCabQuestJournalWidget* Journal =
					State.PlayerController->GetQuestJournalWidget();
				if (!Journal)
				{
					Test->AddError(TEXT("Transition-chain test could not resolve the journal widget."));
					return true;
				}
				Journal->HandleNavigationKey(EKeys::Left);
				Journal->HandleNavigationKey(EKeys::J);
				State.PlayerController->InputKey(
					FInputKeyEventArgs::CreateSimulated(EKeys::A, IE_Released, 0.0f));
				State.PlayerController->InputKey(
					FInputKeyEventArgs::CreateSimulated(EKeys::J, IE_Released, 0.0f));
				Phase = 5;
				return false;
			}

			case 5:
				if (State.PlayerController->IsQuestJournalOpen()
					|| !FMath::IsNearlyZero(State.Pawn->GetTestKeyboardHorizontalInput())
					|| !GetEnhancedPlayerInput(State.PlayerController)
					|| !FMath::IsNearlyZero(
						GetEnhancedPlayerInput(State.PlayerController)->GetActionValue(
							InputAssets.Horizontal).Get<float>()))
				{
					return WaitOrFail(TEXT("Journal close did not return to neutral gameplay input."));
				}
				Test->TestTrue(
					TEXT("A fresh left press reaches the cab after the full transition chain"),
					State.PlayerController->InputKey(
						FInputKeyEventArgs::CreateSimulated(EKeys::A, IE_Pressed, 1.0f)));
				Phase = 6;
				return false;

			case 6:
				if (!FMath::IsNearlyEqual(State.Pawn->GetTestKeyboardHorizontalInput(), -1.0f))
				{
					return WaitOrFail(TEXT("Left input stayed blocked after closing the journal."));
				}
				State.PlayerController->InputKey(
					FInputKeyEventArgs::CreateSimulated(EKeys::A, IE_Released, 0.0f));
				Phase = 7;
				return false;

			default:
				if (!FMath::IsNearlyZero(State.Pawn->GetTestKeyboardHorizontalInput()))
				{
					return WaitOrFail(TEXT("Left input remained latched after the transition-chain release."));
				}
				Test->TestTrue(
					TEXT("The full observer-passenger-journal path ends with neutral input"),
					true);
				return true;
			}
		}

	private:
		bool WaitOrFail(const TCHAR* Message)
		{
			if (FPlatformTime::Seconds() < Deadline)
			{
				return false;
			}
			Test->AddError(Message);
			return true;
		}

		FAutomationTestBase* Test = nullptr;
		double Deadline = 0.0;
		int32 Phase = 0;
		int32 JournalOpenFrames = 0;
	};

	class FFlyingCabVerifyDeferredInputTransitionsCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FFlyingCabVerifyDeferredInputTransitionsCommand(FAutomationTestBase* InTest)
			: Test(InTest)
			, Deadline(FPlatformTime::Seconds() + 20.0)
		{
		}

		virtual bool Update() override
		{
			UWorld* World = AutomationCommon::GetAnyGameWorld();
			AFlyingCabPlayerController* PlayerController = World
				? Cast<AFlyingCabPlayerController>(World->GetFirstPlayerController())
				: nullptr;
			UEnhancedPlayerInput* EnhancedInput = GetEnhancedPlayerInput(PlayerController);
			const FFlyingCabInputAssets& InputAssets = FlyingCabInputData::GetAssets();
			if (!World || !PlayerController || !EnhancedInput || !InputAssets.IsValid())
			{
				return WaitOrFail(TEXT("Deferred input-transition test could not resolve Enhanced Input."));
			}

			AFlyingCabPawn* Vehicle = Cast<AFlyingCabPawn>(PlayerController->GetPawn());
			switch (Phase)
			{
			case 0:
				PlayerController->StartRunMode(EFlyingCabRunMode::Freeroam);
				Test->TestTrue(
					TEXT("D reaches the controller before the journal transition"),
					PlayerController->InputKey(
						FInputKeyEventArgs::CreateSimulated(EKeys::D, IE_Pressed, 1.0f)));
				Phase = 1;
				return false;

			case 1:
				if (!Vehicle
					|| !FMath::IsNearlyEqual(Vehicle->GetTestKeyboardHorizontalInput(), 1.0f)
					|| !FMath::IsNearlyEqual(
						EnhancedInput->GetActionValue(InputAssets.Horizontal).Get<float>(),
						1.0f))
				{
					return WaitOrFail(TEXT("D did not become active before opening the journal."));
				}
				Test->TestTrue(
					TEXT("J reaches the controller while D is held"),
					PlayerController->InputKey(
						FInputKeyEventArgs::CreateSimulated(EKeys::J, IE_Pressed, 1.0f)));
				Phase = 2;
				return false;

			case 2:
				if (!PlayerController->IsQuestJournalOpen())
				{
					return WaitOrFail(TEXT("Deferred J command did not open the journal."));
				}
				if (++WaitFrames < 5)
				{
					return false;
				}
				PlayerController->InputKey(
					FInputKeyEventArgs::CreateSimulated(EKeys::D, IE_Released, 0.0f));
				WaitFrames = 0;
				Phase = 3;
				return false;

			case 3:
				if (++WaitFrames < 3)
				{
					return false;
				}
				if (UFlyingCabQuestJournalWidget* Journal =
					PlayerController->GetQuestJournalWidget())
				{
					Test->TestTrue(
						TEXT("J closes the journal after the held direction was released"),
						Journal->HandleNavigationKey(EKeys::J));
				}
				else
				{
					Test->AddError(TEXT("Deferred transition test lost the journal widget."));
					return true;
				}
				PlayerController->InputKey(
					FInputKeyEventArgs::CreateSimulated(EKeys::J, IE_Released, 0.0f));
				WaitFrames = 0;
				Phase = 4;
				return false;

			case 4:
				if (PlayerController->IsQuestJournalOpen()
					|| !FMath::IsNearlyZero(
						EnhancedInput->GetActionValue(InputAssets.Horizontal).Get<float>()))
				{
					return WaitOrFail(TEXT("A direction value remained latched after closing the journal."));
				}
				Test->TestFalse(
					TEXT("Raw D is neutral after the journal transition"),
					PlayerController->IsInputKeyDown(EKeys::D));
				Test->TestTrue(
					TEXT("W reaches the controller before reset"),
					PlayerController->InputKey(
						FInputKeyEventArgs::CreateSimulated(EKeys::W, IE_Pressed, 1.0f)));
				Phase = 5;
				return false;

			case 5:
				if (!Vehicle
					|| !EnhancedInput->GetActionValue(InputAssets.Thrust).Get<bool>())
				{
					return WaitOrFail(TEXT("W did not become active before reset."));
				}
				Test->TestTrue(
					TEXT("R reaches the controller while W is held"),
					PlayerController->InputKey(
						FInputKeyEventArgs::CreateSimulated(EKeys::R, IE_Pressed, 1.0f)));
				Phase = 6;
				return false;

			case 6:
				if (PlayerController->IsInputKeyDown(EKeys::W))
				{
					return WaitOrFail(TEXT("Deferred reset did not flush the held W key."));
				}
				PlayerController->InputKey(
					FInputKeyEventArgs::CreateSimulated(EKeys::W, IE_Released, 0.0f));
				PlayerController->InputKey(
					FInputKeyEventArgs::CreateSimulated(EKeys::R, IE_Released, 0.0f));
				Phase = 7;
				return false;

			case 7:
				if (EnhancedInput->GetActionValue(InputAssets.Thrust).Get<bool>())
				{
					return WaitOrFail(TEXT("Thrust remained latched after a reset transition."));
				}
				Test->TestFalse(
					TEXT("Raw W is neutral after reset"),
					PlayerController->IsInputKeyDown(EKeys::W));
				Test->TestTrue(
					TEXT("D reaches the controller before exiting the vehicle"),
					PlayerController->InputKey(
						FInputKeyEventArgs::CreateSimulated(EKeys::D, IE_Pressed, 1.0f)));
				Phase = 8;
				return false;

			case 8:
				if (!Vehicle
					|| !FMath::IsNearlyEqual(
						EnhancedInput->GetActionValue(InputAssets.Horizontal).Get<float>(),
						1.0f))
				{
					return WaitOrFail(TEXT("D did not become active before leaving the vehicle."));
				}
				Test->TestTrue(
					TEXT("Q reaches the controller while D is held"),
					PlayerController->InputKey(
						FInputKeyEventArgs::CreateSimulated(EKeys::Q, IE_Pressed, 1.0f)));
				Phase = 9;
				return false;

			case 9:
				if (!Cast<AFlyingCabCharacter>(PlayerController->GetPawn()))
				{
					return WaitOrFail(TEXT("Deferred Q command did not possess the on-foot character."));
				}
				PlayerController->InputKey(
					FInputKeyEventArgs::CreateSimulated(EKeys::D, IE_Released, 0.0f));
				PlayerController->InputKey(
					FInputKeyEventArgs::CreateSimulated(EKeys::Q, IE_Released, 0.0f));
				WaitFrames = 0;
				Phase = 10;
				return false;

			default:
			{
				if (++WaitFrames < 5)
				{
					return false;
				}
				AFlyingCabCharacter* Character =
					Cast<AFlyingCabCharacter>(PlayerController->GetPawn());
				if (!Character
					|| !FMath::IsNearlyZero(
						EnhancedInput->GetActionValue(InputAssets.Horizontal).Get<float>())
					|| !FMath::IsNearlyZero(Character->GetTestKeyboardHorizontalInput()))
				{
					return WaitOrFail(TEXT("Horizontal input remained latched after leaving the vehicle."));
				}
				Test->TestFalse(
					TEXT("Raw D is neutral on foot"),
					PlayerController->IsInputKeyDown(EKeys::D));
				Test->TestTrue(
					TEXT("On-foot movement stays neutral after the Q transition"),
					FMath::IsNearlyZero(Character->GetLastMovementInputVector().X));
				return true;
			}
			}
		}

	private:
		bool WaitOrFail(const TCHAR* Message)
		{
			if (FPlatformTime::Seconds() < Deadline)
			{
				return false;
			}
			Test->AddError(Message);
			return true;
		}

		FAutomationTestBase* Test = nullptr;
		double Deadline = 0.0;
		int32 Phase = 0;
		int32 WaitFrames = 0;
	};

	class FFlyingCabVerifyLivingWorldCycleCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FFlyingCabVerifyLivingWorldCycleCommand(FAutomationTestBase* InTest)
			: Test(InTest)
			, Deadline(FPlatformTime::Seconds() + 30.0)
		{
		}

		virtual bool Update() override
		{
			FFlyingCabPIEState State;
			AFlyingCabWorldBootstrap* Bootstrap = nullptr;
			if (!ResolvePIEState(State)
				|| !(Bootstrap = FindWorldBootstrap(State.World))
				|| !Bootstrap->GetLivingWorldManager())
			{
				return WaitOrFail(TEXT("Living-world cycle test could not resolve the manager."));
			}
			if (!bRunStarted)
			{
				State.PlayerController->StartRunMode(EFlyingCabRunMode::Freeroam);
				bRunStarted = true;
				return false;
			}

			AFlyingCabLivingWorldManager* Manager = Bootstrap->GetLivingWorldManager();
			for (AFlyingCabTrafficVehicle* Vehicle : Manager->GetTrafficVehicles())
			{
				if (!IsValid(Vehicle))
				{
					continue;
				}
				int32 ObservedIndex = ObservedVehicles.IndexOfByPredicate(
					[Vehicle](const TWeakObjectPtr<AFlyingCabTrafficVehicle>& Candidate)
					{
						return Candidate.Get() == Vehicle;
					});
				if (ObservedIndex == INDEX_NONE)
				{
					ObservedIndex = ObservedVehicles.Add(Vehicle);
					VehicleStartLocations.Add(Vehicle->GetActorLocation());
					VehicleMaximumTravel.Add(0.0f);
				}
				VehicleMaximumTravel[ObservedIndex] = FMath::Max(
					VehicleMaximumTravel[ObservedIndex],
					FVector::Distance(
						VehicleStartLocations[ObservedIndex],
						Vehicle->GetActorLocation()));
				const FVector Velocity = Vehicle->GetTrafficVelocity();
				if (FMath::Abs(Velocity.Z) > 100.0f
					&& FMath::Abs(Velocity.Z) > FMath::Abs(Velocity.X) * 1.5f)
				{
					bObservedVerticalVehicleTravel = true;
					MaxVerticalTravelPitch = FMath::Max(
						MaxVerticalTravelPitch,
						FMath::Abs(Vehicle->GetVisualPitchDegrees()));
				}
			}
			if (Manager->GetTotalBoardings() > 0 && Manager->GetTotalPassengerExits() > 0)
			{
				Test->TestTrue(
					TEXT("At least one ambient pedestrian boards a route vehicle"),
					Manager->GetTotalBoardings() > 0);
				Test->TestTrue(
					TEXT("An ambient ride reaches its destination and the pedestrian exits"),
					Manager->GetTotalPassengerExits() > 0);
				Test->TestTrue(
					TEXT("The test observes a predominantly vertical NPC vehicle segment"),
					bObservedVerticalVehicleTravel);
				Test->TestTrue(
					TEXT("NPC body pitch stays within the player-cab presentation range during vertical travel"),
					MaxVerticalTravelPitch <= 12.5f);
				for (int32 VehicleIndex = 0; VehicleIndex < ObservedVehicles.Num(); ++VehicleIndex)
				{
					const AFlyingCabTrafficVehicle* Vehicle = ObservedVehicles[VehicleIndex].Get();
					Test->TestTrue(
						*FString::Printf(
							TEXT("Living World vehicle %s advances along its smoothed route"),
							Vehicle ? *Vehicle->GetLivingRouteId().ToString() : TEXT("invalid")),
						VehicleMaximumTravel.IsValidIndex(VehicleIndex)
							&& VehicleMaximumTravel[VehicleIndex] > 300.0f);
				}
				return true;
			}
			if (FPlatformTime::Seconds() < Deadline)
			{
				return false;
			}
			FString Details = FString::Printf(
				TEXT("Living-world cycle timed out with %d boardings and %d exits."),
				Manager->GetTotalBoardings(),
				Manager->GetTotalPassengerExits());
			for (const AFlyingCabLivingPedestrian* Pedestrian : Manager->GetPedestrians())
			{
				if (IsValid(Pedestrian))
				{
					Details += FString::Printf(
						TEXT(" Ped[%d] at %s speed %.1f wait=%s destination=%s."),
						static_cast<int32>(Pedestrian->GetLivingState()),
						*Pedestrian->GetActorLocation().ToCompactString(),
						Pedestrian->GetCurrentWalkingSpeed(),
						*Pedestrian->GetWaitingStopId().ToString(),
						*Pedestrian->GetDestinationStopId().ToString());
				}
			}
			for (const AFlyingCabTrafficVehicle* Vehicle : Manager->GetTrafficVehicles())
			{
				if (IsValid(Vehicle))
				{
					Details += FString::Printf(
						TEXT(" Vehicle[%s/%d] at %s speed %.1f stop=%s."),
						*Vehicle->GetLivingRouteId().ToString(),
						static_cast<int32>(Vehicle->GetMovementState()),
						*Vehicle->GetActorLocation().ToCompactString(),
						Vehicle->GetCurrentTrafficSpeed(),
						*Vehicle->GetCurrentLivingStopId().ToString());
				}
			}
			Test->AddError(Details);
			return true;
		}

	private:
		bool WaitOrFail(const TCHAR* Message)
		{
			if (FPlatformTime::Seconds() < Deadline)
			{
				return false;
			}
			Test->AddError(Message);
			return true;
		}

		FAutomationTestBase* Test = nullptr;
		double Deadline = 0.0;
		bool bRunStarted = false;
		bool bObservedVerticalVehicleTravel = false;
		float MaxVerticalTravelPitch = 0.0f;
		TArray<TWeakObjectPtr<AFlyingCabTrafficVehicle>> ObservedVehicles;
		TArray<FVector> VehicleStartLocations;
		TArray<float> VehicleMaximumTravel;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlyingCabEnhancedInputReleasePIETest,
	"FlyingCab.Functional.PIE.EnhancedInputRelease",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::ProductFilter)

bool FFlyingCabEnhancedInputReleasePIETest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(FlightLabMap, true))
	{
		AddError(TEXT("FlightLab map could not be opened for the Enhanced Input release test."));
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(FFlyingCabVerifyEnhancedInputReleaseCommand(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlyingCabEnhancedInputFocusFlushPIETest,
	"FlyingCab.Functional.PIE.EnhancedInputFocusFlush",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::ProductFilter)

bool FFlyingCabEnhancedInputFocusFlushPIETest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(FlightLabMap, true))
	{
		AddError(TEXT("FlightLab map could not be opened for the Enhanced Input focus-flush test."));
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(FFlyingCabVerifyEnhancedInputFocusFlushCommand(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlyingCabQuestJournalInputPIETest,
	"FlyingCab.Functional.PIE.QuestJournalInput",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::ProductFilter)

bool FFlyingCabQuestJournalInputPIETest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(FlightLabMap, true))
	{
		AddError(TEXT("FlightLab map could not be opened for the quest journal input test."));
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(FFlyingCabVerifyQuestJournalInputCommand(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlyingCabInputTransitionChainPIETest,
	"FlyingCab.Functional.PIE.InputTransitionChain",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::ProductFilter)

bool FFlyingCabInputTransitionChainPIETest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(FlightLabMap, true))
	{
		AddError(TEXT("FlightLab map could not be opened for the input transition-chain test."));
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(FFlyingCabVerifyInputTransitionChainCommand(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlyingCabDeferredInputTransitionsPIETest,
	"FlyingCab.Functional.PIE.DeferredInputTransitions",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::ProductFilter)

bool FFlyingCabDeferredInputTransitionsPIETest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(FlightLabMap, true))
	{
		AddError(TEXT("FlightLab map could not be opened for the deferred input-transition test."));
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(FFlyingCabVerifyDeferredInputTransitionsCommand(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlyingCabWorldStartupPIETest,
	"FlyingCab.Functional.PIE.WorldStartup",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::ProductFilter)

bool FFlyingCabWorldStartupPIETest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(FlightLabMap, true))
	{
		AddError(TEXT("FlightLab map could not be opened for the world startup test."));
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(FFlyingCabVerifyWorldStartupCommand(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlyingCabDeveloperObserverPIETest,
	"FlyingCab.Functional.PIE.DeveloperObserver",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::ProductFilter)

bool FFlyingCabDeveloperObserverPIETest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(FlightLabMap, true))
	{
		AddError(TEXT("FlightLab map could not be opened for the developer observer test."));
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(FFlyingCabVerifyDeveloperObserverCommand(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlyingCabLivingWorldCyclePIETest,
	"FlyingCab.Functional.PIE.LivingWorldCycle",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::ProductFilter)

bool FFlyingCabLivingWorldCyclePIETest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(FlightLabMap, true))
	{
		AddError(TEXT("FlightLab map could not be opened for the living-world cycle test."));
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(FFlyingCabVerifyLivingWorldCycleCommand(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlyingCabPassengerCoursePIETest,
	"FlyingCab.Functional.PIE.PassengerCourse",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::ProductFilter)

bool FFlyingCabPassengerCoursePIETest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(FlightLabMap, true))
	{
		AddError(TEXT("FlightLab map could not be opened for the passenger course test."));
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(FFlyingCabCompleteCourseCommand(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlyingCabRecoveryInputPIETest,
	"FlyingCab.Functional.PIE.RecoveryInputRelease",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFlyingCabRecoveryInputPIETest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(FlightLabMap, true))
	{
		AddError(TEXT("FlightLab map could not be opened for recovery input test."));
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(FFlyingCabVerifyRecoveryInputCommand(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlyingCabActiveTowPIETest,
	"FlyingCab.Functional.PIE.ActiveTowRecovery",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::ProductFilter)

bool FFlyingCabActiveTowPIETest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(FlightLabMap, true))
	{
		AddError(TEXT("FlightLab map could not be opened for the tow recovery test."));
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(FFlyingCabVerifyActiveTowCommand(this));
	return true;
}

#endif
