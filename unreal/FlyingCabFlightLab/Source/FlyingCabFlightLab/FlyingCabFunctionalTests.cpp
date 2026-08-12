// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "EnhancedInputSubsystems.h"
#include "Components/PrimitiveComponent.h"
#include "EngineUtils.h"
#include "FlyingCabAccessTerminal.h"
#include "FlyingCabCityExpansion.h"
#include "FlyingCabDeliveryZone.h"
#include "FlyingCabDispatchComponent.h"
#include "FlyingCabFuelStation.h"
#include "FlyingCabGameMode.h"
#include "FlyingCabInputData.h"
#include "FlyingCabNightshiftOffice.h"
#include "FlyingCabOnFootPortal.h"
#include "FlyingCabPawn.h"
#include "FlyingCabPlayerController.h"
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
			Test->TestEqual(
				TEXT("World bootstrap creates the expected traffic fleet"),
				Bootstrap->GetTrafficVehicles().Num(),
				AFlyingCabWorldBootstrap::GetExpectedTrafficVehicleCount());
			Test->TestEqual(
				TEXT("All traffic actors exist in the PIE world"),
				CountActors<AFlyingCabTrafficVehicle>(State.World),
				AFlyingCabWorldBootstrap::GetExpectedTrafficVehicleCount());
			Test->TestEqual(TEXT("Three fuel stations are present"), Bootstrap->GetFuelStationCount(), 3);
			Test->TestEqual(TEXT("Two repair stations are present"), Bootstrap->GetRepairStationCount(), 2);
			Test->TestEqual(TEXT("One city expansion actor is present"), CountActors<AFlyingCabCityExpansion>(State.World), 1);
			Test->TestEqual(TEXT("Two on-foot portals are present"), CountActors<AFlyingCabOnFootPortal>(State.World), 2);
			Test->TestEqual(TEXT("One access terminal is present"), CountActors<AFlyingCabAccessTerminal>(State.World), 1);
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
				State.PlayerController->StartRunMode(EFlyingCabRunMode::TimeAttack);
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
				const FFlyingCabImpactResult Impact = Vitals->ApplyImpact(1000000.0f);
				Test->TestTrue(TEXT("The forced impact destroys the active cab"), Impact.bDestroyedNow);
				State.Pawn->OnVehicleDestroyed.Broadcast(State.Pawn);

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
