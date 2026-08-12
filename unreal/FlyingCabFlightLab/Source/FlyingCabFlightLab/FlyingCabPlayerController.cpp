// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "FlyingCabCameraRig.h"
#include "FlyingCabCharacter.h"
#include "FlyingCabGameMode.h"
#include "FlyingCabInteractable.h"
#include "FlyingCabInputData.h"
#include "FlyingCabPawn.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "FlyingCabGameFlowWidget.h"
#include "FlyingCabTouchControls.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabInteraction, Log, All);

AFlyingCabPlayerController::AFlyingCabPlayerController()
{
	bAutoManageActiveCameraTarget = false;
}

void AFlyingCabPlayerController::BeginPlay()
{
	Super::BeginPlay();
	EnsureEnhancedInputContext();
	CreateInterfaceWidget();
	GetWorldTimerManager().SetTimer(
		InterfaceRefreshTimerHandle,
		this,
		&AFlyingCabPlayerController::RefreshInterface,
		FMath::Max(0.05f, InterfaceRefreshInterval),
		true);
	RefreshInterface();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	CameraRig = GetWorld()->SpawnActor<AFlyingCabCameraRig>(
		AFlyingCabCameraRig::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParameters);

	APawn* ControlledPawn = GetPawn();
	if (AFlyingCabPawn* Vehicle = Cast<AFlyingCabPawn>(ControlledPawn))
	{
		ShowInteractionMessage(
			TEXT("Q // EXIT CAB"),
			FColor(60, 235, 255));
	}
	if (CameraRig && ControlledPawn)
	{
		CameraRig->SetFollowTarget(ControlledPawn, true);
		SetViewTarget(CameraRig);
	}
	else if (ControlledPawn)
	{
		SetViewTarget(ControlledPawn);
	}

	ShowInitialModeSelection();
}

void AFlyingCabPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(InterfaceRefreshTimerHandle);
	if (InterfaceWidget)
	{
		InterfaceWidget->ReleaseAllInputs();
		InterfaceWidget->RemoveFromParent();
		InterfaceWidget = nullptr;
	}
	const FFlyingCabInputAssets& InputAssets = FlyingCabInputData::GetAssets();
	if (InputAssets.MappingContext)
	{
		if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
				LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				InputSubsystem->RemoveMappingContext(InputAssets.MappingContext);
			}
		}
	}
	Super::EndPlay(EndPlayReason);
}

void AFlyingCabPlayerController::FlushPressedKeys()
{
	Super::FlushPressedKeys();
	ReleaseInterfaceInputs();
}

void AFlyingCabPlayerController::StartRunMode(EFlyingCabRunMode Mode)
{
	if (Mode == EFlyingCabRunMode::None)
	{
		return;
	}

	AFlyingCabGameMode* GameMode = GetWorld()->GetAuthGameMode<AFlyingCabGameMode>();
	if (!GameMode)
	{
		return;
	}

	GameMode->StartRun(Mode);
	bGameFlowScreenOpen = false;
	if (GameFlowWidget)
	{
		GameFlowWidget->HideFlowScreen();
	}
	SetPause(false);
	FlushPressedKeys();
	RestoreGameplayInputMode();
	ApplyTouchControlsVisibility();
}

void AFlyingCabPlayerController::RestartWithRunMode(EFlyingCabRunMode Mode)
{
	const FString LevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	if (!LevelName.IsEmpty())
	{
		UGameplayStatics::OpenLevel(
			this,
			FName(*LevelName),
			true,
			GetRunModeOption(Mode));
	}
}

void AFlyingCabPlayerController::ReturnToModeSelection()
{
	const FString LevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	if (!LevelName.IsEmpty())
	{
		UGameplayStatics::OpenLevel(this, FName(*LevelName), true);
	}
}

void AFlyingCabPlayerController::ShowTimeAttackResults(
	const FFlyingCabTimeAttackResult& Result,
	const TArray<float>& BestTimes)
{
	if (!GameFlowWidget)
	{
		GameFlowWidget = CreateWidget<UFlyingCabGameFlowWidget>(this);
		if (GameFlowWidget)
		{
			GameFlowWidget->AddToViewport(500);
		}
	}
	if (!GameFlowWidget)
	{
		return;
	}

	FlushPressedKeys();
	bGameFlowScreenOpen = true;
	GameFlowWidget->ShowTimeAttackResults(Result, BestTimes);
	SetPause(true);
	EnterMenuInputMode();
}

void AFlyingCabPlayerController::ShowInitialModeSelection()
{
	GameFlowWidget = CreateWidget<UFlyingCabGameFlowWidget>(this);
	if (!GameFlowWidget)
	{
		StartRunMode(EFlyingCabRunMode::Freeroam);
		return;
	}
	GameFlowWidget->AddToViewport(500);

	const AFlyingCabGameMode* GameMode = GetWorld()->GetAuthGameMode<AFlyingCabGameMode>();
	FString RequestedMode = GameMode
		? UGameplayStatics::ParseOption(GameMode->OptionsString, TEXT("RunMode"))
		: FString();
	if (RequestedMode.IsEmpty())
	{
		FParse::Value(FCommandLine::Get(), TEXT("FlyingCabMode="), RequestedMode);
	}
	const EFlyingCabRunMode Mode = ParseRunMode(RequestedMode);
	if (Mode != EFlyingCabRunMode::None)
	{
		StartRunMode(Mode);
		return;
	}

	GameFlowWidget->ShowModeSelection(
		GameMode ? GameMode->GetBestTimeAttackTimes() : TArray<float>(),
		GameMode ? GameMode->GetTimeAttackTargetCredits() : 1000);
	bGameFlowScreenOpen = true;
	SetPause(true);
	EnterMenuInputMode();
	UE_LOG(LogFlyingCabInteraction, Display, TEXT("Mode selection opened; gameplay paused."));
}

void AFlyingCabPlayerController::EnterMenuInputMode()
{
	FlushPressedKeys();
	bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	SetInputMode(InputMode);
}

void AFlyingCabPlayerController::RestoreGameplayInputMode()
{
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
#if WITH_EDITOR
	bShowMouseCursor = true;
#else
	bShowMouseCursor = false;
#endif
}

EFlyingCabRunMode AFlyingCabPlayerController::ParseRunMode(const FString& Value)
{
	if (Value.Equals(TEXT("TimeAttack"), ESearchCase::IgnoreCase))
	{
		return EFlyingCabRunMode::TimeAttack;
	}
	if (Value.Equals(TEXT("Freeroam"), ESearchCase::IgnoreCase))
	{
		return EFlyingCabRunMode::Freeroam;
	}
	return EFlyingCabRunMode::None;
}

FString AFlyingCabPlayerController::GetRunModeOption(EFlyingCabRunMode Mode)
{
	return Mode == EFlyingCabRunMode::TimeAttack
		? FString(TEXT("RunMode=TimeAttack"))
		: FString(TEXT("RunMode=Freeroam"));
}

void AFlyingCabPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	check(InputComponent);
	const FFlyingCabInputAssets& InputAssets = FlyingCabInputData::GetAssets();
	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
		EnhancedInput && InputAssets.IsValid())
	{
		EnhancedInput->BindAction(
			InputAssets.Interact,
			ETriggerEvent::Started,
			this,
			&AFlyingCabPlayerController::RequestContextInteraction);
		return;
	}
	UE_LOG(
		LogFlyingCabInteraction,
		Error,
		TEXT("Controller input could not bind the Enhanced Input assets."));
}

void AFlyingCabPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	EnsureEnhancedInputContext();
	CachedContextPrompt = FText::GetEmpty();
	CachedContextPromptPawn.Reset();
	LastContextPromptRefreshTime = -1.0;
	if (AFlyingCabPawn* Vehicle = Cast<AFlyingCabPawn>(InPawn))
	{
		PlayerMode = EFlyingCabPlayerMode::Vehicle;
	}
	else if (Cast<AFlyingCabCharacter>(InPawn))
	{
		PlayerMode = EFlyingCabPlayerMode::OnFoot;
	}
	else
	{
		PlayerMode = EFlyingCabPlayerMode::Unknown;
	}
	if (InterfaceWidget)
	{
		InterfaceWidget->SetOnFootMode(PlayerMode == EFlyingCabPlayerMode::OnFoot);
		RefreshInterface();
	}
	if (CameraRig && InPawn)
	{
		CameraRig->SetFollowTarget(InPawn, false);
	}
}

bool AFlyingCabPlayerController::EnsureEnhancedInputContext()
{
	const FFlyingCabInputAssets& InputAssets = FlyingCabInputData::GetAssets();
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!InputAssets.IsValid() || !LocalPlayer)
	{
		return false;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!InputSubsystem)
	{
		return false;
	}
	if (!InputSubsystem->HasMappingContext(InputAssets.MappingContext))
	{
		InputSubsystem->AddMappingContext(InputAssets.MappingContext, 0);
	}
	return true;
}

void AFlyingCabPlayerController::RequestContextInteraction()
{
	if (AFlyingCabPawn* Vehicle = Cast<AFlyingCabPawn>(GetPawn()))
	{
		TryExitVehicle(Vehicle);
		return;
	}

	if (AFlyingCabCharacter* OnFootPawn = Cast<AFlyingCabCharacter>(GetPawn()))
	{
		RefreshInteractionCacheIfNeeded();
		if (TryInteractWithNearbyActor(OnFootPawn))
		{
			return;
		}

		if (AFlyingCabPawn* NearbyVehicle = FindNearestVehicle(OnFootPawn))
		{
			TryEnterVehicle(OnFootPawn, NearbyVehicle);
			return;
		}
		ShowInteractionMessage(TEXT("NO INTERACTION NEARBY"), FColor(255, 170, 35));
	}
}

FText AFlyingCabPlayerController::GetContextPrompt()
{
	AFlyingCabCharacter* OnFootPawn = Cast<AFlyingCabCharacter>(GetPawn());
	if (!OnFootPawn || OnFootPawn->IsDead())
	{
		CachedContextPrompt = FText::GetEmpty();
		CachedContextPromptPawn.Reset();
		return FText::GetEmpty();
	}

	RefreshInteractionCacheIfNeeded();
	const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (CachedContextPromptPawn == OnFootPawn
		&& LastContextPromptRefreshTime >= 0.0
		&& CurrentTime - LastContextPromptRefreshTime < ContextPromptRefreshInterval)
	{
		return CachedContextPrompt;
	}

	if (AActor* InteractableActor = FindNearestInteractable(OnFootPawn))
	{
		if (const IFlyingCabInteractable* Interactable = Cast<IFlyingCabInteractable>(InteractableActor))
		{
			CachedContextPrompt = Interactable->GetInteractionPrompt(OnFootPawn);
			CachedContextPromptPawn = OnFootPawn;
			LastContextPromptRefreshTime = CurrentTime;
			return CachedContextPrompt;
		}
	}

	if (const AFlyingCabPawn* NearbyVehicle = FindNearestVehicle(OnFootPawn))
	{
		CachedContextPrompt = NearbyVehicle->GetEntryPrompt();
		CachedContextPromptPawn = OnFootPawn;
		LastContextPromptRefreshTime = CurrentTime;
		return CachedContextPrompt;
	}

	CachedContextPrompt = FText::FromString(TEXT("EXPLORE ON FOOT"));
	CachedContextPromptPawn = OnFootPawn;
	LastContextPromptRefreshTime = CurrentTime;
	return CachedContextPrompt;
}

void AFlyingCabPlayerController::TryExitVehicle(AFlyingCabPawn* Vehicle)
{
	if (!Vehicle || Vehicle->IsDestroyed())
	{
		ShowInteractionMessage(TEXT("CAB UNAVAILABLE"), FColor(255, 80, 30));
		return;
	}

	if (const AFlyingCabGameMode* GameMode = GetWorld()->GetAuthGameMode<AFlyingCabGameMode>())
	{
		FText FailureReason;
		if (!GameMode->CanPlayerExitVehicle(FailureReason))
		{
			ShowInteractionMessage(FailureReason.ToString(), FColor(255, 170, 35));
			return;
		}
	}

	AFlyingCabCharacter* OnFootPawn = SpawnCharacterBesideVehicle(Vehicle);
	if (!OnFootPawn)
	{
		ShowInteractionMessage(
			TEXT("NO CLEAR EXIT SPACE BESIDE CAB"),
			FColor(255, 100, 35));
		return;
	}

	Possess(OnFootPawn);
	ShowInteractionMessage(
		TEXT("ON FOOT // A-D MOVE // SPACE JUMP // Q ENTER CAB"),
		FColor(60, 235, 255));
	UE_LOG(
		LogFlyingCabInteraction,
		Display,
		TEXT("Player exited cab at %s; on-foot pawn spawned at %s."),
		*Vehicle->GetActorLocation().ToCompactString(),
		*OnFootPawn->GetActorLocation().ToCompactString());
}

void AFlyingCabPlayerController::TryEnterVehicle(
	AFlyingCabCharacter* OnFootPawn,
	AFlyingCabPawn* Vehicle)
{
	if (!OnFootPawn || OnFootPawn->IsDead() || !Vehicle)
	{
		ShowInteractionMessage(TEXT("NO AVAILABLE CAB"), FColor(255, 100, 35));
		return;
	}

	const FVector Delta = Vehicle->GetActorLocation() - OnFootPawn->GetActorLocation();
	const float Distance = FVector2D(Delta.X, Delta.Z).Size();
	if (Distance > VehicleInteractionDistance)
	{
		ShowInteractionMessage(
			FString::Printf(
				TEXT("MOVE CLOSER TO CAB // %.1f M"),
				Distance / 100.0f),
			FColor(255, 170, 35));
		return;
	}

	FText FailureReason;
	if (!Vehicle->CanPlayerEnter(FailureReason))
	{
		ShowInteractionMessage(FailureReason.ToString(), FColor(255, 170, 35));
		return;
	}

	Possess(Vehicle);
	OnFootPawn->Destroy();
	ShowInteractionMessage(
		FString::Printf(TEXT("%s // CONTROL ONLINE"), *Vehicle->GetVehicleDisplayName()),
		FColor(70, 255, 150));
	UE_LOG(
		LogFlyingCabInteraction,
		Display,
		TEXT("Player entered vehicle %s at distance %.1f cm."),
		*Vehicle->GetVehicleDisplayName(),
		Distance);
}

bool AFlyingCabPlayerController::TryInteractWithNearbyActor(AFlyingCabCharacter* OnFootPawn)
{
	AActor* InteractableActor = FindNearestInteractable(OnFootPawn);
	IFlyingCabInteractable* Interactable = InteractableActor
		? Cast<IFlyingCabInteractable>(InteractableActor)
		: nullptr;
	if (!Interactable)
	{
		return false;
	}

	FText InteractionMessage;
	const bool bSucceeded = Interactable->Interact(OnFootPawn, InteractionMessage);
	if (!InteractionMessage.IsEmpty())
	{
		ShowInteractionMessage(
			InteractionMessage.ToString(),
			bSucceeded ? FColor(70, 255, 150) : FColor(255, 170, 35));
	}
	return true;
}

void AFlyingCabPlayerController::RefreshInteractionCacheIfNeeded(bool bForce)
{
	if (!GetWorld())
	{
		CachedInteractables.Reset();
		CachedVehicles.Reset();
		return;
	}

	const double CurrentTime = GetWorld()->GetTimeSeconds();
	if (!bForce
		&& LastInteractionCacheRefreshTime >= 0.0
		&& CurrentTime - LastInteractionCacheRefreshTime < InteractionCacheRefreshInterval)
	{
		return;
	}

	CachedInteractables.Reset();
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Candidate = *It;
		if (Candidate
			&& Candidate->GetClass()->ImplementsInterface(UFlyingCabInteractable::StaticClass()))
		{
			CachedInteractables.Add(Candidate);
		}
	}

	CachedVehicles.Reset();
	for (TActorIterator<AFlyingCabPawn> It(GetWorld()); It; ++It)
	{
		if (AFlyingCabPawn* Candidate = *It)
		{
			CachedVehicles.Add(Candidate);
		}
	}
	LastInteractionCacheRefreshTime = CurrentTime;
}

AActor* AFlyingCabPlayerController::FindNearestInteractable(
	const AFlyingCabCharacter* OnFootPawn) const
{
	if (!OnFootPawn)
	{
		return nullptr;
	}

	AActor* NearestActor = nullptr;
	float NearestDistance = WorldInteractionDistance;
	for (const TWeakObjectPtr<AActor>& CandidatePtr : CachedInteractables)
	{
		AActor* Candidate = CandidatePtr.Get();
		if (!Candidate)
		{
			continue;
		}

		const FVector Delta = Candidate->GetActorLocation() - OnFootPawn->GetActorLocation();
		const float Distance = FVector2D(Delta.X, Delta.Z).Size();
		if (Distance <= NearestDistance)
		{
			NearestDistance = Distance;
			NearestActor = Candidate;
		}
	}
	return NearestActor;
}

AFlyingCabPawn* AFlyingCabPlayerController::FindNearestVehicle(
	const AFlyingCabCharacter* OnFootPawn) const
{
	if (!OnFootPawn)
	{
		return nullptr;
	}

	AFlyingCabPawn* NearestVehicle = nullptr;
	float NearestDistance = VehicleInteractionDistance;
	for (const TWeakObjectPtr<AFlyingCabPawn>& CandidatePtr : CachedVehicles)
	{
		AFlyingCabPawn* Candidate = CandidatePtr.Get();
		if (!Candidate)
		{
			continue;
		}

		const FVector Delta = Candidate->GetActorLocation() - OnFootPawn->GetActorLocation();
		const float Distance = FVector2D(Delta.X, Delta.Z).Size();
		if (Distance <= NearestDistance)
		{
			NearestDistance = Distance;
			NearestVehicle = Candidate;
		}
	}
	return NearestVehicle;
}

AFlyingCabCharacter* AFlyingCabPlayerController::SpawnCharacterBesideVehicle(
	AFlyingCabPawn* Vehicle)
{
	if (!Vehicle || !GetWorld())
	{
		return nullptr;
	}

	const AFlyingCabCharacter* CharacterDefaults = GetDefault<AFlyingCabCharacter>();
	const UCapsuleComponent* CharacterCapsule = CharacterDefaults
		? CharacterDefaults->GetCapsuleComponent()
		: nullptr;
	const float CharacterRadius = CharacterCapsule
		? CharacterCapsule->GetUnscaledCapsuleRadius()
		: 28.0f;
	const float CharacterHalfHeight = CharacterCapsule
		? CharacterCapsule->GetUnscaledCapsuleHalfHeight()
		: 66.0f;

	const UPrimitiveComponent* VehicleRoot = Cast<UPrimitiveComponent>(Vehicle->GetRootComponent());
	const FVector VehicleExtent = VehicleRoot
		? VehicleRoot->Bounds.BoxExtent
		: FVector(110.0f, 45.0f, 35.0f);
	const FVector VehicleLocation = Vehicle->GetActorLocation();
	const float SideOffset = VehicleExtent.X + CharacterRadius + ExitSideClearance;
	const float VehicleBottom = VehicleLocation.Z - VehicleExtent.Z;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FlyingCabExitGround), false, Vehicle);
	for (const float Side : {1.0f, -1.0f})
	{
		const FVector TraceStart(
			VehicleLocation.X + Side * SideOffset,
			VehicleLocation.Y,
			VehicleBottom + 40.0f);
		const FVector TraceEnd = TraceStart - FVector(0.0f, 0.0f, ExitGroundReach + 40.0f);
		FHitResult GroundHit;
		const bool bHasGround = GetWorld()->LineTraceSingleByChannel(
			GroundHit,
			TraceStart,
			TraceEnd,
			ECC_Visibility,
			QueryParams);
		const bool bUseGroundExit = bHasGround && GroundHit.ImpactNormal.Z >= 0.65f;
		const FVector CandidateLocation(
			TraceStart.X,
			VehicleLocation.Y,
			bUseGroundExit
				? GroundHit.ImpactPoint.Z + CharacterHalfHeight + 2.0f
				: VehicleBottom + CharacterHalfHeight);
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
		if (AFlyingCabCharacter* SpawnedOnFootPawn = GetWorld()->SpawnActor<AFlyingCabCharacter>(
			AFlyingCabCharacter::StaticClass(),
			CandidateLocation,
			FRotator::ZeroRotator,
			SpawnParameters))
		{
			FVector ExitVelocity = Vehicle->GetVelocity();
			ExitVelocity.Y = 0.0f;
			if (!bUseGroundExit)
			{
				ExitVelocity.X += Side * AirborneExitSeparationSpeed;
				ExitVelocity.Z += AirborneExitUpwardSpeed;
			}
			SpawnedOnFootPawn->GetCharacterMovement()->Velocity = ExitVelocity;
			return SpawnedOnFootPawn;
		}
	}

	return nullptr;
}

void AFlyingCabPlayerController::ShowInteractionMessage(
	const FString& Message,
	const FColor& Color) const
{
	ShowEventMessage(
		FText::FromString(Message),
		FLinearColor::FromSRGBColor(Color),
		2.5f);
}

void AFlyingCabPlayerController::ShowEventMessage(
	const FText& Message,
	const FLinearColor& Color,
	float DurationSeconds) const
{
	if (UFlyingCabTouchControls* Widget = GetInterfaceWidget())
	{
		Widget->ShowEventMessage(Message, Color, DurationSeconds);
	}
}

void AFlyingCabPlayerController::SetObjectiveStatus(const FText& Status)
{
	if (InterfaceWidget)
	{
		InterfaceWidget->SetObjectiveText(Status);
	}
}

void AFlyingCabPlayerController::SetMinimapState(
	const FVector2D& CabWorldPosition,
	const FVector2D& TargetWorldPosition,
	bool bTargetIsDropoff)
{
	if (InterfaceWidget)
	{
		InterfaceWidget->SetMinimapState(
			CabWorldPosition,
			TargetWorldPosition,
			bTargetIsDropoff);
	}
}

void AFlyingCabPlayerController::SetPassengerOfferMarkers(
	const FVector2D& CabWorldPosition,
	const TArray<FVector2D>& OfferWorldPositions)
{
	if (InterfaceWidget)
	{
		InterfaceWidget->SetPassengerOfferMarkers(CabWorldPosition, OfferWorldPositions);
	}
}

void AFlyingCabPlayerController::ClearMinimapTarget()
{
	if (InterfaceWidget)
	{
		InterfaceWidget->SetMinimapTargetVisible(false);
	}
}

void AFlyingCabPlayerController::SetTimeAttackStatus(
	bool bActive,
	float ElapsedSeconds,
	int32 Credits,
	int32 TargetCredits)
{
	if (InterfaceWidget)
	{
		InterfaceWidget->SetTimeAttackState(
			bActive,
			ElapsedSeconds,
			Credits,
			TargetCredits);
	}
}

void AFlyingCabPlayerController::SetEconomyStatus(int32 Credits, int32 ActiveFare)
{
	const int32 NewCredits = FMath::Max(0, Credits);
	const int32 NewActiveFare = FMath::Max(0, ActiveFare);
	if (DisplayCredits == NewCredits && DisplayActiveFare == NewActiveFare)
	{
		return;
	}
	DisplayCredits = NewCredits;
	DisplayActiveFare = NewActiveFare;
	RefreshInterface();
}

void AFlyingCabPlayerController::SetTrafficAlert(
	const FText& Alert,
	const FLinearColor& Color)
{
	if (InterfaceWidget)
	{
		InterfaceWidget->SetTrafficAlert(Alert, Color);
	}
}

void AFlyingCabPlayerController::ReleaseInterfaceInputs()
{
	if (InterfaceWidget)
	{
		InterfaceWidget->ReleaseAllInputs();
	}
}

void AFlyingCabPlayerController::CreateInterfaceWidget()
{
	if (InterfaceWidget)
	{
		return;
	}

	InterfaceWidget = CreateWidget<UFlyingCabTouchControls>(this);
	if (!InterfaceWidget)
	{
		return;
	}

	InterfaceWidget->AddToViewport(100);
	InterfaceWidget->SetOnFootMode(PlayerMode == EFlyingCabPlayerMode::OnFoot);
	RefreshInterface();
	ApplyTouchControlsVisibility();
	UE_LOG(LogFlyingCabInteraction, Display, TEXT("PlayerController created the persistent touch HUD."));
}

void AFlyingCabPlayerController::RefreshInterface()
{
	if (!InterfaceWidget)
	{
		CreateInterfaceWidget();
		return;
	}

	const AFlyingCabPawn* Vehicle = Cast<AFlyingCabPawn>(GetPawn());
	if (!Vehicle)
	{
		if (const AFlyingCabGameMode* GameMode =
			GetWorld()->GetAuthGameMode<AFlyingCabGameMode>())
		{
			Vehicle = GameMode->GetActiveVehicle();
		}
	}
	InterfaceWidget->SetResourceState(
		Vehicle ? Vehicle->GetFuelPercent() : 0.0f,
		Vehicle ? Vehicle->GetHullPercent() : 0.0f,
		DisplayCredits,
		DisplayActiveFare,
		Vehicle && Vehicle->IsRefuelAvailable(),
		Vehicle ? Vehicle->GetRefuelPricePerUnit() : 0,
		Vehicle && Vehicle->IsRepairAvailable(),
		Vehicle ? Vehicle->GetRepairPricePerHullUnit() : 0,
		!Vehicle || Vehicle->IsDestroyed());
}

void AFlyingCabPlayerController::ApplyTouchControlsVisibility()
{
	if (!InterfaceWidget)
	{
		return;
	}
	InterfaceWidget->SetControlsVisible(true);

#if WITH_EDITOR
	if (bEnableMouseTouchTestingInEditor && !bGameFlowScreenOpen)
	{
		SetShowMouseCursor(true);
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}
#endif
}
