// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "FlyingCabCameraRig.h"
#include "FlyingCabCharacter.h"
#include "FlyingCabGameMode.h"
#include "FlyingCabInteractable.h"
#include "FlyingCabInputData.h"
#include "FlyingCabPawn.h"
#include "FlyingCabQuestSubsystem.h"
#include "FlyingCabQuestDefinition.h"
#include "FlyingCabQuestEventComponent.h"
#include "FlyingCabQuestJournalWidget.h"
#include "FlyingCabQuestTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "FlyingCabGameFlowWidget.h"
#include "FlyingCabTouchControls.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabInteraction, Log, All);

AFlyingCabPlayerController::AFlyingCabPlayerController()
{
	bAutoManageActiveCameraTarget = false;
	ControlInput = CreateDefaultSubobject<UFlyingCabControlInputComponent>(TEXT("ControlInput"));
}

void AFlyingCabPlayerController::BeginPlay()
{
	Super::BeginPlay();
	EnsureEnhancedInputContext();
	CreateInterfaceWidget();
	BindQuestPresentation();
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
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFlyingCabQuestSubsystem* Quests =
			GameInstance->GetSubsystem<UFlyingCabQuestSubsystem>())
		{
			Quests->OnQuestUpdated.RemoveDynamic(
				this,
				&AFlyingCabPlayerController::HandleQuestUpdated);
		}
	}
	if (QuestJournalWidget)
	{
		QuestJournalWidget->RemoveFromParent();
		QuestJournalWidget = nullptr;
	}
	bQuestJournalOpen = false;
	if (InterfaceWidget)
	{
		InterfaceWidget->ReleaseAllInputs();
		InterfaceWidget->RemoveFromParent();
		InterfaceWidget = nullptr;
	}
	RemoveEnhancedInputContext();
	Super::EndPlay(EndPlayReason);
}

void AFlyingCabPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// Enhanced Input has finished evaluating its delegates when Super::PlayerTick
	// returns. Gameplay transitions requested by those delegates are deliberately
	// executed here so their input flush cannot freeze an action value mid-pass.
	if (bDeferredInputTransitionGuard)
	{
		bDeferredInputTransitionGuard = false;
	}
	ProcessDeferredInputCommands();
	if (ControlInput)
	{
		if (UFlyingCabControlInputComponent::IsControlFrameEnabled())
		{
			ControlInput->BuildControlFrame(
				this,
				Cast<UEnhancedInputComponent>(InputComponent),
				GetControlInputBlock());
		}
		else
		{
			ControlInput->ResetControlFrame(GetControlInputBlock());
		}
	}

	if (!bDeveloperObserverMode || !CameraRig)
	{
		return;
	}

	const float Horizontal =
		(IsInputKeyDown(EKeys::D) || IsInputKeyDown(EKeys::Right) ? 1.0f : 0.0f)
		- (IsInputKeyDown(EKeys::A) || IsInputKeyDown(EKeys::Left) ? 1.0f : 0.0f);
	const float Vertical =
		(IsInputKeyDown(EKeys::W) || IsInputKeyDown(EKeys::Up) ? 1.0f : 0.0f)
		- (IsInputKeyDown(EKeys::S) || IsInputKeyDown(EKeys::Down) ? 1.0f : 0.0f);
	const bool bFast = IsInputKeyDown(EKeys::LeftShift)
		|| IsInputKeyDown(EKeys::RightShift);
	CameraRig->MoveDeveloperObserver(FVector2D(Horizontal, Vertical), bFast, DeltaTime);

	const float ZoomInput =
		(IsInputKeyDown(EKeys::PageDown) || IsInputKeyDown(EKeys::Subtract) ? 1.0f : 0.0f)
		- (IsInputKeyDown(EKeys::PageUp) || IsInputKeyDown(EKeys::Add) ? 1.0f : 0.0f);
	CameraRig->AdjustDeveloperObserverZoom(ZoomInput, DeltaTime);
	if (!FMath::IsNearlyZero(ZoomInput))
	{
		RefreshDeveloperObserverHud();
	}
}

bool AFlyingCabPlayerController::InputKey(const FInputKeyEventArgs& Params)
{
	if (ControlInput) { ControlInput->TraceKeyEvent(Params); }
	if (Params.Event == IE_Pressed && Params.Key == EKeys::O)
	{
		ToggleDeveloperObserverMode();
		return true;
	}
	if (bDeveloperObserverMode && Params.Event == IE_Pressed)
	{
		if (Params.Key == EKeys::Home)
		{
			RecenterDeveloperObserver();
			return true;
		}
		if (Params.Key == EKeys::MouseScrollUp)
		{
			ZoomDeveloperObserverIn();
			return true;
		}
		if (Params.Key == EKeys::MouseScrollDown)
		{
			ZoomDeveloperObserverOut();
			return true;
		}
	}
	return Super::InputKey(Params);
}

void AFlyingCabPlayerController::FlushPressedKeys()
{
	Super::FlushPressedKeys();
	if (ControlInput)
	{
		ControlInput->ResetControlFrame(GetControlInputBlock());
	}
	if (AFlyingCabPawn* Vehicle = Cast<AFlyingCabPawn>(GetPawn()))
	{
		Vehicle->ReleaseKeyboardInputState();
	}
	else if (AFlyingCabCharacter* OnFootCharacter = Cast<AFlyingCabCharacter>(GetPawn()))
	{
		OnFootCharacter->ReleaseKeyboardInputState();
	}
	ReleaseInterfaceInputs();
}

void AFlyingCabPlayerController::StartRunMode(EFlyingCabRunMode Mode)
{
	if (Mode == EFlyingCabRunMode::None)
	{
		return;
	}
	if (bQuestJournalOpen)
	{
		CloseQuestJournal();
	}

	AFlyingCabGameMode* GameMode = GetWorld()->GetAuthGameMode<AFlyingCabGameMode>();
	if (!GameMode)
	{
		return;
	}

	GameMode->StartRun(Mode);
	if (GameFlowWidget)
	{
		GameFlowWidget->HideFlowScreen();
	}
	SetPause(false);
	FlushPressedKeys();
	RestoreGameplayInputMode();
	bGameFlowScreenOpen = false;
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
	if (bQuestJournalOpen)
	{
		CloseQuestJournal();
	}
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

	bGameFlowScreenOpen = true;
	FlushPressedKeys();
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
		EnhancedInput->BindActionValue(InputAssets.Horizontal);
		EnhancedInput->BindActionValue(InputAssets.Thrust);
		EnhancedInput->BindActionValue(InputAssets.Service);
		EnhancedInput->BindAction(
			InputAssets.Interact,
			ETriggerEvent::Started,
			this,
			&AFlyingCabPlayerController::RequestContextInteraction);
		EnhancedInput->BindAction(
			InputAssets.Restart,
			ETriggerEvent::Started,
			this,
			&AFlyingCabPlayerController::RequestVehicleReset);
		EnhancedInput->BindAction(
			InputAssets.QuestJournal,
			ETriggerEvent::Started,
			this,
			&AFlyingCabPlayerController::RequestQuestJournalToggle);
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
	if (!bDeveloperObserverMode)
	{
		EnsureEnhancedInputContext();
	}
	CachedContextPrompt = FText::GetEmpty();
	CachedContextPromptPawn.Reset();
	LastContextPromptRefreshTime = -1.0;
	if (AFlyingCabPawn* Vehicle = Cast<AFlyingCabPawn>(InPawn))
	{
		PlayerMode = EFlyingCabPlayerMode::Vehicle;
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UFlyingCabQuestSubsystem* Quests =
				GameInstance->GetSubsystem<UFlyingCabQuestSubsystem>())
			{
				Quests->RecordEvent(FlyingCabQuestEvents::VehicleEntered, Vehicle->GetVehicleId());
			}
		}
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
	if (CameraRig && InPawn && !bDeveloperObserverMode)
	{
		CameraRig->SetFollowTarget(InPawn, false);
	}
}

void AFlyingCabPlayerController::ToggleDeveloperObserverMode()
{
	if (bGameFlowScreenOpen || bQuestJournalOpen)
	{
		return;
	}
	SetDeveloperObserverMode(!bDeveloperObserverMode);
}

void AFlyingCabPlayerController::SetDeveloperObserverMode(bool bEnabled)
{
	if (bDeveloperObserverMode == bEnabled || !CameraRig)
	{
		return;
	}

	if (bEnabled)
	{
		bDeveloperObserverMode = true;
		FlushPressedKeys();
		CameraRig->SetDeveloperObserverEnabled(true);
		FreezeControlledVehicleForObserver();
		RestoreGameplayInputMode();
		RefreshDeveloperObserverHud();
		UE_LOG(
			LogFlyingCabInteraction,
			Display,
			TEXT("Developer observer enabled; gameplay actions are suppressed without rebuilding the input context."));
	}
	else
	{
		// Keep suppression active until the vehicle, camera and input mode are all restored.
		FlushPressedKeys();
		RestoreControlledVehicleAfterObserver();
		CameraRig->SetDeveloperObserverEnabled(false);
		CameraRig->SetFollowTarget(GetPawn(), true);
		EnsureEnhancedInputContext();
		RestoreGameplayInputMode();
		bDeveloperObserverMode = false;
		ApplyTouchControlsVisibility();
		if (InterfaceWidget)
		{
			InterfaceWidget->SetDeveloperObserverState(false, 0.0f);
		}
		UE_LOG(
			LogFlyingCabInteraction,
			Display,
			TEXT("Developer observer disabled; persistent gameplay input restored."));
	}
}

void AFlyingCabPlayerController::RecenterDeveloperObserver()
{
	if (bDeveloperObserverMode && CameraRig)
	{
		CameraRig->RecenterDeveloperObserver();
	}
}

void AFlyingCabPlayerController::ZoomDeveloperObserverIn()
{
	if (bDeveloperObserverMode && CameraRig)
	{
		CameraRig->AdjustDeveloperObserverZoom(-1.0f, 0.12f);
		RefreshDeveloperObserverHud();
	}
}

void AFlyingCabPlayerController::ZoomDeveloperObserverOut()
{
	if (bDeveloperObserverMode && CameraRig)
	{
		CameraRig->AdjustDeveloperObserverZoom(1.0f, 0.12f);
		RefreshDeveloperObserverHud();
	}
}

void AFlyingCabPlayerController::RefreshDeveloperObserverHud()
{
	if (InterfaceWidget && CameraRig)
	{
		InterfaceWidget->SetDeveloperObserverState(
			bDeveloperObserverMode,
			CameraRig->GetCurrentArmLength() / 100.0f);
	}
}

void AFlyingCabPlayerController::FreezeControlledVehicleForObserver()
{
	DeveloperObserverFrozenBody.Reset();
	DeveloperObserverSavedLinearVelocity = FVector::ZeroVector;
	DeveloperObserverSavedAngularVelocity = FVector::ZeroVector;
	AFlyingCabPawn* Vehicle = Cast<AFlyingCabPawn>(GetPawn());
	UPrimitiveComponent* Body = Vehicle
		? Cast<UPrimitiveComponent>(Vehicle->GetRootComponent())
		: nullptr;
	if (!Body || !Body->IsSimulatingPhysics())
	{
		return;
	}

	DeveloperObserverSavedLinearVelocity = Body->GetPhysicsLinearVelocity();
	DeveloperObserverSavedAngularVelocity = Body->GetPhysicsAngularVelocityInDegrees();
	Body->SetPhysicsLinearVelocity(FVector::ZeroVector);
	Body->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	Body->SetSimulatePhysics(false);
	DeveloperObserverFrozenBody = Body;
}

void AFlyingCabPlayerController::RestoreControlledVehicleAfterObserver()
{
	UPrimitiveComponent* Body = DeveloperObserverFrozenBody.Get();
	if (Body)
	{
		Body->SetSimulatePhysics(true);
		Body->SetPhysicsLinearVelocity(DeveloperObserverSavedLinearVelocity);
		Body->SetPhysicsAngularVelocityInDegrees(DeveloperObserverSavedAngularVelocity);
		Body->WakeAllRigidBodies();
	}
	DeveloperObserverFrozenBody.Reset();
	DeveloperObserverSavedLinearVelocity = FVector::ZeroVector;
	DeveloperObserverSavedAngularVelocity = FVector::ZeroVector;
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
	if (ControlInput)
	{
		ControlInput->InvalidateMappedKeys();
	}
	return true;
}

void AFlyingCabPlayerController::RemoveEnhancedInputContext()
{
	const FFlyingCabInputAssets& InputAssets = FlyingCabInputData::GetAssets();
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!InputAssets.MappingContext || !LocalPlayer)
	{
		return;
	}
	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		InputSubsystem->RemoveMappingContext(InputAssets.MappingContext);
	}
	if (ControlInput)
	{
		ControlInput->InvalidateMappedKeys();
	}
}

bool AFlyingCabPlayerController::IsControlFrameEnabled() const
{
	return ControlInput && UFlyingCabControlInputComponent::IsControlFrameEnabled();
}

void AFlyingCabPlayerController::TraceVehicleInput(const AFlyingCabPawn* Vehicle,
	const FVector2D& Keyboard, const FVector2D& Touch, const FVector& AppliedForce, const TCHAR* Reason)
{
	if (ControlInput)
	{
		ControlInput->TraceVehicleState(Vehicle, Keyboard, Touch, AppliedForce, Reason);
	}
}

const FFlyingCabControlFrame& AFlyingCabPlayerController::GetControlFrame() const
{
	static const FFlyingCabControlFrame EmptyFrame;
	return ControlInput ? ControlInput->GetControlFrame() : EmptyFrame;
}

EFlyingCabInputBlock AFlyingCabPlayerController::GetControlInputBlock() const
{
	if (bDeferredInputTransitionGuard)
	{
		return EFlyingCabInputBlock::Transition;
	}
	if (bGameFlowScreenOpen)
	{
		return EFlyingCabInputBlock::Menu;
	}
	if (bQuestJournalOpen)
	{
		return EFlyingCabInputBlock::QuestJournal;
	}
	if (bDeveloperObserverMode)
	{
		return EFlyingCabInputBlock::Observer;
	}
	return EFlyingCabInputBlock::None;
}

void AFlyingCabPlayerController::RequestContextInteraction()
{
	bContextInteractionRequested = true;
}

void AFlyingCabPlayerController::RequestVehicleReset()
{
	bVehicleResetRequested = true;
}

void AFlyingCabPlayerController::RequestQuestJournalToggle()
{
	bQuestJournalToggleRequested = true;
}

void AFlyingCabPlayerController::ProcessDeferredInputCommands()
{
	const bool bToggleJournal = bQuestJournalToggleRequested;
	const bool bInteract = bContextInteractionRequested;
	const bool bResetVehicle = bVehicleResetRequested;
	bQuestJournalToggleRequested = false;
	bContextInteractionRequested = false;
	bVehicleResetRequested = false;

	// Only one state-changing command is accepted per input frame. The journal has
	// priority because it changes pause/focus state; interaction precedes reset.
	if (bToggleJournal)
	{
		bDeferredInputTransitionGuard = true;
		ToggleQuestJournal();
		return;
	}
	if (IsGameplayInputSuppressed())
	{
		return;
	}
	if (bInteract)
	{
		bDeferredInputTransitionGuard = true;
		ExecuteContextInteraction();
		return;
	}
	if (bResetVehicle)
	{
		if (AFlyingCabPawn* Vehicle = Cast<AFlyingCabPawn>(GetPawn()))
		{
			bDeferredInputTransitionGuard = true;
			Vehicle->ResetVehicle();
		}
	}
}

void AFlyingCabPlayerController::ExecuteContextInteraction()
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
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFlyingCabQuestSubsystem* Quests =
			GameInstance->GetSubsystem<UFlyingCabQuestSubsystem>())
		{
			Quests->RecordEvent(FlyingCabQuestEvents::VehicleExited, Vehicle->GetVehicleId());
		}
	}
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
	if (bSucceeded && !InteractableActor->FindComponentByClass<UFlyingCabQuestEventComponent>())
	{
		const FName QuestTargetId = Interactable->GetQuestTargetId();
		if (!QuestTargetId.IsNone())
		{
			if (UGameInstance* GameInstance = GetGameInstance())
			{
				if (UFlyingCabQuestSubsystem* Quests =
					GameInstance->GetSubsystem<UFlyingCabQuestSubsystem>())
				{
					Quests->RecordEvent(
						FlyingCabQuestEvents::InteractionCompleted,
						QuestTargetId);
				}
			}
		}
	}
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

void AFlyingCabPlayerController::ShowMajorAnnouncement(
	const FText& Title,
	const FText& Detail,
	const FLinearColor& Color,
	float DurationSeconds,
	int32 InPriority) const
{
	if (UFlyingCabTouchControls* Widget = GetInterfaceWidget())
	{
		Widget->ShowMajorAnnouncement(
			Title,
			Detail,
			Color,
			DurationSeconds,
			InPriority);
	}
}

void AFlyingCabPlayerController::ToggleQuestJournal()
{
	if (bQuestJournalOpen)
	{
		CloseQuestJournal();
	}
	else
	{
		OpenQuestJournal();
	}
}

void AFlyingCabPlayerController::OpenQuestJournal()
{
	if (bGameFlowScreenOpen || bQuestJournalOpen)
	{
		return;
	}
	UFlyingCabQuestSubsystem* Quests = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFlyingCabQuestSubsystem>()
		: nullptr;
	if (!Quests || !Quests->AreGameplayEventsEnabled())
	{
		ShowEventMessage(
			FText::FromString(TEXT("SHIFT LOG UNAVAILABLE IN THIS MODE")),
			FLinearColor(0.75f, 0.78f, 0.82f),
			1.5f);
		return;
	}

	if (!QuestJournalWidget)
	{
		QuestJournalWidget = CreateWidget<UFlyingCabQuestJournalWidget>(this);
		if (QuestJournalWidget)
		{
			QuestJournalWidget->AddToViewport(450);
		}
	}
	if (!QuestJournalWidget)
	{
		return;
	}

	bQuestJournalOpen = true;
	FlushPressedKeys();
	QuestJournalWidget->ShowJournal();
	SetPause(true);
	bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(QuestJournalWidget->TakeWidget());
	SetInputMode(InputMode);
}

void AFlyingCabPlayerController::CloseQuestJournal()
{
	if (!bQuestJournalOpen)
	{
		return;
	}
	if (QuestJournalWidget)
	{
		QuestJournalWidget->HideJournal();
	}
	SetPause(false);
	// The journal still suppresses gameplay while raw/action state is neutralized.
	bDeferredInputTransitionGuard = true;
	FlushPressedKeys();
	RestoreGameplayInputMode();
	EnsureEnhancedInputContext();
	bQuestJournalOpen = false;
	ApplyTouchControlsVisibility();
}

void AFlyingCabPlayerController::BindQuestPresentation()
{
	UGameInstance* GameInstance = GetGameInstance();
	UFlyingCabQuestSubsystem* Quests = GameInstance
		? GameInstance->GetSubsystem<UFlyingCabQuestSubsystem>()
		: nullptr;
	if (!Quests)
	{
		return;
	}
	Quests->OnQuestUpdated.RemoveDynamic(
		this,
		&AFlyingCabPlayerController::HandleQuestUpdated);
	Quests->OnQuestUpdated.AddDynamic(
		this,
		&AFlyingCabPlayerController::HandleQuestUpdated);
}

void AFlyingCabPlayerController::HandleQuestUpdated(FFlyingCabQuestUpdate Update)
{
	if (Update.ChangeType == EFlyingCabQuestChangeType::Reset)
	{
		return;
	}
	UGameInstance* GameInstance = GetGameInstance();
	const UFlyingCabQuestSubsystem* Quests = GameInstance
		? GameInstance->GetSubsystem<UFlyingCabQuestSubsystem>()
		: nullptr;
	const UFlyingCabQuestDefinition* Definition = Quests
		? Quests->GetQuestDefinition(Update.QuestId)
		: nullptr;
	if (!Definition)
	{
		return;
	}

	switch (Update.ChangeType)
	{
	case EFlyingCabQuestChangeType::Started:
		ShowMajorAnnouncement(
			FText::FromString(TEXT("NEW ASSIGNMENT")),
			Definition->Title,
			FLinearColor(1.0f, 0.68f, 0.08f),
			2.6f,
			20);
		break;
	case EFlyingCabQuestChangeType::ObjectiveCompleted:
		ShowMajorAnnouncement(
			FText::FromString(TEXT("OBJECTIVE COMPLETE")),
			Definition->Objectives.IsValidIndex(Update.ObjectiveIndex)
				? Definition->Objectives[Update.ObjectiveIndex].Description
				: Definition->Title,
			FLinearColor(0.10f, 0.93f, 1.0f),
			2.1f,
			10);
		break;
	case EFlyingCabQuestChangeType::ReadyToTurnIn:
		ShowMajorAnnouncement(
			FText::FromString(TEXT("OBJECTIVES COMPLETE")),
			FText::Format(
				NSLOCTEXT(
					"FlyingCab",
					"QuestReturnAnnouncement",
					"{0}  //  RETURN TO QUEST GIVER"),
				Definition->Title),
			FLinearColor(0.20f, 1.0f, 0.58f),
			2.7f,
			30);
		break;
	case EFlyingCabQuestChangeType::Completed:
	{
		const FText Detail = Definition->Reward.Credits > 0
			? FText::Format(
				NSLOCTEXT(
					"FlyingCab",
					"QuestCompleteRewardAnnouncement",
					"{0}  //  +{1} CR"),
				Definition->Title,
				FText::AsNumber(Definition->Reward.Credits))
			: Definition->Title;
		ShowMajorAnnouncement(
			FText::FromString(TEXT("JOB COMPLETE")),
			Detail,
			FLinearColor(0.20f, 1.0f, 0.58f),
			3.0f,
			40);
		break;
	}
	case EFlyingCabQuestChangeType::Progressed:
	case EFlyingCabQuestChangeType::Reset:
	default:
		break;
	}
}

void AFlyingCabPlayerController::SetObjectiveStatus(const FText& Status)
{
	if (InterfaceWidget)
	{
		InterfaceWidget->SetObjectiveText(Status);
	}
}

void AFlyingCabPlayerController::SetQuestStatus(const FText& Status)
{
	if (InterfaceWidget)
	{
		InterfaceWidget->SetQuestText(Status);
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
