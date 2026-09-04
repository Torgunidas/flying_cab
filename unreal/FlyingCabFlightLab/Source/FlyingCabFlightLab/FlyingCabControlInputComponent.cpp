// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabControlInputComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "Components/PrimitiveComponent.h"
#include "FlyingCabInputData.h"
#include "FlyingCabPawn.h"
#include "FlyingCabPlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputKeyEventArgs.h"
#include "InputMappingContext.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabControlInput, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabInputTrace, Log, All);

namespace
{
	TAutoConsoleVariable<int32> CVarFlyingCabUseControlFrame(
		TEXT("flyingcab.UseControlFrame"),
		0,
		TEXT("Use the controller-owned control frame for continuous cab input. 0 restores the legacy pawn path."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarFlyingCabInputTrace(
		TEXT("flyingcab.InputTrace"), 1,
		TEXT("Development-only gameplay key edges and changed input/propulsion snapshots."),
		ECVF_Default);

	const TCHAR* KeyEventName(EInputEvent Event)
	{
		switch (Event)
		{
		case IE_Pressed: return TEXT("Pressed");
		case IE_Released: return TEXT("Released");
		case IE_Repeat: return TEXT("Repeat");
		default: return TEXT("Other");
		}
	}
}

UFlyingCabControlInputComponent::UFlyingCabControlInputComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UFlyingCabControlInputComponent::IsControlFrameEnabled()
{
	return CVarFlyingCabUseControlFrame.GetValueOnGameThread() != 0;
}

void UFlyingCabControlInputComponent::BuildControlFrame(
	AFlyingCabPlayerController* PlayerController,
	const UEnhancedInputComponent* EnhancedInput,
	EFlyingCabInputBlock Block)
{
	ControlFrame = FFlyingCabControlFrame();
	ControlFrame.Sequence = ++NextSequence;
	ControlFrame.Block = Block;
	if (Block != EFlyingCabInputBlock::None || !PlayerController || !EnhancedInput)
	{
		HorizontalStaleFrames = 0;
		ThrustStaleFrames = 0;
		ServiceStaleFrames = 0;
		return;
	}

	const FFlyingCabInputAssets& InputAssets = FlyingCabInputData::GetAssets();
	if (!InputAssets.IsValid())
	{
		return;
	}
	if (!bMappedKeysCached)
	{
		CacheMappedKeys(PlayerController);
	}

	ControlFrame.Horizontal = FMath::Clamp(
		ValidateActionValue(
			TEXT("Horizontal"),
			EnhancedInput->GetBoundActionValue(InputAssets.Horizontal).Get<float>(),
			HorizontalKeys,
			PlayerController,
			HorizontalStaleFrames,
			bHorizontalStaleReported),
		-1.0f,
		1.0f);
	ControlFrame.Thrust = FMath::Clamp(
		ValidateActionValue(
			TEXT("Thrust"),
			EnhancedInput->GetBoundActionValue(InputAssets.Thrust).Get<bool>() ? 1.0f : 0.0f,
			ThrustKeys,
			PlayerController,
			ThrustStaleFrames,
			bThrustStaleReported),
		0.0f,
		1.0f);
	ControlFrame.bService = ValidateActionValue(
		TEXT("Service"),
		EnhancedInput->GetBoundActionValue(InputAssets.Service).Get<bool>() ? 1.0f : 0.0f,
		ServiceKeys,
		PlayerController,
		ServiceStaleFrames,
		bServiceStaleReported) > 0.5f;

	if (!FMath::IsNearlyZero(ControlFrame.Horizontal))
	{
		AddActiveSources(PlayerController, HorizontalKeys, ControlFrame.Sources);
	}
	if (ControlFrame.Thrust > UE_SMALL_NUMBER)
	{
		AddActiveSources(PlayerController, ThrustKeys, ControlFrame.Sources);
	}
	if (ControlFrame.bService)
	{
		AddActiveSources(PlayerController, ServiceKeys, ControlFrame.Sources);
	}
}

void UFlyingCabControlInputComponent::ResetControlFrame(EFlyingCabInputBlock Block)
{
	ControlFrame = FFlyingCabControlFrame();
	ControlFrame.Sequence = ++NextSequence;
	ControlFrame.Block = Block;
	HorizontalStaleFrames = 0;
	ThrustStaleFrames = 0;
	ServiceStaleFrames = 0;
}

void UFlyingCabControlInputComponent::InvalidateMappedKeys()
{
	bMappedKeysCached = false;
	HorizontalKeys.Reset();
	ThrustKeys.Reset();
	ServiceKeys.Reset();
}

void UFlyingCabControlInputComponent::CacheMappedKeys(
	AFlyingCabPlayerController* PlayerController)
{
	InvalidateMappedKeys();
	if (!PlayerController || !PlayerController->GetLocalPlayer())
	{
		return;
	}

	const UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		PlayerController->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	const FFlyingCabInputAssets& InputAssets = FlyingCabInputData::GetAssets();
	if (!InputSubsystem || !InputAssets.IsValid())
	{
		return;
	}

	HorizontalKeys = InputSubsystem->QueryKeysMappedToAction(InputAssets.Horizontal);
	ThrustKeys = InputSubsystem->QueryKeysMappedToAction(InputAssets.Thrust);
	ServiceKeys = InputSubsystem->QueryKeysMappedToAction(InputAssets.Service);
	// AddMappingContext rebuilds asynchronously. Retry on following frames instead
	// of permanently caching the transient empty result from the startup frame.
	bMappedKeysCached = !HorizontalKeys.IsEmpty()
		&& !ThrustKeys.IsEmpty()
		&& !ServiceKeys.IsEmpty();
	if (!bMappedKeysCached)
	{
		return;
	}
	UE_LOG(
		LogFlyingCabControlInput,
		Display,
		TEXT("Control frame cached %d horizontal, %d thrust and %d service mappings."),
		HorizontalKeys.Num(),
		ThrustKeys.Num(),
		ServiceKeys.Num());
}

bool UFlyingCabControlInputComponent::HasActiveKey(
	const AFlyingCabPlayerController* PlayerController,
	const TArray<FKey>& Keys) const
{
	if (!PlayerController)
	{
		return false;
	}
	for (const FKey& Key : Keys)
	{
		if (PlayerController->IsInputKeyDown(Key))
		{
			return true;
		}
	}
	return false;
}

void UFlyingCabControlInputComponent::AddActiveSources(
	const AFlyingCabPlayerController* PlayerController,
	const TArray<FKey>& Keys,
	EFlyingCabInputSource& Sources) const
{
	if (!PlayerController)
	{
		return;
	}
	for (const FKey& Key : Keys)
	{
		if (!PlayerController->IsInputKeyDown(Key))
		{
			continue;
		}
		Sources |= Key.IsGamepadKey()
			? EFlyingCabInputSource::Gamepad
			: EFlyingCabInputSource::Keyboard;
	}
}

float UFlyingCabControlInputComponent::ValidateActionValue(
	const TCHAR* ActionName,
	float Value,
	const TArray<FKey>& Keys,
	const AFlyingCabPlayerController* PlayerController,
	uint8& StaleFrames,
	bool& bStaleReported)
{
	if (FMath::IsNearlyZero(Value) || HasActiveKey(PlayerController, Keys))
	{
		StaleFrames = 0;
		bStaleReported = false;
		return Value;
	}

	// Two is the terminal state, not a running duration counter. Saturating here
	// prevents long-lived stale values from wrapping back into the grace frame.
	StaleFrames = static_cast<uint8>(FMath::Min(static_cast<int32>(StaleFrames) + 1, 2));
	if (StaleFrames < 2)
	{
		return Value;
	}
	if (!bStaleReported)
	{
		bStaleReported = true;
		UE_LOG(
			LogFlyingCabControlInput,
			Warning,
			TEXT("STALE_ACTION_VALUE action=%s value=%.2f mapped_keys=%d; control frame forced it to zero."),
			ActionName,
			Value,
			Keys.Num());
	}
	return 0.0f;
}

void UFlyingCabControlInputComponent::TraceKeyEvent(const FInputKeyEventArgs& Params)
{
#if !UE_BUILD_SHIPPING
	if (!CVarFlyingCabInputTrace.GetValueOnGameThread()
		|| (Params.Event != IE_Pressed && Params.Event != IE_Released && Params.Event != IE_Repeat))
	{
		return;
	}
	const FFlyingCabInputAssets& Assets = FlyingCabInputData::GetAssets();
	if (!Assets.IsValid() || !Assets.MappingContext->GetMappings().ContainsByPredicate(
		[&](const FEnhancedActionKeyMapping& Mapping) { return Mapping.Key == Params.Key; }))
	{
		return;
	}
	FKeyTraceHistory& History = KeyTraceHistory.FindOrAdd(Params.Key);
	// Store every repeat timestamp, but only log the first repeat in a series.
	// Engine-generated flush releases do not pass this controller boundary; that
	// distinction is intentional, so delivered key-up events remain identifiable.
	if (Params.Event != IE_Repeat || History.LastEvent != IE_Repeat)
	{
		UE_LOG(LogFlyingCabInputTrace, Display, TEXT("KEY %s %s"),
			*Params.Key.ToString(), KeyEventName(Params.Event));
	}
	History.LastEvent = Params.Event;
	History.LastEventTime = FPlatformTime::Seconds();
#endif
}

void UFlyingCabControlInputComponent::TraceVehicleState(
	const AFlyingCabPawn* Vehicle, const FVector2D& Keyboard, const FVector2D& Touch,
	const FVector& AppliedForce, const TCHAR* Reason)
{
#if !UE_BUILD_SHIPPING
	AFlyingCabPlayerController* PC = Cast<AFlyingCabPlayerController>(GetOwner());
	if (!CVarFlyingCabInputTrace.GetValueOnGameThread() || !PC || !Vehicle)
	{
		return;
	}
	if (!bMappedKeysCached) { CacheMappedKeys(PC); }
	TArray<FKey> Keys = HorizontalKeys;
	for (const FKey& Key : ThrustKeys) { Keys.AddUnique(Key); }
	for (const FKey& Key : ServiceKeys) { Keys.AddUnique(Key); }
	FString Raw;
	for (const FKey& Key : Keys)
	{
		Raw += FString::Printf(TEXT("%s:%d "), *Key.ToString(), PC->IsInputKeyDown(Key));
	}
	const FFlyingCabInputAssets& Assets = FlyingCabInputData::GetAssets();
	const UEnhancedPlayerInput* EI = Cast<UEnhancedPlayerInput>(PC->PlayerInput);
	const float ActionH = EI && Assets.IsValid() ? EI->GetActionValue(Assets.Horizontal).Get<float>() : 0.0f;
	const bool ActionT = EI && Assets.IsValid() && EI->GetActionValue(Assets.Thrust).Get<bool>();
	const UPrimitiveComponent* Body = Cast<UPrimitiveComponent>(Vehicle->GetRootComponent());
	const bool bPhysics = Body && Body->IsSimulatingPhysics();
	const TCHAR* Gate = Vehicle->IsDestroyed() ? TEXT("Destroyed")
		: !bPhysics ? TEXT("PhysicsOff")
		: PC->IsGameplayInputSuppressed() ? TEXT("Suppressed")
		: Vehicle->GetFuel() <= UE_SMALL_NUMBER ? TEXT("NoFuel") : TEXT("None");
	const FString Signature = FString::Printf(
		TEXT("pawn=%s raw=[%s] act=(%.2f,%d) frame_on=%d frame=(%.2f,%.2f) keyboard=(%.2f,%.2f) touch=(%.2f,%.2f) block=%d gate=%s phys=%d force=%s"),
		*Vehicle->GetName(), *Raw, ActionH, ActionT, IsControlFrameEnabled(),
		ControlFrame.Horizontal, ControlFrame.Thrust, Keyboard.X, Keyboard.Y,
		Touch.X, Touch.Y, static_cast<int32>(PC->GetControlInputBlock()), Gate, bPhysics,
		*AppliedForce.ToCompactString());
	if (!Reason && LastTraceSignature == Signature) { return; }
	LastTraceSignature = Signature;
	FString LastEvents;
	for (const FKey& Key : Keys)
	{
		if (const FKeyTraceHistory* History = KeyTraceHistory.Find(Key))
		{
			LastEvents += FString::Printf(TEXT("%s:%s/%.2fs "), *Key.ToString(),
				KeyEventName(History->LastEvent), FPlatformTime::Seconds() - History->LastEventTime);
		}
	}
	UE_LOG(LogFlyingCabInputTrace, Display,
		TEXT("STATE reason=%s %s fuel=%.2f hull=%.2f velocity=%s position=%s delivered=[%s]"),
		Reason ? Reason : TEXT("change"), *Signature, Vehicle->GetFuel(),
		Vehicle->GetHullPercent() * 100.0f,
		*(Body ? Body->GetPhysicsLinearVelocity() : FVector::ZeroVector).ToCompactString(),
		*Vehicle->GetActorLocation().ToCompactString(), *LastEvents);
#endif
}
