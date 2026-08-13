// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlyingCabQuestTypes.h"
#include "FlyingCabRunTypes.h"
#include "GameFramework/PlayerController.h"
#include "FlyingCabPlayerController.generated.h"

class AFlyingCabCharacter;
class AFlyingCabCameraRig;
class AFlyingCabPawn;
class UFlyingCabGameFlowWidget;
class UFlyingCabQuestJournalWidget;
class UFlyingCabTouchControls;

enum class EFlyingCabPlayerMode : uint8
{
	Unknown,
	Vehicle,
	OnFoot
};

/** Owns transitions between the persistent cab and the temporary on-foot pawn. */
UCLASS()
class FLYINGCABFLIGHTLAB_API AFlyingCabPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFlyingCabPlayerController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void FlushPressedKeys() override;
	virtual bool ShouldFlushKeysWhenViewportFocusChanges() const override { return true; }

	UFUNCTION(BlueprintCallable, Category = "Flying Cab|Interaction")
	void RequestContextInteraction();
	FText GetContextPrompt();
	void ShowEventMessage(
		const FText& Message,
		const FLinearColor& Color,
		float DurationSeconds = 2.5f) const;
	void ShowMajorAnnouncement(
		const FText& Title,
		const FText& Detail,
		const FLinearColor& Color,
		float DurationSeconds = 2.8f,
		int32 InPriority = 0) const;
	void CloseQuestJournal();
	void SetQuestStatus(const FText& Status);
	void SetObjectiveStatus(const FText& Status);
	void SetMinimapState(
		const FVector2D& CabWorldPosition,
		const FVector2D& TargetWorldPosition,
		bool bTargetIsDropoff);
	void SetPassengerOfferMarkers(
		const FVector2D& CabWorldPosition,
		const TArray<FVector2D>& OfferWorldPositions);
	void ClearMinimapTarget();
	void SetTimeAttackStatus(
		bool bActive,
		float ElapsedSeconds,
		int32 Credits,
		int32 TargetCredits);
	void SetEconomyStatus(int32 Credits, int32 ActiveFare);
	void SetTrafficAlert(const FText& Alert, const FLinearColor& Color);
	void ReleaseInterfaceInputs();
	void StartRunMode(EFlyingCabRunMode Mode);
	void RestartWithRunMode(EFlyingCabRunMode Mode);
	void ReturnToModeSelection();
	void ShowTimeAttackResults(
		const FFlyingCabTimeAttackResult& Result,
		const TArray<float>& BestTimes);
	bool IsGameFlowScreenOpen() const { return bGameFlowScreenOpen; }
	bool IsQuestJournalOpen() const { return bQuestJournalOpen; }
	bool IsGameplayInputSuppressed() const
	{
		return bGameFlowScreenOpen || bQuestJournalOpen;
	}
	EFlyingCabPlayerMode GetPlayerMode() const { return PlayerMode; }

protected:
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;

private:
	void TryExitVehicle(AFlyingCabPawn* Vehicle);
	void TryEnterVehicle(AFlyingCabCharacter* OnFootPawn, AFlyingCabPawn* Vehicle);
	bool TryInteractWithNearbyActor(AFlyingCabCharacter* OnFootPawn);
	void RefreshInteractionCacheIfNeeded(bool bForce = false);
	AActor* FindNearestInteractable(const AFlyingCabCharacter* OnFootPawn) const;
	AFlyingCabPawn* FindNearestVehicle(const AFlyingCabCharacter* OnFootPawn) const;
	AFlyingCabCharacter* SpawnCharacterBesideVehicle(AFlyingCabPawn* Vehicle);
	void ShowInteractionMessage(const FString& Message, const FColor& Color) const;
	UFlyingCabTouchControls* GetInterfaceWidget() const { return InterfaceWidget; }
	void CreateInterfaceWidget();
	void RefreshInterface();
	void ApplyTouchControlsVisibility();
	void ShowInitialModeSelection();
	void ToggleQuestJournal();
	void OpenQuestJournal();
	void BindQuestPresentation();
	void EnterMenuInputMode();
	void RestoreGameplayInputMode();
	bool EnsureEnhancedInputContext();
	void RemoveEnhancedInputContext();
	static EFlyingCabRunMode ParseRunMode(const FString& Value);
	static FString GetRunModeOption(EFlyingCabRunMode Mode);

	UFUNCTION()
	void HandleQuestUpdated(FFlyingCabQuestUpdate Update);

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|On Foot", meta = (ClampMin = "0.0"))
	float ExitGroundReach = 120.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|On Foot", meta = (ClampMin = "0.0"))
	float ExitSideClearance = 26.0f;

	/** Small push keeps an airborne character clear of the falling cab. */
	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|On Foot", meta = (ClampMin = "0.0"))
	float AirborneExitSeparationSpeed = 140.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|On Foot", meta = (ClampMin = "0.0"))
	float AirborneExitUpwardSpeed = 80.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|On Foot", meta = (ClampMin = "0.0"))
	float VehicleInteractionDistance = 320.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|On Foot", meta = (ClampMin = "0.0"))
	float WorldInteractionDistance = 250.0f;

	/** Full world discovery is rare; normal prompt queries use the small weak-reference cache. */
	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|On Foot", meta = (ClampMin = "0.1"))
	float InteractionCacheRefreshInterval = 1.0f;

	/** Context text includes rounded values and does not need a per-frame world query. */
	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|On Foot", meta = (ClampMin = "0.05"))
	float ContextPromptRefreshInterval = 0.2f;

	/** Dynamic vehicle resources are sampled at 10 Hz instead of invalidating Slate every frame. */
	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Interface", meta = (ClampMin = "0.05"))
	float InterfaceRefreshInterval = 0.1f;

	/** Shows a cursor and uses Game+UI input mode for mouse testing in the editor. */
	UPROPERTY(EditDefaultsOnly, Category = "Flying Cab|Interface")
	bool bEnableMouseTouchTestingInEditor = true;

	UPROPERTY(Transient)
	TObjectPtr<AFlyingCabCameraRig> CameraRig;

	UPROPERTY(Transient)
	TObjectPtr<UFlyingCabGameFlowWidget> GameFlowWidget;

	UPROPERTY(Transient)
	TObjectPtr<UFlyingCabQuestJournalWidget> QuestJournalWidget;

	UPROPERTY(Transient)
	TObjectPtr<UFlyingCabTouchControls> InterfaceWidget;

	TArray<TWeakObjectPtr<AActor>> CachedInteractables;
	TArray<TWeakObjectPtr<AFlyingCabPawn>> CachedVehicles;
	TWeakObjectPtr<AFlyingCabCharacter> CachedContextPromptPawn;
	FText CachedContextPrompt;
	double LastInteractionCacheRefreshTime = -1.0;
	double LastContextPromptRefreshTime = -1.0;
	EFlyingCabPlayerMode PlayerMode = EFlyingCabPlayerMode::Unknown;
	int32 DisplayCredits = 0;
	int32 DisplayActiveFare = 0;
	FTimerHandle InterfaceRefreshTimerHandle;

	bool bGameFlowScreenOpen = false;
	bool bQuestJournalOpen = false;
};
