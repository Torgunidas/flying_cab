#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "FlyingCabPawn.h"
#include "FlyingCabPlayerController.h"
#include "FlyingCabThrusterVisualComponent.h"
#include "FlyingCabVehicleVitalsComponent.h"
#include "ImageUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/BufferArchive.h"
#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyingCabThrustDemandTest,
	"FlyingCab.Core.Flight.ThrusterDemand", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FFlyingCabThrustDemandTest::RunTest(const FString&)
{
	const auto Lift = FThrusterVisualDemand::Solve(FVector(0,0,2350), FVector::ZeroVector);
	TestTrue(TEXT("Lift expels gas downward"), Lift.ExhaustDirection.Equals(FVector(0,0,-1)));
	TestEqual(TEXT("Full lift has nominal visual power"), Lift.Power, 1.f);
	const auto Right = FThrusterVisualDemand::Solve(FVector(1400,0,2350), FVector::ZeroVector);
	TestTrue(TEXT("Combined acceleration produces one diagonal exhaust vector"),
		Right.ExhaustDirection.Equals(-FVector(1400,0,2350).GetSafeNormal(), .0001));
	const auto Brake = FThrusterVisualDemand::Solve(FVector::ZeroVector, FVector(-500,0,0));
	TestTrue(TEXT("Braking rightward motion expels gas to the right"), Brake.ExhaustDirection.Equals(FVector(1,0,0)));
	TestTrue(TEXT("Partial demand is dimmer than full demand"), Brake.Power > 0 && Brake.Power < Lift.Power);
	const auto Off = FThrusterVisualDemand::Solve(FVector::ZeroVector, FVector::ZeroVector);
	TestEqual(TEXT("No force has no firing, regardless of vehicle speed or gravity"), Off.Power, 0.f);
	const auto Cancel = FThrusterVisualDemand::Solve(FVector(100,0,0), FVector(-100,0,0));
	TestEqual(TEXT("Opposing sampled forces cancel"), Cancel.Power, 0.f);
	return true;
}

namespace
{
class FThrusterPlayCommand final : public IAutomationLatentCommand
{
public:
	explicit FThrusterPlayCommand(FAutomationTestBase* InTest) : Test(InTest) {}
	virtual bool Update() override
	{
		UWorld* World = AutomationCommon::GetAnyGameWorld();
		auto* PC = World ? Cast<AFlyingCabPlayerController>(World->GetFirstPlayerController()) : nullptr;
		auto* Cab = PC ? Cast<AFlyingCabPawn>(PC->GetPawn()) : nullptr;
		if (!Cab)
		{
			if (FPlatformTime::Seconds() - Started < 15.) return false;
			Test->AddError(TEXT("No playable cab for thruster verification"));
			return true;
		}
		if (FParse::Param(FCommandLine::Get(), TEXT("FlyingCabCaptureThrusters")))
		{
			if (FAssetCompilingManager::Get().GetNumRemainingAssets() > 0)
			{
				if (FPlatformTime::Seconds() - Started < 240.) return false;
				Test->AddError(TEXT("Thruster capture timed out waiting for material compilation"));
				return true;
			}
			if (RenderReadyAt == 0.) RenderReadyAt = FPlatformTime::Seconds();
			if (FPlatformTime::Seconds() - RenderReadyAt < 1.) return false;
		}
		PC->StartRunMode(EFlyingCabRunMode::Freeroam);
		auto* Fx = Cab->FindComponentByClass<UFlyingCabThrusterVisualComponent>();
		auto* Body = Cast<UPrimitiveComponent>(Cab->GetRootComponent());
		auto* Vitals = Cab->FindComponentByClass<UFlyingCabVehicleVitalsComponent>();
		if (!Test->TestNotNull(TEXT("Live Blueprint inherits thrusters"), Fx) || !Body || !Vitals) return true;
		Cab->ResetVehicle();
		Cab->SetActorLocation(FVector(-2200,0,240),false,nullptr,ETeleportType::TeleportPhysics);
		Cab->SetTouchThrustPressed(true);
		for (int32 Index=0; Index<30; ++Index)
		{
			Cab->Tick(.016f);
			Fx->TickComponent(.016f, LEVELTICK_All, nullptr);
		}
		Test->TestTrue(TEXT("Powered lift is visible"), Fx->GetDisplayedPower() > .95f);
		Test->TestTrue(TEXT("Lift nozzle points down"), Fx->GetExhaustDirection().Z < -.99);
		TArray<UStaticMeshComponent*> Meshes;
		Cab->GetComponents(Meshes);
		int32 NozzleCount=0, PlumeCount=0;
		for (auto* Mesh : Meshes)
		{
			if (!Mesh->GetName().StartsWith(TEXT("Thruster"))) continue;
			Test->TestEqual(TEXT("Effects add no collision"), Mesh->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
			Test->TestNotNull(TEXT("Thruster mesh is a saved asset"), Mesh->GetStaticMesh().Get());
			if (Mesh->GetName().EndsWith(TEXT("Nozzle"))) ++NozzleCount;
			if (Mesh->GetName().EndsWith(TEXT("Plume")) && Mesh->IsVisible()) ++PlumeCount;
		}
		Test->TestEqual(TEXT("Exactly two physical nozzles"), NozzleCount, 2);
		Test->TestEqual(TEXT("Exactly two firing plumes"), PlumeCount, 2);
		if (FParse::Param(FCommandLine::Get(), TEXT("FlyingCabCaptureThrusters"))) Capture(World, Cab, TEXT("Lift"));
		Cab->SetTouchHorizontalInput(1);
		for (int32 Index=0; Index<25; ++Index)
		{
			Cab->Tick(.016f);
			Fx->TickComponent(.016f, LEVELTICK_All, nullptr);
		}
		Test->TestTrue(TEXT("Diagonal demand points down and left"), Fx->GetExhaustDirection().X < -.45 && Fx->GetExhaustDirection().Z < -.8);
		if (FParse::Param(FCommandLine::Get(), TEXT("FlyingCabCaptureThrusters"))) Capture(World, Cab, TEXT("Accelerate"));
		Cab->SetTouchHorizontalInput(0);
		Cab->SetTouchThrustPressed(false);
		for (int32 Index=0; Index<25; ++Index)
		{
			Body->SetPhysicsLinearVelocity(FVector(700,0,0));
			Cab->Tick(.016f);
			Fx->TickComponent(.016f, LEVELTICK_All, nullptr);
		}
		Test->TestTrue(TEXT("Existing release damping becomes rightward counterjet"), Fx->GetExhaustDirection().X > .99);
		if (FParse::Param(FCommandLine::Get(), TEXT("FlyingCabCaptureThrusters"))) Capture(World, Cab, TEXT("Brake"));
		const FVector VelocityBefore = Body->GetPhysicsLinearVelocity();
		const float FuelBefore = Cab->GetFuel();
		Fx->SubmitAcceleration(FVector(0,0,2350), FVector::ZeroVector);
		Fx->TickComponent(.016f, LEVELTICK_All, nullptr);
		Test->TestTrue(TEXT("Visual updates do not alter velocity"), Body->GetPhysicsLinearVelocity().Equals(VelocityBefore));
		Test->TestEqual(TEXT("Visual counterthrust does not alter fuel accounting"), Cab->GetFuel(), FuelBefore);
		for (int32 Index=0; Index<60; ++Index) Fx->TickComponent(.016f, LEVELTICK_All, nullptr);
		Test->TestEqual(TEXT("No new samples cannot latch engine firing"), Fx->GetDisplayedPower(), 0.f);
		Cab->SetTouchThrustPressed(true);
		Cab->Tick(.016f);
		Fx->TickComponent(.016f, LEVELTICK_All, nullptr);
		Vitals->Advance(1000,1,1,0);
		Fx->TickComponent(.016f, LEVELTICK_All, nullptr);
		Test->TestEqual(TEXT("Empty tank immediately hides thrust including coast effects"), Fx->GetDisplayedPower(), 0.f);
		Cab->RecoverVehicle(.35f);
		Test->TestEqual(TEXT("Recovery resets visuals"), Fx->GetDisplayedPower(), 0.f);
		Cab->SetTouchThrustPressed(true);
		Cab->Tick(.016f);
		Fx->TickComponent(.016f, LEVELTICK_All, nullptr);
		Vitals->ApplyImpact(10000);
		Fx->TickComponent(.016f, LEVELTICK_All, nullptr);
		Test->TestEqual(TEXT("Wreck has no active jets"), Fx->GetDisplayedPower(), 0.f);
		Cab->RecoverVehicle(.35f);
		PC->UnPossess();
		Fx->SubmitAcceleration(FVector(0,0,2350), FVector::ZeroVector);
		Fx->TickComponent(.016f, LEVELTICK_All, nullptr);
		Test->TestEqual(TEXT("Unoccupied taxi cannot fire"), Fx->GetDisplayedPower(), 0.f);
		PC->Possess(Cab);
		Cab->ResetVehicle();
		Test->TestEqual(TEXT("Manual reset clears visuals"), Fx->GetDisplayedPower(), 0.f);
		return true;
	}
private:
	void Capture(UWorld* World, AFlyingCabPawn* Cab, const TCHAR* State)
	{
		auto* Actor = World->SpawnActor<ASceneCapture2D>();
		auto* Capture = Actor->GetCaptureComponent2D();
		auto* Target = NewObject<UTextureRenderTarget2D>();
		Target->InitCustomFormat(1440,900,PF_B8G8R8A8,false);
		Capture->TextureTarget = Target;
		Capture->ProjectionType = ECameraProjectionMode::Orthographic;
		Capture->OrthoWidth = 1000;
		Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		Capture->bCaptureEveryFrame = false;
		Capture->bAlwaysPersistRenderingState = true;
		Actor->SetActorLocationAndRotation(Cab->GetActorLocation()+FVector(0,3000,-65),FRotator(0,-90,0));
		Capture->CaptureScene();
		// UE can request a translucent permutation on its first visible draw. Read
		// back that warm-up frame before waiting, then capture the compiled effect.
		FBufferArchive Warmup;
		FImageUtils::ExportRenderTarget2DAsPNG(Target, Warmup);
		if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
		Capture->CaptureScene();
		FBufferArchive Png;
		const FString Path = FPaths::ProjectSavedDir()/TEXT("Automation")/FString::Printf(TEXT("Thrusters_%s.png"),State);
		Test->TestTrue(TEXT("Thruster render saved"),FImageUtils::ExportRenderTarget2DAsPNG(Target,Png)
			&& FFileHelper::SaveArrayToFile(Png,*Path));
		Actor->Destroy();
	}
	FAutomationTestBase* Test;
	double Started = FPlatformTime::Seconds();
	double RenderReadyAt = 0.;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyingCabThrustPlayTest,
	"FlyingCab.Functional.PIE.ThrusterVisuals", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FFlyingCabThrustPlayTest::RunTest(const FString&)
{
	if (!AutomationOpenMap(TEXT("/Game/Maps/FlightLab"),true)) { AddError(TEXT("Cannot open FlightLab")); return false; }
	ADD_LATENT_AUTOMATION_COMMAND(FThrusterPlayCommand(this));
	return true;
}
#endif
