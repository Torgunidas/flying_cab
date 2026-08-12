// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/GameInstance.h"
#include "FlyingCabCityData.h"
#include "FlyingCabCityLayoutAsset.h"
#include "FlyingCabDispatchComponent.h"
#include "FlyingCabEconomyAsset.h"
#include "FlyingCabEconomyComponent.h"
#include "FlyingCabInputData.h"
#include "FlyingCabPlayerController.h"
#include "FlyingCabProgressionSubsystem.h"
#include "FlyingCabRunComponent.h"
#include "FlyingCabTrafficAwarenessComponent.h"
#include "FlyingCabVehicleVitalsComponent.h"
#include "FlyingCabWorldBootstrap.h"
#include "Misc/AutomationTest.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlyingCabDispatchFareTest,
	"FlyingCab.Core.Dispatch.FareCalculation",
	EAutomationTestFlags_ApplicationContextMask
		| EAutomationTestFlags::ProductFilter)

bool FFlyingCabDispatchFareTest::RunTest(const FString& Parameters)
{
	const int32 EstimatedFare = UFlyingCabDispatchComponent::CalculateEstimatedFare(
		FVector::ZeroVector,
		FVector(1000.0f, 0.0f, 0.0f),
		20.0f,
		1.1f);
	TestEqual(TEXT("A ten-meter route estimates base plus distance fare"), EstimatedFare, 31);

	const float FareAfterApproach = UFlyingCabDispatchComponent::CalculateUpdatedFare(
		20.0f,
		1000.0f,
		500.0f,
		20.0f,
		1.1f,
		0.5f);
	TestTrue(
		TEXT("Approaching the destination raises the fare at the full rate"),
		FMath::IsNearlyEqual(FareAfterApproach, 25.5f));

	const float FareAfterBacktrack = UFlyingCabDispatchComponent::CalculateUpdatedFare(
		FareAfterApproach,
		500.0f,
		700.0f,
		20.0f,
		1.1f,
		0.5f);
	TestTrue(
		TEXT("Backtracking applies the configured half-rate penalty"),
		FMath::IsNearlyEqual(FareAfterBacktrack, 24.4f));

	const float FareAtFloor = UFlyingCabDispatchComponent::CalculateUpdatedFare(
		20.0f,
		500.0f,
		5000.0f,
		20.0f,
		1.1f,
		0.5f);
	TestTrue(TEXT("Backtracking never lowers fare below base"), FMath::IsNearlyEqual(FareAtFloor, 20.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlyingCabServicePurchaseLimitsTest,
	"FlyingCab.Core.Economy.ServicePurchaseLimits",
	EAutomationTestFlags_ApplicationContextMask
		| EAutomationTestFlags::ProductFilter)

bool FFlyingCabServicePurchaseLimitsTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("The request caps the number of service units"),
		UFlyingCabEconomyComponent::CalculateServicePurchaseUnits(4, 20.0f, 100, 2),
		4);
	TestEqual(
		TEXT("Fractional resource need is rounded up to a whole service unit"),
		UFlyingCabEconomyComponent::CalculateServicePurchaseUnits(20, 8.2f, 100, 2),
		9);
	TestEqual(
		TEXT("Available credits cap the service transaction"),
		UFlyingCabEconomyComponent::CalculateServicePurchaseUnits(20, 50.0f, 15, 2),
		7);
	TestEqual(
		TEXT("A full resource rejects a purchase"),
		UFlyingCabEconomyComponent::CalculateServicePurchaseUnits(20, 0.0f, 100, 2),
		0);
	TestEqual(
		TEXT("Insufficient credits reject a purchase"),
		UFlyingCabEconomyComponent::CalculateServicePurchaseUnits(20, 50.0f, 1, 2),
		0);
	TestEqual(
		TEXT("Invalid request and price values reject a purchase"),
		UFlyingCabEconomyComponent::CalculateServicePurchaseUnits(0, 50.0f, 100, 0),
		0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlyingCabTrafficThreatPredictionTest,
	"FlyingCab.Core.Traffic.ThreatPrediction",
	EAutomationTestFlags_ApplicationContextMask
		| EAutomationTestFlags::ProductFilter)

bool FFlyingCabTrafficThreatPredictionTest::RunTest(const FString& Parameters)
{
	const FVector PawnLocation = FVector::ZeroVector;
	const FVector PawnVelocity(100.0f, 0.0f, 0.0f);
	const FFlyingCabTrafficSample Samples[] = {
		{FVector(1000.0f, 0.0f, 100.0f), FVector(-400.0f, 0.0f, 0.0f)},
		{FVector(-400.0f, 0.0f, 80.0f), FVector(400.0f, 0.0f, 0.0f)},
		{FVector(200.0f, 0.0f, 500.0f), FVector(-400.0f, 0.0f, 0.0f)}};

	const FFlyingCabTrafficThreat Threat =
		UFlyingCabTrafficAwarenessComponent::FindClosestThreat(
			PawnLocation,
			PawnVelocity,
			Samples,
			3.0f,
			260.0f);
	TestTrue(TEXT("A converging traffic sample is detected"), Threat.bFound);
	TestTrue(TEXT("The closest valid threat approaches from the left"), Threat.bFromLeft);
	TestTrue(
		TEXT("The closest impact time uses relative horizontal velocity"),
		FMath::IsNearlyEqual(Threat.ImpactTime, 4.0f / 3.0f));

	const FFlyingCabTrafficThreat NoThreat =
		UFlyingCabTrafficAwarenessComponent::FindClosestThreat(
			PawnLocation,
			PawnVelocity,
			MakeArrayView(&Samples[2], 1),
			3.0f,
			260.0f);
	TestFalse(TEXT("Traffic outside the vertical warning range is ignored"), NoThreat.bFound);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlyingCabWorldBootstrapConfigurationTest,
	"FlyingCab.Core.World.BootstrapConfiguration",
	EAutomationTestFlags_ApplicationContextMask
		| EAutomationTestFlags::ProductFilter)

bool FFlyingCabWorldBootstrapConfigurationTest::RunTest(const FString& Parameters)
{
	const TConstArrayView<FFlyingCabTrafficRouteDefinition> Routes =
		AFlyingCabWorldBootstrap::GetTrafficRoutes();
	TestEqual(TEXT("The world bootstrap exposes eight traffic routes"), Routes.Num(), 8);

	int32 ExpansionRouteCount = 0;
	for (int32 RouteIndex = 0; RouteIndex < Routes.Num(); ++RouteIndex)
	{
		const FFlyingCabTrafficRouteDefinition& Route = Routes[RouteIndex];
		TestFalse(
			*FString::Printf(TEXT("Traffic route %d has distinct endpoints"), RouteIndex),
			Route.Start.Equals(Route.End));
		TestTrue(
			*FString::Printf(TEXT("Traffic route %d has positive speed"), RouteIndex),
			Route.Speed > 0.0f);
		TestTrue(
			*FString::Printf(TEXT("Traffic route %d starts within its path"), RouteIndex),
			Route.InitialAlpha >= 0.0f && Route.InitialAlpha <= 1.0f);
		TestTrue(
			*FString::Printf(TEXT("Traffic route %d remains in the flight plane"), RouteIndex),
			FMath::IsNearlyEqual(Route.Start.Y, Route.End.Y));
		ExpansionRouteCount += FMath::Min(Route.Start.X, Route.End.X) > 5000.0f ? 1 : 0;
	}

	TestEqual(
		TEXT("Four traffic routes cover the eastern city expansion"),
		ExpansionRouteCount,
		4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlyingCabInputFocusLossTest,
	"FlyingCab.Core.Input.FocusLossFlushPolicy",
	EAutomationTestFlags_ApplicationContextMask
		| EAutomationTestFlags::ProductFilter)

bool FFlyingCabInputFocusLossTest::RunTest(const FString& Parameters)
{
	const AFlyingCabPlayerController* ControllerDefaults =
		GetDefault<AFlyingCabPlayerController>();
	TestNotNull(TEXT("Flying Cab player controller defaults are available"), ControllerDefaults);
	if (!ControllerDefaults)
	{
		return false;
	}

	TestTrue(
		TEXT("Viewport focus loss always flushes held gameplay input"),
		ControllerDefaults->ShouldFlushKeysWhenViewportFocusChanges());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlyingCabProgressionAccessTest,
	"FlyingCab.Core.Progression.AccessLifecycle",
	EAutomationTestFlags_ApplicationContextMask
		| EAutomationTestFlags::ProductFilter)

bool FFlyingCabProgressionAccessTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UFlyingCabProgressionSubsystem* Progression =
		NewObject<UFlyingCabProgressionSubsystem>(GameInstance);
	TestNotNull(TEXT("Progression subsystem can be created for an isolated test"), Progression);
	if (!Progression)
	{
		return false;
	}

	const FName ServiceAccess(TEXT("Vehicle.Service"));
	TestFalse(TEXT("No permission is granted initially"), Progression->HasAccess(ServiceAccess));
	TestFalse(TEXT("NAME_None cannot be granted"), Progression->GrantAccess(NAME_None));
	TestTrue(TEXT("A new permission is granted once"), Progression->GrantAccess(ServiceAccess));
	TestTrue(TEXT("Granted permission can be queried"), Progression->HasAccess(ServiceAccess));
	TestFalse(TEXT("Duplicate grants are rejected"), Progression->GrantAccess(ServiceAccess));

	Progression->ResetAccess();
	TestFalse(TEXT("Competitive-run reset removes session access"), Progression->HasAccess(ServiceAccess));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlyingCabRunResultAggregationTest,
	"FlyingCab.Core.Run.ResultAggregation",
	EAutomationTestFlags_ApplicationContextMask
		| EAutomationTestFlags::ProductFilter)

bool FFlyingCabRunResultAggregationTest::RunTest(const FString& Parameters)
{
	UFlyingCabRunComponent* Run = NewObject<UFlyingCabRunComponent>();
	TestNotNull(TEXT("Run component can be created for an isolated test"), Run);
	if (!Run)
	{
		return false;
	}

	bool bCompletionBroadcast = false;
	FFlyingCabTimeAttackResult CompletedResult;
	Run->OnTimeAttackCompleted.AddLambda(
		[&bCompletionBroadcast, &CompletedResult](const FFlyingCabTimeAttackResult& Result)
		{
			bCompletionBroadcast = true;
			CompletedResult = Result;
		});

	TestFalse(TEXT("A run cannot start without selecting a mode"), Run->StartRun(EFlyingCabRunMode::None));
	TestTrue(TEXT("Time Attack starts from an idle state"), Run->StartRun(EFlyingCabRunMode::TimeAttack));
	TestFalse(TEXT("A second run cannot replace an active run"), Run->StartRun(EFlyingCabRunMode::Freeroam));

	Run->RecordDelivery(84);
	Run->RecordNearMiss(3);
	Run->RecordFuelPurchase(6);
	Run->RecordRepairPurchase(4);
	Run->RecordTow(35);
	TestFalse(TEXT("Credits below the target do not finish Time Attack"), Run->CheckTimeAttackGoal(999));
	TestTrue(TEXT("Reaching the target finishes Time Attack"), Run->CheckTimeAttackGoal(1000));

	TestTrue(TEXT("Completion broadcasts the aggregated result"), bCompletionBroadcast);
	TestFalse(TEXT("A completed run is no longer active"), Run->IsRunActive());
	TestTrue(TEXT("The completed state remains available to the HUD"), Run->IsRunCompleted());
	TestEqual(TEXT("The result keeps final credits"), CompletedResult.FinalCredits, 1000);
	TestEqual(TEXT("The result keeps the configured target"), CompletedResult.TargetCredits, 1000);
	TestEqual(TEXT("The result counts completed deliveries"), CompletedResult.CompletedDeliveries, 1);
	TestEqual(TEXT("The result sums delivery income"), CompletedResult.DeliveryCreditsEarned, 84);
	TestEqual(TEXT("The result counts near misses"), CompletedResult.NearMissCount, 1);
	TestEqual(TEXT("The result sums near-miss income"), CompletedResult.NearMissCreditsEarned, 3);
	TestEqual(TEXT("The result sums fuel spending"), CompletedResult.FuelCreditsSpent, 6);
	TestEqual(TEXT("The result sums repair spending"), CompletedResult.RepairCreditsSpent, 4);
	TestEqual(TEXT("The result counts towing"), CompletedResult.TowCount, 1);
	TestEqual(TEXT("The result sums towing spending"), CompletedResult.TowCreditsSpent, 35);
	TestFalse(TEXT("Completion cannot be emitted twice"), Run->CheckTimeAttackGoal(1000));

	TArray<float> Times = {30.0f, -1.0f, 10.0f, 0.0f, 20.0f, 15.0f};
	Times = UFlyingCabRunComponent::NormalizeLeaderboard(MoveTemp(Times), 3);
	TestEqual(TEXT("The leaderboard keeps its configured capacity"), Times.Num(), 3);
	TestTrue(TEXT("The leaderboard sorts the best time first"), FMath::IsNearlyEqual(Times[0], 10.0f));
	TestTrue(TEXT("The leaderboard keeps the second-best time"), FMath::IsNearlyEqual(Times[1], 15.0f));
	TestTrue(TEXT("The leaderboard discards slower times"), FMath::IsNearlyEqual(Times[2], 20.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlyingCabVehicleVitalsLifecycleTest,
	"FlyingCab.Core.Vehicle.VitalsLifecycle",
	EAutomationTestFlags_ApplicationContextMask
		| EAutomationTestFlags::ProductFilter)

bool FFlyingCabVehicleVitalsLifecycleTest::RunTest(const FString& Parameters)
{
	UFlyingCabVehicleVitalsComponent* Vitals =
		NewObject<UFlyingCabVehicleVitalsComponent>();
	TestNotNull(TEXT("Vehicle vitals can be created for an isolated test"), Vitals);
	if (!Vitals)
	{
		return false;
	}

	FFlyingCabVehicleVitalsConfig Config;
	Config.MaxFuel = 10.0f;
	Config.StartingFuel = 5.0f;
	Config.VerticalFuelPerSecond = 2.0f;
	Config.HorizontalFuelPerSecond = 1.0f;
	Config.DescentRegenerationPerSecond = 0.5f;
	Config.RegenerationFullSpeed = 1000.0f;
	Config.MaxHull = 100.0f;
	Config.DamageImpactSpeedThreshold = 10.0f;
	Config.DamageFullHullSpeed = 20.0f;
	Config.CollisionDamageExponent = 1.0f;
	Config.CollisionDamageCooldown = 0.5f;
	Vitals->InitializeVitals(Config);

	TestTrue(TEXT("Configured resources start at their expected values"),
		FMath::IsNearlyEqual(Vitals->GetFuel(), 5.0f)
			&& FMath::IsNearlyEqual(Vitals->GetHull(), 100.0f));
	TestFalse(TEXT("Fuel above zero does not emit the empty transition"),
		Vitals->Advance(1.0f, 1.0f, 1.0f, 0.0f));
	TestTrue(TEXT("Horizontal and vertical thrust consume configured fuel"),
		FMath::IsNearlyEqual(Vitals->GetFuel(), 2.0f));
	TestTrue(TEXT("Fuel depletion emits a single empty transition"),
		Vitals->Advance(1.0f, 1.0f, 1.0f, 0.0f));
	TestFalse(TEXT("An empty reserve does not repeat its transition"),
		Vitals->Advance(0.0f, 0.0f, 0.0f, 0.0f));
	Vitals->Advance(1.0f, 0.0f, 0.0f, -1000.0f);
	TestTrue(TEXT("Descending without thrust regenerates the reserve"),
		FMath::IsNearlyEqual(Vitals->GetFuel(), 0.5f));
	TestTrue(TEXT("Refuelling returns the amount actually accepted"),
		FMath::IsNearlyEqual(Vitals->AddFuel(2.0f), 2.0f));
	Vitals->Advance(1.0f, 0.0f, 1.0f, 0.0f);
	TestTrue(TEXT("The recovery scenario begins below its guaranteed reserve"),
		FMath::IsNearlyEqual(Vitals->GetFuel(), 0.5f));

	const FFlyingCabImpactResult SafeImpact = Vitals->ApplyImpact(10.0f);
	TestTrue(TEXT("An impact at the safe threshold deals no damage"),
		FMath::IsNearlyZero(SafeImpact.Damage));
	const FFlyingCabImpactResult DestructiveImpact = Vitals->ApplyImpact(20.0f);
	TestTrue(TEXT("A full-speed impact consumes the complete hull"),
		FMath::IsNearlyEqual(DestructiveImpact.Damage, 100.0f));
	TestTrue(TEXT("Hull depletion marks the vehicle destroyed"),
		DestructiveImpact.bDestroyedNow && Vitals->IsDestroyed());
	TestTrue(TEXT("A destroyed vehicle cannot be repaired in place"),
		FMath::IsNearlyZero(Vitals->AddHull(50.0f)));

	Vitals->Recover(0.25f);
	TestFalse(TEXT("Recovery clears the destroyed state"), Vitals->IsDestroyed());
	TestTrue(TEXT("Recovery restores the hull"),
		FMath::IsNearlyEqual(Vitals->GetHullPercent(), 1.0f));
	TestTrue(TEXT("Recovery guarantees the configured fuel reserve"),
		FMath::IsNearlyEqual(Vitals->GetFuel(), 2.5f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlyingCabCityDataConsistencyTest,
	"FlyingCab.Core.CityData.LayoutConsistency",
	EAutomationTestFlags_ApplicationContextMask
		| EAutomationTestFlags::ProductFilter)

bool FFlyingCabCityDataConsistencyTest::RunTest(const FString& Parameters)
{
	const TConstArrayView<FFlyingCabDistrictDefinition> Districts =
		FlyingCabCityData::GetDistricts();
	const TArray<FFlyingCabServiceDefinition> FuelStations =
		FlyingCabCityData::GetFuelStations();
	const TArray<FFlyingCabServiceDefinition> RepairStations =
		FlyingCabCityData::GetRepairStations();
	const FVector2D WorldMin = FlyingCabCityData::GetMinimapWorldMin();
	const FVector2D WorldMax = FlyingCabCityData::GetMinimapWorldMax();

	TestEqual(TEXT("The city exposes ten passenger districts"), Districts.Num(), 10);
	TestEqual(TEXT("The city exposes three fuel stations"), FuelStations.Num(), 3);
	TestEqual(TEXT("The city exposes two repair stations"), RepairStations.Num(), 2);
	TestTrue(TEXT("Minimap X bounds are ordered"), WorldMin.X < WorldMax.X);
	TestTrue(TEXT("Minimap Z bounds are ordered"), WorldMin.Y < WorldMax.Y);

	TSet<FString> DistrictNames;
	TSet<FString> DistrictCodes;
	int32 RuntimeDistrictCount = 0;
	for (const FFlyingCabDistrictDefinition& District : Districts)
	{
		const FString Name(District.DisplayName);
		const FString Code(District.MinimapCode);
		TestTrue(*FString::Printf(TEXT("District name '%s' is unique"), *Name), !DistrictNames.Contains(Name));
		TestTrue(*FString::Printf(TEXT("District code '%s' is unique"), *Code), !DistrictCodes.Contains(Code));
		TestEqual(*FString::Printf(TEXT("District code '%s' has two characters"), *Code), Code.Len(), 2);
		DistrictNames.Add(Name);
		DistrictCodes.Add(Code);

		const FVector2D Position = District.GetMapPosition();
		TestTrue(
			*FString::Printf(TEXT("District '%s' is inside minimap bounds"), *Name),
			Position.X >= WorldMin.X && Position.X <= WorldMax.X
				&& Position.Y >= WorldMin.Y && Position.Y <= WorldMax.Y);
		RuntimeDistrictCount += District.BuildsRuntimeGeometry() ? 1 : 0;
	}
	TestEqual(TEXT("Four districts are built by the runtime east expansion"), RuntimeDistrictCount, 4);

	auto TestServiceLocations = [this, WorldMin, WorldMax](
		const TArray<FFlyingCabServiceDefinition>& Stations,
		const TCHAR* ServiceType)
	{
		for (const FFlyingCabServiceDefinition& Station : Stations)
		{
			const FVector2D Position = Station.GetMapPosition();
			TestTrue(
				*FString::Printf(TEXT("%s '%s' is inside minimap bounds"), ServiceType, *Station.DisplayName),
				Position.X >= WorldMin.X && Position.X <= WorldMax.X
					&& Position.Y >= WorldMin.Y && Position.Y <= WorldMax.Y);
		}
	};
	TestServiceLocations(FuelStations, TEXT("Fuel station"));
	TestServiceLocations(RepairStations, TEXT("Repair station"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlyingCabDataAssetValidationTest,
	"FlyingCab.Core.DataAssets.Validation",
	EAutomationTestFlags_ApplicationContextMask
		| EAutomationTestFlags::ProductFilter)

bool FFlyingCabDataAssetValidationTest::RunTest(const FString& Parameters)
{
	UFlyingCabCityLayoutAsset* CityAsset = LoadObject<UFlyingCabCityLayoutAsset>(
		nullptr,
		TEXT("/Game/Data/DA_FlyingCabCityLayout.DA_FlyingCabCityLayout"));
	TestNotNull(TEXT("The editable city layout asset can be loaded"), CityAsset);
	if (CityAsset)
	{
		FString Error;
		TestTrue(TEXT("The city layout asset passes structural validation"), CityAsset->IsConfigurationValid(Error));
		TestEqual(TEXT("The city asset contains all districts"), CityAsset->Districts.Num(), 10);
		TestEqual(TEXT("The city asset contains all traffic routes"), CityAsset->TrafficRoutes.Num(), 8);
	}
	UFlyingCabCityLayoutAsset* InvalidCity = NewObject<UFlyingCabCityLayoutAsset>();
	InvalidCity->Districts[0].RuntimePlatformHalfWidth = 10.0f;
	FString CityValidationError;
	TestFalse(
		TEXT("Mismatched runtime geometry tuning is rejected"),
		InvalidCity->IsConfigurationValid(CityValidationError));

	UFlyingCabEconomyAsset* EconomyAsset = LoadObject<UFlyingCabEconomyAsset>(
		nullptr,
		UFlyingCabEconomyAsset::GetDefaultAssetPath());
	TestNotNull(TEXT("The editable economy asset can be loaded"), EconomyAsset);
	if (EconomyAsset)
	{
		FString Error;
		TestTrue(TEXT("The economy asset passes range validation"), EconomyAsset->IsConfigurationValid(Error));
		TestEqual(TEXT("The asset owns the fuel price"), EconomyAsset->FuelPricePerUnit, 2);
		TestEqual(TEXT("The asset owns the Time Attack target"), EconomyAsset->TimeAttackTargetCredits, 1000);
	}

	UFlyingCabEconomyAsset* InvalidEconomy = NewObject<UFlyingCabEconomyAsset>();
	InvalidEconomy->FareBacktrackPenaltyRatio = 2.0f;
	FString ValidationError;
	TestFalse(
		TEXT("Out-of-range economy tuning is rejected"),
		InvalidEconomy->IsConfigurationValid(ValidationError));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlyingCabEnhancedInputAssetsTest,
	"FlyingCab.Core.Input.EnhancedAssetConfiguration",
	EAutomationTestFlags_ApplicationContextMask
		| EAutomationTestFlags::ProductFilter)

bool FFlyingCabEnhancedInputAssetsTest::RunTest(const FString& Parameters)
{
	const FFlyingCabInputAssets& Assets = FlyingCabInputData::GetAssets();
	TestTrue(TEXT("All Enhanced Input assets load from Content/Input"), Assets.IsValid());
	if (!Assets.IsValid())
	{
		return false;
	}

	TestEqual(
		TEXT("Horizontal movement uses a one-dimensional action"),
		Assets.Horizontal->ValueType,
		EInputActionValueType::Axis1D);
	TestEqual(
		TEXT("Opposite horizontal keys cancel each other"),
		Assets.Horizontal->AccumulationBehavior,
		EInputActionAccumulationBehavior::Cumulative);
	TestEqual(
		TEXT("The shared gameplay context contains every keyboard mapping"),
		Assets.MappingContext->GetMappings().Num(),
		11);

	auto HasMapping = [&Assets](
		const UInputAction* Action,
		const FKey& Key,
		bool bExpectNegate)
	{
		for (const FEnhancedActionKeyMapping& Mapping : Assets.MappingContext->GetMappings())
		{
			if (Mapping.Action != Action || Mapping.Key != Key)
			{
				continue;
			}
			const bool bHasNegate = Mapping.Modifiers.ContainsByPredicate(
				[](const UInputModifier* Modifier)
				{
					return Modifier && Modifier->IsA<UInputModifierNegate>();
				});
			return bHasNegate == bExpectNegate;
		}
		return false;
	};

	TestTrue(TEXT("A maps to negative horizontal movement"),
		HasMapping(Assets.Horizontal, EKeys::A, true));
	TestTrue(TEXT("D maps to positive horizontal movement"),
		HasMapping(Assets.Horizontal, EKeys::D, false));
	TestTrue(TEXT("W maps to thrust"), HasMapping(Assets.Thrust, EKeys::W, false));
	TestTrue(TEXT("E maps to vehicle service"), HasMapping(Assets.Service, EKeys::E, false));
	TestTrue(TEXT("R maps to vehicle reset"), HasMapping(Assets.Restart, EKeys::R, false));
	TestTrue(TEXT("F3 maps to flight telemetry"), HasMapping(Assets.Telemetry, EKeys::F3, false));
	TestTrue(TEXT("Q maps to contextual interaction"), HasMapping(Assets.Interact, EKeys::Q, false));
	return true;
}

#endif
