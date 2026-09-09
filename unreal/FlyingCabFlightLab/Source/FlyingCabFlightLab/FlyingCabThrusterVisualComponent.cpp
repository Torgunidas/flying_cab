#include "FlyingCabThrusterVisualComponent.h"

#include "Components/PointLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "FlyingCabPawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"

FThrusterVisualDemand FThrusterVisualDemand::Solve(const FVector& ControlAcceleration, const FVector& CoastAcceleration)
{
	FThrusterVisualDemand Result;
	Result.Acceleration = ControlAcceleration + CoastAcceleration;
	Result.Acceleration.Y = 0;
	if (Result.Acceleration.ContainsNaN()) Result.Acceleration = FVector::ZeroVector;
	const float Magnitude = Result.Acceleration.Size();
	if (Magnitude > 1.f)
	{
		Result.ExhaustDirection = -Result.Acceleration / Magnitude;
		// Full vertical thrust is 2350 cm/s²; horizontal thrust is 1400 cm/s².
		Result.Power = FMath::Clamp(Magnitude / 2350.f, 0.f, 1.25f);
	}
	return Result;
}

UFlyingCabThrusterVisualComponent::UFlyingCabThrusterVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	NozzleMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Effects/Thrusters/SM_VectorNozzle.SM_VectorNozzle")));
	CardMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Effects/Thrusters/SM_ThrustCard.SM_ThrustCard")));
	SphereMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Sphere.Sphere")));
	PlumeBase = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Effects/Thrusters/M_ThrustPlume.M_ThrustPlume")));
	HeatBase = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Effects/Thrusters/M_ThrustHeat.M_ThrustHeat")));
	GlowBase = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Effects/Thrusters/M_NozzleGlow.M_NozzleGlow")));
	MetalBase = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Effects/Thrusters/M_NozzleMetal.M_NozzleMetal")));
	DustBase = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Effects/Thrusters/M_ThrustDust.M_ThrustDust")));
}

UStaticMeshComponent* UFlyingCabThrusterVisualComponent::CreateMesh(FName Name, UStaticMesh* Mesh, UMaterialInterface* Material)
{
	auto* Part = NewObject<UStaticMeshComponent>(GetOwner(), Name);
	Part->SetupAttachment(GetOwner()->GetRootComponent());
	Part->SetStaticMesh(Mesh);
	if (Material) Part->SetMaterial(0, Material);
	Part->SetMobility(EComponentMobility::Movable);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Part->SetGenerateOverlapEvents(false);
	Part->SetCanEverAffectNavigation(false);
	Part->SetCastShadow(false);
	Part->SetReceivesDecals(false);
	GetOwner()->AddInstanceComponent(Part);
	Part->RegisterComponent();
	return Part;
}

void UFlyingCabThrusterVisualComponent::BeginPlay()
{
	Super::BeginPlay();
	AddTickPrerequisiteActor(GetOwner());
	TArray<UStaticMeshComponent*> Meshes;
	GetOwner()->GetComponents(Meshes);
	for (auto* Mesh : Meshes)
	{
		if (Mesh->GetFName() == TEXT("VisualMesh")) BodyVisual = Mesh;
	}
	for (int32 Index = 0; Index < 2; ++Index)
	{
		auto& Jet = Nozzles.AddDefaulted_GetRef();
		auto Name = [Index](const TCHAR* Suffix) { return FName(*FString::Printf(TEXT("Thruster%d%s"), Index, Suffix)); };
		Jet.Joint = CreateMesh(Name(TEXT("Joint")), SphereMesh.LoadSynchronous(), MetalBase.LoadSynchronous());
		Jet.Nozzle = CreateMesh(Name(TEXT("Nozzle")), NozzleMesh.LoadSynchronous(), nullptr);
		Jet.Core = CreateMesh(Name(TEXT("Core")), SphereMesh.LoadSynchronous(), GlowBase.LoadSynchronous());
		Jet.Plume = CreateMesh(Name(TEXT("Plume")), CardMesh.LoadSynchronous(), PlumeBase.LoadSynchronous());
		Jet.Heat = CreateMesh(Name(TEXT("Heat")), CardMesh.LoadSynchronous(), HeatBase.LoadSynchronous());
		Jet.Plume->SetTranslucentSortPriority(2);
		Jet.Heat->SetTranslucentSortPriority(1);
		Jet.PlumeMaterial = Jet.Plume->CreateDynamicMaterialInstance(0);
		Jet.HeatMaterial = Jet.Heat->CreateDynamicMaterialInstance(0);
		Jet.CoreMaterial = Jet.Core->CreateDynamicMaterialInstance(0);
		if (Jet.PlumeMaterial) Jet.PlumeMaterial->SetScalarParameterValue(TEXT("Seed"), Index * 2.71f);
		if (Jet.HeatMaterial) Jet.HeatMaterial->SetScalarParameterValue(TEXT("Seed"), Index * 2.71f);
		Jet.Light = NewObject<UPointLightComponent>(GetOwner(), Name(TEXT("Light")));
		Jet.Light->SetupAttachment(GetOwner()->GetRootComponent());
		Jet.Light->SetMobility(EComponentMobility::Movable);
		Jet.Light->SetLightColor(FLinearColor(1.f, .39f, .09f));
		Jet.Light->SetAttenuationRadius(180.f);
		Jet.Light->SetCastShadows(false);
		Jet.Light->SetIntensity(0.f);
		GetOwner()->AddInstanceComponent(Jet.Light);
		Jet.Light->RegisterComponent();
	}
	ResetVisuals();
}

void UFlyingCabThrusterVisualComponent::SubmitAcceleration(const FVector& ControlAcceleration, const FVector& CoastAcceleration)
{
	PendingDemand = FThrusterVisualDemand::Solve(ControlAcceleration, CoastAcceleration);
	bHasSample = true;
}

FVector UFlyingCabThrusterVisualComponent::GetExhaustDirection() const
{
	return FRotator(NozzlePitch, 0, 0).Vector();
}

void UFlyingCabThrusterVisualComponent::ResetVisuals()
{
	Demand = PendingDemand = FThrusterVisualDemand();
	bHasSample = false;
	DisplayedPower = MetalHeat = SurfaceTime = 0.f;
	NozzlePitch = -90.f;
	for (auto& Particle : Particles)
	{
		Particle.Lifetime = 0.f;
		Particle.Mesh->SetVisibility(false);
	}
	for (auto& Jet : Nozzles) Jet.SurfaceStrength = 0.f;
	UpdateNozzles(0.f);
}

void UFlyingCabThrusterVisualComponent::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);
	const auto* Cab = Cast<AFlyingCabPawn>(GetOwner());
	const auto* Body = Cab ? Cast<UPrimitiveComponent>(Cab->GetRootComponent()) : nullptr;
	if (!Cab || !Cab->IsPlayerControlled() || Cab->IsDestroyed() || Cab->GetFuel() <= UE_SMALL_NUMBER
		|| !Body || !Body->IsSimulatingPhysics())
	{
		ResetVisuals();
		return;
	}
	const float Dt = FMath::Clamp(DeltaSeconds, 0.f, .1f);
	Demand = bHasSample ? PendingDemand : FThrusterVisualDemand();
	bHasSample = false; // A missing vehicle tick can never latch an old firing command.
	VisualTime = FMath::Fmod(VisualTime + Dt, 4096.f);
	DisplayedPower = FMath::FInterpTo(DisplayedPower, Demand.Power, Dt, Demand.Power > DisplayedPower ? 14.f : 22.f);
	if (DisplayedPower < .002f) DisplayedPower = 0.f;
	if (Demand.Power > .003f)
	{
		const float TargetPitch = FMath::RadiansToDegrees(FMath::Atan2(Demand.ExhaustDirection.Z, Demand.ExhaustDirection.X));
		float Turn = FMath::FindDeltaAngleDegrees(NozzlePitch, TargetPitch);
		// A horizontal reversal sweeps through the space below the chassis.
		if (FMath::Abs(Turn) > 175.f) Turn = GetExhaustDirection().X < 0.f ? 180.f : -180.f;
		NozzlePitch = FRotator::NormalizeAxis(NozzlePitch + FMath::Clamp(Turn, -900.f * Dt, 900.f * Dt));
	}
	else if (DisplayedPower < .02f)
	{
		NozzlePitch = FMath::FixedTurn(NozzlePitch, -90.f, 100.f * Dt);
	}
	MetalHeat = FMath::FInterpTo(MetalHeat, FMath::Min(DisplayedPower, 1.f), Dt, Demand.Power > 0.f ? 5.f : 1.8f);
	UpdateNozzles(Dt);
	UpdateSurface(Dt);
}

void UFlyingCabThrusterVisualComponent::UpdateNozzles(float DeltaSeconds)
{
	const FVector Exhaust = GetExhaustDirection();
	const FQuat Rotation = FRotationMatrix::MakeFromXZ(Exhaust, FVector(0, 1, 0)).ToQuat();
	const FQuat BodyRotation = BodyVisual ? BodyVisual->GetComponentQuat() : GetOwner()->GetActorQuat();
	const float Alignment = Demand.Power > .003f ? FMath::Max(0.f, FVector::DotProduct(Exhaust, Demand.ExhaustDirection)) : 1.f;
	const float JetPower = DisplayedPower * Alignment * Alignment;
	for (int32 Index = 0; Index < Nozzles.Num(); ++Index)
	{
		auto& Jet = Nozzles[Index];
		// Outboard gimbals let a braking jet turn upward without passing through the hull.
		const FVector Mount = GetOwner()->GetActorLocation() + BodyRotation.RotateVector(FVector(Index == 0 ? -84 : 84, 65, 4));
		Jet.Mouth = Mount + Exhaust * 34.f;
		Jet.Joint->SetWorldLocation(Mount);
		Jet.Joint->SetWorldScale3D(FVector(.29f));
		Jet.Nozzle->SetWorldLocationAndRotation(Mount, Rotation);
		Jet.Nozzle->SetWorldScale3D(FVector(1));
		Jet.Core->SetWorldLocationAndRotation(Mount + Exhaust * 23.f, Rotation);
		Jet.Core->SetWorldScale3D(FVector(.06f, .26f, .26f));
		if (Jet.CoreMaterial) Jet.CoreMaterial->SetScalarParameterValue(TEXT("Heat"), MetalHeat);
		const float Length = (95.f + 175.f * FMath::Sqrt(FMath::Max(0.f, JetPower))) * PlumeLengthScale;
		for (auto* Mesh : {Jet.Plume.Get(), Jet.Heat.Get()}) Mesh->SetWorldLocationAndRotation(Jet.Mouth, Rotation);
		Jet.Plume->SetWorldScale3D(FVector(Length, 110.f, 1));
		Jet.Heat->SetWorldScale3D(FVector(Length * 1.65f, 155.f, 1));
		Jet.Plume->SetVisibility(JetPower > .005f);
		Jet.Heat->SetVisibility(bEnableHeatDistortion && JetPower > .02f);
		for (auto* Mat : {Jet.PlumeMaterial.Get(), Jet.HeatMaterial.Get()})
		{
			if (!Mat) continue;
			Mat->SetScalarParameterValue(TEXT("Power"), JetPower);
			Mat->SetScalarParameterValue(TEXT("Clock"), VisualTime);
		}
		Jet.Light->SetWorldLocation(Jet.Mouth + Exhaust * 12.f);
		Jet.Light->SetIntensity(65.f * JetPower * (.96f + .04f * FMath::Sin(VisualTime * 41.f + Index * 1.7f)));
	}
}

void UFlyingCabThrusterVisualComponent::CreateSurfacePool()
{
	if (!Particles.IsEmpty()) return;
	for (int32 Index = 0; Index < 24; ++Index)
	{
		auto& Particle = Particles.AddDefaulted_GetRef();
		Particle.Mesh = CreateMesh(FName(*FString::Printf(TEXT("ThrusterOutwash%d"), Index)), CardMesh.LoadSynchronous(), DustBase.LoadSynchronous());
		Particle.Mesh->SetVisibility(false);
		Particle.Material = Particle.Mesh->CreateDynamicMaterialInstance(0);
		if (Particle.Material) Particle.Material->SetScalarParameterValue(TEXT("Seed"), Index * .731f);
	}
}

void UFlyingCabThrusterVisualComponent::SpawnSurfaceParticle(const FThrusterNozzleVisual& Jet, float Side)
{
	if (Jet.SurfaceStrength < .02f) return;
	CreateSurfacePool();
	auto& Particle = Particles[NextParticle++ % Particles.Num()];
	const FVector Tangent = FVector::CrossProduct(Jet.SurfaceNormal, FVector(0, 1, 0)).GetSafeNormal();
	Particle.Position = Jet.SurfacePoint + Jet.SurfaceNormal * 9.f + FVector(0, 8, 0);
	Particle.Velocity = Tangent * Side * SurfaceRandom.FRandRange(90.f, 190.f) * FMath::Sqrt(Jet.SurfaceStrength)
		+ Jet.SurfaceNormal * SurfaceRandom.FRandRange(14.f, 35.f);
	Particle.Age = 0.f;
	Particle.bWet = Jet.bWetSurface;
	Particle.Lifetime = Particle.bWet ? .32f : .65f;
	Particle.Size = SurfaceRandom.FRandRange(24.f, 42.f);
	Particle.Strength = FMath::Min(Jet.SurfaceStrength, 1.f);
	Particle.Mesh->SetVisibility(true);
	if (Particle.Material) Particle.Material->SetVectorParameterValue(TEXT("Tint"),
		Particle.bWet ? FLinearColor(.38f, .48f, .53f) : FLinearColor(.27f, .24f, .20f));
}

void UFlyingCabThrusterVisualComponent::UpdateSurface(float DeltaSeconds)
{
	SurfaceTime += DeltaSeconds;
	if (SurfaceTime >= .05f)
	{
		SurfaceTime = FMath::Fmod(SurfaceTime, .05f);
		const FVector Exhaust = GetExhaustDirection();
		FCollisionQueryParams Query(SCENE_QUERY_STAT(ThrusterOutwash), false, GetOwner());
		for (auto& Jet : Nozzles)
		{
			Jet.SurfaceStrength = 0.f;
			FHitResult Hit;
			if (bEnableSurfaceOutwash && DisplayedPower > .03f && Demand.Power > .01f
				&& GetWorld()->LineTraceSingleByChannel(Hit, Jet.Mouth, Jet.Mouth + Exhaust * 360.f, ECC_Visibility, Query))
			{
				const auto* HitComponent = Hit.GetComponent();
				if (HitComponent && !HitComponent->ComponentHasTag(TEXT("ThrustNoDust")))
				{
					Jet.SurfacePoint = Hit.ImpactPoint;
					Jet.SurfaceNormal = Hit.ImpactNormal;
					Jet.SurfaceStrength = DisplayedPower * FMath::Square(1.f - Hit.Distance / 360.f)
						* FMath::Max(0.f, -FVector::DotProduct(Exhaust, Hit.ImpactNormal));
					Jet.bWetSurface = HitComponent->ComponentHasTag(TEXT("ThrustWet"));
				}
			}
			SpawnSurfaceParticle(Jet, -1.f);
			SpawnSurfaceParticle(Jet, 1.f);
		}
	}
	for (auto& Particle : Particles)
	{
		if (Particle.Lifetime <= 0.f) continue;
		Particle.Age += DeltaSeconds;
		const float Age = Particle.Age / Particle.Lifetime;
		if (Age >= 1.f)
		{
			Particle.Lifetime = 0.f;
			Particle.Mesh->SetVisibility(false);
			continue;
		}
		Particle.Position += Particle.Velocity * DeltaSeconds;
		Particle.Velocity *= FMath::Exp(-1.5f * DeltaSeconds);
		if (Particle.bWet) Particle.Velocity.Z -= 160.f * DeltaSeconds;
		const float Size = Particle.Size * (1.f + Age * 2.5f);
		// Cards stay in world space as the cab departs; they never drag the ground effect along.
		Particle.Mesh->SetWorldLocationAndRotation(Particle.Position - FVector(Size * .5f, 0, 0), FRotator(0, 0, 90));
		Particle.Mesh->SetWorldScale3D(FVector(Size, Size * .6f, 1));
		if (Particle.Material) Particle.Material->SetScalarParameterValue(TEXT("Alpha"),
			.30f * Particle.Strength * FMath::Sin(Age * PI));
	}
}
