#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "EngineUtils.h"
#include "Components/PrimitiveComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "FlyingCabCityData.h"
#include "FlyingCabCharacter.h"
#include "FlyingCabCityExpansion.h"
#include "FlyingCabCityLayoutAsset.h"
#include "FlyingCabEconomyAsset.h"
#include "FlyingCabEconomyComponent.h"
#include "FlyingCabPawn.h"
#include "FlyingCabPlayerController.h"
#include "FlyingCabQuestHubData.h"
#include "FlyingCabVehicleVitalsComponent.h"
#include "ImageUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/BufferArchive.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyingCabResidentialContractTest,
	"FlyingCab.Core.City.ResidentialContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FFlyingCabResidentialContractTest::RunTest(const FString&)
{
	auto* Vitals = NewObject<UFlyingCabVehicleVitalsComponent>();
	Vitals->InitializeVitals(FFlyingCabVehicleVitalsConfig());
	TestEqual(TEXT("Capacity is 200 fuel units"),Vitals->GetMaxFuel(),200.f);
	TestEqual(TEXT("Starting reserve doubles to 130"),Vitals->GetFuel(),130.f);
	TestTrue(TEXT("Starting HUD still reads 65 percent"),FMath::IsNearlyEqual(Vitals->GetFuelPercent(),.65f));
	TestEqual(TEXT("Filling is capped at remaining tank space"),Vitals->AddFuel(1000),70.f);
	Vitals->Advance(30,1,1,0);
	TestTrue(TEXT("Bigger tank does not change fuel consumption"),FMath::IsNearlyEqual(Vitals->GetFuel(),139.25f));
	Vitals->ResetResources();
	TestEqual(TEXT("Reset restores new starting reserve"),Vitals->GetFuel(),130.f);
	Vitals->Advance(10000,1,1,0);
	Vitals->ApplyImpact(10000);
	Vitals->Recover(.25f);
	TestEqual(TEXT("Empty wreck recovery provides 25 percent of larger tank"),Vitals->GetFuel(),50.f);
	const auto* Economy = UFlyingCabEconomyAsset::LoadDefaultAsset();
	if (TestNotNull(TEXT("Economy asset exists"),Economy))
		TestEqual(TEXT("Unit price did not increase"),Economy->FuelPricePerUnit,2);
	TestEqual(TEXT("Fuel purchase still respects the starting balance"),
		UFlyingCabEconomyComponent::CalculateServicePurchaseUnits(70,70,100,2),50);
	for (const auto& District : FlyingCabCityData::GetDistricts())
	{
		const FVector Pickup = FlyingCabCityData::GetPassengerPickupLocation(District.StopLocation);
		const FVector Dropoff = FlyingCabCityData::GetPassengerDropoffLocation(District.StopLocation);
		TestTrue(TEXT("Opposite curbs separated by 1640 cm"),FMath::IsNearlyEqual(Dropoff.X-Pickup.X,1640.));
		const double HalfWidth = FMath::Max(District.RuntimePlatformHalfWidth,22.f)*50.;
		TestTrue(TEXT("Both 520 cm wide curb zones fit on platform"),Dropoff.X-District.StopLocation.X+260 <= HalfWidth);
		for (const auto& Strip : FlyingCabCityData::GetHighwayStrips())
		{
			// Complete solid building and platform envelope stays out of highway strips.
			const FVector2D Center(District.StopLocation.X,District.StopLocation.Z+(District.ResidentialTowerHeight-480)*.5);
			const FVector2D Extent(HalfWidth,(District.ResidentialTowerHeight+180)*.5);
			const bool bOverlap = FMath::Abs(Center.X-Strip.Center.X)<Extent.X+Strip.HalfSize.X
				&& FMath::Abs(Center.Y-Strip.Center.Y)<Extent.Y+Strip.HalfSize.Y;
			TestFalse(*FString::Printf(TEXT("%s does not obstruct %s"),*District.DisplayName,Strip.Name),bOverlap);
		}
	}
	return true;
}

namespace
{
class FResidentialAccessCommand final : public IAutomationLatentCommand
{
public:
	explicit FResidentialAccessCommand(FAutomationTestBase* InTest) : Test(InTest) {}
	virtual bool Update() override
	{
		UWorld* World = AutomationCommon::GetAnyGameWorld();
		auto* PC = World ? Cast<AFlyingCabPlayerController>(World->GetFirstPlayerController()) : nullptr;
		AFlyingCabCityExpansion* City = nullptr;
		if (World) for (TActorIterator<AFlyingCabCityExpansion> It(World); It; ++It) { City=*It; break; }
		if (!City || !PC)
		{
			if (FPlatformTime::Seconds()-Started<15) return false;
			Test->AddError(TEXT("Residential city did not finish loading")); return true;
		}
		PC->StartRunMode(EFlyingCabRunMode::Freeroam);
		const auto* FootCapsule = GetDefault<AFlyingCabCharacter>()->GetCapsuleComponent();
		const FVector FootExtent(FootCapsule->GetUnscaledCapsuleRadius(),FootCapsule->GetUnscaledCapsuleRadius(),
			FootCapsule->GetUnscaledCapsuleHalfHeight());
		// Ignore ambient agents: validate static access geometry, not momentary traffic occupancy.
		FCollisionQueryParams Params;
		for (TActorIterator<AActor> It(World); It; ++It) if (*It!=City) Params.AddIgnoredActor(*It);
		auto Clear = [&](const FString& Name, const FVector& From, const FVector& To, const FVector& Extent)
		{
			FHitResult Hit;
			const bool bBlocked = World->SweepSingleByChannel(Hit,From,To,FQuat::Identity,ECC_Pawn,
				FCollisionShape::MakeBox(Extent),Params);
			Test->TestFalse(*FString::Printf(TEXT("Clear %s (obstacle: %s)"),*Name,*GetNameSafe(Hit.GetComponent())),bBlocked);
		};
		auto Support = [&](const FString& Name, const FVector& Point)
		{
			FHitResult Hit;
			Test->TestTrue(*FString::Printf(TEXT("Solid landing floor at %s"),*Name),World->LineTraceSingleByChannel(
				Hit,Point,Point-FVector(0,0,220),ECC_Pawn,Params));
		};
		for (const auto& District : FlyingCabCityData::GetDistricts())
		{
			for (bool bRight : {false,true})
			{
				const FVector P = bRight ? FlyingCabCityData::GetPassengerDropoffLocation(District.StopLocation)
					: FlyingCabCityData::GetPassengerPickupLocation(District.StopLocation);
				const FString Name = District.MinimapCode+(bRight ? TEXT(" dropoff") : TEXT(" pickup"));
				Clear(Name+TEXT(" vertical approach"),P+FVector(0,0,600),P,FVector(135,55,48));
				Clear(Name+TEXT(" outside approach"),P+FVector(bRight ? 600 : -600,0,0),P,FVector(135,55,48));
				Clear(Name+TEXT(" landed envelope"),P-FVector(0,0,105),P-FVector(0,0,104),FVector(110,45,35));
				Support(Name,P);
				const FVector Entry = FlyingCabCityData::GetResidentialEntranceLocation(District,bRight);
				Clear(Name+TEXT(" future foot entrance"),Entry+FVector(0,0,10),Entry+FVector(0,0,11),FootExtent);
				Test->TestTrue(TEXT("Future entrance matches standing character height"),
					FMath::IsNearlyEqual(Entry.Z-FootExtent.Z,District.StopLocation.Z-150.));
			}
			FHitResult Wall;
			const FVector Center = District.StopLocation+FVector(0,0,District.ResidentialTowerHeight*.5-150);
			Test->TestTrue(TEXT("Residential towers physically block crossing flights"),World->LineTraceSingleByChannel(
				Wall,Center-FVector(700,0,0),Center+FVector(700,0,0),ECC_Pawn,Params));
		}
		TArray<FFlyingCabServiceDefinition> Services = FlyingCabCityData::GetFuelStations();
		Services.Append(FlyingCabCityData::GetRepairStations());
		for (const auto& Service : Services)
		{
			Clear(Service.DisplayName,Service.Location+FVector(0,0,600),Service.Location,FVector(135,55,48));
			Clear(Service.DisplayName+TEXT(" landed"),Service.Location-FVector(0,0,105),
				Service.Location-FVector(0,0,104),FVector(110,45,35));
			Support(Service.DisplayName,Service.Location);
		}
		for (const auto& Hub : FlyingCabQuestHubData::GetQuestHubs())
			Clear(Hub.DisplayName+TEXT(" plaza"),Hub.WorldLocation,Hub.WorldLocation+FVector(0,0,300),FVector(110,45,35));
		TArray<USceneComponent*> Components;
		City->GetComponents(Components);
		int32 Entrances=0;
		for (auto* Component : Components) Entrances += Component->ComponentHasTag(TEXT("FutureResidentialInterior"));
		Test->TestEqual(TEXT("Two stable future entrance anchors on all 24 platforms"),Entrances,48);
		if (FParse::Param(FCommandLine::Get(),TEXT("FlyingCabCaptureResidential")))
		{
			auto* Actor = World->SpawnActor<ASceneCapture2D>();
			auto* Capture = Actor->GetCaptureComponent2D();
			auto* Target = NewObject<UTextureRenderTarget2D>();
			Target->InitCustomFormat(1600,700,PF_B8G8R8A8,false);
			Capture->TextureTarget = Target;
			Capture->ProjectionType = ECameraProjectionMode::Orthographic;
			Capture->OrthoWidth = 16000;
			Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
			Capture->bCaptureEveryFrame = false;
			Actor->SetActorLocationAndRotation(FVector(-4500,30000,3500),FRotator(0,-90,0));
			Capture->CaptureScene();
			FBufferArchive Png;
			const FString Path = FPaths::ProjectSavedDir()/TEXT("Automation/ResidentialNeighborhood.png");
			Test->TestTrue(TEXT("Residential preview saved"),FImageUtils::ExportRenderTarget2DAsPNG(Target,Png)
				&& FFileHelper::SaveArrayToFile(Png,*Path));
			auto* DetailTarget = NewObject<UTextureRenderTarget2D>();
			DetailTarget->InitCustomFormat(1400,900,PF_B8G8R8A8,false);
			Capture->TextureTarget = DetailTarget;
			Capture->OrthoWidth = 3400;
			Actor->SetActorLocationAndRotation(FVector(-500,10000,3900),FRotator(0,-90,0));
			Capture->CaptureScene();
			FBufferArchive DetailPng;
			const FString DetailPath = FPaths::ProjectSavedDir()/TEXT("Automation/ResidentialPlatform.png");
			Test->TestTrue(TEXT("Platform detail saved"),FImageUtils::ExportRenderTarget2DAsPNG(DetailTarget,DetailPng)
				&& FFileHelper::SaveArrayToFile(DetailPng,*DetailPath));
			Actor->Destroy();
		}
		return true;
	}
private:
	FAutomationTestBase* Test;
	double Started=FPlatformTime::Seconds();
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyingCabResidentialAccessTest,
	"FlyingCab.Functional.PIE.ResidentialAccess", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FFlyingCabResidentialAccessTest::RunTest(const FString&)
{
	if (!AutomationOpenMap(TEXT("/Game/Maps/FlightLab"),true)) { AddError(TEXT("Cannot open FlightLab")); return false; }
	ADD_LATENT_AUTOMATION_COMMAND(FResidentialAccessCommand(this));
	return true;
}
#endif
