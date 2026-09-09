#include "FlyingCabInputDiagnosticsSubsystem.h"

#include "Async/Async.h"
#include "Components/PrimitiveComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "FlyingCabInputData.h"
#include "FlyingCabPawn.h"
#include "FlyingCabPlayerController.h"
#include "FlyingCabVehicleVitalsComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/ThreadSafeCounter.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Misc/FileHelper.h"
#include "Misc/OutputDevice.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/Paths.h"
#include "Widgets/SViewport.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabInputDiagnostics, Log, All);

namespace
{
TAutoConsoleVariable<int32> CVarInputDiagnostics(TEXT("flyingcab.InputDiagnostics"), 1,
	TEXT("Passive development input history and incident dumps. Never modifies controls."));
FThreadSafeCounter PendingWrites;
bool IsEnabled() { return !UE_BUILD_SHIPPING && CVarInputDiagnostics.GetValueOnGameThread() != 0; }
bool IsTraceEnabled()
{
	const IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("flyingcab.InputTrace"));
	return Var && Var->GetInt() != 0;
}
const TCHAR* BlockName(EFlyingCabInputBlock Block)
{
	switch (Block)
	{
	case EFlyingCabInputBlock::None: return TEXT("None");
	case EFlyingCabInputBlock::Menu: return TEXT("Menu");
	case EFlyingCabInputBlock::QuestJournal: return TEXT("QuestJournal");
	case EFlyingCabInputBlock::Observer: return TEXT("Observer");
	case EFlyingCabInputBlock::Transition: return TEXT("Transition");
	default: return TEXT("Unknown");
	}
}
FAutoConsoleCommandWithWorld DumpInputCommand(TEXT("flyingcab.DumpInput"),
	TEXT("Save the last five seconds of passive input evidence to Saved/Logs/InputDiagnostics."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (World)
		{
			if (auto* Diagnostics = World->GetSubsystem<UFlyingCabInputDiagnosticsSubsystem>())
			{ Diagnostics->RequestDump(); }
		}
	}));
}

// Synchronous producer-thread observation is necessary: the default buffered
// output device runs on the log thread, where inspecting UObjects is unsafe.
class FFlyingCabDiagnosticLogObserver final : public FOutputDevice
{
public:
	explicit FFlyingCabDiagnosticLogObserver(UFlyingCabInputDiagnosticsSubsystem* InOwner) : Owner(InOwner)
	{ GLog->AddOutputDevice(this); }
	virtual ~FFlyingCabDiagnosticLogObserver() { GLog->RemoveOutputDevice(this); }
	virtual bool CanBeUsedOnAnyThread() const override { return true; }
	virtual bool CanBeUsedOnMultipleThreads() const override { return true; }
	virtual void Serialize(const TCHAR* Text, ELogVerbosity::Type, const FName& Category) override
	{
		if (IsInGameThread() && Category == FName(TEXT("LogFlyingCabInputTrace")) && IsEnabled())
		{ Owner->ObserveLog(Text); }
	}
private:
	UFlyingCabInputDiagnosticsSubsystem* Owner;
};

bool UFlyingCabInputDiagnosticsSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return !UE_BUILD_SHIPPING && Super::ShouldCreateSubsystem(Outer);
}
bool UFlyingCabInputDiagnosticsSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}
TStatId UFlyingCabInputDiagnosticsSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UFlyingCabInputDiagnosticsSubsystem, STATGROUP_Tickables);
}
void UFlyingCabInputDiagnosticsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LogObserver = new FFlyingCabDiagnosticLogObserver(this);
	if (FSlateApplication::IsInitialized())
	{
		ActivationHandle = FSlateApplication::Get().OnApplicationActivationStateChanged().AddUObject(
			this, &UFlyingCabInputDiagnosticsSubsystem::ObserveActivation);
	}
}
void UFlyingCabInputDiagnosticsSubsystem::Deinitialize()
{
	delete LogObserver;
	LogObserver = nullptr;
	if (FSlateApplication::IsInitialized())
	{ FSlateApplication::Get().OnApplicationActivationStateChanged().Remove(ActivationHandle); }
	if (IsEnabled()) { WritePendingDump(); }
	Super::Deinitialize();
}
void UFlyingCabInputDiagnosticsSubsystem::Record(const FString& Text)
{
	const double Now = FPlatformTime::Seconds();
	const FString Entry = FString::Printf(TEXT("frame=%llu %s"), GFrameCounter, *Text);
	History.Add(Now, Entry);
	// Preserve the first incident's five-second prelude AND subsequent evidence
	// while rate-limited requests coalesce. Keep even a stalled writer bounded.
	if (!PendingReasons.IsEmpty())
	{
		if (PendingPayload.Len() < 8 * 1024 * 1024)
		{ PendingPayload += FString::Printf(TEXT("%+.6fs %s\n"), Now - PendingIncidentAt, *Entry.Left(2048)); }
		else if (!bPendingTruncated) { PendingPayload += TEXT("[pending evidence truncated]\n"); bPendingTruncated = true; }
	}
}
FString UFlyingCabInputDiagnosticsSubsystem::GetHistoryText()
{
	return History.Export(FPlatformTime::Seconds());
}
void UFlyingCabInputDiagnosticsSubsystem::ObserveActivation(bool bActive)
{
	if (!IsInGameThread() || !IsEnabled()) { return; }
	Record(FString::Printf(TEXT("APP_ACTIVATION_EVENT active=%d"), bActive));
}
void UFlyingCabInputDiagnosticsSubsystem::ObserveLog(const TCHAR* Text)
{
	// KEY messages have no controller id in the canonical trace. Input delivery
	// can occur while GWorld is the editor (including automation/Slate callbacks).
	// Accept it only when there is exactly one local gameplay world. Never guess
	// the source of a key when several PIE worlds are running in this process.
	UWorld* World = GetWorld();
	auto* PC = Cast<AFlyingCabPlayerController>(World->GetFirstPlayerController());
	if (!PC || !PC->IsLocalController()) { return; }
	const FString Line(Text);
	if (Line.StartsWith(TEXT("KEY ")) || GWorld != World)
	{
		int32 LocalWorlds = 0;
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* Candidate = Context.World();
				APlayerController* Controller = Candidate && Candidate->IsGameWorld() ? Candidate->GetFirstPlayerController() : nullptr;
				if (Controller && Controller->IsLocalController()) { ++LocalWorlds; }
			}
		}
		if (LocalWorlds != 1) { return; }
	}
	if (Line.StartsWith(TEXT("KEY "))) { Record(Line); return; }
	auto* Cab = Cast<AFlyingCabPawn>(PC->GetPawn());
	if (!Cab || !Line.Contains(TEXT("pawn=") + Cab->GetName() + TEXT(" "))) { return; }
	Record(Line);
	if (Line.StartsWith(TEXT("STATE reason=change ")))
	{
		TracePawn = Cab;
		FString Before, After, ForceText;
		bTraceKnown = Line.Split(TEXT(" force="), &Before, &After)
			&& After.Split(TEXT(" fuel="), &ForceText, &After) && ForceText.StartsWith(TEXT("V("));
		LastForce = ForceText;
		FVector Force = FVector::ZeroVector;
		FParse::Value(*ForceText, TEXT("X="), Force.X);
		FParse::Value(*ForceText, TEXT("Z="), Force.Z);
		double KeyboardH = 0, KeyboardT = 0, TouchH = 0, TouchT = 0;
		auto ReadPair = [&](const TCHAR* Prefix, double& H, double& T)
		{
			FString Pair, Tail;
			if (!Line.Split(Prefix, &Before, &Tail) || !Tail.Split(TEXT(")"), &Pair, &Tail)) { return false; }
			FString Left, Right;
			if (!Pair.Split(TEXT(","), &Left, &Right)) { return false; }
			H = FCString::Atod(*Left); T = FCString::Atod(*Right); return true;
		};
		const bool bPairsKnown = ReadPair(TEXT("keyboard=("), KeyboardH, KeyboardT)
			&& ReadPair(TEXT("touch=("), TouchH, TouchT);
		LastCabRequest = bPairsKnown ? FString::Printf(TEXT("keyboard=(%.2f,%.2f) touch=(%.2f,%.2f)"), KeyboardH, KeyboardT, TouchH, TouchT) : TEXT("unknown");
		bMissingForce = bTraceKnown && bPairsKnown && Line.Contains(TEXT(" gate=None "))
			&& ((FMath::Abs(KeyboardH + TouchH) > 0.1 && FMath::IsNearlyZero(Force.X))
				|| (FMath::Max(KeyboardT, TouchT) > 0.1 && FMath::IsNearlyZero(Force.Z)));
	}
	else
	{
		// Explicit transition snapshots can contain force from the previous tick.
		bTraceKnown = bMissingForce = false;
		TransitionGraceUntil = FPlatformTime::Seconds() + 0.25;
		if (Line.StartsWith(TEXT("STATE reason=vehicle destroyed "))) { RequestDump(TEXT("vehicle_destroyed")); }
		if (Line.StartsWith(TEXT("STATE reason=recovery complete "))) { RequestDump(TEXT("recovery_complete")); }
		if (Line.StartsWith(TEXT("STATE reason=manual reset "))) { RequestDump(TEXT("manual_reset")); }
	}
}
void UFlyingCabInputDiagnosticsSubsystem::ResetObservations()
{
	LastState.Reset(); LastGate.Reset(); LastForce.Reset(); LastCabRequest.Reset(); TracePawn.Reset();
	bTraceKnown = bMissingForce = false;
	LastFocus = LastActivation = -2;
	for (auto& Latch : StaleActions) { Latch = {}; }
	MissingForce = {};
}
void UFlyingCabInputDiagnosticsSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!IsEnabled())
	{
		if (bEnabledLastTick)
		{ History.Reset(); PendingReasons.Reset(); PendingPayload.Reset(); bPendingManual = false; ResetObservations(); }
		bEnabledLastTick = false;
		return;
	}
	bEnabledLastTick = true;
	const double Now = FPlatformTime::Seconds();
	auto* PC = Cast<AFlyingCabPlayerController>(GetWorld()->GetFirstPlayerController());
	if (!PC || !PC->IsLocalController()) { return; }
	APawn* Pawn = PC->GetPawn();
	auto* Cab = Cast<AFlyingCabPawn>(Pawn);
	UPrimitiveComponent* Body = Cab ? Cast<UPrimitiveComponent>(Cab->GetRootComponent()) : nullptr;
	const auto* Vitals = Cab ? Cab->FindComponentByClass<UFlyingCabVehicleVitalsComponent>() : nullptr;
	const FString Gate = !Cab ? TEXT("NotVehicle") : Cab->IsDestroyed() ? TEXT("Destroyed")
		: !Body || !Body->IsSimulatingPhysics() ? TEXT("PhysicsOff")
		: PC->IsGameplayInputSuppressed() ? FString(BlockName(PC->GetControlInputBlock()))
		: GetWorld()->IsPaused() ? TEXT("WorldPaused")
		: !Vitals ? TEXT("UnknownVitals") : !Vitals->CanUseThrusters() ? TEXT("NoFuel") : TEXT("None");
	if (Gate != LastGate)
	{
		Record(TEXT("PROPULSION_GATE ") + LastGate + TEXT(" -> ") + Gate);
		if (Gate == TEXT("NoFuel")) { RequestDump(TEXT("no_fuel")); }
		LastGate = Gate;
	}
	const int32 Activation = FSlateApplication::IsInitialized() ? (FSlateApplication::Get().IsActive() ? 1 : 0) : -1;
	const TSharedPtr<SViewport> Viewport = GetWorld()->GetGameViewport() ? GetWorld()->GetGameViewport()->GetGameViewportWidget() : nullptr;
	const int32 Focus = Viewport.IsValid() ? (Viewport->HasAnyUserFocusOrFocusedDescendants() ? 1 : 0) : -1;
	if (Focus != LastFocus || Activation != LastActivation)
	{
		Record(FString::Printf(TEXT("FOCUS_CHANGED viewport=%d->%d app=%d->%d (-1=unavailable)"), LastFocus, Focus, LastActivation, Activation));
		LastFocus = Focus; LastActivation = Activation;
	}
	const FString State = FString::Printf(TEXT("pawn=%s mode=%d input_mode=%s block=%s journal=%d observer=%d menu=%d paused=%d cursor=%d pc_input=%p pawn_input=%p"),
		*GetPathNameSafe(Pawn), static_cast<int32>(PC->GetPlayerMode()), *PC->GetCurrentInputModeDebugString(), BlockName(PC->GetControlInputBlock()),
		PC->IsQuestJournalOpen(), PC->IsDeveloperObserverMode(), PC->IsGameFlowScreenOpen(), GetWorld()->IsPaused(), PC->bShowMouseCursor,
		PC->InputComponent.Get(), Pawn ? Pawn->InputComponent.Get() : nullptr);
	if (State != LastState)
	{
		Record(TEXT("INPUT_MODE_CHANGED ") + State);
		LastState = State;
		TransitionGraceUntil = Now + 0.25;
	}
	const auto& Assets = FlyingCabInputData::GetAssets();
	const auto* Input = Cast<UEnhancedPlayerInput>(PC->PlayerInput);
	bool Raw[3] = {};
	float Values[3] = {};
	int32 Triggers[3] = {-1, -1, -1};
	FString RawText;
	const UInputAction* Actions[] = {Assets.Horizontal, Assets.Thrust, Assets.Service};
	const TCHAR* Names[] = {TEXT("Horizontal"), TEXT("Thrust"), TEXT("Service")};
	if (Input && Assets.IsValid())
	{
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Values[Index] = Input->GetActionValue(Actions[Index]).GetMagnitude();
			if (Index == 0) { Values[Index] = Input->GetActionValue(Actions[Index]).Get<float>(); }
			if (const FInputActionInstance* Instance = Input->FindActionInstanceData(Actions[Index]))
			{ Triggers[Index] = static_cast<int32>(Instance->GetTriggerEvent()); }
		}
		for (const FEnhancedActionKeyMapping& Mapping : Assets.MappingContext->GetMappings())
		{
			const bool bDown = PC->IsInputKeyDown(Mapping.Key);
			RawText += FString::Printf(TEXT("%s:%d "), *Mapping.Key.ToString(), bDown);
			for (int32 Index = 0; Index < 3; ++Index) { Raw[Index] |= Mapping.Action == Actions[Index] && bDown; }
		}
	}
	const bool bCanAssess = Input && Assets.IsValid() && Now >= TransitionGraceUntil && !PC->IsGameplayInputSuppressed() && !GetWorld()->IsPaused();
	for (int32 Index = 0; Index < 3; ++Index)
	{
		if (StaleActions[Index].Observe(bCanAssess && FMath::Abs(Values[Index]) > 0.1f && !Raw[Index], Now))
		{
			Record(FString::Printf(TEXT("ACTION_WITHOUT_MAPPED_KEY action=%s value=%.3f raw=[%s] observation_only=1"), Names[Index], Values[Index], *RawText));
			RequestDump(FString(TEXT("action_without_key_")) + Names[Index]);
		}
	}
	if (!IsTraceEnabled() || TracePawn.Get() != Cab) { bTraceKnown = bMissingForce = false; }
	if (MissingForce.Observe(bCanAssess && bTraceKnown && bMissingForce && Gate == TEXT("None"), Now))
	{
		Record(TEXT("THRUST_REQUESTED_NOT_APPLIED observation_only=1 source=last_completed_cab_trace"));
		RequestDump(TEXT("thrust_requested_not_applied"));
	}
	if (Now >= NextSampleAt)
	{
		const ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
		const auto* Subsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
		Record(FString::Printf(TEXT("SAMPLE %s gate=%s focus=%d app=%d mapping=%d frame_on=%d trace=%d raw=[%s] actions=(%.3f,%.3f,%.3f) triggers=(%d,%d,%d) fuel=%.3f hull=%.3f phys=%d awake=%d last_cab_trace_known=%d last_force_known=%d last_force=%s last_cab_request=[%s] movement=%s velocity=%s"),
			*State, *Gate, Focus, Activation, Subsystem && Assets.MappingContext && Subsystem->HasMappingContext(Assets.MappingContext), PC->IsControlFrameEnabled(), IsTraceEnabled(),
			*RawText, Values[0], Values[1], Values[2], Triggers[0], Triggers[1], Triggers[2], Vitals ? Vitals->GetFuel() : -1.f, Vitals ? Vitals->GetHull() : -1.f,
			Body && Body->IsSimulatingPhysics(), Body && Body->IsAnyRigidBodyAwake(), bTraceKnown, bTraceKnown, *LastForce, *LastCabRequest,
			*(Pawn ? Pawn->GetLastMovementInputVector() : FVector::ZeroVector).ToCompactString(), *(Pawn ? Pawn->GetVelocity() : FVector::ZeroVector).ToCompactString()));
		NextSampleAt = Now + 0.1;
	}
	if (Now >= NextDumpAt) { WritePendingDump(); }
}
void UFlyingCabInputDiagnosticsSubsystem::RequestDump(const FString& Reason)
{
	if (!IsEnabled()) { return; }
	const bool bManual = Reason == TEXT("manual");
	if (AutomaticDumps >= 64 && !bManual) { return; }
	Record(TEXT("DUMP_REQUEST reason=") + Reason);
	if (PendingReasons.IsEmpty())
	{
		PendingIncidentAt = FPlatformTime::Seconds();
		bPendingTruncated = false;
		PendingPayload = FString::Printf(TEXT("Flying Cab passive input evidence v1\nutc=%s world=%s\nCanonical input is not modified. Times are relative to the incident.\n"),
			*FDateTime::UtcNow().ToIso8601(), *GetPathNameSafe(GetWorld())) + GetHistoryText();
	}
	PendingReasons.AddUnique(Reason);
	bPendingManual |= bManual;
}
void UFlyingCabInputDiagnosticsSubsystem::WritePendingDump()
{
	if (PendingReasons.IsEmpty() || PendingWrites.GetValue() >= 2) { return; }
	LastDumpPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectLogDir() / TEXT("InputDiagnostics") /
		(FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S_")) + FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".log")));
	const FString Reasons = FString::Join(PendingReasons, TEXT(","));
	const FString Payload = TEXT("reasons=") + Reasons + TEXT("\n") + PendingPayload;
	const FString Path = LastDumpPath;
	PendingWrites.Increment();
	// No UObjects captured. Disk I/O never runs in the input or log callback.
	Async(EAsyncExecution::ThreadPool, [Path, Payload]()
	{
		const bool bWritten = IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true)
			&& FFileHelper::SaveStringToFile(Payload, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
		if (bWritten) { UE_LOG(LogFlyingCabInputDiagnostics, Display, TEXT("Input evidence saved: %s"), *Path); }
		else { UE_LOG(LogFlyingCabInputDiagnostics, Warning, TEXT("Could not write input evidence: %s"), *Path); }
		PendingWrites.Decrement();
	});
	UE_LOG(LogFlyingCabInputDiagnostics, Display, TEXT("Input evidence queued reasons=%s path=%s"), *Reasons, *Path);
	if (!bPendingManual) { ++AutomaticDumps; }
	bPendingManual = false;
	PendingReasons.Reset(); PendingPayload.Reset();
	NextDumpAt = FPlatformTime::Seconds() + 1.0;
}
