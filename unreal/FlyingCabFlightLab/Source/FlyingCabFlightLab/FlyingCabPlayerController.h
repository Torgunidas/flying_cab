// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlyingCabRunTypes.h"
#include "GameFramework/PlayerController.h"
#include "FlyingCabPlayerController.generated.h"

class AFlyingCabCharacter;
class AFlyingCabCameraRig;
class AFlyingCabPawn;
class UFlyingCabGameFlowWidget;

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

	UFUNCTION(BlueprintCallable, Category = "Flying Cab|Interaction")
	void RequestContextInteraction();
	FText GetContextPrompt();
	void ShowEventMessage(
		const FText& Message,
		const FLinearColor& Color,
		float DurationSeconds = 2.5f) const;
	void StartRunMode(EFlyingCabRunMode Mode);
	void RestartWithRunMode(EFlyingCabRunMode Mode);
	void ReturnToModeSelection();
	void ShowTimeAttackResults(
		const FFlyingCabTimeAttackResult& Result,
		const TArray<float>& BestTimes);
	bool IsGameFlowScreenOpen() const { return bGameFlowScreenOpen; }
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
	class UFlyingCabTouchControls* GetInterfaceWidget() const;
	void ShowInitialModeSelection();
	void EnterMenuInputMode();
	void RestoreGameplayInputMode();
	static EFlyingCabRunMode ParseRunMode(const FString& Value);
	static FString GetRunModeOption(EFlyingCabRunMode Mode);

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

	UPROPERTY(Transient)
	TObjectPtr<AFlyingCabPawn> ActiveVehicle;

	UPROPERTY(Transient)
	TObjectPtr<AFlyingCabCameraRig> CameraRig;

	UPROPERTY(Transient)
	TObjectPtr<UFlyingCabGameFlowWidget> GameFlowWidget;

	TArray<TWeakObjectPtr<AActor>> CachedInteractables;
	TArray<TWeakObjectPtr<AFlyingCabPawn>> CachedVehicles;
	TWeakObjectPtr<AFlyingCabCharacter> CachedContextPromptPawn;
	FText CachedContextPrompt;
	double LastInteractionCacheRefreshTime = -1.0;
	double LastContextPromptRefreshTime = -1.0;
	EFlyingCabPlayerMode PlayerMode = EFlyingCabPlayerMode::Unknown;

	bool bGameFlowScreenOpen = false;
};
