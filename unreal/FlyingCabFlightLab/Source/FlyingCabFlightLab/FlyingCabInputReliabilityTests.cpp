// Input-only fixtures: never change the canonical production implementation.
#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/TextBlock.h"
#include "EnhancedPlayerInput.h"
#include "FlyingCabCharacter.h"
#include "FlyingCabInputData.h"
#include "FlyingCabInputDiagnosticsSubsystem.h"
#include "FlyingCabPawn.h"
#include "FlyingCabPlayerController.h"
#include "FlyingCabQuestJournalWidget.h"
#include "FlyingCabTouchControls.h"
#include "FlyingCabVehicleVitalsComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "InputKeyEventArgs.h"
#include "Misc/AutomationTest.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/OutputDevice.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"

namespace FlyingCabInputTests
{
constexpr const TCHAR* Map = TEXT("/Game/Maps/FlightLab");
const FVector FixtureLocation(-30000.0, 0.0, 20000.0);
const FKey Keys[] = {EKeys::A, EKeys::Left, EKeys::D, EKeys::Right,
	EKeys::W, EKeys::Up, EKeys::SpaceBar, EKeys::E};
const TCHAR* Buttons[] = {TEXT("LeftButton"), TEXT("RightButton"),
	TEXT("ThrustButton"), TEXT("RefuelButton")};

void Send(AFlyingCabPlayerController* PC, FKey Key, EInputEvent Event)
{
	PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, Event, Event == IE_Released ? 0.0f : 1.0f));
}

// Observe existing development telemetry instead of adding test accessors to the
// frozen production files. Only game-thread messages from the fixture cab count.
class FTraceRecorder final : public FOutputDevice
{
public:
	FString PawnName;
	FString State;
	FVector Force = FVector::ZeroVector;
	int32 EmptyWarnings = 0;
	bool bHasForce = false;
	FTraceRecorder() { GLog->AddOutputDevice(this); }
	virtual ~FTraceRecorder() { GLog->RemoveOutputDevice(this); }
	// Receive synchronously on the producer thread, not the dedicated log thread.
	// Serialize discards every non-game-thread call before accessing mutable state.
	virtual bool CanBeUsedOnAnyThread() const override { return true; }
	virtual bool CanBeUsedOnMultipleThreads() const override { return true; }
	virtual void Serialize(const TCHAR* Text, ELogVerbosity::Type, const FName& Category) override
	{
		if (!IsInGameThread()) { return; }
		const FString Line(Text);
		if (Category == FName(TEXT("LogFlyingCabFlight")) && Line.Contains(TEXT("Fuel exhausted")))
		{
			++EmptyWarnings;
		}
		if (Category != FName(TEXT("LogFlyingCabInputTrace")) || PawnName.IsEmpty()
			|| !Line.StartsWith(TEXT("STATE reason=change "))
			|| !Line.Contains(TEXT("pawn=") + PawnName + TEXT(" "))) { return; }
		State = Line;
		FString Before, After, ForceText;
		if (Line.Split(TEXT(" force="), &Before, &After) && After.Split(TEXT(" fuel="), &ForceText, &After))
		{
			Force = FVector::ZeroVector;
			FParse::Value(*ForceText, TEXT("X="), Force.X);
			FParse::Value(*ForceText, TEXT("Y="), Force.Y);
			FParse::Value(*ForceText, TEXT("Z="), Force.Z);
			bHasForce = ForceText.StartsWith(TEXT("V("));
		}
	}
};

struct FFixture
{
	TWeakObjectPtr<AFlyingCabPlayerController> PC;
	TWeakObjectPtr<AFlyingCabPawn> Cab;
	TWeakObjectPtr<UFlyingCabTouchControls> Hud;
	FTraceRecorder Trace;
	bool Initialize()
	{
		UWorld* World = AutomationCommon::GetAnyGameWorld();
		PC = World ? Cast<AFlyingCabPlayerController>(World->GetFirstPlayerController()) : nullptr;
		Cab = PC.IsValid() ? Cast<AFlyingCabPawn>(PC->GetPawn()) : nullptr;
		if (!Cab.IsValid()) { return false; }
		for (TObjectIterator<UFlyingCabTouchControls> It; It; ++It)
		{
			if (It->GetWorld() == World && It->GetOwningPlayer() == PC.Get()) { Hud = *It; break; }
		}
		Trace.PawnName = Cab->GetName();
		return Hud.IsValid() && Body() && Vitals() && Input();
	}
	UPrimitiveComponent* Body() const { return Cab.IsValid() ? Cast<UPrimitiveComponent>(Cab->GetRootComponent()) : nullptr; }
	UFlyingCabVehicleVitalsComponent* Vitals() const { return Cab.IsValid() ? Cab->FindComponentByClass<UFlyingCabVehicleVitalsComponent>() : nullptr; }
	UEnhancedPlayerInput* Input() const { return PC.IsValid() ? Cast<UEnhancedPlayerInput>(PC->PlayerInput) : nullptr; }
	void Isolate(bool bRefill)
	{
		// No incidental city collision, no falling death, but pawn/input/physics ticks
		// remain real. Never disable collision entirely: that would disable physics.
		Body()->SetEnableGravity(false);
		Body()->SetCollisionResponseToAllChannels(ECR_Ignore);
		Cab->SetActorLocation(FixtureLocation, false, nullptr, ETeleportType::TeleportPhysics);
		if (Body()->IsSimulatingPhysics())
		{
			Body()->SetPhysicsLinearVelocity(FVector::ZeroVector);
			Body()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		}
		if (bRefill) { Cab->AddFuel(100.0f); }
		Cab->SetRefuelAvailable(true, 2); // Request is observable; no station overlaps here.
		if (AFlyingCabCharacter* Character = Cast<AFlyingCabCharacter>(PC->GetPawn()))
		{
			Character->GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
			Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
			Character->GetCharacterMovement()->StopMovementImmediately();
			Character->SetActorLocation(FixtureLocation + FVector(200, 0, 0), false, nullptr, ETeleportType::TeleportPhysics);
		}
	}
	bool Touch(int32 Index, bool bPressed)
	{
		UButton* Button = Hud.IsValid() && Hud->WidgetTree ? Cast<UButton>(Hud->WidgetTree->FindWidget(Buttons[Index])) : nullptr;
		if (!Button) { return false; }
		if (bPressed) { Button->OnPressed.Broadcast(); }
		else { Button->OnReleased.Broadcast(); }
		return true;
	}
	float Acceleration(const TCHAR* Name) const
	{
		const FFloatProperty* Property = FindFProperty<FFloatProperty>(Cab->GetClass(), Name);
		return Property ? Property->GetPropertyValue_InContainer(Cab.Get()) : 0.0f;
	}
};

bool CheckTraceEnabled(FAutomationTestBase* Test)
{
	const IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("flyingcab.InputTrace"));
	return Test->TestTrue(TEXT("Existing read-only input trace must be enabled for force observation"), Var && Var->GetInt() == 1);
}

class FPassiveDiagnosticsCommand final : public IAutomationLatentCommand
{
public:
	explicit FPassiveDiagnosticsCommand(FAutomationTestBase* InTest) : Test(InTest) {}
	virtual ~FPassiveDiagnosticsCommand()
	{
		if (DiagnosticsCVar) { DiagnosticsCVar->Set(SavedDiagnosticsValue, ECVF_SetByCode); }
	}
	virtual bool Update() override
	{
		const double Now = FPlatformTime::Seconds();
		if (Now - Started > 20.0) { Test->AddError(TEXT("Passive diagnostics test timeout")); return true; }
		if (Phase == 0)
		{
			if (!Fixture.Initialize()) { return false; }
			Diagnostics = Fixture.PC->GetWorld()->GetSubsystem<UFlyingCabInputDiagnosticsSubsystem>();
			if (!Test->TestNotNull(TEXT("Development observer exists in PIE"), Diagnostics.Get())) { return true; }
			if (!CheckTraceEnabled(Test)) { return true; }
			DiagnosticsCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("flyingcab.InputDiagnostics"));
			SavedDiagnosticsValue = DiagnosticsCVar ? DiagnosticsCVar->GetInt() : 0;
			if (!Test->TestTrue(TEXT("Passive fixture requires diagnostics enabled"), SavedDiagnosticsValue == 1)) { return true; }
			Fixture.PC->StartRunMode(EFlyingCabRunMode::Freeroam);
			Until = Now + 0.35; Phase = 1;
		}
		if (!Fixture.PC.IsValid() || !Fixture.Cab.IsValid() || !Diagnostics.IsValid())
		{ Test->AddError(TEXT("Lost diagnostic fixture")); return true; }
		Fixture.Isolate(true);
		switch (Phase)
		{
		case 1:
			if (Now < Until) { return false; }
			Send(Fixture.PC.Get(), EKeys::W, IE_Pressed);
			Until = Now + 0.2; Phase = 2;
			return false;
		case 2:
			if (Now < Until) { return false; }
			Test->TestTrue(TEXT("Observer records key delivered to controller"), Diagnostics->GetHistoryText().Contains(TEXT("KEY W Pressed")));
			Test->TestTrue(TEXT("Observer records completed propulsion evidence"), Diagnostics->GetHistoryText().Contains(TEXT("last_force_known=1")));
			Diagnostics->RequestDump();
			Until = Now + 0.2; Phase = 3;
			return false;
		case 3:
		{
			if (Now < Until) { return false; }
			FString Payload;
			if (!FFileHelper::LoadFileToString(Payload, *Diagnostics->GetLastScheduledDumpPath())) { return false; }
			Test->TestTrue(TEXT("Dump includes pre-incident delivered key"), Payload.Contains(TEXT("KEY W Pressed")));
			Test->TestTrue(TEXT("Dump declares history window"), Payload.Contains(TEXT("history_seconds=5")));
			Test->TestTrue(TEXT("Manual dump did not release held thrust"), Fixture.Cab->GetTestKeyboardThrustInput() > 0.5f && Fixture.Trace.Force.Z > 0.0);
			Send(Fixture.PC.Get(), EKeys::W, IE_Released);
			Send(Fixture.PC.Get(), EKeys::J, IE_Pressed);
			Send(Fixture.PC.Get(), EKeys::J, IE_Released);
			Until = Now + 0.3; Phase = 4;
			return false;
		}
		case 4:
			if (Now < Until) { return false; }
			Test->TestTrue(TEXT("Journal is really paused"), Fixture.PC->IsQuestJournalOpen() && Fixture.PC->GetWorld()->IsPaused());
			Test->TestTrue(TEXT("Observer continues sampling paused journal"), Diagnostics->GetHistoryText().Contains(TEXT("journal=1"))
				&& Diagnostics->GetHistoryText().Contains(TEXT("paused=1")));
			Test->TestTrue(TEXT("Focus availability is explicitly recorded"), Diagnostics->GetHistoryText().Contains(TEXT("FOCUS_CHANGED")));
			Fixture.PC->CloseQuestJournal();
			Until = Now + 0.35; Phase = 5;
			return false;
		case 5:
			if (Now < Until) { return false; }
			// Deliberate diagnostic fixture: action without a mapped key. The observer
			// must report it, but must NOT neutralize the canonical pawn's input.
			Fixture.Input()->InjectInputForAction(FlyingCabInputData::GetAssets().Thrust, FInputActionValue(true));
			Until = Now + 0.3; Phase = 6;
			return false;
		case 6:
			if (Now < Until)
			{
				Fixture.Input()->InjectInputForAction(FlyingCabInputData::GetAssets().Thrust, FInputActionValue(true));
				return false;
			}
			Test->TestTrue(TEXT("Passive mismatch detector fired"), Diagnostics->GetHistoryText().Contains(TEXT("ACTION_WITHOUT_MAPPED_KEY action=Thrust")));
			Test->TestTrue(TEXT("Diagnostic report does not correct input or force"), Fixture.Cab->GetTestKeyboardThrustInput() > 0.5f && Fixture.Trace.Force.Z > 0.0);
			Until = Now + 0.2; Phase = 7;
			return false;
		case 7:
			if (Now < Until) { return false; }
			Test->TestTrue(TEXT("Input clears normally when injection stops"), Fixture.Cab->GetTestKeyboardThrustInput() < 0.1f);
			LastDumpBeforeRecovery = Diagnostics->GetLastScheduledDumpPath();
			Fixture.Body()->OnComponentHit.Broadcast(Fixture.Body(), nullptr, nullptr,
				FVector(0, 0, Fixture.Body()->GetMass() * 1000000.0f), FHitResult());
			Test->TestTrue(TEXT("Real destruction callback invoked"), Fixture.Cab->IsDestroyed());
			Phase = 8;
			return false;
		case 8:
			if (Fixture.Cab->IsDestroyed()) { return false; }
			Until = Now + 1.2; Phase = 9;
			return false;
		case 9:
		{
			if (Now < Until) { return false; }
			FString Payload;
			if (!FFileHelper::LoadFileToString(Payload, *Diagnostics->GetLastScheduledDumpPath())) { return false; }
			Test->TestTrue(TEXT("Recovery automatically created a new dump"), Diagnostics->GetLastScheduledDumpPath() != LastDumpBeforeRecovery);
			Test->TestTrue(TEXT("Recovery reason retained"), Payload.Contains(TEXT("recovery_complete")));
			Test->AddInfo(TEXT("Diagnostic evidence: ") + Diagnostics->GetLastScheduledDumpPath());
			Send(Fixture.PC.Get(), EKeys::W, IE_Pressed);
			Until = Now + 0.15; Phase = 10;
			return false;
		}
		case 10:
			if (Now < Until) { return false; }
			DiagnosticsCVar->Set(0, ECVF_SetByCode);
			Until = Now + 0.15; Phase = 11;
			return false;
		case 11:
			if (Now < Until) { return false; }
			Test->TestTrue(TEXT("Disabling diagnostics clears only diagnostic history"), Diagnostics->GetHistoryText().Contains(TEXT("samples=0")));
			Test->TestTrue(TEXT("Disabling diagnostics preserves held thrust"), Fixture.Cab->GetTestKeyboardThrustInput() > 0.5f && Fixture.Trace.Force.Z > 0.0);
			DiagnosticsCVar->Set(1, ECVF_SetByCode);
			Until = Now + 0.15; Phase = 12;
			return false;
		case 12:
			if (Now < Until) { return false; }
			Test->TestTrue(TEXT("Re-enabling diagnostics resumes observation"), Diagnostics->GetHistoryText().Contains(TEXT("SAMPLE")));
			Test->TestTrue(TEXT("Re-enabling diagnostics preserves held thrust"), Fixture.Cab->GetTestKeyboardThrustInput() > 0.5f && Fixture.Trace.Force.Z > 0.0);
			Send(Fixture.PC.Get(), EKeys::W, IE_Released);
			return true;
		}
		return false;
	}
private:
	FAutomationTestBase* Test;
	FFixture Fixture;
	TWeakObjectPtr<UFlyingCabInputDiagnosticsSubsystem> Diagnostics;
	FString LastDumpBeforeRecovery;
	IConsoleVariable* DiagnosticsCVar = nullptr;
	int32 SavedDiagnosticsValue = 1;
	double Started = FPlatformTime::Seconds(), Until = 0.0;
	int32 Phase = 0;
};

class FFuelCutoffCommand final : public IAutomationLatentCommand
{
public:
	explicit FFuelCutoffCommand(FAutomationTestBase* InTest) : Test(InTest) {}
	virtual bool Update() override
	{
		if (FPlatformTime::Seconds() - Started > 25.0) { return Fail(TEXT("Fuel cutoff test timeout")); }
		if (Phase == 0)
		{
			if (!Fixture.Initialize()) { return false; }
			if (!CheckTraceEnabled(Test)) { return true; }
			Fixture.PC->StartRunMode(EFlyingCabRunMode::Freeroam);
			FFlyingCabVehicleVitalsConfig Config;
			Config.StartingFuel = 1.0f;
			Config.DescentRegenerationPerSecond = 0.0f; // Isolate fuel cutoff, not regeneration.
			Fixture.Vitals()->InitializeVitals(Config);
			Fixture.Isolate(false);
			Phase = 1;
			return false;
		}
		if (!Fixture.PC.IsValid() || !Fixture.Cab.IsValid()) { return Fail(TEXT("Lost fuel fixture")); }
		const FVector Velocity = Fixture.Body()->GetPhysicsLinearVelocity();
		Fixture.Isolate(false);
		switch (Phase)
		{
		case 1:
			if (++Frames < 3) { return false; }
			Send(Fixture.PC.Get(), EKeys::W, IE_Pressed);
			Frames = 0; Phase = 2;
			return false;
		case 2:
			if (++Frames % 2 == 0) { Send(Fixture.PC.Get(), EKeys::W, IE_Repeat); }
			if (Fixture.Cab->GetTestKeyboardThrustInput() < 0.5f) { return Fail(TEXT("W input dropped while held before exhaustion")); }
			if (Fixture.Cab->GetFuel() > UE_SMALL_NUMBER)
			{
				bSawPoweredMotion |= Velocity.Z > 0.1;
				if (!Fixture.Trace.bHasForce || Fixture.Trace.Force.Z <= 0.0 || !Fixture.Vitals()->CanUseThrusters())
				{ return Fail(TEXT("Fuel remains but thrust is not applied")); }
				return false;
			}
			// Fuel is consumed after AddForce in the canonical tick. The final paid
			// frame may still apply force; check cutoff starting on the following tick.
			Frames = 0; Phase = 3;
			return false;
		case 3:
			if (++Frames % 2 == 0) { Send(Fixture.PC.Get(), EKeys::W, IE_Repeat); }
			if (!IsEmptyAndUnpowered(true)) { return Fail(TEXT("Held W with empty fuel did not cleanly cut propulsion")); }
			if (Frames < 20) { return false; }
			if (const UTextBlock* Message = Cast<UTextBlock>(Fixture.Hud->WidgetTree->FindWidget(TEXT("EventMessageText"))))
			{
				const FString Text = Message->GetText().ToString();
				// HUD messages are queued. Keep checking cutoff while earlier messages
				// finish, rather than requiring this warning to jump the queue.
				if (!Text.Contains(TEXT("ENERGY EMPTY")) || !Text.Contains(TEXT("THRUSTERS OFF"))) { return false; }
			}
			else { return Fail(TEXT("Missing event-message HUD")); }
			Test->TestTrue(TEXT("Fueled input produced real upward motion"), bSawPoweredMotion);
			Test->TestEqual(TEXT("Empty-fuel warning is emitted only once while held"), Fixture.Trace.EmptyWarnings, 1);
			Test->TestTrue(TEXT("Trace distinguishes NoFuel from missing input"), Fixture.Trace.State.Contains(TEXT("gate=NoFuel")));
			Test->AddInfo(TEXT("HUD displayed ENERGY EMPTY and THRUSTERS OFF."));
			Send(Fixture.PC.Get(), EKeys::W, IE_Released);
			Frames = 0; Phase = 4;
			return false;
		case 4:
			if (++Frames < 3) { return false; }
			if (!IsEmptyAndUnpowered(false)) { return Fail(TEXT("Release did not clear input at zero fuel")); }
			Send(Fixture.PC.Get(), EKeys::W, IE_Pressed);
			Frames = 0; Phase = 5;
			return false;
		case 5:
			if (++Frames < 3) { return false; }
			if (!IsEmptyAndUnpowered(true)) { return Fail(TEXT("Pressing W bypassed the empty-fuel gate")); }
			Test->TestEqual(TEXT("Re-pressing W does not repeat the empty warning"), Fixture.Trace.EmptyWarnings, 1);
			Fixture.Cab->AddFuel(20.0f);
			Frames = 0; Phase = 6;
			return false;
		case 6:
			if (++Frames < 3) { return false; }
			Test->TestTrue(TEXT("Adding fuel restores thrust without another key press"),
				Fixture.Cab->GetTestKeyboardThrustInput() > 0.5f && Fixture.Vitals()->CanUseThrusters()
				&& Fixture.Trace.Force.Z > 0.0 && Velocity.Z > 0.1);
			Send(Fixture.PC.Get(), EKeys::W, IE_Released);
			Frames = 0; Phase = 7;
			return false;
		default:
			if (++Frames < 3) { return false; }
			Test->TestTrue(TEXT("Release after refueling clears both input and force"),
				FMath::IsNearlyZero(Fixture.Cab->GetTestKeyboardThrustInput()) && Fixture.Trace.Force.IsNearlyZero());
			return true;
		}
	}
private:
	bool IsEmptyAndUnpowered(bool bHeld) const
	{
		return Fixture.Cab->GetFuel() <= UE_SMALL_NUMBER && !Fixture.Vitals()->CanUseThrusters()
			&& Fixture.Trace.bHasForce && Fixture.Trace.Force.IsNearlyZero()
			&& FMath::IsNearlyEqual(Fixture.Cab->GetTestKeyboardThrustInput(), bHeld ? 1.0f : 0.0f)
			&& Fixture.PC->IsInputKeyDown(EKeys::W) == bHeld
			&& Fixture.Input()->GetActionValue(FlyingCabInputData::GetAssets().Thrust).Get<bool>() == bHeld;
	}
	bool Fail(const TCHAR* Message) { Test->AddError(FString(Message) + TEXT(" | ") + Fixture.Trace.State); return true; }
	FAutomationTestBase* Test;
	FFixture Fixture;
	double Started = FPlatformTime::Seconds();
	int32 Phase = 0, Frames = 0;
	bool bSawPoweredMotion = false;
};

// Independent state model: Physical survives a flush, Delivered does not. Only a
// later press/repeat re-arms a held key. No expected value is copied from UE input.
class FInputSoakCommand final : public IAutomationLatentCommand
{
public:
	FInputSoakCommand(FAutomationTestBase* InTest, int32 InSeed)
		: Test(InTest), Seed(InSeed), Random(InSeed) {}
	virtual ~FInputSoakCommand()
	{
		if (bInitialized)
		{
			FApp::SetFixedDeltaTime(PreviousFixedDelta);
			FApp::SetUseFixedTimeStep(bPreviousFixedStep);
		}
	}
	virtual bool Update() override
	{
		if (FPlatformTime::Seconds() - Started > 180.0) { return Finish(TEXT("Soak timeout")); }
		if (!bInitialized)
		{
			if (!Fixture.Initialize()) { return false; }
			if (!CheckTraceEnabled(Test)) { return true; }
			bPreviousFixedStep = FApp::UseFixedTimeStep();
			PreviousFixedDelta = FApp::GetFixedDeltaTime();
			FApp::SetFixedDeltaTime(1.0 / 60.0);
			FApp::SetUseFixedTimeStep(true);
			Fixture.PC->StartRunMode(EFlyingCabRunMode::Freeroam);
			Fixture.Isolate(true);
			bInitialized = true;
			Grace = 3;
			Lines.Add(TEXT("step,frame,event,physical,delivered,touch,mode,expected_h,expected_t,actual_h,actual_t,force_x,force_z,result,last_cab_snapshot"));
			return false;
		}
		if (!Fixture.PC.IsValid() || !Fixture.Cab.IsValid()) { return Finish(TEXT("Lost soak fixture")); }
		if (LastFrame == GFrameCounter) { return false; }
		LastFrame = GFrameCounter;
		Fixture.Isolate(true);
		// Warm-up (audit 2026-09-10, A-10): one exit/re-entry cycle before step 1 loads the on-foot
		// assets, so the first real transition is not measured against a cold cache with the
		// two-frame grace. The seeded sequence and the model start unchanged after it.
		if (WarmupStage < 4)
		{
			if (PendingRelease.IsValid()) { Send(Fixture.PC.Get(), PendingRelease, IE_Released); PendingRelease = FKey(); return false; }
			const bool bCharacter = Cast<AFlyingCabCharacter>(Fixture.PC->GetPawn()) != nullptr;
			if (WarmupStage == 0) { Pulse(EKeys::Q); WarmupStage = 1; WarmupFrames = 0; return false; }
			if (WarmupStage == 1)
			{
				if (bCharacter) { WarmupStage = 2; }
				else if (++WarmupFrames > 120) { return Finish(TEXT("Warm-up: exit to on-foot did not happen")); }
				return false;
			}
			if (WarmupStage == 2) { Pulse(EKeys::Q); WarmupStage = 3; WarmupFrames = 0; return false; }
			if (!bCharacter && Fixture.PC->GetPawn() == Fixture.Cab.Get())
			{
				WarmupStage = 4; ResetModelAfterFlush(); Grace = 3; Event = TEXT("Start");
			}
			else if (++WarmupFrames > 120) { return Finish(TEXT("Warm-up: return to the cab did not happen")); }
			return false;
		}
		if (bRecovering && !Fixture.Cab->IsDestroyed())
		{
			bRecovering = false;
			ResetModelAfterFlush();
			Event += TEXT(";Recovered");
		}
		const FString Error = Validate();
		if (!Error.IsEmpty()) { return Finish(*Error); }
		if (Grace > 0) { --Grace; }
		if (++Step >= 3000)
		{
			if (!bDraining)
			{
				bDraining = true;
				if (bJournal) { CloseJournal(); }
				if (bObserver) { Pulse(EKeys::O); bObserver = false; ResetModelAfterFlush(); }
				for (int32 I = 0; I < 8; ++I) { ChangeKey(I, false); }
				for (int32 I = 0; I < 4; ++I) { Fixture.Touch(I, false); Touch[I] = false; }
			}
			if (++DrainFrames < 20 || bRecovering) { return false; }
			for (const TCHAR* Kind : {TEXT("Key"), TEXT("Repeat"), TEXT("Touch"), TEXT("Journal"),
				TEXT("Observer"), TEXT("Possess"), TEXT("Reset"), TEXT("Focus"), TEXT("Crash")})
			{
				if (Counts.FindRef(Kind) == 0) { return Finish(*FString::Printf(TEXT("Missing coverage: %s"), Kind)); }
			}
			return Finish(nullptr);
		}
		Event.Reset();
		if (Grace > 0) { RepeatHeldKeys(); return false; }
		if ((bJournal || bObserver) && --ModeFrames <= 0)
		{
			if (bJournal) { CloseJournal(); }
			else { Pulse(EKeys::O); bObserver = false; ResetModelAfterFlush(); }
			return false;
		}
		// Periodic destruction adds three real recovery timers to each random run.
		if (Step >= NextCrash && !bJournal && !bObserver && !bOnFoot && !bRecovering)
		{
			NextCrash += 800;
			Fixture.Body()->OnComponentHit.Broadcast(Fixture.Body(), nullptr, nullptr,
				FVector(0, 0, Fixture.Body()->GetMass() * 1000000.0f), FHitResult());
			if (!Fixture.Cab->IsDestroyed()) { return Finish(TEXT("Crash fixture did not destroy cab")); }
			bRecovering = true;
			ResetModelAfterFlush(); Count(TEXT("Crash"));
			return false;
		}
		const int32 Choice = Random.RandRange(0, 99);
		if (Choice < 40)
		{
			const int32 Index = Random.RandRange(0, 7);
			ChangeKey(Index, !Physical[Index]); Count(TEXT("Key"));
		}
		else if (Choice < 48 && !bJournal && !bObserver && !bRecovering)
		{
			Pulse(EKeys::J); bJournal = true; ModeFrames = Random.RandRange(5, 40);
			ResetModelAfterFlush(); Count(TEXT("Journal")); return false;
		}
		else if (Choice < 52 && !bJournal && !bObserver && !bOnFoot && !bRecovering)
		{
			Pulse(EKeys::R); ResetModelAfterFlush(); Count(TEXT("Reset")); return false;
		}
		else if (Choice < 56 && !bJournal && !bObserver && !bOnFoot && !bRecovering)
		{
			Pulse(EKeys::O); bObserver = true; ModeFrames = Random.RandRange(5, 30);
			ResetModelAfterFlush(); Count(TEXT("Observer")); return false;
		}
		else if (Choice < 59 && !bJournal && !bObserver && !bRecovering)
		{
			Pulse(EKeys::Q); bOnFoot = !bOnFoot;
			ResetModelAfterFlush(); Count(TEXT("Possess")); return false;
		}
		else if (Choice < 62 && !bJournal)
		{
			Fixture.PC->FlushPressedKeys(); ResetModelAfterFlush(); Count(TEXT("Focus")); return false;
		}
		else if (Choice < 70 && !bJournal && !bObserver && !bRecovering)
		{
			const int32 Index = Random.RandRange(0, bOnFoot ? 2 : 3);
			Touch[Index] = !Touch[Index];
			if (!Fixture.Touch(Index, Touch[Index])) { return Finish(TEXT("Missing touch button")); }
			Event += FString::Printf(TEXT("Touch%d=%d;"), Index, Touch[Index]); Count(TEXT("Touch"));
		}
		RepeatHeldKeys();
		return false;
	}
private:
	void Count(const TCHAR* Kind) { ++Counts.FindOrAdd(Kind); Event += FString(Kind) + TEXT(";"); }
	void ResetModelAfterFlush()
	{
		for (bool& Value : Delivered) { Value = false; }
		for (bool& Value : Touch) { Value = false; }
		Grace = 2;
	}
	void Pulse(FKey Key)
	{
		Send(Fixture.PC.Get(), Key, IE_Pressed);
		// Release on the following Update, not in the same input evaluation.
		PendingRelease = Key;
		Event += Key.ToString() + TEXT("Pressed;");
	}
	void ChangeKey(int32 Index, bool bPressed)
	{
		Physical[Index] = bPressed;
		HeldSince[Index] = Step;
		Event += FString::Printf(TEXT("%s=%d;"), *Keys[Index].ToString(), bPressed);
		// UIOnly consumes key-up/repeat before PlayerController; model that routing.
		if (!bJournal)
		{
			// Opt-in negative control: simulate one lost release at the delivery
			// boundary. The independent model still expects release and MUST fail.
			if (!bPressed && Delivered[Index] && !bDroppedRelease
				&& FParse::Param(FCommandLine::Get(), TEXT("FlyingCabSoakDropRelease")))
			{
				bDroppedRelease = true; Delivered[Index] = false;
				Event += TEXT("NEGATIVE_DroppedRelease;"); return;
			}
			Send(Fixture.PC.Get(), Keys[Index], bPressed ? IE_Pressed : IE_Released);
			Delivered[Index] = bPressed;
		}
	}
	void RepeatHeldKeys()
	{
		if (bJournal) { return; }
		for (int32 I = 0; I < 8; ++I)
		{
			if (Physical[I] && Step - HeldSince[I] >= 15 && Step % 2 == 0)
			{
				Send(Fixture.PC.Get(), Keys[I], IE_Repeat); Delivered[I] = true;
				Event += Keys[I].ToString() + TEXT("Repeat;"); ++Counts.FindOrAdd(TEXT("Repeat"));
			}
		}
	}
	void CloseJournal()
	{
		UFlyingCabQuestJournalWidget* Journal = Fixture.PC->GetQuestJournalWidget();
		if (Journal) { Journal->HandleNavigationKey(EKeys::J); }
		bJournal = false; ResetModelAfterFlush(); Event += TEXT("CloseJournal;");
	}
	FString Validate()
	{
		if (PendingRelease.IsValid()) { Send(Fixture.PC.Get(), PendingRelease, IE_Released); PendingRelease = FKey(); }
		const float ExpectedActionH = -float(Delivered[0]) - float(Delivered[1]) + float(Delivered[2]) + float(Delivered[3]);
		const float ExpectedT = Delivered[4] || Delivered[5] || Delivered[6] ? 1.0f : 0.0f;
		const bool bBlocked = bJournal || bObserver;
		const float ExpectedH = bBlocked ? 0.0f : FMath::Clamp(ExpectedActionH, -1.0f, 1.0f);
		const float ExpectedPawnT = bBlocked ? 0.0f : ExpectedT;
		const float EffectiveH = FMath::Clamp(ExpectedH + float(Touch[1]) - float(Touch[0]), -1.0f, 1.0f);
		const AFlyingCabCharacter* Character = Cast<AFlyingCabCharacter>(Fixture.PC->GetPawn());
		const float ActualH = Character ? Character->GetTestKeyboardHorizontalInput() : Fixture.Cab->GetTestKeyboardHorizontalInput();
		const float ActualT = Character ? Character->GetTestKeyboardThrustInput() : Fixture.Cab->GetTestKeyboardThrustInput();
		FString Error, PhysicalText, DeliveredText, TouchText;
		for (bool Value : Touch) { TouchText += Value ? TEXT("1") : TEXT("0"); }
		for (int32 I = 0; I < 8; ++I)
		{
			PhysicalText += Physical[I] ? TEXT("1") : TEXT("0");
			DeliveredText += Delivered[I] ? TEXT("1") : TEXT("0");
			if (Grace == 0 && Fixture.PC->IsInputKeyDown(Keys[I]) != Delivered[I]) { Error = TEXT("Raw key/model mismatch: ") + Keys[I].ToString(); }
		}
		if (Grace == 0)
		{
			const FFlyingCabInputAssets& Assets = FlyingCabInputData::GetAssets();
			if (Fixture.PC->IsQuestJournalOpen() != bJournal || Fixture.PC->IsDeveloperObserverMode() != bObserver
				|| (Character != nullptr) != bOnFoot) { Error = TEXT("Expected mode/possession transition did not happen"); }
			if (!FMath::IsNearlyEqual(Fixture.Input()->GetActionValue(Assets.Horizontal).Get<float>(), ExpectedActionH)
				|| Fixture.Input()->GetActionValue(Assets.Thrust).Get<bool>() != (ExpectedT > 0.5f)
				|| Fixture.Input()->GetActionValue(Assets.Service).Get<bool>() != Delivered[7]) { Error = TEXT("Enhanced action/model mismatch"); }
			if (!FMath::IsNearlyEqual(ActualH, ExpectedH) || !FMath::IsNearlyEqual(ActualT, ExpectedPawnT)) { Error = TEXT("Pawn keyboard/model mismatch"); }
			const float EffectiveT = FMath::Max(ExpectedPawnT, Touch[2] ? 1.0f : 0.0f);
			if (!bOnFoot && !bJournal)
			{
				const FVector ExpectedForce = bObserver || bRecovering ? FVector::ZeroVector : FVector(
					EffectiveH * Fixture.Acceleration(TEXT("HorizontalThrustAcceleration")) * Fixture.Body()->GetMass(), 0,
					EffectiveT * Fixture.Acceleration(TEXT("VerticalThrustAcceleration")) * Fixture.Body()->GetMass());
				if (!Fixture.Trace.bHasForce || !Fixture.Trace.Force.Equals(ExpectedForce, 1.0)) { Error = TEXT("Applied force/model mismatch (including touch)"); }
				if (!bRecovering && Fixture.Cab->IsRefuelRequested() != (!bBlocked && (Delivered[7] || Touch[3]))) { Error = TEXT("Service/model mismatch"); }
			}
			if (Character && !bBlocked)
			{
				const float Movement = Character->GetLastMovementInputVector().X;
				if (!FMath::IsNearlyEqual(Movement, EffectiveH) && !FMath::IsNearlyEqual(Movement, PreviousEffectiveH))
				{ Error = FString::Printf(TEXT("On-foot movement %.2f differs from expected %.2f and previous %.2f"), Movement, EffectiveH, PreviousEffectiveH); }
			}
		}
		PreviousEffectiveH = EffectiveH; // Includes neutral frames during transitions.
		FString Snapshot = Fixture.Trace.State.Replace(TEXT("\""), TEXT("\"\""));
		Lines.Add(FString::Printf(TEXT("%d,%llu,%s,%s,%s,%s,%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%s,\"%s\""),
			Step, GFrameCounter, *Event, *PhysicalText, *DeliveredText, *TouchText,
			bJournal ? TEXT("Journal") : bObserver ? TEXT("Observer") : bOnFoot ? TEXT("OnFoot") : bRecovering ? TEXT("Recovery") : TEXT("Cab"),
			ExpectedH, ExpectedPawnT, ActualH, ActualT, Fixture.Trace.Force.X, Fixture.Trace.Force.Z,
			Grace > 0 ? TEXT("TransitionGrace") : Error.IsEmpty() ? TEXT("OK") : *Error, *Snapshot));
		return Error;
	}
	bool Finish(const TCHAR* Error)
	{
		const FString Directory = FPaths::ProjectSavedDir() / TEXT("Automation");
		IFileManager::Get().MakeDirectory(*Directory, true);
		const FString Path = Directory / FString::Printf(TEXT("InputSoak_%d%s.csv"), Seed,
			FParse::Param(FCommandLine::Get(), TEXT("FlyingCabSoakDropRelease")) ? TEXT("_negative") : TEXT(""));
		if (!FFileHelper::SaveStringArrayToFile(Lines, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
		{ Test->AddError(TEXT("Could not save input soak replay trace")); }
		if (Error)
		{
			Test->AddError(FString::Printf(TEXT("InputSoak seed=%d step=%d: %s | %s"), Seed, Step, Error, *Fixture.Trace.State));
			for (int32 I = FMath::Max(1, Lines.Num() - 50); I < Lines.Num(); ++I) { Test->AddInfo(Lines[I]); }
		}
		else
		{
			Test->AddInfo(FString::Printf(TEXT("InputSoak seed=%d completed %d steps; trace=%s"), Seed, Step, *Path));
			for (const auto& Pair : Counts) { Test->AddInfo(FString::Printf(TEXT("Coverage %s=%d"), *Pair.Key, Pair.Value)); }
		}
		return true;
	}
	FAutomationTestBase* Test;
	int32 Seed;
	FRandomStream Random;
	FFixture Fixture;
	TArray<FString> Lines;
	TMap<FString, int32> Counts;
	FString Event = TEXT("Start");
	FKey PendingRelease;
	double Started = FPlatformTime::Seconds();
	double PreviousFixedDelta = 0.0;
	uint64 LastFrame = MAX_uint64;
	int32 Step = 0, Grace = 0, ModeFrames = 0, NextCrash = 600, DrainFrames = 0, WarmupStage = 0, WarmupFrames = 0;
	int32 HeldSince[8] = {};
	bool Physical[8] = {}, Delivered[8] = {}, Touch[4] = {};
	bool bInitialized = false, bJournal = false, bObserver = false, bOnFoot = false, bRecovering = false, bDraining = false;
	bool bPreviousFixedStep = false;
	bool bDroppedRelease = false;
	float PreviousEffectiveH = 0.0f;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyingCabPassiveDiagnosticsTest,
	"FlyingCab.Functional.PIE.PassiveInputDiagnostics", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FFlyingCabPassiveDiagnosticsTest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(FlyingCabInputTests::Map, true)) { AddError(TEXT("Cannot open FlightLab")); return false; }
	ADD_LATENT_AUTOMATION_COMMAND(FlyingCabInputTests::FPassiveDiagnosticsCommand(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyingCabFuelCutoffWhileHoldingThrustTest,
	"FlyingCab.Functional.PIE.FuelCutoffWhileHoldingThrust", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FFlyingCabFuelCutoffWhileHoldingThrustTest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(FlyingCabInputTests::Map, true)) { AddError(TEXT("Cannot open FlightLab")); return false; }
	ADD_LATENT_AUTOMATION_COMMAND(FlyingCabInputTests::FFuelCutoffCommand(this));
	return true;
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FFlyingCabInputSoakTest,
	"FlyingCab.Functional.PIE.InputSoak", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
void FFlyingCabInputSoakTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	int32 SecondSeed = 9042026;
	FParse::Value(FCommandLine::Get(), TEXT("FlyingCabSoakSeed="), SecondSeed);
	for (int32 Seed : {1977, SecondSeed})
	{
		Names.Add(FString::Printf(TEXT("Seed%d"), Seed));
		Commands.Add(FString::FromInt(Seed));
	}
}
bool FFlyingCabInputSoakTest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(FlyingCabInputTests::Map, true)) { AddError(TEXT("Cannot open FlightLab")); return false; }
	ADD_LATENT_AUTOMATION_COMMAND(FlyingCabInputTests::FInputSoakCommand(this, FCString::Atoi(*Parameters)));
	return true;
}
#endif
