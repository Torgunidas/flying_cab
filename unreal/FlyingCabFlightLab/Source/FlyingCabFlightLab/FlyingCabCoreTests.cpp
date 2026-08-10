// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/GameInstance.h"
#include "FlyingCabCityData.h"
#include "FlyingCabDispatchComponent.h"
#include "FlyingCabPlayerController.h"
#include "FlyingCabProgressionSubsystem.h"
#include "FlyingCabRunComponent.h"
#include "FlyingCabTrafficAwarenessComponent.h"
#include "FlyingCabWorldBootstrap.h"
#include "Misc/AutomationTest.h"

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
				*FString::Printf(TEXT("%s '%s' is inside minimap bounds"), ServiceType, Station.DisplayName),
				Position.X >= WorldMin.X && Position.X <= WorldMax.X
					&& Position.Y >= WorldMin.Y && Position.Y <= WorldMax.Y);
		}
	};
	TestServiceLocations(FuelStations, TEXT("Fuel station"));
	TestServiceLocations(RepairStations, TEXT("Repair station"));
	return true;
}

#endif
