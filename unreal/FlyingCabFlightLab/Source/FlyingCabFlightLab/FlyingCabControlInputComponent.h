// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "FlyingCabControlInputComponent.generated.h"

class AFlyingCabPlayerController;
class AFlyingCabPawn;
class UEnhancedInputComponent;
struct FInputKeyEventArgs;

enum class EFlyingCabInputSource : uint8
{
	None = 0,
	Keyboard = 1 << 0,
	Gamepad = 1 << 1,
	Touch = 1 << 2
};
ENUM_CLASS_FLAGS(EFlyingCabInputSource);

enum class EFlyingCabInputBlock : uint8
{
	None,
	Menu,
	QuestJournal,
	Observer,
	Transition
};

/** Immutable command snapshot produced once per player-controller tick. */
struct FLYINGCABFLIGHTLAB_API FFlyingCabControlFrame
{
	float Horizontal = 0.0f;
	float Thrust = 0.0f;
	bool bService = false;
	EFlyingCabInputSource Sources = EFlyingCabInputSource::None;
	EFlyingCabInputBlock Block = EFlyingCabInputBlock::None;
	uint64 Sequence = 0;
};

/**
 * Owns continuous Enhanced Input state independently of the currently possessed pawn.
 * The first migration stage feeds the cab; on-foot and touch consumers remain on their
 * existing paths until their dedicated follow-up stages.
 */
UCLASS(ClassGroup = (FlyingCab))
class FLYINGCABFLIGHTLAB_API UFlyingCabControlInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFlyingCabControlInputComponent();

	static bool IsControlFrameEnabled();

	void BuildControlFrame(
		AFlyingCabPlayerController* PlayerController,
		const UEnhancedInputComponent* EnhancedInput,
		EFlyingCabInputBlock Block);
	void ResetControlFrame(EFlyingCabInputBlock Block = EFlyingCabInputBlock::Transition);
	void InvalidateMappedKeys();
	void TraceKeyEvent(const FInputKeyEventArgs& Params);
	void TraceVehicleState(const AFlyingCabPawn* Vehicle, const FVector2D& Keyboard,
		const FVector2D& Touch, const FVector& AppliedForce, const TCHAR* Reason = nullptr);

	const FFlyingCabControlFrame& GetControlFrame() const { return ControlFrame; }

private:
#if WITH_DEV_AUTOMATION_TESTS
	friend class FFlyingCabControlFrameValidationTest;
#endif

	void CacheMappedKeys(AFlyingCabPlayerController* PlayerController);
	bool HasActiveKey(
		const AFlyingCabPlayerController* PlayerController,
		const TArray<FKey>& Keys) const;
	void AddActiveSources(
		const AFlyingCabPlayerController* PlayerController,
		const TArray<FKey>& Keys,
		EFlyingCabInputSource& Sources) const;
	float ValidateActionValue(
		const TCHAR* ActionName,
		float Value,
		const TArray<FKey>& Keys,
		const AFlyingCabPlayerController* PlayerController,
		uint8& StaleFrames,
		bool& bStaleReported);

	FFlyingCabControlFrame ControlFrame;
	TArray<FKey> HorizontalKeys;
	TArray<FKey> ThrustKeys;
	TArray<FKey> ServiceKeys;
	uint64 NextSequence = 0;
	uint8 HorizontalStaleFrames = 0;
	uint8 ThrustStaleFrames = 0;
	uint8 ServiceStaleFrames = 0;
	bool bHorizontalStaleReported = false;
	bool bThrustStaleReported = false;
	bool bServiceStaleReported = false;
	bool bMappedKeysCached = false;

	struct FKeyTraceHistory
	{
		EInputEvent LastEvent = IE_MAX;
		double LastEventTime = 0.0;
	};
	TMap<FKey, FKeyTraceHistory> KeyTraceHistory;
	FString LastTraceSignature;
};
