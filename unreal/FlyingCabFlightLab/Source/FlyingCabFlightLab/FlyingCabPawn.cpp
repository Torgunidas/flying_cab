// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/SpringArmComponent.h"
#include "PhysicsEngine/BodyInstance.h"
#include "UObject/ConstructorHelpers.h"

AFlyingCabPawn::AFlyingCabPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	CollisionBody = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBody"));
	SetRootComponent(CollisionBody);
	CollisionBody->InitBoxExtent(FVector(110.0f, 45.0f, 35.0f));
	CollisionBody->SetCollisionProfileName(TEXT("Pawn"));
	CollisionBody->SetSimulatePhysics(true);
	CollisionBody->SetEnableGravity(true);
	CollisionBody->SetLinearDamping(0.05f);
	CollisionBody->SetAngularDamping(10.0f);
	CollisionBody->SetMassOverrideInKg(NAME_None, 100.0f, true);

	// Flight takes place on the X/Z plane. Y is the visual depth axis only.
	CollisionBody->BodyInstance.bLockYTranslation = true;
	CollisionBody->BodyInstance.bLockXRotation = true;
	CollisionBody->BodyInstance.bLockYRotation = true;
	CollisionBody->BodyInstance.bLockZRotation = true;
	CollisionBody->BodyInstance.SetDOFLock(EDOFMode::SixDOF);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(CollisionBody);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetRelativeScale3D(FVector(2.2f, 0.9f, 0.7f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CubeMesh.Object);
	}

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(CollisionBody);
	CameraBoom->TargetArmLength = 1600.0f;
	CameraBoom->SetRelativeRotation(FRotator(-5.0f, -90.0f, 0.0f));
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 5.0f;
	CameraBoom->bUsePawnControlRotation = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	Camera->FieldOfView = 55.0f;
	Camera->bUsePawnControlRotation = false;
}

void AFlyingCabPawn::BeginPlay()
{
	Super::BeginPlay();

	SpawnTransform = GetActorTransform();
	CollisionBody->WakeAllRigidBodies();

	if (GEngine && IsLocallyControlled())
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			15.0f,
			FColor(255, 210, 40),
			TEXT("FLIGHT LAB  |  W / SPACE / UP: thrust  |  A/D or arrows: steer  |  R: reset"));
	}
}

void AFlyingCabPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!CollisionBody || !CollisionBody->IsSimulatingPhysics())
	{
		return;
	}

	const float HorizontalInput = GetHorizontalInput();
	const float ThrustInput = GetThrustInput();
	const float Mass = CollisionBody->GetMass();

	CollisionBody->AddForce(FVector(
		HorizontalInput * HorizontalThrustAcceleration * Mass,
		0.0f,
		ThrustInput * VerticalThrustAcceleration * Mass));

	FVector Velocity = CollisionBody->GetPhysicsLinearVelocity();
	Velocity.Y = 0.0f;

	if (FMath::IsNearlyZero(HorizontalInput))
	{
		Velocity.X = FMath::FInterpTo(Velocity.X, 0.0f, DeltaSeconds, HorizontalCoastDamping);
	}

	if (FMath::IsNearlyZero(ThrustInput) && Velocity.Z > 0.0f)
	{
		Velocity.Z = FMath::FInterpTo(Velocity.Z, 0.0f, DeltaSeconds, UpwardCoastDamping);
	}

	Velocity.X = FMath::Clamp(Velocity.X, -MaxHorizontalSpeed, MaxHorizontalSpeed);
	Velocity.Z = FMath::Clamp(Velocity.Z, -MaxFallSpeed, MaxClimbSpeed);
	CollisionBody->SetPhysicsLinearVelocity(Velocity);
}

void AFlyingCabPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);
	PlayerInputComponent->BindAxis(TEXT("FlyHorizontal"), this, &AFlyingCabPawn::SetKeyboardHorizontalInput);
	PlayerInputComponent->BindAxis(TEXT("FlyThrust"), this, &AFlyingCabPawn::SetKeyboardThrustInput);
	PlayerInputComponent->BindAction(TEXT("RestartFlight"), IE_Pressed, this, &AFlyingCabPawn::ResetVehicle);
}

void AFlyingCabPawn::SetTouchHorizontalInput(float Value)
{
	TouchHorizontalInput = FMath::Clamp(Value, -1.0f, 1.0f);
}

void AFlyingCabPawn::SetTouchThrustPressed(bool bPressed)
{
	TouchThrustInput = bPressed ? 1.0f : 0.0f;
}

void AFlyingCabPawn::ResetVehicle()
{
	KeyboardHorizontalInput = 0.0f;
	KeyboardThrustInput = 0.0f;
	TouchHorizontalInput = 0.0f;
	TouchThrustInput = 0.0f;

	CollisionBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
	CollisionBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
	CollisionBody->WakeAllRigidBodies();
}

void AFlyingCabPawn::SetKeyboardHorizontalInput(float Value)
{
	KeyboardHorizontalInput = FMath::Clamp(Value, -1.0f, 1.0f);
}

void AFlyingCabPawn::SetKeyboardThrustInput(float Value)
{
	KeyboardThrustInput = FMath::Clamp(Value, 0.0f, 1.0f);
}

float AFlyingCabPawn::GetHorizontalInput() const
{
	return FMath::Clamp(KeyboardHorizontalInput + TouchHorizontalInput, -1.0f, 1.0f);
}

float AFlyingCabPawn::GetThrustInput() const
{
	return FMath::Max(KeyboardThrustInput, TouchThrustInput);
}
