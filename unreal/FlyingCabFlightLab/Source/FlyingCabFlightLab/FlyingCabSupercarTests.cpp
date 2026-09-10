#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "ImageUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/BufferArchive.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "FlyingCabPawn.h"
#include "FlyingCabPlayerController.h"
#include "FlyingCabWorldBootstrap.h"
#include "FlyingCabTrafficVehicle.h"

namespace
{
class FSupercarCheck final : public IAutomationLatentCommand
{
public:
	explicit FSupercarCheck(FAutomationTestBase* InTest) : Test(InTest) {}
	bool Update() override
	{
		UWorld* World = AutomationCommon::GetAnyGameWorld();
		auto* PC = World ? Cast<AFlyingCabPlayerController>(World->GetFirstPlayerController()) : nullptr;
		auto* Original = PC ? Cast<AFlyingCabPawn>(PC->GetPawn()) : nullptr;
		if (!Original)
		{
			if (FPlatformTime::Seconds() - Started < 20) return false;
			Test->AddError(TEXT("No default player cab")); return true;
		}
		PC->StartRunMode(EFlyingCabRunMode::Freeroam);
		Test->TestFalse(TEXT("Default player vehicle is still the original cab"), Original->IsSupercar());
		AFlyingCabWorldBootstrap* Bootstrap = nullptr;
		for (TActorIterator<AFlyingCabWorldBootstrap> It(World); It; ++It) Bootstrap = *It;
		if (!Test->TestNotNull(TEXT("World bootstrap"), Bootstrap)) return true;
		Test->TestEqual(TEXT("Four public parked A_R7s"), Bootstrap->GetSupercars().Num(), 4);
		for (AFlyingCabPawn* Car : Bootstrap->GetSupercars())
		{
			FText Reason;
			Test->TestTrue(TEXT("Public access without a license or purchase"), Car->CanPlayerEnter(Reason));
			Test->TestTrue(TEXT("A_R7 configuration applied"), Car->IsSupercar());
			Test->TestTrue(TEXT("Car starts fueled"), Car->GetFuelPercent() > .99f);
			auto* Mesh = Car->FindComponentByClass<UStaticMeshComponent>();
			Test->TestTrue(TEXT("Imported A_R7 is loaded"), Mesh && Mesh->GetStaticMesh()
				&& Mesh->GetStaticMesh()->GetName() == TEXT("SM_A_R7_Supercar"));
			FHitResult Ground;
			const FVector Position = Car->GetActorLocation();
			Test->TestTrue(TEXT("Parking bay has a nearby solid floor"), World->LineTraceSingleByObjectType(Ground,
				Position, Position-FVector(0,0,80), FCollisionObjectQueryParams(ECC_WorldStatic),
				FCollisionQueryParams(SCENE_QUERY_STAT(SupercarTest),false,Car)) && Ground.ImpactNormal.Z > .65f);
		}
		int32 SupercarTraffic = 0;
		for (AFlyingCabTrafficVehicle* Car : Bootstrap->GetTrafficVehicles())
		{
			if (Car->IsSupercar())
			{
				++SupercarTraffic;
				Test->TestTrue(TEXT("NPC A_R7 follows a living highway route"), Car->UsesLivingRoute());
			}
		}
		Test->TestTrue(TEXT("A_R7 present in highway traffic"), SupercarTraffic > 0);
		if (!Bootstrap->GetSupercars().IsEmpty())
		{
			AFlyingCabPawn* Car = Bootstrap->GetSupercars()[0];
			if (FParse::Param(FCommandLine::Get(),TEXT("FlyingCabCaptureSupercar")))
			{
				auto* Camera = World->SpawnActor<ASceneCapture2D>();
				auto* Capture = Camera->GetCaptureComponent2D();
				auto* Target = NewObject<UTextureRenderTarget2D>();
				Target->InitCustomFormat(1200,700,PF_B8G8R8A8,false);
				Capture->TextureTarget = Target;
				Capture->ProjectionType = ECameraProjectionMode::Orthographic;
				Capture->OrthoWidth = 650;
				Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
				Capture->bCaptureEveryFrame = false;
				const FVector Offset(200,800,200);
				Camera->SetActorLocationAndRotation(Car->GetActorLocation()+Offset,(-Offset).Rotation());
				Capture->CaptureScene();
				FBufferArchive Png;
				Test->TestTrue(TEXT("In-game A_R7 capture saved"), FImageUtils::ExportRenderTarget2DAsPNG(Target,Png)
					&& FFileHelper::SaveArrayToFile(Png,*(FPaths::ProjectSavedDir()/TEXT("Automation/A_R7_InGame.png"))));
				Camera->Destroy();
			}
			PC->Possess(Car);
			Test->TestTrue(TEXT("Player can possess optional A_R7"), PC->GetPawn() == Car);
			auto* Body = Cast<UPrimitiveComponent>(Car->GetRootComponent());
			Car->SetTouchHorizontalInput(1);
			Body->SetPhysicsLinearVelocity(FVector(6000,0,0));
			Car->Tick(.016f);
			Test->TestTrue(TEXT("A_R7 has triple normal horizontal cap"), FMath::IsNearlyEqual(Body->GetPhysicsLinearVelocity().X,3150.f,1.f));
			Car->SetTouchHorizontalInput(0);
			Car->ResetVehicle();
			PC->Possess(Original);
			Test->TestTrue(TEXT("Player can return to the regular cab"),PC->GetPawn() == Original);
		}
		return true;
	}
	FAutomationTestBase* Test;
	double Started = FPlatformTime::Seconds();
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyingCabSupercarIntegration,
	"FlyingCab.Functional.PIE.Supercar", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FFlyingCabSupercarIntegration::RunTest(const FString&)
{
	if (!AutomationOpenMap(TEXT("/Game/Maps/FlightLab"),true)) return false;
	ADD_LATENT_AUTOMATION_COMMAND(FSupercarCheck(this));
	return true;
}
#endif
