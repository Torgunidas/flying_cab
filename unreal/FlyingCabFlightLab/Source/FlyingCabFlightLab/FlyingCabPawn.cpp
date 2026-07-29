// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InputComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoreGlobals.h"
#include "Engine/Engine.h"
#include "FlyingCabTouchControls.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "PhysicsEngine/BodyInstance.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabFlight, Log, All);

namespace
{
	constexpr uint64 FlightTelemetryMessageKey = 0xFCAB0001ULL;
}

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
	CollisionBody->SetNotifyRigidBodyCollision(true);
	CollisionBody->OnComponentHit.AddDynamic(this, &AFlyingCabPawn::HandleCollisionHit);

	// Flight takes place on the X/Z plane. Y is the visual depth axis only.
	CollisionBody->BodyInstance.bLockYTranslation = true;
	CollisionBody->BodyInstance.bLockXRotation = true;
	CollisionBody->BodyInstance.bLockYRotation = true;
	CollisionBody->BodyInstance.bLockZRotation = true;

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

	DamageLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("DamageLight"));
	DamageLight->SetupAttachment(CollisionBody);
	DamageLight->SetRelativeLocation(FVector(0.0f, 70.0f, 0.0f));
	DamageLight->SetLightColor(FLinearColor(1.0f, 0.05f, 0.0f));
	DamageLight->SetIntensity(0.0f);
	DamageLight->SetAttenuationRadius(500.0f);
	DamageLight->SetCastShadows(false);
}

void AFlyingCabPawn::BeginPlay()
{
	Super::BeginPlay();

	CollisionBody->SetMassOverrideInKg(NAME_None, 100.0f, true);
	CollisionBody->BodyInstance.SetDOFLock(EDOFMode::SixDOF);
	SpawnTransform = GetActorTransform();
	CurrentFuel = FMath::Clamp(StartingFuel, 0.0f, MaxFuel);
	CurrentHull = MaxHull;
	CollisionBody->WakeAllRigidBodies();

	UE_LOG(
		LogFlyingCabFlight,
		Display,
		TEXT("Using pawn class %s (visual pitch response %.2f, return %.2f)."),
		*GetClass()->GetPathName(),
		VisualPitchResponseSpeed,
		VisualPitchReturnSpeed);

	TryCreateTouchControls();
	RefreshResourceUI();
}

void AFlyingCabPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (TouchControlsWidget)
	{
		TouchControlsWidget->ReleaseAllInputs();
		TouchControlsWidget->RemoveFromParent();
		TouchControlsWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void AFlyingCabPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TryCreateTouchControls();
	UpdateKeyboardInputState();

	if (!CollisionBody || !CollisionBody->IsSimulatingPhysics())
	{
		return;
	}

	DamageCooldownRemaining = FMath::Max(0.0f, DamageCooldownRemaining - DeltaSeconds);
	DamageFlashRemaining = FMath::Max(0.0f, DamageFlashRemaining - DeltaSeconds);
	if (DamageLight)
	{
		const float DamageIntensity = bDestroyed
			? 6500.0f
			: (DamageFlashRemaining > 0.0f ? 3500.0f : 0.0f);
		DamageLight->SetIntensity(DamageIntensity);
	}

	const float RequestedHorizontalInput = GetHorizontalInput();
	const float RequestedThrustInput = GetThrustInput();
	const bool bCanUseThrusters = !bDestroyed && CurrentFuel > UE_SMALL_NUMBER;
	const float HorizontalInput = bCanUseThrusters ? RequestedHorizontalInput : 0.0f;
	const float ThrustInput = bCanUseThrusters ? RequestedThrustInput : 0.0f;
	const float Mass = CollisionBody->GetMass();

	CollisionBody->AddForce(FVector(
		HorizontalInput * HorizontalThrustAcceleration * Mass,
		0.0f,
		ThrustInput * VerticalThrustAcceleration * Mass));

	FVector Velocity = CollisionBody->GetPhysicsLinearVelocity();
	Velocity.Y = 0.0f;
	ConsumeOrRegenerateFuel(
		DeltaSeconds,
		RequestedHorizontalInput,
		RequestedThrustInput,
		Velocity);

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

	UpdateVisualResponse(DeltaSeconds, Velocity);
	DrawFlightTelemetry(HorizontalInput, ThrustInput, Velocity);
	RefreshResourceUI();
}

void AFlyingCabPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);
	PlayerInputComponent->BindAction(TEXT("RestartFlight"), IE_Pressed, this, &AFlyingCabPawn::ResetVehicle);
	PlayerInputComponent->BindAction(TEXT("ToggleFlightTelemetry"), IE_Pressed, this, &AFlyingCabPawn::ToggleFlightTelemetry);
	PlayerInputComponent->BindAction(TEXT("ToggleTouchControls"), IE_Pressed, this, &AFlyingCabPawn::ToggleTouchControls);
}

void AFlyingCabPawn::SetTouchHorizontalInput(float Value)
{
	TouchHorizontalInput = FMath::Clamp(Value, -1.0f, 1.0f);
}

void AFlyingCabPawn::SetTouchThrustPressed(bool bPressed)
{
	TouchThrustInput = bPressed ? 1.0f : 0.0f;
}

void AFlyingCabPawn::SetTouchRefuelPressed(bool bPressed)
{
	bTouchRefuelPressed = bPressed;
}

void AFlyingCabPawn::ResetVehicle()
{
	ClearAllInputState(TEXT("manual reset"), true);
	if (bDestroyed)
	{
		return;
	}

	CurrentFuel = FMath::Clamp(StartingFuel, 0.0f, MaxFuel);
	CurrentHull = MaxHull;
	DamageCooldownRemaining = 0.0f;
	DamageFlashRemaining = 0.0f;
	bFuelEmptyWarningShown = false;
	VisualMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(1.0f, 1.0f, 1.0f));
	if (DamageLight)
	{
		DamageLight->SetIntensity(0.0f);
	}

	CollisionBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
	CollisionBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
	CollisionBody->WakeAllRigidBodies();
	VisualMesh->SetRelativeRotation(FRotator::ZeroRotator);
	CameraBoom->TargetOffset = FVector::ZeroVector;
	PreviousHorizontalVelocity = 0.0f;
	VisualHorizontalAcceleration = 0.0f;
	bHasPreviousHorizontalVelocity = false;
	RefreshResourceUI();
	UE_LOG(
		LogFlyingCabFlight,
		Display,
		TEXT("Manual reset restored starting vehicle resources: fuel %.1f/%.1f, hull %.1f/%.1f."),
		CurrentFuel,
		MaxFuel,
		CurrentHull,
		MaxHull);
}

void AFlyingCabPawn::SetObjectiveStatus(const FText& Status)
{
	ObjectiveStatus = Status;
	if (TouchControlsWidget)
	{
		TouchControlsWidget->SetObjectiveText(ObjectiveStatus);
	}
}

void AFlyingCabPawn::SetMinimapState(
	const FVector2D& CabWorldPosition,
	const FVector2D& TargetWorldPosition,
	bool bTargetIsDropoff)
{
	MinimapCabPosition = CabWorldPosition;
	MinimapTargetPosition = TargetWorldPosition;
	bMinimapTargetIsDropoff = bTargetIsDropoff;
	bHasMinimapState = true;
	bMinimapTargetVisible = true;

	if (TouchControlsWidget)
	{
		TouchControlsWidget->SetMinimapState(
			MinimapCabPosition,
			MinimapTargetPosition,
			bMinimapTargetIsDropoff);
	}
}

void AFlyingCabPawn::ClearMinimapTarget()
{
	bMinimapTargetVisible = false;
	if (TouchControlsWidget)
	{
		TouchControlsWidget->SetMinimapTargetVisible(false);
	}
}

void AFlyingCabPawn::SetEconomyStatus(int32 Credits, int32 ActiveFare)
{
	DisplayCredits = FMath::Max(0, Credits);
	DisplayActiveFare = FMath::Max(0, ActiveFare);
	RefreshResourceUI();
}

void AFlyingCabPawn::SetTrafficAlert(const FText& Alert, const FLinearColor& Color)
{
	TrafficAlert = Alert;
	TrafficAlertColor = Color;
	if (TouchControlsWidget)
	{
		TouchControlsWidget->SetTrafficAlert(TrafficAlert, TrafficAlertColor);
	}
}

void AFlyingCabPawn::SetRefuelAvailable(bool bAvailable, int32 PricePerUnit)
{
	bRefuelAvailable = bAvailable && !bDestroyed;
	RefuelPricePerUnit = FMath::Max(0, PricePerUnit);
	if (!bRefuelAvailable && !bRepairAvailable)
	{
		bTouchRefuelPressed = false;
	}
	RefreshResourceUI();
}

void AFlyingCabPawn::SetRepairAvailable(bool bAvailable, int32 PricePerHullUnit)
{
	bRepairAvailable = bAvailable && !bDestroyed;
	RepairPricePerHullUnit = FMath::Max(0, PricePerHullUnit);
	if (!bRepairAvailable && !bRefuelAvailable)
	{
		bTouchRefuelPressed = false;
	}
	RefreshResourceUI();
}

bool AFlyingCabPawn::IsRefuelRequested() const
{
	return bRefuelAvailable && !bDestroyed
		&& (bKeyboardRefuelPressed || bTouchRefuelPressed);
}

bool AFlyingCabPawn::IsRepairRequested() const
{
	return bRepairAvailable && !bDestroyed
		&& (bKeyboardRefuelPressed || bTouchRefuelPressed);
}

float AFlyingCabPawn::AddFuel(float Units)
{
	if (Units <= 0.0f || MaxFuel <= UE_SMALL_NUMBER)
	{
		return 0.0f;
	}

	const float PreviousFuel = CurrentFuel;
	CurrentFuel = FMath::Clamp(CurrentFuel + Units, 0.0f, MaxFuel);
	if (CurrentFuel > 1.0f)
	{
		bFuelEmptyWarningShown = false;
	}
	RefreshResourceUI();
	return CurrentFuel - PreviousFuel;
}

float AFlyingCabPawn::AddHull(float Units)
{
	if (Units <= 0.0f || MaxHull <= UE_SMALL_NUMBER || bDestroyed)
	{
		return 0.0f;
	}

	const float PreviousHull = CurrentHull;
	CurrentHull = FMath::Clamp(CurrentHull + Units, 0.0f, MaxHull);
	RefreshResourceUI();
	return CurrentHull - PreviousHull;
}

void AFlyingCabPawn::RecoverVehicle(float RecoveryFuelPercent)
{
	ClearAllInputState(TEXT("tow recovery"), true);
	bDestroyed = false;
	CurrentHull = MaxHull;
	CurrentFuel = FMath::Max(
		CurrentFuel,
		MaxFuel * FMath::Clamp(RecoveryFuelPercent, 0.0f, 1.0f));
	DamageCooldownRemaining = 0.0f;
	DamageFlashRemaining = 0.0f;
	bFuelEmptyWarningShown = false;

	CollisionBody->SetSimulatePhysics(true);
	CollisionBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
	CollisionBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
	CollisionBody->WakeAllRigidBodies();
	VisualMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(1.0f, 1.0f, 1.0f));
	if (DamageLight)
	{
		DamageLight->SetIntensity(0.0f);
	}
	RefreshResourceUI();
}

void AFlyingCabPawn::UpdateKeyboardInputState()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		KeyboardHorizontalInput = 0.0f;
		KeyboardThrustInput = 0.0f;
		bKeyboardRefuelPressed = false;
		bKeyA = false;
		bKeyD = false;
		bKeyLeft = false;
		bKeyRight = false;
		bKeyW = false;
		bKeyUp = false;
		bKeySpace = false;
		bKeyService = false;
		return;
	}

	bKeyA = PlayerController->IsInputKeyDown(EKeys::A);
	bKeyD = PlayerController->IsInputKeyDown(EKeys::D);
	bKeyLeft = PlayerController->IsInputKeyDown(EKeys::Left);
	bKeyRight = PlayerController->IsInputKeyDown(EKeys::Right);
	bKeyW = PlayerController->IsInputKeyDown(EKeys::W);
	bKeyUp = PlayerController->IsInputKeyDown(EKeys::Up);
	bKeySpace = PlayerController->IsInputKeyDown(EKeys::SpaceBar);
	bKeyService = PlayerController->IsInputKeyDown(EKeys::E);

	const float RawHorizontalInput = FMath::Clamp(
		static_cast<float>(bKeyD || bKeyRight) - static_cast<float>(bKeyA || bKeyLeft),
		-1.0f,
		1.0f);
	const float RawThrustInput = (bKeyW || bKeyUp || bKeySpace) ? 1.0f : 0.0f;
	bKeyboardRefuelPressed = bKeyService;

	if (bSuppressKeyboardHorizontalUntilNeutral)
	{
		if (FMath::IsNearlyZero(RawHorizontalInput) && GFrameCounter > KeyboardInputSuppressionFrame)
		{
			bSuppressKeyboardHorizontalUntilNeutral = false;
		}
		KeyboardHorizontalInput = 0.0f;
	}
	else
	{
		KeyboardHorizontalInput = RawHorizontalInput;
	}

	if (bSuppressKeyboardThrustUntilNeutral)
	{
		if (FMath::IsNearlyZero(RawThrustInput) && GFrameCounter > KeyboardInputSuppressionFrame)
		{
			bSuppressKeyboardThrustUntilNeutral = false;
		}
		KeyboardThrustInput = 0.0f;
	}
	else
	{
		KeyboardThrustInput = RawThrustInput;
	}
}

void AFlyingCabPawn::ClearAllInputState(const TCHAR* Reason, bool bFlushPressedKeys)
{
	const float PreviousKeyboardHorizontal = KeyboardHorizontalInput;
	const float PreviousKeyboardThrust = KeyboardThrustInput;
	const float PreviousTouchHorizontal = TouchHorizontalInput;
	const float PreviousTouchThrust = TouchThrustInput;
	const bool bHadActiveInput = !FMath::IsNearlyZero(PreviousKeyboardHorizontal)
		|| !FMath::IsNearlyZero(PreviousKeyboardThrust)
		|| !FMath::IsNearlyZero(PreviousTouchHorizontal)
		|| !FMath::IsNearlyZero(PreviousTouchThrust)
		|| bKeyboardRefuelPressed
		|| bTouchRefuelPressed;

	if (bFlushPressedKeys)
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
		{
			PlayerController->FlushPressedKeys();
		}
	}
	if (TouchControlsWidget)
	{
		TouchControlsWidget->ReleaseAllInputs();
	}

	KeyboardHorizontalInput = 0.0f;
	KeyboardThrustInput = 0.0f;
	TouchHorizontalInput = 0.0f;
	TouchThrustInput = 0.0f;
	bKeyboardRefuelPressed = false;
	bTouchRefuelPressed = false;
	KeyboardInputSuppressionFrame = GFrameCounter;
	bSuppressKeyboardHorizontalUntilNeutral = true;
	bSuppressKeyboardThrustUntilNeutral = true;
	bKeyA = false;
	bKeyD = false;
	bKeyLeft = false;
	bKeyRight = false;
	bKeyW = false;
	bKeyUp = false;
	bKeySpace = false;
	bKeyService = false;
	++ForcedInputResetCount;

	const FString ClearSummary = FString::Printf(
		TEXT("Input state cleared during %s (keyboard X %.2f, thrust %.2f; touch X %.2f, thrust %.2f). Waiting for neutral axes."),
		Reason ? Reason : TEXT("unknown transition"),
		PreviousKeyboardHorizontal,
		PreviousKeyboardThrust,
		PreviousTouchHorizontal,
		PreviousTouchThrust);
	if (bHadActiveInput)
	{
		UE_LOG(LogFlyingCabFlight, Warning, TEXT("%s"), *ClearSummary);
	}
	else
	{
		UE_LOG(LogFlyingCabFlight, Display, TEXT("%s"), *ClearSummary);
	}
}

void AFlyingCabPawn::ToggleFlightTelemetry()
{
	bShowFlightTelemetry = !bShowFlightTelemetry;

	if (!bShowFlightTelemetry && GEngine)
	{
		GEngine->RemoveOnScreenDebugMessage(FlightTelemetryMessageKey);
	}
}

void AFlyingCabPawn::TryCreateTouchControls()
{
	if (TouchControlsWidget || !IsLocallyControlled())
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	TouchControlsWidget = CreateWidget<UFlyingCabTouchControls>(PlayerController);
	if (!TouchControlsWidget)
	{
		return;
	}

	TouchControlsWidget->AddToViewport(100);
	TouchControlsWidget->SetObjectiveText(ObjectiveStatus);
	TouchControlsWidget->SetTrafficAlert(TrafficAlert, TrafficAlertColor);
	RefreshResourceUI();
	if (bHasMinimapState)
	{
		TouchControlsWidget->SetMinimapState(
			MinimapCabPosition,
			MinimapTargetPosition,
			bMinimapTargetIsDropoff);
	}
	TouchControlsWidget->SetMinimapTargetVisible(bMinimapTargetVisible);
	ApplyTouchControlsVisibility();
	UE_LOG(LogFlyingCabFlight, Display, TEXT("Touch controls created (F4 toggles visibility)."));
}

void AFlyingCabPawn::ToggleTouchControls()
{
	bShowTouchControls = !bShowTouchControls;
	ApplyTouchControlsVisibility();
}

void AFlyingCabPawn::ApplyTouchControlsVisibility()
{
	if (!TouchControlsWidget)
	{
		return;
	}

	TouchControlsWidget->SetControlsVisible(bShowTouchControls);

#if WITH_EDITOR
	if (bEnableMouseTouchTestingInEditor)
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
		{
			PlayerController->SetShowMouseCursor(bShowTouchControls);
			if (bShowTouchControls)
			{
				FInputModeGameAndUI InputMode;
				InputMode.SetHideCursorDuringCapture(false);
				PlayerController->SetInputMode(InputMode);
			}
			else
			{
				PlayerController->SetInputMode(FInputModeGameOnly());
			}
		}
	}
#endif
}

void AFlyingCabPawn::RefreshResourceUI()
{
	if (!TouchControlsWidget)
	{
		return;
	}

	const float FuelPercent = MaxFuel > UE_SMALL_NUMBER ? CurrentFuel / MaxFuel : 0.0f;
	const float HullPercent = MaxHull > UE_SMALL_NUMBER ? CurrentHull / MaxHull : 0.0f;
	TouchControlsWidget->SetResourceState(
		FuelPercent,
		HullPercent,
		DisplayCredits,
		DisplayActiveFare,
		bRefuelAvailable,
		RefuelPricePerUnit,
		bRepairAvailable,
		RepairPricePerHullUnit,
		bDestroyed);
}

void AFlyingCabPawn::ConsumeOrRegenerateFuel(
	float DeltaSeconds,
	float HorizontalInput,
	float ThrustInput,
	const FVector& Velocity)
{
	if (bDestroyed || MaxFuel <= UE_SMALL_NUMBER)
	{
		return;
	}

	const bool bUsingThrusters = CurrentFuel > UE_SMALL_NUMBER
		&& (!FMath::IsNearlyZero(HorizontalInput) || ThrustInput > UE_SMALL_NUMBER);
	if (bUsingThrusters)
	{
		const float FuelUsed = (
			FMath::Abs(HorizontalInput) * HorizontalFuelPerSecond
			+ ThrustInput * VerticalFuelPerSecond) * DeltaSeconds;
		CurrentFuel = FMath::Max(0.0f, CurrentFuel - FuelUsed);
	}
	else if (FMath::IsNearlyZero(HorizontalInput)
		&& ThrustInput <= UE_SMALL_NUMBER
		&& Velocity.Z < 0.0f)
	{
		const float RegenerationRatio = RegenerationFullSpeed > UE_SMALL_NUMBER
			? FMath::Clamp(-Velocity.Z / RegenerationFullSpeed, 0.0f, 1.0f)
			: 1.0f;
		CurrentFuel = FMath::Min(
			MaxFuel,
			CurrentFuel + DescentRegenerationPerSecond * RegenerationRatio * DeltaSeconds);
	}

	if (CurrentFuel <= UE_SMALL_NUMBER && !bFuelEmptyWarningShown)
	{
		bFuelEmptyWarningShown = true;
		if (GEngine && IsLocallyControlled())
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				3.0f,
				FColor(255, 80, 30),
				TEXT("ENERGY EMPTY // DESCEND TO RECOVER A RESERVE OR REACH A FUEL STATION"));
		}
	}
}

void AFlyingCabPawn::HandleCollisionHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (bDestroyed
		|| DamageCooldownRemaining > 0.0f
		|| !CollisionBody)
	{
		return;
	}

	const float Mass = FMath::Max(CollisionBody->GetMass(), 1.0f);
	const float NormalSpeedChange = NormalImpulse.Size() / Mass;
	ApplyCollisionDamage(NormalSpeedChange);
}

void AFlyingCabPawn::ApplyCollisionDamage(float NormalSpeedChange)
{
	if (NormalSpeedChange <= DamageImpactSpeedThreshold || bDestroyed)
	{
		return;
	}

	const float DamageRange = FMath::Max(
		DamageFullHullSpeed - DamageImpactSpeedThreshold,
		1.0f);
	const float ImpactAlpha = FMath::Clamp(
		(NormalSpeedChange - DamageImpactSpeedThreshold) / DamageRange,
		0.0f,
		1.0f);
	const float Damage = MaxHull * FMath::Pow(ImpactAlpha, CollisionDamageExponent);
	if (Damage <= UE_SMALL_NUMBER)
	{
		return;
	}

	CurrentHull = FMath::Max(0.0f, CurrentHull - Damage);
	DamageCooldownRemaining = CollisionDamageCooldown;
	DamageFlashRemaining = 0.12f;
	UE_LOG(
		LogFlyingCabFlight,
		Display,
		TEXT("Collision damage %.1f from normal speed change %.1f cm/s. Hull %.1f/%.1f."),
		Damage,
		NormalSpeedChange,
		CurrentHull,
		MaxHull);

	if (GEngine && IsLocallyControlled())
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			1.2f,
			FColor(255, 110, 35),
			FString::Printf(TEXT("IMPACT // HULL %.0f%%"), 100.0f * CurrentHull / MaxHull));
	}

	if (CurrentHull <= UE_SMALL_NUMBER)
	{
		EnterDestroyedState();
	}
}

void AFlyingCabPawn::EnterDestroyedState()
{
	if (bDestroyed)
	{
		return;
	}

	bDestroyed = true;
	ClearAllInputState(TEXT("vehicle destroyed"), true);
	bRefuelAvailable = false;
	bRepairAvailable = false;
	VisualMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.20f, 0.01f, 0.0f));
	RefreshResourceUI();

	UE_LOG(LogFlyingCabFlight, Warning, TEXT("Vehicle destroyed."));
	OnVehicleDestroyed.Broadcast(this);
}

void AFlyingCabPawn::DrawFlightTelemetry(float HorizontalInput, float ThrustInput, const FVector& Velocity) const
{
	if (!bShowFlightTelemetry || !GEngine || !IsLocallyControlled())
	{
		return;
	}

	const FVector Position = GetActorLocation();
	const float PlanarSpeed = FVector2D(Velocity.X, Velocity.Z).Size();
	const TCHAR* PhysicsState = CollisionBody->IsAnyRigidBodyAwake() ? TEXT("AWAKE") : TEXT("SLEEPING");
	const float VisualPitch = VisualMesh->GetRelativeRotation().Pitch;
	const FVector CameraOffset = CameraBoom->TargetOffset;

	const FString Telemetry = FString::Printf(
		TEXT("FLIGHT TELEMETRY [F3]  |  Physics: %s\n")
		TEXT("Horizontal  keyboard:%+.2f  touch:%+.2f  effective:%+.2f\n")
		TEXT("Thrust      keyboard:%+.2f  touch:%+.2f  effective:%+.2f\n")
		TEXT("Keys        A:%d D:%d LEFT:%d RIGHT:%d  W:%d UP:%d SPACE:%d E:%d\n")
		TEXT("Velocity    X:%+7.1f  Z:%+7.1f  planar:%7.1f cm/s\n")
		TEXT("Command     X:%+7.1f  Z:%+7.1f cm/s^2\n")
		TEXT("Position    X:%+7.1f  Z:%+7.1f cm\n")
		TEXT("Presentation accel X:%+7.1f  pitch:%+5.1f deg  camera X:%+6.1f  Z:%+6.1f cm\n")
		TEXT("Resources   fuel:%5.1f/%5.1f  hull:%5.1f/%5.1f  destroyed:%s\n")
		TEXT("Input reset guard  horizontal:%s  thrust:%s  forced clears:%u"),
		PhysicsState,
		KeyboardHorizontalInput,
		TouchHorizontalInput,
		HorizontalInput,
		KeyboardThrustInput,
		TouchThrustInput,
		ThrustInput,
		bKeyA ? 1 : 0,
		bKeyD ? 1 : 0,
		bKeyLeft ? 1 : 0,
		bKeyRight ? 1 : 0,
		bKeyW ? 1 : 0,
		bKeyUp ? 1 : 0,
		bKeySpace ? 1 : 0,
		bKeyService ? 1 : 0,
		Velocity.X,
		Velocity.Z,
		PlanarSpeed,
		HorizontalInput * HorizontalThrustAcceleration,
		ThrustInput * VerticalThrustAcceleration,
		Position.X,
		Position.Z,
		VisualHorizontalAcceleration,
		VisualPitch,
		CameraOffset.X,
		CameraOffset.Z,
		CurrentFuel,
		MaxFuel,
		CurrentHull,
		MaxHull,
		bDestroyed ? TEXT("YES") : TEXT("NO"),
		bSuppressKeyboardHorizontalUntilNeutral ? TEXT("WAITING") : TEXT("READY"),
		bSuppressKeyboardThrustUntilNeutral ? TEXT("WAITING") : TEXT("READY"),
		ForcedInputResetCount);

	GEngine->AddOnScreenDebugMessage(
		FlightTelemetryMessageKey,
		0.1f,
		FColor(80, 220, 255),
		Telemetry,
		false);
}

void AFlyingCabPawn::UpdateVisualResponse(float DeltaSeconds, const FVector& Velocity)
{
	if (bHasPreviousHorizontalVelocity && DeltaSeconds > UE_SMALL_NUMBER)
	{
		VisualHorizontalAcceleration = (Velocity.X - PreviousHorizontalVelocity) / DeltaSeconds;
	}
	else
	{
		VisualHorizontalAcceleration = 0.0f;
	}

	PreviousHorizontalVelocity = Velocity.X;
	bHasPreviousHorizontalVelocity = true;

	const float AccelerationRatio = VisualPitchFullAcceleration > UE_SMALL_NUMBER
		? FMath::Clamp(VisualHorizontalAcceleration / VisualPitchFullAcceleration, -1.0f, 1.0f)
		: 0.0f;
	const float TargetPitch = -AccelerationRatio * MaxVisualPitchDegrees;
	const float CurrentPitch = VisualMesh->GetRelativeRotation().Pitch;
	const bool bReturningToLevel = FMath::IsNearlyZero(TargetPitch)
		|| FMath::Abs(TargetPitch) < FMath::Abs(CurrentPitch);
	const float PitchInterpSpeed = bReturningToLevel
		? VisualPitchReturnSpeed
		: VisualPitchResponseSpeed;
	const float NewPitch = FMath::FInterpTo(
		CurrentPitch,
		TargetPitch,
		DeltaSeconds,
		PitchInterpSpeed);
	VisualMesh->SetRelativeRotation(FRotator(NewPitch, 0.0f, 0.0f));

	const float HorizontalRatio = MaxHorizontalSpeed > UE_SMALL_NUMBER
		? FMath::Clamp(Velocity.X / MaxHorizontalSpeed, -1.0f, 1.0f)
		: 0.0f;
	const float VerticalSpeedLimit = Velocity.Z >= 0.0f ? MaxClimbSpeed : MaxFallSpeed;
	const float VerticalRatio = VerticalSpeedLimit > UE_SMALL_NUMBER
		? FMath::Clamp(Velocity.Z / VerticalSpeedLimit, -1.0f, 1.0f)
		: 0.0f;
	const FVector TargetOffset(
		HorizontalRatio * HorizontalCameraLookAhead,
		0.0f,
		VerticalRatio * VerticalCameraLookAhead);

	CameraBoom->TargetOffset = FMath::VInterpTo(
		CameraBoom->TargetOffset,
		TargetOffset,
		DeltaSeconds,
		CameraLookAheadInterpSpeed);
}

float AFlyingCabPawn::GetHorizontalInput() const
{
	return FMath::Clamp(KeyboardHorizontalInput + TouchHorizontalInput, -1.0f, 1.0f);
}

float AFlyingCabPawn::GetThrustInput() const
{
	return FMath::Max(KeyboardThrustInput, TouchThrustInput);
}
