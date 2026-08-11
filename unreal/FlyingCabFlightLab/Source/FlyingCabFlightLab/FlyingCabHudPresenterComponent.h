// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "FlyingCabHudPresenterComponent.generated.h"

/** Projects gameplay state into the persistent HUD and in-world guidance. */
UCLASS(ClassGroup = "Flying Cab", meta = (BlueprintSpawnableComponent))
class FLYINGCABFLIGHTLAB_API UFlyingCabHudPresenterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFlyingCabHudPresenterComponent();

	void InitializePresenter(
		class UFlyingCabDispatchComponent* InDispatch,
		class UFlyingCabRunComponent* InRun);
	void Refresh(
		float DeltaSeconds,
		class AFlyingCabPawn* Pawn,
		int32 Credits,
		bool bForce = false);
	void PushEconomyStatus(int32 Credits) const;
	void UpdateRunModeStatus(int32 Credits) const;
	void ShowEventMessage(
		const FText& Message,
		const FLinearColor& Color,
		float DurationSeconds) const;
	void SetTrafficAlert(const FText& Alert, const FLinearColor& Color) const;
	void ClearMinimapTarget() const;

private:
	class AFlyingCabPlayerController* GetPlayerController() const;
	bool IsPlayerOnFoot() const;
	void UpdateProximityGuidance(class AFlyingCabPawn* Pawn) const;
	void UpdateObjectiveStatus(class AFlyingCabPawn* Pawn, int32 Credits) const;

	/** Exact in-world pointer appears only inside this range. */
	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Delivery", meta = (ClampMin = "0.0"))
	float ProximityGuidanceRange = 1800.0f;

	/** Text and minimap presentation do not need per-frame Slate invalidation. */
	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Interface", meta = (ClampMin = "0.05"))
	float HudRefreshInterval = 0.1f;

	UPROPERTY(Transient)
	TObjectPtr<class UFlyingCabDispatchComponent> Dispatch;

	UPROPERTY(Transient)
	TObjectPtr<class UFlyingCabRunComponent> Run;

	float HudRefreshElapsed = 0.0f;
};
