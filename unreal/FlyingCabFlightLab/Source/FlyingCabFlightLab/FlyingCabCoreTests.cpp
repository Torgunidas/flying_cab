// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/GameInstance.h"
#include "FlyingCabCityData.h"
#include "FlyingCabPlayerController.h"
#include "FlyingCabProgressionSubsystem.h"
#include "FlyingCabTrafficAwarenessComponent.h"
#include "Misc/AutomationTest.h"

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
