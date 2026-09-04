// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabCameraRig.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "FlyingCabCharacter.h"
#include "FlyingCabPawn.h"
#include "GameFramework/SpringArmComponent.h"

AFlyingCabCameraRig::AFlyingCabCameraRig()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(SceneRoot);
	CameraBoom->TargetArmLength = VehicleArmLength;
	CameraBoom->SetRelativeRotation(FRotator(-5.0f, -90.0f, 0.0f));
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bEnableCameraLag = false;
	CameraBoom->bUsePawnControlRotation = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	Camera->FieldOfView = 55.0f;
	Camera->bUsePawnControlRotation = false;
}

void AFlyingCabCameraRig::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bDeveloperObserverEnabled || !FollowTarget)
	{
		return;
	}

	const FVector DesiredLocation = GetDesiredRigLocation();
	const FVector CurrentLocation = GetActorLocation();
	if (FVector::DistSquared(CurrentLocation, DesiredLocation)
		>= FMath::Square(TeleportSnapDistance))
	{
		SetActorLocation(DesiredLocation);
		return;
	}

	// This is the same position lerp used by scripts/CameraController.gd.
	const float FollowAlpha = FMath::Clamp(FollowSpeed * DeltaSeconds, 0.0f, 1.0f);
	SetActorLocation(FMath::Lerp(CurrentLocation, DesiredLocation, FollowAlpha));
}

void AFlyingCabCameraRig::SetFollowTarget(AActor* NewTarget, bool bSnapToTarget)
{
	FollowTarget = NewTarget;
	ApplyTargetFraming();
	if (FollowTarget && bSnapToTarget)
	{
		SetActorLocation(GetDesiredRigLocation());
	}
}

void AFlyingCabCameraRig::SetDeveloperObserverEnabled(bool bEnabled)
{
	if (bDeveloperObserverEnabled == bEnabled)
	{
		return;
	}

	bDeveloperObserverEnabled = bEnabled;
	if (bDeveloperObserverEnabled)
	{
		CameraBoom->TargetArmLength = FMath::Clamp(
			FMath::Max(CameraBoom->TargetArmLength, DeveloperObserverInitialArmLength),
			DeveloperObserverMinArmLength,
			DeveloperObserverMaxArmLength);
	}
	else
	{
		ApplyTargetFraming();
	}
}

void AFlyingCabCameraRig::MoveDeveloperObserver(
	const FVector2D& PanInput,
	bool bFast,
	float DeltaSeconds)
{
	if (!bDeveloperObserverEnabled || PanInput.IsNearlyZero())
	{
		return;
	}

	const FVector2D Direction = PanInput.GetClampedToMaxSize(1.0f);
	const float Speed = DeveloperObserverPanSpeed
		* (bFast ? DeveloperObserverFastMultiplier : 1.0f);
	AddActorWorldOffset(
		FVector(Direction.X, 0.0f, Direction.Y) * Speed * FMath::Max(0.0f, DeltaSeconds));
}

void AFlyingCabCameraRig::AdjustDeveloperObserverZoom(float ZoomInput, float DeltaSeconds)
{
	if (!bDeveloperObserverEnabled || FMath::IsNearlyZero(ZoomInput))
	{
		return;
	}

	CameraBoom->TargetArmLength = FMath::Clamp(
		CameraBoom->TargetArmLength
			+ ZoomInput * DeveloperObserverZoomSpeed * FMath::Max(0.0f, DeltaSeconds),
		DeveloperObserverMinArmLength,
		DeveloperObserverMaxArmLength);
}

void AFlyingCabCameraRig::RecenterDeveloperObserver()
{
	if (!bDeveloperObserverEnabled)
	{
		return;
	}
	SetActorLocation(GetDesiredRigLocation());
}

float AFlyingCabCameraRig::GetCurrentArmLength() const
{
	return CameraBoom ? CameraBoom->TargetArmLength : 0.0f;
}

FVector AFlyingCabCameraRig::GetDesiredRigLocation() const
{
	if (!FollowTarget)
	{
		return GetActorLocation();
	}

	FVector TargetOffset = FVector::ZeroVector;
	if (const AFlyingCabPawn* Vehicle = Cast<AFlyingCabPawn>(FollowTarget))
	{
		TargetOffset = Vehicle->GetCameraTrackingOffset();
	}
	else if (Cast<AFlyingCabCharacter>(FollowTarget))
	{
		TargetOffset = OnFootTargetOffset;
	}
	return FollowTarget->GetActorLocation() + TargetOffset;
}

void AFlyingCabCameraRig::ApplyTargetFraming()
{
	CameraBoom->TargetArmLength = Cast<AFlyingCabPawn>(FollowTarget)
		? VehicleArmLength
		: OnFootArmLength;
}
