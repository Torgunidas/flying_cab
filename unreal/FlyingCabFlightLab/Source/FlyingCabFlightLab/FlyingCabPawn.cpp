// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabPawn.h"

#include "EnhancedInputComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InputComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "FlyingCabInputData.h"
#include "FlyingCabProgressionSubsystem.h"
#include "FlyingCabPlayerController.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInterface.h"
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
	Vitals = CreateDefaultSubobject<UFlyingCabVehicleVitalsComponent>(TEXT("Vitals"));

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(CollisionBody);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetRelativeScale3D(FVector(2.2f, 0.9f, 0.7f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (CubeMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CubeMesh.Object);
	}

	DamageLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("DamageLight"));
	DamageLight->SetupAttachment(CollisionBody);
	DamageLight->SetRelativeLocation(FVector(0.0f, 70.0f, 0.0f));
	DamageLight->SetLightColor(FLinearColor(1.0f, 0.05f, 0.0f));
	DamageLight->SetIntensity(0.0f);
	DamageLight->SetAttenuationRadius(500.0f);
	DamageLight->SetCastShadows(false);

	VehicleLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("VehicleLabel"));
	VehicleLabel->SetupAttachment(CollisionBody);
	VehicleLabel->SetRelativeLocation(FVector(0.0f, 58.0f, 92.0f));
	VehicleLabel->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	VehicleLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	VehicleLabel->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	VehicleLabel->SetWorldSize(23.0f);
	VehicleLabel->SetVisibility(false, true);

	AccessLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("AccessLight"));
	AccessLight->SetupAttachment(CollisionBody);
	AccessLight->SetRelativeLocation(FVector(0.0f, 70.0f, 72.0f));
	AccessLight->SetIntensity(0.0f);
	AccessLight->SetAttenuationRadius(420.0f);
	AccessLight->SetCastShadows(false);

	PlayerFocusHalo = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlayerFocusHalo"));
	PlayerFocusHalo->SetupAttachment(CollisionBody);
	PlayerFocusHalo->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlayerFocusHalo->SetRelativeLocation(FVector(0.0f, -52.0f, 0.0f));
	PlayerFocusHalo->SetRelativeRotation(FRotator(0.0f, 0.0f, 90.0f));
	PlayerFocusHalo->SetRelativeScale3D(FVector(3.0f, 1.35f, 0.03f));
	PlayerFocusHalo->SetCastShadow(false);
	PlayerFocusHalo->SetReceivesDecals(false);
	PlayerFocusHalo->SetVisibility(false, true);
	if (CylinderMesh.Succeeded())
	{
		PlayerFocusHalo->SetStaticMesh(CylinderMesh.Object);
	}
	if (BasicMaterial.Succeeded())
	{
		PlayerFocusHalo->SetMaterial(0, BasicMaterial.Object);
	}

	PlayerFocusLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PlayerFocusLight"));
	PlayerFocusLight->SetupAttachment(CollisionBody);
	PlayerFocusLight->SetRelativeLocation(FVector(0.0f, 75.0f, 0.0f));
	PlayerFocusLight->SetLightColor(PlayerFocusColor);
	PlayerFocusLight->SetIntensity(0.0f);
	PlayerFocusLight->SetAttenuationRadius(460.0f);
	PlayerFocusLight->SetCastShadows(false);

	GuidanceArrowRoot = CreateDefaultSubobject<USceneComponent>(TEXT("GuidanceArrowRoot"));
	GuidanceArrowRoot->SetupAttachment(CollisionBody);
	GuidanceArrowRoot->SetRelativeLocation(FVector(0.0f, 72.0f, 155.0f));
	GuidanceArrowRoot->SetMobility(EComponentMobility::Movable);

	GuidanceArrowShaft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GuidanceArrowShaft"));
	GuidanceArrowShaft->SetupAttachment(GuidanceArrowRoot);
	GuidanceArrowShaft->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GuidanceArrowShaft->SetRelativeLocation(FVector(30.0f, 0.0f, 0.0f));
	GuidanceArrowShaft->SetRelativeScale3D(FVector(0.60f, 0.07f, 0.07f));
	if (CubeMesh.Succeeded())
	{
		GuidanceArrowShaft->SetStaticMesh(CubeMesh.Object);
	}

	GuidanceArrowTip = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GuidanceArrowTip"));
	GuidanceArrowTip->SetupAttachment(GuidanceArrowRoot);
	GuidanceArrowTip->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GuidanceArrowTip->SetRelativeLocation(FVector(70.0f, 0.0f, 0.0f));
	GuidanceArrowTip->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	GuidanceArrowTip->SetRelativeScale3D(FVector(0.22f, 0.22f, 0.30f));
	if (ConeMesh.Succeeded())
	{
		GuidanceArrowTip->SetStaticMesh(ConeMesh.Object);
	}
	for (UStaticMeshComponent* ArrowPart : {GuidanceArrowShaft, GuidanceArrowTip})
	{
		if (BasicMaterial.Succeeded())
		{
			ArrowPart->SetMaterial(0, BasicMaterial.Object);
		}
		ArrowPart->SetVisibility(false, true);
	}
}

void AFlyingCabPawn::BeginPlay()
{
	Super::BeginPlay();

	CollisionBody->SetMassOverrideInKg(NAME_None, 100.0f, true);
	CollisionBody->BodyInstance.SetDOFLock(EDOFMode::SixDOF);
	SpawnTransform = GetActorTransform();
	if (Vitals)
	{
		FFlyingCabVehicleVitalsConfig Config;
		Config.MaxFuel = MaxFuel;
		Config.StartingFuel = StartingFuel;
		Config.VerticalFuelPerSecond = VerticalFuelPerSecond;
		Config.HorizontalFuelPerSecond = HorizontalFuelPerSecond;
		Config.DescentRegenerationPerSecond = DescentRegenerationPerSecond;
		Config.RegenerationFullSpeed = RegenerationFullSpeed;
		Config.MaxHull = MaxHull;
		Config.DamageImpactSpeedThreshold = DamageImpactSpeedThreshold;
		Config.DamageFullHullSpeed = DamageFullHullSpeed;
		Config.CollisionDamageExponent = CollisionDamageExponent;
		Config.CollisionDamageCooldown = CollisionDamageCooldown;
		Vitals->InitializeVitals(Config);
	}
	CollisionBody->WakeAllRigidBodies();
	RefreshPlayerFocusAppearance();

	UE_LOG(
		LogFlyingCabFlight,
		Display,
		TEXT("Using pawn class %s (visual pitch response %.2f, return %.2f)."),
		*GetClass()->GetPathName(),
		VisualPitchResponseSpeed,
		VisualPitchReturnSpeed);

	RefreshVehicleIdentityAppearance(true);
}

void AFlyingCabPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	RefreshVehicleIdentityAppearance();
	RefreshPlayerFocusAppearance();
	RefreshKeyboardInputState();

	if (!CollisionBody || !CollisionBody->IsSimulatingPhysics())
	{
		return;
	}

	const float RequestedHorizontalInput = GetHorizontalInput();
	const float RequestedThrustInput = GetThrustInput();
	const bool bCanUseThrusters = Vitals && Vitals->CanUseThrusters();
	const float HorizontalInput = bCanUseThrusters ? RequestedHorizontalInput : 0.0f;
	const float ThrustInput = bCanUseThrusters ? RequestedThrustInput : 0.0f;
	const float Mass = CollisionBody->GetMass();

	CollisionBody->AddForce(FVector(
		HorizontalInput * HorizontalThrustAcceleration * Mass,
		0.0f,
		ThrustInput * VerticalThrustAcceleration * Mass));

	FVector Velocity = CollisionBody->GetPhysicsLinearVelocity();
	Velocity.Y = 0.0f;
	if (Vitals && Vitals->Advance(
		DeltaSeconds,
		RequestedHorizontalInput,
		RequestedThrustInput,
		Velocity.Z))
	{
		ShowFuelEmptyWarning();
	}
	if (DamageLight && Vitals)
	{
		const float DamageIntensity = Vitals->IsDestroyed()
			? 6500.0f
			: (Vitals->IsDamageFlashActive() ? 3500.0f : 0.0f);
		DamageLight->SetIntensity(DamageIntensity);
	}

	const bool bHasDriverControl = IsPlayerControlled();
	if (bHasDriverControl && FMath::IsNearlyZero(HorizontalInput))
	{
		Velocity.X = FMath::FInterpTo(Velocity.X, 0.0f, DeltaSeconds, HorizontalCoastDamping);
	}

	if (bHasDriverControl && FMath::IsNearlyZero(ThrustInput) && Velocity.Z > 0.0f)
	{
		Velocity.Z = FMath::FInterpTo(Velocity.Z, 0.0f, DeltaSeconds, UpwardCoastDamping);
	}

	Velocity.X = FMath::Clamp(Velocity.X, -MaxHorizontalSpeed, MaxHorizontalSpeed);
	Velocity.Z = FMath::Clamp(Velocity.Z, -MaxFallSpeed, MaxClimbSpeed);
	CollisionBody->SetPhysicsLinearVelocity(Velocity);

	UpdateVisualResponse(DeltaSeconds, Velocity);
	DrawFlightTelemetry(HorizontalInput, ThrustInput, Velocity);
}

void AFlyingCabPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);
	const FFlyingCabInputAssets& InputAssets = FlyingCabInputData::GetAssets();
	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
		EnhancedInput && InputAssets.IsValid())
	{
		EnhancedInput->BindAction(InputAssets.Restart, ETriggerEvent::Started,
			this, &AFlyingCabPawn::ResetVehicle);
		EnhancedInput->BindAction(InputAssets.Telemetry, ETriggerEvent::Started,
			this, &AFlyingCabPawn::ToggleFlightTelemetry);
		return;
	}
	UE_LOG(LogFlyingCabFlight, Error, TEXT("Cab input could not bind the Enhanced Input assets."));
}

void AFlyingCabPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	ClearAllInputState(TEXT("driver entered vehicle"), true);
	RefreshVehicleIdentityAppearance(true);
}

void AFlyingCabPawn::UnPossessed()
{
	ClearAllInputState(TEXT("driver exited vehicle"), true);
	Super::UnPossessed();
	RefreshVehicleIdentityAppearance(true);
}

void AFlyingCabPawn::ReleaseKeyboardInputState()
{
	KeyboardHorizontalInput = 0.0f;
	KeyboardThrustInput = 0.0f;
	bKeyboardRefuelPressed = false;
}

#if WITH_DEV_AUTOMATION_TESTS
bool AFlyingCabPawn::IsTestPlayerFocusVisible() const
{
	return PlayerFocusHalo && PlayerFocusHalo->IsVisible();
}
#endif

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
	if (IsDestroyed())
	{
		return;
	}

	if (Vitals)
	{
		Vitals->ResetResources();
	}
	RefreshVehicleIdentityAppearance(true);
	if (DamageLight)
	{
		DamageLight->SetIntensity(0.0f);
	}

	CollisionBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
	CollisionBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
	CollisionBody->WakeAllRigidBodies();
	VisualMesh->SetRelativeRotation(FRotator::ZeroRotator);
	CameraTrackingOffset = FVector::ZeroVector;
	PreviousHorizontalVelocity = 0.0f;
	VisualHorizontalAcceleration = 0.0f;
	bHasPreviousHorizontalVelocity = false;
	UE_LOG(
		LogFlyingCabFlight,
		Display,
		TEXT("Manual reset restored starting vehicle resources: fuel %.1f/%.1f, hull %.1f/%.1f."),
		GetFuel(),
		GetMaxFuel(),
		Vitals ? Vitals->GetHull() : 0.0f,
		Vitals ? Vitals->GetMaxHull() : MaxHull);
}

void AFlyingCabPawn::SetProximityGuidance(
	bool bVisible,
	const FVector2D& TargetWorldPosition,
	bool bTargetIsDropoff)
{
	if (!GuidanceArrowRoot || !GuidanceArrowShaft || !GuidanceArrowTip)
	{
		return;
	}

	const FVector2D CabPosition(GetActorLocation().X, GetActorLocation().Z);
	const FVector2D Delta = TargetWorldPosition - CabPosition;
	const bool bShouldShow = bVisible && !Delta.IsNearlyZero();
	GuidanceArrowShaft->SetVisibility(bShouldShow, true);
	GuidanceArrowTip->SetVisibility(bShouldShow, true);
	if (!bShouldShow)
	{
		return;
	}

	const float PitchDegrees = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
	GuidanceArrowRoot->SetRelativeRotation(FRotator(PitchDegrees, 0.0f, 0.0f));
	const FLinearColor ArrowColor = bTargetIsDropoff
		? FLinearColor(1.0f, 0.18f, 0.04f)
		: FLinearColor(0.0f, 0.90f, 1.0f);
	for (UStaticMeshComponent* ArrowPart : {GuidanceArrowShaft, GuidanceArrowTip})
	{
		ArrowPart->SetVectorParameterValueOnMaterials(
			TEXT("Color"),
			FVector(ArrowColor.R, ArrowColor.G, ArrowColor.B));
	}
}

FVector AFlyingCabPawn::GetCameraTrackingOffset() const
{
	return CameraTrackingOffset;
}

void AFlyingCabPawn::SetRefuelAvailable(bool bAvailable, int32 PricePerUnit)
{
	bRefuelAvailable = bAvailable && !IsDestroyed();
	RefuelPricePerUnit = FMath::Max(0, PricePerUnit);
	if (!bRefuelAvailable && !bRepairAvailable)
	{
		bTouchRefuelPressed = false;
	}
}

void AFlyingCabPawn::SetRepairAvailable(bool bAvailable, int32 PricePerHullUnit)
{
	bRepairAvailable = bAvailable && !IsDestroyed();
	RepairPricePerHullUnit = FMath::Max(0, PricePerHullUnit);
	if (!bRepairAvailable && !bRefuelAvailable)
	{
		bTouchRefuelPressed = false;
	}
}

void AFlyingCabPawn::ConfigureVehicleIdentity(
	FName InVehicleId,
	const FString& InDisplayName,
	FName InRequiredAccessId,
	const FLinearColor& InVehicleColor)
{
	VehicleId = InVehicleId.IsNone() ? FName(TEXT("Vehicle.Unregistered")) : InVehicleId;
	VehicleDisplayName = InDisplayName.IsEmpty() ? FString(TEXT("CAB")) : InDisplayName;
	RequiredAccessId = InRequiredAccessId;
	VehicleColor = InVehicleColor;
	bIdentityConfigured = true;
	bHasCachedAccessState = false;
	RefreshVehicleIdentityAppearance(true);
}

bool AFlyingCabPawn::CanPlayerEnter(FText& OutFailureReason) const
{
	if (IsDestroyed())
	{
		OutFailureReason = FText::FromString(TEXT("VEHICLE UNAVAILABLE // HULL FAILURE"));
		return false;
	}
	if (!HasRequiredVehicleAccess())
	{
		OutFailureReason = FText::FromString(TEXT("ACCESS DENIED // SERVICE LICENSE REQUIRED"));
		return false;
	}

	OutFailureReason = FText::GetEmpty();
	return true;
}

FText AFlyingCabPawn::GetEntryPrompt() const
{
	if (IsDestroyed())
	{
		return FText::FromString(TEXT("Q // VEHICLE UNAVAILABLE"));
	}
	if (!HasRequiredVehicleAccess())
	{
		return FText::FromString(TEXT("Q // SERVICE LICENSE REQUIRED"));
	}
	return FText::FromString(FString::Printf(TEXT("Q // ENTER %s"), *VehicleDisplayName));
}

bool AFlyingCabPawn::IsRefuelRequested() const
{
	return bRefuelAvailable && !IsDestroyed()
		&& (bKeyboardRefuelPressed || bTouchRefuelPressed);
}

bool AFlyingCabPawn::IsRepairRequested() const
{
	return bRepairAvailable && !IsDestroyed()
		&& (bKeyboardRefuelPressed || bTouchRefuelPressed);
}

float AFlyingCabPawn::AddFuel(float Units)
{
	return Vitals ? Vitals->AddFuel(Units) : 0.0f;
}

float AFlyingCabPawn::AddHull(float Units)
{
	return Vitals ? Vitals->AddHull(Units) : 0.0f;
}

void AFlyingCabPawn::RecoverVehicle(float RecoveryFuelPercent)
{
	ClearAllInputState(TEXT("tow recovery"), true);
	if (Vitals)
	{
		Vitals->Recover(RecoveryFuelPercent);
	}

	CollisionBody->SetSimulatePhysics(true);
	CollisionBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
	CollisionBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
	CollisionBody->WakeAllRigidBodies();
	RefreshVehicleIdentityAppearance(true);
	if (DamageLight)
	{
		DamageLight->SetIntensity(0.0f);
	}
}

void AFlyingCabPawn::SetKeyboardHorizontalInput(float Value)
{
	KeyboardHorizontalInput = FMath::Clamp(Value, -1.0f, 1.0f);
}

void AFlyingCabPawn::SetKeyboardThrustInput(float Value)
{
	KeyboardThrustInput = FMath::Clamp(Value, 0.0f, 1.0f);
}

void AFlyingCabPawn::SetKeyboardServiceInput(float Value)
{
	bKeyboardRefuelPressed = Value > 0.5f;
}

void AFlyingCabPawn::RefreshKeyboardInputState()
{
	const AFlyingCabPlayerController* PlayerController =
		Cast<AFlyingCabPlayerController>(GetController());
	if (!PlayerController || PlayerController->IsGameplayInputSuppressed()
		|| !PlayerController->PlayerInput)
	{
		ReleaseKeyboardInputState();
		return;
	}

	// Continuous controls are sampled from the current key state every frame. Keeping
	// them independent from cached action-value bindings prevents a skipped action
	// update from leaving a thruster latched for the rest of a play session.
	const float PositiveHorizontal =
		(PlayerController->IsInputKeyDown(EKeys::D)
			|| PlayerController->IsInputKeyDown(EKeys::Right))
			? 1.0f
			: 0.0f;
	const float NegativeHorizontal =
		(PlayerController->IsInputKeyDown(EKeys::A)
			|| PlayerController->IsInputKeyDown(EKeys::Left))
			? 1.0f
			: 0.0f;
	const bool bThrustPressed = PlayerController->IsInputKeyDown(EKeys::W)
		|| PlayerController->IsInputKeyDown(EKeys::Up)
		|| PlayerController->IsInputKeyDown(EKeys::SpaceBar);

	SetKeyboardHorizontalInput(PositiveHorizontal - NegativeHorizontal);
	SetKeyboardThrustInput(bThrustPressed ? 1.0f : 0.0f);
	SetKeyboardServiceInput(PlayerController->IsInputKeyDown(EKeys::E) ? 1.0f : 0.0f);
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
	if (AFlyingCabPlayerController* PlayerController =
		Cast<AFlyingCabPlayerController>(GetController()))
	{
		PlayerController->ReleaseInterfaceInputs();
	}

	ReleaseKeyboardInputState();
	TouchHorizontalInput = 0.0f;
	TouchThrustInput = 0.0f;
	bTouchRefuelPressed = false;
	++ForcedInputResetCount;

	const FString ClearSummary = FString::Printf(
		TEXT("Input state cleared during %s (keyboard X %.2f, thrust %.2f; touch X %.2f, thrust %.2f)."),
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

bool AFlyingCabPawn::HasRequiredVehicleAccess() const
{
	if (RequiredAccessId.IsNone())
	{
		return true;
	}

	UGameInstance* GameInstance = GetGameInstance();
	const UFlyingCabProgressionSubsystem* Progression = GameInstance
		? GameInstance->GetSubsystem<UFlyingCabProgressionSubsystem>()
		: nullptr;
	return Progression && Progression->HasAccess(RequiredAccessId);
}

void AFlyingCabPawn::RefreshVehicleIdentityAppearance(bool bForce)
{
	if (!bIdentityConfigured)
	{
		if (!IsDestroyed() && VisualMesh)
		{
			VisualMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(1.0f));
		}
		return;
	}

	const bool bHasAccess = HasRequiredVehicleAccess();
	if (!bForce && bHasCachedAccessState && bCachedAccessState == bHasAccess)
	{
		return;
	}
	bHasCachedAccessState = true;
	bCachedAccessState = bHasAccess;

	const FLinearColor StatusColor = bHasAccess
		? FLinearColor(0.10f, 1.0f, 0.48f)
		: FLinearColor(1.0f, 0.28f, 0.04f);
	if (VisualMesh && !IsDestroyed())
	{
		const FLinearColor DisplayColor = bHasAccess
			? VehicleColor
			: VehicleColor * 0.32f;
		VisualMesh->SetVectorParameterValueOnMaterials(
			TEXT("Color"),
			FVector(DisplayColor.R, DisplayColor.G, DisplayColor.B));
	}
	if (AccessLight)
	{
		AccessLight->SetLightColor(StatusColor);
		AccessLight->SetIntensity(IsDestroyed() ? 0.0f : 1500.0f);
	}
	if (VehicleLabel)
	{
		VehicleLabel->SetVisibility(true, true);
		VehicleLabel->SetTextRenderColor(StatusColor.ToFColor(true));
		const FString Status = IsDestroyed()
			? FString(TEXT("OUT OF SERVICE"))
			: (IsPlayerControlled()
				? FString(TEXT("ACTIVE"))
				: (bHasAccess ? FString(TEXT("Q // ENTER")) : FString(TEXT("LICENSE REQUIRED"))));
		VehicleLabel->SetText(FText::FromString(FString::Printf(
			TEXT("%s\n%s"),
			*VehicleDisplayName,
			*Status)));
	}
}

void AFlyingCabPawn::RefreshPlayerFocusAppearance()
{
	const bool bShouldShowFocus = IsPlayerControlled() && !IsDestroyed();
	if (PlayerFocusHalo)
	{
		PlayerFocusHalo->SetVisibility(bShouldShowFocus, true);
		if (bShouldShowFocus)
		{
			PlayerFocusHalo->SetVectorParameterValueOnMaterials(
				TEXT("Color"),
				FVector(PlayerFocusColor.R, PlayerFocusColor.G, PlayerFocusColor.B));
		}
	}
	if (PlayerFocusLight)
	{
		PlayerFocusLight->SetLightColor(PlayerFocusColor);
		PlayerFocusLight->SetIntensity(
			bShouldShowFocus ? PlayerFocusLightIntensity : 0.0f);
	}
}

void AFlyingCabPawn::ShowFuelEmptyWarning() const
{
	UE_LOG(
		LogFlyingCabFlight,
		Warning,
		TEXT("Fuel exhausted; thrusters disabled at %s while requested input was X %.2f, thrust %.2f."),
		*GetActorLocation().ToCompactString(),
		GetHorizontalInput(),
		GetThrustInput());

	if (!IsLocallyControlled())
	{
		return;
	}
	if (const AFlyingCabPlayerController* PlayerController =
		Cast<AFlyingCabPlayerController>(GetController()))
	{
		PlayerController->ShowEventMessage(
			FText::FromString(TEXT(
				"ENERGY EMPTY // DESCEND TO RECOVER A RESERVE OR REACH A FUEL STATION")),
			FLinearColor::FromSRGBColor(FColor(255, 80, 30)),
			3.0f);
	}
}

void AFlyingCabPawn::HandleCollisionHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (!CollisionBody)
	{
		return;
	}

	const float Mass = FMath::Max(CollisionBody->GetMass(), 1.0f);
	const float NormalSpeedChange = NormalImpulse.Size() / Mass;
	ApplyCollisionDamage(NormalSpeedChange);
}

void AFlyingCabPawn::ApplyCollisionDamage(float NormalSpeedChange)
{
	if (!Vitals)
	{
		return;
	}

	const FFlyingCabImpactResult Impact = Vitals->ApplyImpact(NormalSpeedChange);
	if (Impact.Damage <= UE_SMALL_NUMBER)
	{
		return;
	}

	UE_LOG(
		LogFlyingCabFlight,
		Display,
		TEXT("Collision damage %.1f from normal speed change %.1f cm/s. Hull %.1f/%.1f."),
		Impact.Damage,
		NormalSpeedChange,
		Vitals->GetHull(),
		Vitals->GetMaxHull());

	if (IsLocallyControlled())
	{
		if (const AFlyingCabPlayerController* PlayerController =
			Cast<AFlyingCabPlayerController>(GetController()))
		{
			PlayerController->ShowEventMessage(
				FText::FromString(FString::Printf(
					TEXT("IMPACT // HULL %.0f%%"),
					100.0f * Vitals->GetHullPercent())),
				FLinearColor::FromSRGBColor(FColor(255, 110, 35)),
				1.2f);
		}
	}

	if (Impact.bDestroyedNow)
	{
		EnterDestroyedState();
	}
}

void AFlyingCabPawn::EnterDestroyedState()
{
	if (!Vitals || !Vitals->IsDestroyed())
	{
		return;
	}

	ClearAllInputState(TEXT("vehicle destroyed"), true);
	bRefuelAvailable = false;
	bRepairAvailable = false;
	VisualMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.20f, 0.01f, 0.0f));
	RefreshVehicleIdentityAppearance(true);

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
	const FVector CameraOffset = CameraTrackingOffset;

	const FString Telemetry = FString::Printf(
		TEXT("FLIGHT TELEMETRY [F3]  |  Physics: %s\n")
		TEXT("Horizontal  keyboard:%+.2f  touch:%+.2f  effective:%+.2f\n")
		TEXT("Thrust      keyboard:%+.2f  touch:%+.2f  effective:%+.2f\n")
		TEXT("Velocity    X:%+7.1f  Z:%+7.1f  planar:%7.1f cm/s\n")
		TEXT("Command     X:%+7.1f  Z:%+7.1f cm/s^2\n")
		TEXT("Position    X:%+7.1f  Z:%+7.1f cm\n")
		TEXT("Presentation accel X:%+7.1f  pitch:%+5.1f deg  camera X:%+6.1f  Z:%+6.1f cm\n")
		TEXT("Resources   fuel:%5.1f/%5.1f  hull:%5.1f/%5.1f  destroyed:%s\n")
		TEXT("Thrusters  %s  |  input source: direct keyboard state + touch  |  forced clears:%u"),
		PhysicsState,
		KeyboardHorizontalInput,
		TouchHorizontalInput,
		HorizontalInput,
		KeyboardThrustInput,
		TouchThrustInput,
		ThrustInput,
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
		GetFuel(),
		GetMaxFuel(),
		Vitals ? Vitals->GetHull() : 0.0f,
		Vitals ? Vitals->GetMaxHull() : MaxHull,
		IsDestroyed() ? TEXT("YES") : TEXT("NO"),
		Vitals && Vitals->CanUseThrusters() ? TEXT("ENABLED") : TEXT("DISABLED"),
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

	CameraTrackingOffset = FMath::VInterpTo(
		CameraTrackingOffset,
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
