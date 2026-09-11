#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "FlyingCabCityData.h"
#include "FlyingCabHighwayAssistComponent.h"
#include "FlyingCabHighwayTile.h"
#include "FlyingCabHighwayMapLayer.h"
#include "FlyingCabTouchControls.h"
#include "Blueprint/WidgetTree.h"
#include "Slate/WidgetRenderer.h"
#include "UObject/UObjectIterator.h"
#include "FlyingCabPawn.h"
#include "FlyingCabPlayerController.h"
#include "FlyingCabVehicleVitalsComponent.h"
#include "ImageUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/BufferArchive.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyingCabHighwayContractTest,
	"FlyingCab.Core.City.HighwayTurbo", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FFlyingCabHighwayContractTest::RunTest(const FString&)
{
	for (const auto& Strip : FlyingCabCityData::GetHighwayStrips())
	{
		const FVector Center(Strip.Center.X,0,Strip.Center.Y);
		TestTrue(TEXT("Each visual strip is a turbo zone"),FlyingCabCityData::IsOnHighway(Center));
		const FVector Across = Strip.HalfSize.X > Strip.HalfSize.Y ? FVector(0,0,1) : FVector(1,0,0);
		const double HalfWidth = FMath::Min(Strip.HalfSize.X,Strip.HalfSize.Y);
		for (double Sign : {-1.,1.})
		{
			TestTrue(TEXT("Visible edge is inside"),FlyingCabCityData::IsOnHighway(Center+Across*(HalfWidth*Sign)));
			// Central strips overlap at their centers; sample their quarter length instead.
			const FVector Along = FVector(Across.Z,0,Across.X);
			const FVector Sample = Center+Along*(FMath::Max(Strip.HalfSize.X,Strip.HalfSize.Y)*.5);
			TestFalse(TEXT("Outside visible lane gets no bonus"),FlyingCabCityData::IsOnHighway(Sample+Across*((HalfWidth+1)*Sign)));
		}
		TestFalse(TEXT("Background depth is not a gameplay lane"),FlyingCabCityData::IsOnHighway(Center+FVector(0,400,0)));
	}
	for (const auto& District : FlyingCabCityData::GetDistricts())
	{
		TestFalse(TEXT("No turbo on passenger or service pads"),FlyingCabCityData::IsOnHighway(District.StopLocation));
	}
	TestFalse(TEXT("Spawn is not a highway"),FlyingCabCityData::IsOnHighway(FVector(-2200,0,50)));
	TestFalse(TEXT("Separate interior gets no bonus"),FlyingCabCityData::IsOnHighway(FVector(50000,0,650)));
	auto* Vitals = NewObject<UFlyingCabVehicleVitalsComponent>();
	Vitals->InitializeVitals(FFlyingCabVehicleVitalsConfig());
	Vitals->Advance(10,1,1,0,.5f);
	TestTrue(TEXT("Turbo halves actual two-axis fuel use"),FMath::IsNearlyEqual(Vitals->GetFuel(),119.875f));
	Vitals->Advance(1,0,0,-900,.5f);
	TestTrue(TEXT("Turbo does not multiply regeneration"),FMath::IsNearlyEqual(Vitals->GetFuel(),119.995f,1.e-4f));
	Vitals->Advance(1000,1,1,0,.5f);
	TestFalse(TEXT("Turbo never bypasses an empty tank"),Vitals->CanUseThrusters());
	return true;
}

namespace
{
class FHighwayTurboCommand final : public IAutomationLatentCommand
{
public:
	explicit FHighwayTurboCommand(FAutomationTestBase* InTest) : Test(InTest) {}
	virtual bool Update() override
	{
		UWorld* World = AutomationCommon::GetAnyGameWorld();
		auto* PC = World ? Cast<AFlyingCabPlayerController>(World->GetFirstPlayerController()) : nullptr;
		auto* Cab = PC ? Cast<AFlyingCabPawn>(PC->GetPawn()) : nullptr;
		if (!Cab)
		{
			if (FPlatformTime::Seconds()-Started < 15.) return false;
			Test->AddError(TEXT("No playable cab for highway test")); return true;
		}
		PC->StartRunMode(EFlyingCabRunMode::Freeroam);
		auto* Assist = Cab->FindComponentByClass<UFlyingCabHighwayAssistComponent>();
		auto* Body = Cast<UPrimitiveComponent>(Cab->GetRootComponent());
		auto* Vitals = Cab->FindComponentByClass<UFlyingCabVehicleVitalsComponent>();
		if (!Test->TestNotNull(TEXT("Live Blueprint inherits highway component"),Assist) || !Body || !Vitals) return true;
		Cab->ResetVehicle();
		const auto Strip = FlyingCabCityData::GetHighwayStrips()[0];
		const FVector Highway(Strip.Center.X-5000,0,Strip.Center.Y);
		Cab->SetActorLocation(Highway,false,nullptr,ETeleportType::TeleportPhysics);
		Assist->Advance(.1f);
		Test->TestTrue(TEXT("Entry is gradual"),Assist->GetSpeedMultiplier()>1 && Assist->GetSpeedMultiplier()<1.5f);
		Assist->Advance(.4f);
		Test->TestTrue(TEXT("Full bonus is 50 percent more speed"),FMath::IsNearlyEqual(Assist->GetSpeedMultiplier(),1.5f));
		Cab->SetTouchHorizontalInput(1);
		Cab->SetTouchThrustPressed(true);
		Body->SetPhysicsLinearVelocity(FVector(4000,0,4000));
		const float FuelBefore = Cab->GetFuel();
		Cab->Tick(.1f);
		Test->TestTrue(TEXT("Live cab uses half fuel"),FMath::IsNearlyEqual(FuelBefore-Cab->GetFuel(),.10125f,1.e-4f));
		Test->TestTrue(TEXT("Horizontal and climb speed caps are raised"),
			Body->GetPhysicsLinearVelocity().Equals(FVector(1575,0,1725),.1));
		Body->SetPhysicsLinearVelocity(FVector(-4000,0,-4000));
		Cab->Tick(.016f);
		Test->TestTrue(TEXT("Leftward and downward limits also receive bonus"),
			Body->GetPhysicsLinearVelocity().Equals(FVector(-1575,0,-1950),.1));
		Cab->SetTouchHorizontalInput(0);
		Cab->SetTouchThrustPressed(false);
		Body->SetPhysicsLinearVelocity(FVector(1500,0,0));
		const float FuelOnRelease = Cab->GetFuel();
		Cab->Tick(.016f);
		Test->TestTrue(TEXT("Release immediately stops fuel burn; turbo cannot hold thrust"),FMath::IsNearlyEqual(FuelOnRelease,Cab->GetFuel()));
		Test->TestTrue(TEXT("Ordinary release braking stays enabled"),Body->GetPhysicsLinearVelocity().X < 1500);
		TArray<UTextRenderComponent*> Labels;
		Cab->GetComponents(Labels);
		UTextRenderComponent* TurboLabel = nullptr;
		for (auto* Label : Labels) if (Label->GetFName()==TEXT("HighwayTurboLabel")) TurboLabel=Label;
		Test->TestTrue(TEXT("Turbo is explicitly labelled on the vehicle"),TurboLabel && TurboLabel->IsVisible());
		TArray<UStaticMeshComponent*> Meshes;
		Cab->GetComponents(Meshes);
		int32 VisibleStreaks = 0;
		for (auto* Mesh : Meshes) if (Mesh->GetName().StartsWith(TEXT("HighwayStreak")))
		{
			VisibleStreaks += Mesh->IsVisible();
			Test->TestEqual(TEXT("Effect never adds collision"),Mesh->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
		}
		Test->TestEqual(TEXT("Three speed streaks are visible"),VisibleStreaks,3);
		if (FParse::Param(FCommandLine::Get(),TEXT("FlyingCabCaptureTurbo"))) Capture(World,Highway);
		Cab->SetActorLocation(Highway+FVector(0,0,800),false,nullptr,ETeleportType::TeleportPhysics);
		Assist->Advance(.1f);
		Test->TestTrue(TEXT("Exit eases back to normal speed"),Assist->GetSpeedMultiplier()>1 && Assist->GetSpeedMultiplier()<1.5f);
		Assist->Advance(.7f);
		Test->TestTrue(TEXT("Exit restores unmodified speed and fuel"),FMath::IsNearlyEqual(Assist->GetSpeedMultiplier(),1.f)
			&& FMath::IsNearlyEqual(Assist->GetFuelMultiplier(),1.f));
		Test->TestTrue(TEXT("Turbo label hides outside highway"),TurboLabel && !TurboLabel->IsVisible());
		Cab->SetActorLocation(Highway,false,nullptr,ETeleportType::TeleportPhysics);
		Assist->Advance(.5f);
		Vitals->Advance(1000,1,1,0);
		Assist->Advance(.016f);
		Test->TestEqual(TEXT("Empty tank disables assist"),Assist->GetBlend(),0.f);
		Cab->RecoverVehicle(.35f);
		Test->TestEqual(TEXT("Recovery clears turbo state"),Assist->GetBlend(),0.f);
		Cab->SetActorLocation(Highway,false,nullptr,ETeleportType::TeleportPhysics);
		Assist->Advance(.5f);
		Vitals->ApplyImpact(10000);
		Assist->Advance(.016f);
		Test->TestEqual(TEXT("Wreck cannot keep turbo active"),Assist->GetBlend(),0.f);
		Cab->RecoverVehicle(.35f);
		Cab->SetActorLocation(Highway,false,nullptr,ETeleportType::TeleportPhysics);
		Assist->Advance(.5f);
		Cab->ResetVehicle();
		Test->TestEqual(TEXT("Manual reset clears turbo state"),Assist->GetBlend(),0.f);
		Cab->SetActorLocation(Highway,false,nullptr,ETeleportType::TeleportPhysics);
		Assist->Advance(.5f);
		PC->UnPossess();
		Assist->Advance(.016f);
		Test->TestEqual(TEXT("Unoccupied vehicle loses turbo"),Assist->GetBlend(),0.f);
		PC->Possess(Cab);
		Cab->ResetVehicle();
		// A placed Blueprint must drive the real vehicle outside all legacy rectangles.
		auto* TileClass = LoadClass<AFlyingCabHighwayTile>(nullptr,TEXT("/Game/Tile_Set/Highway/BP_HighwayTile.BP_HighwayTile_C"));
		if (!Test->TestNotNull(TEXT("Reusable tile asset loads"),TileClass)) return true;
		const FVector TileCenter(80000,0,80000);
		auto* Tile = World->SpawnActor<AFlyingCabHighwayTile>(TileClass,TileCenter,FRotator::ZeroRotator);
		Tile->SpeedMultiplier = 1.8f;
		Tile->FuelConsumptionMultiplier = .3f;
		Cab->SetActorLocation(TileCenter,false,nullptr,ETeleportType::TeleportPhysics);
		Assist->Advance(.5f);
		Test->TestEqual(TEXT("Placed Blueprint grants custom speed"),Assist->GetSpeedMultiplier(),1.8f);
		Cab->SetTouchHorizontalInput(1);
		Cab->SetTouchThrustPressed(true);
		Body->SetPhysicsLinearVelocity(FVector(4000,0,4000));
		const float TileFuelBefore = Cab->GetFuel();
		Cab->Tick(.1f);
		Test->TestTrue(TEXT("Tile applies actual fuel discount"),FMath::IsNearlyEqual(TileFuelBefore-Cab->GetFuel(),.06075f,1.e-4f));
		Test->TestTrue(TEXT("Tile applies actual speed limits"),Body->GetPhysicsLinearVelocity().Equals(FVector(1890,0,2070),.1));
		UFlyingCabTouchControls* Hud = nullptr;
		for (TObjectIterator<UFlyingCabTouchControls> It; It; ++It)
			if (It->GetWorld()==World && It->IsInViewport()) { Hud = *It; break; }
		auto* MapLayer = Hud && Hud->WidgetTree ? Cast<UFlyingCabHighwayMapLayer>(Hud->WidgetTree->FindWidget(TEXT("HighwayTileRoads"))) : nullptr;
		Test->TestNotNull(TEXT("Live HUD includes highway tile overlay"),MapLayer);
		if (MapLayer)
			Test->TestEqual(TEXT("Road overlay never intercepts controls"),MapLayer->GetVisibility(),ESlateVisibility::HitTestInvisible);
		if (Hud && FParse::Param(FCommandLine::Get(),TEXT("FlyingCabCaptureTileMinimap")))
		{
			const FVector2D Mid = (FlyingCabCityData::GetMinimapWorldMin()+FlyingCabCityData::GetMinimapWorldMax())*.5;
			Tile->SetActorLocation(FVector(Mid.X,0,Mid.Y));
			Tile->SetActorScale3D(FVector(6,1,1));
			Tile->SetActorRotation(FRotator(45,0,0));
			UWidget* Map = Hud->WidgetTree->FindWidget(TEXT("MinimapFrame"));
			if (Test->TestNotNull(TEXT("Map can be rendered"),Map))
			{
				FWidgetRenderer Renderer(true);
				auto* Target = Renderer.DrawWidget(Map->TakeWidget(),FVector2D(320,240));
				FBufferArchive Png;
				Test->TestTrue(TEXT("Tile minimap screenshot saved"),Target && FImageUtils::ExportRenderTarget2DAsPNG(Target,Png)
					&& FFileHelper::SaveArrayToFile(Png,*(FPaths::ProjectSavedDir()/TEXT("Automation/TileMinimap.png"))));
			}
		}
		Tile->Destroy();
		Assist->Advance(.8f);
		Test->TestEqual(TEXT("Removing tile restores speed"),Assist->GetSpeedMultiplier(),1.f);
		Test->TestEqual(TEXT("Removing tile restores fuel cost"),Assist->GetFuelMultiplier(),1.f);
		Cab->SetTouchHorizontalInput(0);
		Cab->SetTouchThrustPressed(false);
		Cab->ResetVehicle();
		return true;
	}
private:
	void Capture(UWorld* World, const FVector& Center)
	{
		auto* Actor = World->SpawnActor<ASceneCapture2D>();
		auto* Capture = Actor->GetCaptureComponent2D();
		auto* Target = NewObject<UTextureRenderTarget2D>();
		Target->InitCustomFormat(1200,700,PF_B8G8R8A8,false);
		Capture->TextureTarget = Target;
		Capture->ProjectionType = ECameraProjectionMode::Orthographic;
		Capture->OrthoWidth = 2800;
		Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		Capture->bCaptureEveryFrame = false;
		Actor->SetActorLocationAndRotation(Center+FVector(0,10000,0),FRotator(0,-90,0));
		Capture->CaptureScene();
		FBufferArchive Png;
		const FString Path = FPaths::ProjectSavedDir()/TEXT("Automation/HighwayTurbo.png");
		Test->TestTrue(TEXT("Turbo visual capture saved"),FImageUtils::ExportRenderTarget2DAsPNG(Target,Png)
			&& FFileHelper::SaveArrayToFile(Png,*Path));
		Actor->Destroy();
	}
	FAutomationTestBase* Test;
	double Started = FPlatformTime::Seconds();
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyingCabHighwayPlayTest,
	"FlyingCab.Functional.PIE.HighwayTurbo", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FFlyingCabHighwayPlayTest::RunTest(const FString&)
{
	if (!AutomationOpenMap(TEXT("/Game/Maps/FlightLab"),true)) { AddError(TEXT("Cannot open FlightLab")); return false; }
	ADD_LATENT_AUTOMATION_COMMAND(FHighwayTurboCommand(this));
	return true;
}
#endif
