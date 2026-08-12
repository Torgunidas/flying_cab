// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabCharacter.h"

#include "EnhancedInputComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "FlyingCabGameMode.h"
#include "FlyingCabPlayerController.h"
#include "FlyingCabInputData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabCharacter, Log, All);

AFlyingCabCharacter::AFlyingCabCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(28.0f, 66.0f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
	GetCapsuleComponent()->SetNotifyRigidBodyCollision(true);
	GetCapsuleComponent()->OnComponentHit.AddDynamic(
		this,
		&AFlyingCabCharacter::HandleCapsuleHit);

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = 300.0f;
	Movement->JumpZVelocity = 440.0f;
	Movement->AirControl = 0.45f;
	Movement->GravityScale = 1.25f;
	Movement->GroundFriction = 8.0f;
	Movement->BrakingDecelerationWalking = 1500.0f;
	Movement->bConstrainToPlane = true;
	Movement->SetPlaneConstraintNormal(FVector::YAxisVector);
	Movement->bSnapToPlaneAtStart = true;
	Movement->bEnablePhysicsInteraction = false;

	GetMesh()->SetVisibility(false, true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -12.0f));
	BodyMesh->SetRelativeScale3D(FVector(0.38f, 0.38f, 0.82f));
	if (CylinderMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(CylinderMesh.Object);
	}
	if (BasicMaterial.Succeeded())
	{
		BodyMesh->SetMaterial(0, BasicMaterial.Object);
	}

	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
	HeadMesh->SetupAttachment(GetCapsuleComponent());
	HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeadMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 42.0f));
	HeadMesh->SetRelativeScale3D(FVector(0.30f));
	if (SphereMesh.Succeeded())
	{
		HeadMesh->SetStaticMesh(SphereMesh.Object);
	}
	if (BasicMaterial.Succeeded())
	{
		HeadMesh->SetMaterial(0, BasicMaterial.Object);
	}

	const FVector CharacterColor(0.05f, 0.75f, 1.0f);
	BodyMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), CharacterColor);
	HeadMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(1.0f, 0.65f, 0.18f));

	RunningLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("RunningLight"));
	RunningLight->SetupAttachment(GetCapsuleComponent());
	RunningLight->SetRelativeLocation(FVector(0.0f, 45.0f, 15.0f));
	RunningLight->SetLightColor(FLinearColor(0.05f, 0.75f, 1.0f));
	RunningLight->SetIntensity(900.0f);
	RunningLight->SetAttenuationRadius(260.0f);
	RunningLight->SetCastShadows(false);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = 1050.0f;
	CameraBoom->SetRelativeRotation(FRotator(-5.0f, -90.0f, 0.0f));
	CameraBoom->TargetOffset = FVector(0.0f, 0.0f, 80.0f);
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 7.0f;
	CameraBoom->bUsePawnControlRotation = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	Camera->FieldOfView = 55.0f;
	Camera->bUsePawnControlRotation = false;
}

void AFlyingCabCharacter::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
	UpdateDamageAppearance();
}

void AFlyingCabCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RestartLevelTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void AFlyingCabCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);
	const FFlyingCabInputAssets& InputAssets = FlyingCabInputData::GetAssets();
	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
		EnhancedInput && InputAssets.IsValid())
	{
		EnhancedInput->BindAction(InputAssets.Horizontal, ETriggerEvent::Triggered,
			this, &AFlyingCabCharacter::HandleEnhancedHorizontal);
		EnhancedInput->BindAction(InputAssets.Horizontal, ETriggerEvent::Completed,
			this, &AFlyingCabCharacter::ReleaseEnhancedHorizontal);
		EnhancedInput->BindAction(InputAssets.Horizontal, ETriggerEvent::Canceled,
			this, &AFlyingCabCharacter::ReleaseEnhancedHorizontal);
		EnhancedInput->BindAction(InputAssets.Thrust, ETriggerEvent::Triggered,
			this, &AFlyingCabCharacter::HandleEnhancedThrust);
		EnhancedInput->BindAction(InputAssets.Thrust, ETriggerEvent::Completed,
			this, &AFlyingCabCharacter::ReleaseEnhancedThrust);
		EnhancedInput->BindAction(InputAssets.Thrust, ETriggerEvent::Canceled,
			this, &AFlyingCabCharacter::ReleaseEnhancedThrust);
		return;
	}
	UE_LOG(LogFlyingCabCharacter, Error, TEXT("On-foot input could not bind the Enhanced Input assets."));
}

void AFlyingCabCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	DamageCooldownRemaining = FMath::Max(0.0f, DamageCooldownRemaining - DeltaSeconds);
	DamageFlashRemaining = FMath::Max(0.0f, DamageFlashRemaining - DeltaSeconds);
	if (GetCharacterMovement()->IsFalling())
	{
		MaximumTrackedDownwardSpeed = FMath::Max(
			MaximumTrackedDownwardSpeed,
			-GetVelocity().Z);
	}
	UpdateDamageAppearance();

	if (bDead)
	{
		return;
	}

	const float HorizontalInput = FMath::Clamp(
		KeyboardHorizontalInput + TouchHorizontalInput,
		-1.0f,
		1.0f);
	AddMovementInput(FVector::XAxisVector, HorizontalInput);

	const bool bKeyboardJumpPressed = KeyboardThrustInput > 0.5f;
	const bool bJumpPressed = bKeyboardJumpPressed || bTouchJumpPressed;
	if (bJumpPressed && !bPreviousJumpPressed)
	{
		Jump();
	}
	else if (!bJumpPressed && bPreviousJumpPressed)
	{
		StopJumping();
	}
	bPreviousJumpPressed = bJumpPressed;
}

float AFlyingCabCharacter::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	const float AppliedDamage = Super::TakeDamage(
		DamageAmount,
		DamageEvent,
		EventInstigator,
		DamageCauser);
	ApplyCharacterDamage(AppliedDamage, TEXT("DAMAGE"));
	return AppliedDamage;
}

float AFlyingCabCharacter::GetHealthPercent() const
{
	return MaxHealth > UE_SMALL_NUMBER
		? FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f)
		: 0.0f;
}

void AFlyingCabCharacter::Landed(const FHitResult& Hit)
{
	const float ImpactSpeed = FMath::Max(MaximumTrackedDownwardSpeed, -GetVelocity().Z);
	Super::Landed(Hit);
	MaximumTrackedDownwardSpeed = 0.0f;

	if (bDead || ImpactSpeed <= FallDamageSpeedThreshold)
	{
		return;
	}

	const float DamageRange = FMath::Max(FatalFallSpeed - FallDamageSpeedThreshold, 1.0f);
	const float ImpactAlpha = FMath::Clamp(
		(ImpactSpeed - FallDamageSpeedThreshold) / DamageRange,
		0.0f,
		1.0f);
	const float Damage = MaxHealth * FMath::Pow(ImpactAlpha, FallDamageExponent);
	ApplyCharacterDamage(Damage, TEXT("FALL"));
	UE_LOG(
		LogFlyingCabCharacter,
		Display,
		TEXT("Fall impact %.1f cm/s dealt %.1f damage."),
		ImpactSpeed,
		Damage);
}

void AFlyingCabCharacter::SetTouchHorizontalInput(float Value)
{
	TouchHorizontalInput = FMath::Clamp(Value, -1.0f, 1.0f);
}

void AFlyingCabCharacter::SetTouchJumpPressed(bool bPressed)
{
	bTouchJumpPressed = bPressed;
}

void AFlyingCabCharacter::ReleaseKeyboardInputState()
{
	KeyboardHorizontalInput = 0.0f;
	KeyboardThrustInput = 0.0f;
	bPreviousJumpPressed = false;
	StopJumping();
}

void AFlyingCabCharacter::SetKeyboardHorizontalInput(float Value)
{
	KeyboardHorizontalInput = FMath::Clamp(Value, -1.0f, 1.0f);
}

void AFlyingCabCharacter::SetKeyboardThrustInput(float Value)
{
	KeyboardThrustInput = FMath::Clamp(Value, 0.0f, 1.0f);
}

void AFlyingCabCharacter::HandleEnhancedHorizontal(const FInputActionValue& Value)
{
	SetKeyboardHorizontalInput(Value.Get<float>());
}

void AFlyingCabCharacter::HandleEnhancedThrust(const FInputActionValue& Value)
{
	SetKeyboardThrustInput(Value.Get<bool>() ? 1.0f : 0.0f);
}

void AFlyingCabCharacter::ReleaseEnhancedHorizontal()
{
	SetKeyboardHorizontalInput(0.0f);
}

void AFlyingCabCharacter::ReleaseEnhancedThrust()
{
	SetKeyboardThrustInput(0.0f);
}

void AFlyingCabCharacter::UnPossessed()
{
	ClearInputState();
	Super::UnPossessed();
}

void AFlyingCabCharacter::ClearInputState()
{
	ReleaseKeyboardInputState();
	TouchHorizontalInput = 0.0f;
	bTouchJumpPressed = false;
}

void AFlyingCabCharacter::HandleCapsuleHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (bDead || DamageCooldownRemaining > 0.0f || !OtherActor
		|| Hit.ImpactNormal.Z > 0.55f)
	{
		return;
	}

	const FVector OtherVelocity = OtherComponent
		? OtherComponent->GetComponentVelocity()
		: OtherActor->GetVelocity();
	const FVector RelativeVelocity = GetVelocity() - OtherVelocity;
	const float ImpactSpeed = FMath::Abs(FVector::DotProduct(RelativeVelocity, Hit.ImpactNormal));
	if (ImpactSpeed <= CollisionDamageSpeedThreshold)
	{
		return;
	}

	const float DamageRange = FMath::Max(
		FatalCollisionSpeed - CollisionDamageSpeedThreshold,
		1.0f);
	const float ImpactAlpha = FMath::Clamp(
		(ImpactSpeed - CollisionDamageSpeedThreshold) / DamageRange,
		0.0f,
		1.0f);
	const float Damage = MaxHealth * FMath::Square(ImpactAlpha);
	DamageCooldownRemaining = CollisionDamageCooldown;
	ApplyCharacterDamage(Damage, TEXT("IMPACT"));
	UE_LOG(
		LogFlyingCabCharacter,
		Display,
		TEXT("Body impact %.1f cm/s dealt %.1f damage."),
		ImpactSpeed,
		Damage);
}

void AFlyingCabCharacter::ApplyCharacterDamage(float DamageAmount, const TCHAR* DamageSource)
{
	if (bDead || DamageAmount <= UE_SMALL_NUMBER)
	{
		return;
	}

	CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);
	DamageFlashRemaining = 0.18f;
	if (const AFlyingCabPlayerController* PlayerController =
		Cast<AFlyingCabPlayerController>(GetController()))
	{
		PlayerController->ShowEventMessage(
			FText::FromString(FString::Printf(
				TEXT("%s // HEALTH %.0f%%"),
				DamageSource ? DamageSource : TEXT("DAMAGE"),
				GetHealthPercent() * 100.0f)),
			FLinearColor::FromSRGBColor(FColor(255, 80, 30)),
			1.5f);
	}

	if (CurrentHealth <= UE_SMALL_NUMBER)
	{
		EnterDeathState();
	}
}

void AFlyingCabCharacter::EnterDeathState()
{
	if (bDead)
	{
		return;
	}

	bDead = true;
	ClearInputState();
	GetCharacterMovement()->DisableMovement();
	UpdateDamageAppearance();
	if (const AFlyingCabPlayerController* PlayerController =
		Cast<AFlyingCabPlayerController>(GetController()))
	{
		PlayerController->ShowEventMessage(
			FText::FromString(TEXT("DRIVER DOWN // RELOADING LEVEL")),
			FLinearColor::FromSRGBColor(FColor(255, 35, 20)),
			DeathRestartDelay);
	}
	UE_LOG(LogFlyingCabCharacter, Warning, TEXT("On-foot character died; reloading current level."));

	GetWorldTimerManager().SetTimer(
		RestartLevelTimerHandle,
		this,
		&AFlyingCabCharacter::RestartCurrentLevel,
		DeathRestartDelay,
		false);
}

void AFlyingCabCharacter::RestartCurrentLevel()
{
	const AFlyingCabGameMode* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<AFlyingCabGameMode>()
		: nullptr;
	if (GameMode && GameMode->GetCurrentRunMode() != EFlyingCabRunMode::None)
	{
		if (AFlyingCabPlayerController* PlayerController =
			Cast<AFlyingCabPlayerController>(GetController()))
		{
			PlayerController->RestartWithRunMode(GameMode->GetCurrentRunMode());
			return;
		}
	}

	const FString LevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	if (!LevelName.IsEmpty())
	{
		UGameplayStatics::OpenLevel(this, FName(*LevelName), false);
	}
}

void AFlyingCabCharacter::UpdateDamageAppearance()
{
	const float HealthPercent = GetHealthPercent();
	const FVector HealthyColor(0.05f, 0.75f, 1.0f);
	const FVector HurtColor(1.0f, 0.16f, 0.02f);
	const FVector BodyColor = bDead || DamageFlashRemaining > 0.0f
		? HurtColor
		: FMath::Lerp(HurtColor, HealthyColor, HealthPercent);
	BodyMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), BodyColor);
	RunningLight->SetLightColor(bDead || HealthPercent <= 0.3f
		? FLinearColor(1.0f, 0.05f, 0.01f)
		: FLinearColor(0.05f, 0.75f, 1.0f));
}
