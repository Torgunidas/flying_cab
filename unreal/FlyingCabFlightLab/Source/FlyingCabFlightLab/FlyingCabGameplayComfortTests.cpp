#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "FlyingCabCityData.h"
#include "FlyingCabDeliveryZone.h"
#include "FlyingCabLivingPedestrian.h"
#include "FlyingCabLivingRoute.h"
#include "FlyingCabPawn.h"
#include "FlyingCabPlayerController.h"
#include "FlyingCabTouchControls.h"
#include "FlyingCabTrafficVehicle.h"
#include "FlyingCabVehicleVitalsComponent.h"
#include "ImageUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/BufferArchive.h"
#include "Slate/WidgetRenderer.h"
#include "Tests/AutomationCommon.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"

namespace
{
float ReadFloat(const UObject* Object, const TCHAR* Name)
{
	const FFloatProperty* Property = FindFProperty<FFloatProperty>(Object->GetClass(), Name);
	return Property ? Property->GetPropertyValue_InContainer(Object) : -1.f;
}
UStaticMeshComponent* FindMesh(const AActor* Actor, const TCHAR* Name)
{
	TArray<UStaticMeshComponent*> Meshes;
	Actor->GetComponents(Meshes);
	for (UStaticMeshComponent* Mesh : Meshes) { if (Mesh->GetFName() == Name) { return Mesh; } }
	return nullptr;
}
class FGameplayComfortCommand final : public IAutomationLatentCommand
{
public:
	explicit FGameplayComfortCommand(FAutomationTestBase* InTest) : Test(InTest) {}
	virtual bool Update() override
	{
		UWorld* World = AutomationCommon::GetAnyGameWorld();
		auto* PC = World ? Cast<AFlyingCabPlayerController>(World->GetFirstPlayerController()) : nullptr;
		auto* Cab = PC ? Cast<AFlyingCabPawn>(PC->GetPawn()) : nullptr;
		UFlyingCabTouchControls* Hud = nullptr;
		for (TObjectIterator<UFlyingCabTouchControls> It; It; ++It)
		{
			if (It->GetWorld() == World && It->GetOwningPlayer() == PC) { Hud = *It; break; }
		}
		if (!Cab || !Hud || !Hud->WidgetTree)
		{
			if (FPlatformTime::Seconds() - Started < 15.0) { return false; }
			Test->AddError(TEXT("Comfort test could not resolve gameplay and HUD")); return true;
		}
		PC->StartRunMode(EFlyingCabRunMode::Freeroam);
		// Check the actual Blueprint and map instance, not just C++ fallback values.
		const UObject* Configurations[] = {Cab, Cab->GetClass()->GetDefaultObject()};
		for (const UObject* Defaults : Configurations)
		{
			Test->TestTrue(TEXT("Vertical fuel consumption is 25% lower"), FMath::IsNearlyEqual(ReadFloat(Defaults, TEXT("VerticalFuelPerSecond")), 1.35f));
			Test->TestTrue(TEXT("Horizontal fuel consumption is 25% lower"), FMath::IsNearlyEqual(ReadFloat(Defaults, TEXT("HorizontalFuelPerSecond")), 0.675f));
			Test->TestTrue(TEXT("Metro tank capacity doubled"), FMath::IsNearlyEqual(ReadFloat(Defaults, TEXT("MaxFuel")), 200.f));
			Test->TestTrue(TEXT("Starting tank still 65 percent, now 130 units"), FMath::IsNearlyEqual(ReadFloat(Defaults, TEXT("StartingFuel")), 130.f));
			Test->TestTrue(TEXT("Blueprint and live vehicle inherit stronger impact resistance"),
				FMath::IsNearlyEqual(ReadFloat(Defaults, TEXT("DamageFullHullSpeed")), 2000.f));
		}
		UFlyingCabVehicleVitalsComponent* Vitals = NewObject<UFlyingCabVehicleVitalsComponent>();
		Vitals->InitializeVitals(FFlyingCabVehicleVitalsConfig());
		Vitals->Advance(10.f, 1.f, 1.f, 0.f);
		Test->TestTrue(TEXT("Ten seconds of full two-axis thrust still consumes 20.25 fuel"), FMath::IsNearlyEqual(Vitals->GetFuel(), 109.75f));

		const FVector Origin(-40000, 0, 20000);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		auto* Route = World->SpawnActor<AFlyingCabLivingRoute>(AFlyingCabLivingRoute::StaticClass(), Origin, FRotator::ZeroRotator, Params);
		auto* Ped = World->SpawnActor<AFlyingCabLivingPedestrian>(AFlyingCabLivingPedestrian::StaticClass(), Origin, FRotator::ZeroRotator, Params);
		auto* Taxi = World->SpawnActor<AFlyingCabTrafficVehicle>(AFlyingCabTrafficVehicle::StaticClass(), Origin + FVector(0, 0, 1000), FRotator::ZeroRotator, Params);
		if (!Route || !Ped || !Taxi) { Test->AddError(TEXT("Cannot spawn passenger test fixture")); return true; }
		FFlyingCabLivingRouteDefinition Definition;
		Definition.RouteId = TEXT("ComfortTest.Passenger");
		Definition.AgentKind = EFlyingCabLivingAgentKind::Pedestrian;
		Definition.Nodes.SetNum(3);
		Definition.Nodes[0].Action = EFlyingCabLivingRouteAction::BoardVehicle;
		Definition.Nodes[0].StopId = TEXT("Test.Start");
		Definition.Nodes[1].LocalLocation = FVector(200, 0, 0);
		Definition.Nodes[1].Action = EFlyingCabLivingRouteAction::ExitVehicle;
		Definition.Nodes[1].StopId = TEXT("Test.End");
		Definition.Nodes[2].LocalLocation = FVector(400, 0, 0);
		Route->Configure(Definition);
		Ped->Configure(Route, 0.f, FLinearColor::Yellow);
		Ped->Tick(0.016f);
		Test->TestTrue(TEXT("Passenger boards ambient taxi"), Ped->BoardVehicle(Taxi));
		Test->TestTrue(TEXT("Riding passenger is hidden and has no collision"), Ped->IsHidden()
			&& Cast<UPrimitiveComponent>(Ped->GetRootComponent())->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
		Test->TestTrue(TEXT("Passenger exits at destination"), Ped->CompleteRideAtStop(TEXT("Test.End")));
		auto* Body = Cast<UPrimitiveComponent>(Ped->GetRootComponent());
		Test->TestTrue(TEXT("Exit restores query-only proxy, never physical collision"), !Ped->IsHidden()
			&& Body->GetCollisionEnabled() == ECollisionEnabled::QueryOnly);
		Test->TestEqual(TEXT("Player pawn is ignored"), Body->GetCollisionResponseToChannel(ECC_Pawn), ECR_Ignore);
		Test->TestEqual(TEXT("Traffic does not physically collide with passenger"), Body->GetCollisionResponseToChannel(ECC_WorldDynamic), ECR_Overlap);
		const FVector Start = Ped->GetActorLocation() - FVector(100, 0, 0);
		const FVector End = Ped->GetActorLocation() + FVector(100, 0, 0);
		FHitResult BlockingHit;
		Test->TestFalse(TEXT("Player movement sweep passes through exiting passenger"), World->SweepSingleByChannel(
			BlockingHit, Start, End, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(20)));
		TArray<FHitResult> SensorHits;
		FCollisionObjectQueryParams ObjectTypes;
		ObjectTypes.AddObjectTypesToQuery(ECC_WorldDynamic);
		World->SweepMultiByObjectType(SensorHits, Start, End, FQuat::Identity, ObjectTypes, FCollisionShape::MakeSphere(20));
		Test->TestTrue(TEXT("Living-world object sensor still sees passenger"), SensorHits.ContainsByPredicate(
			[Ped](const FHitResult& Hit) { return Hit.GetActor() == Ped; }));
		const AFlyingCabDeliveryZone* PassengerReference = GetDefault<AFlyingCabDeliveryZone>();
		for (const auto& Names : {TPair<const TCHAR*, const TCHAR*>(TEXT("VisualMesh"), TEXT("PassengerBody")),
			TPair<const TCHAR*, const TCHAR*>(TEXT("PassengerHead"), TEXT("PassengerHead"))})
		{
			UStaticMeshComponent* Visual = FindMesh(Ped, Names.Key);
			UStaticMeshComponent* Reference = FindMesh(PassengerReference, Names.Value);
			Test->TestTrue(TEXT("Ambient passenger matches player passenger silhouette"), Visual && Reference
				&& Visual->GetStaticMesh() == Reference->GetStaticMesh()
				&& Visual->GetRelativeScale3D().Equals(Reference->GetRelativeScale3D())
				&& Visual->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
		}
		Ped->Destroy(); Taxi->Destroy(); Route->Destroy();

		auto CheckBadges = [&](const TCHAR* Prefix, int32 Count, const TCHAR* Label)
		{
			for (int32 Index = 0; Index < Count; ++Index)
			{
				const FString Name = FString::Printf(TEXT("%s%d"), Prefix, Index);
				auto* Badge = Cast<UBorder>(Hud->WidgetTree->FindWidget(FName(*Name)));
				auto* Text = Cast<UTextBlock>(Hud->WidgetTree->FindWidget(FName(*(Name + TEXT("Label")))));
				auto* Slot = Badge ? Cast<UCanvasPanelSlot>(Badge->Slot) : nullptr;
				Test->TestTrue(TEXT("Service marker is a large, labelled, non-interactive badge"), Badge && Text && Slot
					&& Text->GetText().ToString() == Label && Slot->GetSize().Y >= 20.f && Slot->GetZOrder() > 5
					&& Badge->GetVisibility() == ESlateVisibility::HitTestInvisible);
			}
		};
		CheckBadges(TEXT("FuelStationBadge"), FlyingCabCityData::GetFuelStations().Num(), TEXT("FUEL"));
		CheckBadges(TEXT("RepairStationBadge"), FlyingCabCityData::GetRepairStations().Num(), TEXT("REPAIR"));
		if (FParse::Param(FCommandLine::Get(), TEXT("FlyingCabCaptureMinimap")))
		{
			UWidget* Map = Hud->WidgetTree->FindWidget(TEXT("MinimapFrame"));
			if (Test->TestNotNull(TEXT("Minimap available for visual verification"), Map))
			{
				TArray<FVector2D> Offers;
				for (const auto& District : FlyingCabCityData::GetDistricts())
				{ if (District.FuelStationName.IsEmpty()) continue;
					const FVector P = FlyingCabCityData::GetPassengerPickupLocation(District.StopLocation); Offers.Add(FVector2D(P.X, P.Z)); }
				Hud->SetPassengerOfferMarkers(FVector2D(-2200, 50), Offers);
				FWidgetRenderer Renderer(true);
				UTextureRenderTarget2D* Target = Renderer.DrawWidget(Map->TakeWidget(), FVector2D(320, 240));
				FBufferArchive Png;
				const FString Path = FPaths::ProjectSavedDir() / TEXT("Automation/ComfortMinimap.png");
				Test->TestTrue(TEXT("Minimap screenshot saved"), Target && FImageUtils::ExportRenderTarget2DAsPNG(Target, Png)
					&& FFileHelper::SaveArrayToFile(Png, *Path));
				Test->AddInfo(TEXT("Minimap screenshot: ") + Path);
				auto* CaptureActor = World->SpawnActor<ASceneCapture2D>();
				auto* Capture = CaptureActor->GetCaptureComponent2D();
				auto* WorldTarget = NewObject<UTextureRenderTarget2D>();
				WorldTarget->InitCustomFormat(1600,600,PF_B8G8R8A8,false);
				Capture->TextureTarget = WorldTarget;
				Capture->ProjectionType = ECameraProjectionMode::Orthographic;
				Capture->OrthoWidth = 42000.f;
				Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
				Capture->bCaptureEveryFrame = false;
				CaptureActor->SetActorLocationAndRotation(FVector(5000,30000,6500),FRotator(0,-90,0));
				Capture->CaptureScene();
				FBufferArchive WorldPng;
				const FString WorldPath = FPaths::ProjectSavedDir() / TEXT("Automation/MetroOverview.png");
				Test->TestTrue(TEXT("Metro overview saved"),FImageUtils::ExportRenderTarget2DAsPNG(WorldTarget,WorldPng)
					&& FFileHelper::SaveArrayToFile(WorldPng,*WorldPath));
				CaptureActor->Destroy();
			}
		}
		return true;
	}
private:
	FAutomationTestBase* Test;
	double Started = FPlatformTime::Seconds();
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyingCabGameplayComfortTest,
	"FlyingCab.Functional.PIE.GameplayComfort", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FFlyingCabGameplayComfortTest::RunTest(const FString&)
{
	if (!AutomationOpenMap(TEXT("/Game/Maps/FlightLab"), true)) { AddError(TEXT("Cannot open FlightLab")); return false; }
	ADD_LATENT_AUTOMATION_COMMAND(FGameplayComfortCommand(this));
	return true;
}
#endif
