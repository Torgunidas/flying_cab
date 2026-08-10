// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "FlyingCabPawn.generated.h"

class UBoxComponent;
class UCameraComponent;
class UPointLightComponent;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class USpringArmComponent;
class UFlyingCabTouchControls;
class AFlyingCabPawn;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnFlyingCabDestroyed, AFlyingCabPawn*);

/**
 * Minimal, force-driven 2.5D vehicle used to tune the flight feel.
 * Keyboard and touch commands meet here so both platforms use one physics model.
 */
UCLASS()
class FLYINGCABFLIGHTLAB_API AFlyingCabPawn : public APawn
{
	GENERATED_BODY()

public:
	AFlyingCabPawn();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	/** Horizontal input for the future mobile UI: -1 is left, +1 is right. */
	UFUNCTION(BlueprintCallable, Category = "Flying Cab|Input")
	void SetTouchHorizontalInput(float Value);

	/** Press/release input for the future mobile thrust button. */
	UFUNCTION(BlueprintCallable, Category = "Flying Cab|Input")
	void SetTouchThrustPressed(bool bPressed);

	UFUNCTION(BlueprintCallable, Category = "Flying Cab|Input")
	void SetTouchRefuelPressed(bool bPressed);

	UFUNCTION(BlueprintCallable, Category = "Flying Cab|Flight")
	void ResetVehicle();

	void SetObjectiveStatus(const FText& Status);
	void SetMinimapState(
		const FVector2D& CabWorldPosition,
		const FVector2D& TargetWorldPosition,
		bool bTargetIsDropoff);
	void SetPassengerOfferMarkers(
		const FVector2D& CabWorldPosition,
		const TArray<FVector2D>& OfferWorldPositions);
	void ClearMinimapTarget();
	void SetProximityGuidance(
		bool bVisible,
		const FVector2D& TargetWorldPosition,
		bool bTargetIsDropoff);
	void SetTimeAttackStatus(
		bool bActive,
		float ElapsedSeconds,
		int32 Credits,
		int32 TargetCredits);
	void SetEconomyStatus(int32 Credits, int32 ActiveFare);
	void SetTrafficAlert(const FText& Alert, const FLinearColor& Color);
	void SetRefuelAvailable(bool bAvailable, int32 PricePerUnit);
	void SetRepairAvailable(bool bAvailable, int32 PricePerHullUnit);
	void ConfigureVehicleIdentity(
		FName InVehicleId,
		const FString& InDisplayName,
		FName InRequiredAccessId,
		const FLinearColor& InVehicleColor);
	bool CanPlayerEnter(FText& OutFailureReason) const;
	FText GetEntryPrompt() const;
	const FString& GetVehicleDisplayName() const { return VehicleDisplayName; }
	UFlyingCabTouchControls* GetTouchControlsWidget() const { return TouchControlsWidget; }
	UFlyingCabTouchControls* DetachTouchControlsWidget();
	void AdoptTouchControlsWidget(UFlyingCabTouchControls* Widget);
	bool IsRefuelRequested() const;
	bool IsRepairRequested() const;
	bool IsDestroyed() const { return bDestroyed; }
	float GetFuel() const { return CurrentFuel; }
	float GetMaxFuel() const { return MaxFuel; }
	float GetFuelNeeded() const { return FMath::Max(0.0f, MaxFuel - CurrentFuel); }
	float GetHullNeeded() const { return FMath::Max(0.0f, MaxHull - CurrentHull); }
	FVector GetCameraTrackingOffset() const;
	float AddFuel(float Units);
	float AddHull(float Units);
	void RecoverVehicle(float RecoveryFuelPercent);

	FOnFlyingCabDestroyed OnVehicleDestroyed;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void SetKeyboardHorizontalInput(float Value);
	void SetKeyboardThrustInput(float Value);
	void SetKeyboardServiceInput(float Value);
	void ClearAllInputState(const TCHAR* Reason, bool bFlushPressedKeys);
	void ToggleFlightTelemetry();
	void DrawFlightTelemetry(float HorizontalInput, float ThrustInput, const FVector& Velocity) const;
	void UpdateVisualResponse(float DeltaSeconds, const FVector& Velocity);
	void TryCreateTouchControls();
	void ToggleTouchControls();
	void ApplyTouchControlsVisibility();
	void RefreshResourceUI();
	void RefreshVehicleIdentityAppearance(bool bForce = false);
	bool HasRequiredVehicleAccess() const;
	void ConsumeOrRegenerateFuel(
		float DeltaSeconds,
		float HorizontalInput,
		float ThrustInput,
		const FVector& Velocity);
	void ApplyCollisionDamage(float NormalSpeedChange);
	void EnterDestroyedState();

	UFUNCTION()
	void HandleCollisionHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse,
		const FHitResult& Hit);

	float GetHorizontalInput() const;
	float GetThrustInput() const;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Components")
	TObjectPtr<UBoxComponent> CollisionBody;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Components")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Components")
	TObjectPtr<UPointLightComponent> DamageLight;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Components")
	TObjectPtr<UTextRenderComponent> VehicleLabel;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Components")
	TObjectPtr<UPointLightComponent> AccessLight;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Components")
	TObjectPtr<USceneComponent> GuidanceArrowRoot;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Components")
	TObjectPtr<UStaticMeshComponent> GuidanceArrowShaft;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Components")
	TObjectPtr<UStaticMeshComponent> GuidanceArrowTip;

	UPROPERTY(Transient)
	TObjectPtr<UFlyingCabTouchControls> TouchControlsWidget;

	/** Acceleration produced by full vertical thrust, in cm/s^2. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Flight", meta = (ClampMin = "0.0"))
	float VerticalThrustAcceleration = 2350.0f;

	/** Horizontal acceleration produced by either side thruster, in cm/s^2. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Flight", meta = (ClampMin = "0.0"))
	float HorizontalThrustAcceleration = 1400.0f;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Flight", meta = (ClampMin = "0.0"))
	float MaxClimbSpeed = 1150.0f;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Flight", meta = (ClampMin = "0.0"))
	float MaxFallSpeed = 1300.0f;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Flight", meta = (ClampMin = "0.0"))
	float MaxHorizontalSpeed = 1050.0f;

	/** How quickly horizontal drift fades after releasing A/D. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Flight", meta = (ClampMin = "0.0"))
	float HorizontalCoastDamping = 1.7f;

	/** Reproduces the old game's soft braking of upward motion after releasing thrust. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Flight", meta = (ClampMin = "0.0"))
	float UpwardCoastDamping = 1.5f;

	/** Screen-space pitch used to communicate horizontal thrust without rotating the collider. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Presentation", meta = (ClampMin = "0.0", ClampMax = "45.0"))
	float MaxVisualPitchDegrees = 12.0f;

	/** Horizontal acceleration that produces the full configured visual pitch. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Presentation", meta = (ClampMin = "0.0"))
	float VisualPitchFullAcceleration = 1400.0f;

	/** Responsiveness when acceleration asks the vehicle to lean. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Presentation", meta = (ClampMin = "0.0"))
	float VisualPitchResponseSpeed = 7.0f;

	/** Slower settling used when horizontal acceleration ends. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Presentation", meta = (ClampMin = "0.0"))
	float VisualPitchReturnSpeed = 2.5f;

	/** How far the camera framing moves ahead of horizontal and vertical velocity. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Presentation", meta = (ClampMin = "0.0"))
	float HorizontalCameraLookAhead = 240.0f;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Presentation", meta = (ClampMin = "0.0"))
	float VerticalCameraLookAhead = 140.0f;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Presentation", meta = (ClampMin = "0.0"))
	float CameraLookAheadInterpSpeed = 2.5f;

	/** Runtime readout used while tuning the FlightLab prototype. Toggle with F3. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Debug")
	bool bShowFlightTelemetry = false;

	/** Prototype touch overlay. Toggle with F4 while testing on desktop. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Input")
	bool bShowTouchControls = true;

	/** Shows a cursor and uses Game+UI input mode for mouse testing in the editor. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Input")
	bool bEnableMouseTouchTestingInEditor = true;

	/** Normalized energy capacity. Values deliberately use percentages for fast tuning. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Resources", meta = (ClampMin = "1.0"))
	float MaxFuel = 100.0f;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Resources", meta = (ClampMin = "0.0"))
	float StartingFuel = 65.0f;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Resources", meta = (ClampMin = "0.0"))
	float VerticalFuelPerSecond = 1.8f;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Resources", meta = (ClampMin = "0.0"))
	float HorizontalFuelPerSecond = 0.9f;

	/** Regenerative recovery while descending without thrust. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Resources", meta = (ClampMin = "0.0"))
	float DescentRegenerationPerSecond = 0.12f;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Resources", meta = (ClampMin = "0.0"))
	float RegenerationFullSpeed = 900.0f;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Damage", meta = (ClampMin = "1.0"))
	float MaxHull = 100.0f;

	/** Normal impulse divided by mass must exceed this speed change before damage starts. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Damage", meta = (ClampMin = "0.0"))
	float DamageImpactSpeedThreshold = 700.0f;

	/** An impact at this speed change deals a full hull of damage. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Damage", meta = (ClampMin = "1.0"))
	float DamageFullHullSpeed = 1400.0f;

	/** Shapes the damage ramp above the safe threshold; 2 gives a forgiving quadratic curve. */
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Damage", meta = (ClampMin = "1.0"))
	float CollisionDamageExponent = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Damage", meta = (ClampMin = "0.0"))
	float CollisionDamageCooldown = 0.15f;

	FText ObjectiveStatus = FText::FromString(TEXT("FLIGHT LAB"));
	FText TrafficAlert;
	FLinearColor TrafficAlertColor = FLinearColor::Transparent;
	FVector2D MinimapCabPosition = FVector2D::ZeroVector;
	FVector2D MinimapTargetPosition = FVector2D::ZeroVector;
	TArray<FVector2D> MinimapPassengerOfferPositions;
	bool bMinimapTargetIsDropoff = false;
	bool bHasMinimapState = false;
	bool bMinimapTargetVisible = false;
	bool bTimeAttackStatusActive = false;
	float TimeAttackElapsedSeconds = 0.0f;
	int32 TimeAttackTargetCredits = 1000;
	int32 DisplayCredits = 0;
	int32 DisplayActiveFare = 0;
	int32 RefuelPricePerUnit = 0;
	int32 RepairPricePerHullUnit = 0;
	float CurrentFuel = 0.0f;
	float CurrentHull = 0.0f;
	float DamageCooldownRemaining = 0.0f;
	float DamageFlashRemaining = 0.0f;
	bool bRefuelAvailable = false;
	bool bRepairAvailable = false;
	bool bDestroyed = false;
	bool bFuelEmptyWarningShown = false;
	FName VehicleId = TEXT("Vehicle.PlayerCab");
	FName RequiredAccessId = NAME_None;
	FString VehicleDisplayName = TEXT("CAB");
	FLinearColor VehicleColor = FLinearColor::White;
	bool bIdentityConfigured = false;
	bool bHasCachedAccessState = false;
	bool bCachedAccessState = false;

	FTransform SpawnTransform;

	float KeyboardHorizontalInput = 0.0f;
	float KeyboardThrustInput = 0.0f;
	uint32 ForcedInputResetCount = 0;
	float PreviousHorizontalVelocity = 0.0f;
	float VisualHorizontalAcceleration = 0.0f;
	bool bHasPreviousHorizontalVelocity = false;
	float TouchHorizontalInput = 0.0f;
	float TouchThrustInput = 0.0f;
	bool bKeyboardRefuelPressed = false;
	bool bTouchRefuelPressed = false;
};
