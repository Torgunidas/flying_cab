#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"
#include "Engine/OverlapResult.h"
#include "Components/PrimitiveComponent.h"
#include "FlyingCabCityData.h"
#include "FlyingCabCityExpansion.h"
#include "FlyingCabDispatchComponent.h"
#include "FlyingCabLivingRoute.h"
#include "FlyingCabLivingWorldProfile.h"
#include "FlyingCabLivingWorldManager.h"
#include "FlyingCabLivingPedestrian.h"
#include "FlyingCabPlayerController.h"
#include "FlyingCabTrafficVehicle.h"
#include "FlyingCabTrafficSignals.h"
#include "FlyingCabVehicleVitalsComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyingCabMetroContractTest,
	"FlyingCab.Core.City.MetroContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FFlyingCabMetroContractTest::RunTest(const FString&)
{
	const FVector2D Size = FlyingCabCityData::GetMinimapWorldMax() - FlyingCabCityData::GetMinimapWorldMin();
	TestTrue(TEXT("City area is four times the previous 20000 x 6500"),
		FMath::IsNearlyEqual(Size.X*Size.Y, 4.*20000.*6500.));
	TestEqual(TEXT("Four distinct neighborhoods"), FlyingCabCityData::GetNeighborhoods().Num(),4);
	const auto Stops = FlyingCabCityData::GetDistricts();
	for (const auto& Neighborhood : FlyingCabCityData::GetNeighborhoods())
	{
		int32 Count=0,Fuel=0,Repair=0;
		for (const auto& Stop : Stops)
		{
			if (Stop.NeighborhoodId != Neighborhood.NeighborhoodId) continue;
			++Count; Fuel += !Stop.FuelStationName.IsEmpty(); Repair += !Stop.RepairStationName.IsEmpty();
		}
		TestEqual(TEXT("Six residential taxi platforms per estate"),Count,6);
		TestEqual(TEXT("One fuel station per estate"),Fuel,1);
		const bool HasRepair = Neighborhood.NeighborhoodId == TEXT("Neighborhood.NW") || Neighborhood.NeighborhoodId == TEXT("Neighborhood.SE");
		TestEqual(TEXT("Repairs only in NW and SE"),Repair,HasRepair ? 1 : 0);
	}
	auto* Dispatch = NewObject<UFlyingCabDispatchComponent>();
	for (int32 A=0; A<Stops.Num(); ++A)
	{
		for (int32 B=0; B<Stops.Num(); ++B)
		{
			const float Rate = Stops[A].NeighborhoodId == Stops[B].NeighborhoodId ? 1.1f : 1.65f;
			TestTrue(TEXT("Inter-estate distance rate is 50% higher, local stays unchanged"),
				FMath::IsNearlyEqual(Dispatch->GetFareRateForJourney(A,B),Rate,1.e-5f));
		}
	}
	for (int32 Second=0; Second<120; ++Second)
	{
		TestFalse(TEXT("Crossing axes never receive green together"),
			FlyingCabTrafficSignals::IsGreen(TEXT("Horizontal"),Second) && FlyingCabTrafficSignals::IsGreen(TEXT("Vertical"),Second));
	}
	TestTrue(TEXT("Horizontal starts green"),FlyingCabTrafficSignals::IsGreen(TEXT("Horizontal"),0));
	TestFalse(TEXT("All-red interval clears horizontal traffic"),FlyingCabTrafficSignals::IsGreen(TEXT("Vertical"),18));
	TestTrue(TEXT("Vertical gets a turn"),FlyingCabTrafficSignals::IsGreen(TEXT("Vertical"),24));
	auto* Profile = UFlyingCabLivingWorldProfile::LoadDefaultAsset();
	if (TestNotNull(TEXT("Editable metro traffic profile exists"),Profile))
	{
		FString Error;
		TestTrue(*FString::Printf(TEXT("Traffic profile validates: %s"),*Error),Profile->IsConfigurationValid(Error));
		TestEqual(TEXT("40 vehicles"),UFlyingCabLivingWorldProfile::CountAgents(Profile->Routes,EFlyingCabLivingAgentKind::Vehicle),40);
		TestEqual(TEXT("8 pedestrians"),UFlyingCabLivingWorldProfile::CountAgents(Profile->Routes,EFlyingCabLivingAgentKind::Pedestrian),8);
	}
	auto* Vitals = NewObject<UFlyingCabVehicleVitalsComponent>();
	FFlyingCabVehicleVitalsConfig Config;
	Vitals->InitializeVitals(Config); Vitals->ResetResources();
	const auto Impact = Vitals->ApplyImpact(1400);
	TestFalse(TEXT("An impact that previously destroyed a new cab is now survivable"),Impact.bDestroyedNow);
	TestTrue(TEXT("At least 70 percent hull survives the former one-hit kill speed"),Vitals->GetHullPercent()>.7f);
	return true;
}

namespace
{
class FMetroFlowCommand final : public IAutomationLatentCommand
{
public:
	explicit FMetroFlowCommand(FAutomationTestBase* InTest) : Test(InTest) {}
	virtual bool Update() override
	{
		UWorld* World = AutomationCommon::GetAnyGameWorld();
		auto* PC = World ? Cast<AFlyingCabPlayerController>(World->GetFirstPlayerController()) : nullptr;
		AFlyingCabLivingWorldManager* Manager = nullptr;
		if (World)
		{
			TActorIterator<AFlyingCabLivingWorldManager> It(World);
			if (It) Manager = *It;
		}
		if (!Manager || !PC)
		{
			if (FPlatformTime::Seconds()-WallStart<20) return false;
			Test->AddError(TEXT("Metro test could not resolve PIE world")); return true;
		}
		if (!bStarted)
		{
			PC->StartRunMode(EFlyingCabRunMode::Freeroam);
			StartTime = World->GetTimeSeconds(); bStarted=true;
			World->GetWorldSettings()->SetTimeDilation(4.f);
			// Actual smoothed spline, full upright vehicle envelope, actual generated geometry.
			FCollisionObjectQueryParams Types; Types.AddObjectTypesToQuery(ECC_WorldStatic); Types.AddObjectTypesToQuery(ECC_WorldDynamic);
			int32 Obstructed=0;
			for (const AFlyingCabLivingRoute* Route : Manager->GetRoutes())
			{
				if (Route->GetAgentKind()!=EFlyingCabLivingAgentKind::Vehicle) continue;
				for (float Distance=0; Distance<Route->GetRouteLength(); Distance+=100.f)
				{
					TArray<FOverlapResult> Hits;
					World->OverlapMultiByObjectType(Hits,Route->GetWorldLocationAtDistance(Distance),FQuat::Identity,
						Types,FCollisionShape::MakeBox(FVector(135,55,48)));
					for (const auto& Hit : Hits)
					{
						if (!Cast<AFlyingCabCityExpansion>(Hit.GetActor())) continue;
						if (Obstructed++<12) Test->AddError(FString::Printf(TEXT("Road intersects city: %s at %s (%s)"),
							*Route->GetRouteId().ToString(),*Route->GetWorldLocationAtDistance(Distance).ToCompactString(),
							*GetNameSafe(Hit.GetComponent())));
					}
				}
			}
			Test->TestEqual(TEXT("All NPC routes are clear of solid city geometry"),Obstructed,0);
		}
		const float Now = World->GetTimeSeconds();
		for (AFlyingCabLivingPedestrian* Pedestrian : Manager->GetPedestrians())
		{
			if (Pedestrian->GetLivingState()==EFlyingCabPedestrianState::Riding) HasRidden.Add(Pedestrian);
			else if (HasRidden.Contains(Pedestrian)) HasExited.Add(Pedestrian);
		}
		for (AFlyingCabTrafficVehicle* Vehicle : Manager->GetTrafficVehicles())
		{
			FProgress* Progress = Observed.Find(Vehicle);
			if (!Progress) { Observed.Add(Vehicle,{Vehicle->GetActorLocation(),Now,0,false}); continue; }
			const float Travel = FVector::Distance(Progress->Position,Vehicle->GetActorLocation());
			if (Travel>=100.f) { Progress->Distance+=Travel; Progress->Position=Vehicle->GetActorLocation(); Progress->LastAdvance=Now; }
			if (Now-Progress->LastAdvance>45.f && !Progress->bReported)
			{
				Progress->bReported=true;
				Test->AddError(FString::Printf(TEXT("Traffic stalled >45 game seconds: %s at %s state=%d"),
					*Vehicle->GetLivingRouteId().ToString(),*Vehicle->GetActorLocation().ToCompactString(),int32(Vehicle->GetMovementState())));
				Test->AddInfo(FString::Printf(TEXT("Last obstacle: %s"),*GetNameSafe(Vehicle->GetLastLivingObstacle())));
				for (TActorIterator<AActor> It(World); It; ++It)
				{
					if (FVector::Dist(It->GetActorLocation(),Vehicle->GetActorLocation())>1600) continue;
					if (auto* V=Cast<AFlyingCabTrafficVehicle>(*It)) Test->AddInfo(FString::Printf(
						TEXT("Neighbor %s %s state=%d speed=%.1f stop=%s"),*V->GetName(),*V->GetActorLocation().ToCompactString(),
						int32(V->GetMovementState()),V->GetCurrentTrafficSpeed(),*V->GetCurrentLivingStopId().ToString()));
					if (auto* P=Cast<AFlyingCabLivingPedestrian>(*It)) Test->AddInfo(FString::Printf(
						TEXT("Ped %s %s state=%d wait=%s"),*P->GetName(),*P->GetActorLocation().ToCompactString(),int32(P->GetLivingState()),*P->GetWaitingStopId().ToString()));
				}
			}
		}
		if (Now-StartTime<180.f && FPlatformTime::Seconds()-WallStart<75) return false;
		World->GetWorldSettings()->SetTimeDilation(1.f);
		Test->TestTrue(TEXT("Three game minutes of live traffic simulated"),Now-StartTime>=180.f);
		Test->TestTrue(TEXT("Ambient passengers board across the city"),Manager->GetTotalBoardings()>=4);
		Test->TestTrue(TEXT("Ambient passengers disembark"),Manager->GetTotalPassengerExits()>=4);
		Test->TestEqual(TEXT("Every pedestrian boards during the session"),HasRidden.Num(),8);
		Test->TestEqual(TEXT("Every pedestrian completes at least one ride"),HasExited.Num(),8);
		for (const auto& Pair : Observed)
			Test->TestTrue(*FString::Printf(TEXT("Vehicle %s travels at least 100 metres"),*Pair.Key->GetName()),Pair.Value.Distance>10000.f);
		return true;
	}
private:
	struct FProgress { FVector Position; float LastAdvance; float Distance; bool bReported; };
	TMap<AFlyingCabTrafficVehicle*,FProgress> Observed;
	TSet<AFlyingCabLivingPedestrian*> HasRidden;
	TSet<AFlyingCabLivingPedestrian*> HasExited;
	FAutomationTestBase* Test;
	double WallStart = FPlatformTime::Seconds();
	float StartTime=0;
	bool bStarted=false;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyingCabMetroFlowTest,
	"FlyingCab.Functional.PIE.MetroTrafficFlow",EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FFlyingCabMetroFlowTest::RunTest(const FString&)
{
	if (!AutomationOpenMap(TEXT("/Game/Maps/FlightLab"),true)) return false;
	ADD_LATENT_AUTOMATION_COMMAND(FMetroFlowCommand(this));
	return true;
}
#endif
