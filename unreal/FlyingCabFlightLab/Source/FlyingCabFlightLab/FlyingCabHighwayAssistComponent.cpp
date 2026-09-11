#include "FlyingCabHighwayAssistComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "FlyingCabCityData.h"
#include "FlyingCabHighwayTile.h"
#include "FlyingCabPawn.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

UFlyingCabHighwayAssistComponent::UFlyingCabHighwayAssistComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // Advanced by the vehicle before its speed limits.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Basic(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CubeMesh = Cube.Object;
	Material = Basic.Object;
}

void UFlyingCabHighwayAssistComponent::BeginPlay()
{
	Super::BeginPlay();
	AActor* Owner = GetOwner();
	for (int32 Index = 0; Index < 3; ++Index)
	{
		auto* Streak = NewObject<UStaticMeshComponent>(Owner,
			FName(*FString::Printf(TEXT("HighwayStreak%d"), Index)));
		Streak->SetupAttachment(Owner->GetRootComponent());
		Streak->SetStaticMesh(CubeMesh);
		Streak->SetMaterial(0, Material);
		Streak->SetMobility(EComponentMobility::Movable);
		Streak->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Streak->SetGenerateOverlapEvents(false);
		Streak->SetCastShadow(false);
		Streak->SetReceivesDecals(false);
		Streak->SetVisibility(false);
		Owner->AddInstanceComponent(Streak);
		Streak->RegisterComponent();
		Streak->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(.55,1,.025));
		Streaks.Add(Streak);
	}
	Label = NewObject<UTextRenderComponent>(Owner, TEXT("HighwayTurboLabel"));
	Label->SetupAttachment(Owner->GetRootComponent());
	Label->SetMobility(EComponentMobility::Movable);
	Label->SetRelativeLocation(FVector(0,80,-120));
	Label->SetRelativeRotation(FRotator(0,90,0));
	Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	Label->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	Label->SetWorldSize(44);
	Label->SetText(FText::FromString(TEXT("HWY TURBO")));
	Label->SetTextRenderColor(FColor(195,255,65));
	Label->SetVisibility(false);
	Owner->AddInstanceComponent(Label);
	Label->RegisterComponent();
}

void UFlyingCabHighwayAssistComponent::Advance(float DeltaSeconds)
{
	const auto* Cab = Cast<AFlyingCabPawn>(GetOwner());
	const auto* Body = Cab ? Cast<UPrimitiveComponent>(Cab->GetRootComponent()) : nullptr;
	if (!Cab || !Cab->IsPlayerControlled() || Cab->IsDestroyed()
		|| Cab->GetFuel() <= UE_SMALL_NUMBER || !Body || !Body->IsSimulatingPhysics())
	{
		ResetAssist();
		return;
	}
	float ZoneSpeed, ZoneFuel;
	const bool bInHighway = AFlyingCabHighwayTile::GetBonuses(GetWorld(), Cab->GetActorLocation(),
		SpeedMultiplier, FuelConsumptionMultiplier, ZoneSpeed, ZoneFuel);
	if (bInHighway)
	{
		ActiveSpeedMultiplier = ZoneSpeed;
		ActiveFuelMultiplier = ZoneFuel;
	}
	const float Duration = FMath::Max(.1f, bInHighway ? EntrySeconds : ExitSeconds);
	Blend = FMath::FInterpConstantTo(Blend, bInHighway ? 1.f : 0.f,
		FMath::Max(0.f, DeltaSeconds), 1.f / Duration);
	VisualTime += FMath::Max(0.f, DeltaSeconds);
	UpdateAppearance(Body->GetPhysicsLinearVelocity());
}

float UFlyingCabHighwayAssistComponent::GetSpeedMultiplier() const
{
	return FMath::Lerp(1.f, ActiveSpeedMultiplier, Blend);
}

float UFlyingCabHighwayAssistComponent::GetFuelMultiplier() const
{
	return FMath::Lerp(1.f, ActiveFuelMultiplier, Blend);
}

void UFlyingCabHighwayAssistComponent::ResetAssist()
{
	Blend = 0.f;
	ActiveSpeedMultiplier = 1.f;
	ActiveFuelMultiplier = 1.f;
	VisualTime = 0.f;
	UpdateAppearance(FVector::ZeroVector);
}

void UFlyingCabHighwayAssistComponent::UpdateAppearance(const FVector& Velocity)
{
	if (Label) Label->SetVisibility(Blend > .02f);
	const FVector PlanarVelocity(Velocity.X,0,Velocity.Z);
	const float Speed = PlanarVelocity.Size();
	const FVector Direction = PlanarVelocity.GetSafeNormal();
	const FVector Across(-Direction.Z,0,Direction.X);
	const float Pitch = FMath::RadiansToDegrees(FMath::Atan2(Direction.Z,Direction.X));
	for (int32 Index = 0; Index < Streaks.Num(); ++Index)
	{
		UStaticMeshComponent* Streak = Streaks[Index];
		Streak->SetVisibility(Blend > .02f && Speed > 120.f);
		const float Pulse = .9f + .1f*FMath::Sin(VisualTime*8.f + Index*2.f);
		const float Length = (180.f + FMath::Min(Speed,2000.f)*.15f)*Blend*Pulse;
		Streak->SetRelativeLocation(-Direction*(160.f+Length*.5f)
			+ Across*((Index-1)*42.f) + FVector(0,65,0));
		Streak->SetRelativeRotation(FRotator(Pitch,0,0));
		Streak->SetRelativeScale3D(FVector(FMath::Max(.001f,Length/100.f),.035,.055));
	}
}
