// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FlyingCabTouchControls.generated.h"

class AFlyingCabPawn;
class UBorder;
class UButton;
class UCanvasPanel;
class UTextBlock;

/** Minimal portrait touch overlay used to validate the mobile control scheme. */
UCLASS()
class FLYINGCABFLIGHTLAB_API UFlyingCabTouchControls : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetControlsVisible(bool bVisible);
	void SetObjectiveText(const FText& Text);
	void SetTrafficAlert(const FText& Text, const FLinearColor& Color);
	void SetMinimapState(
		const FVector2D& CabWorldPosition,
		const FVector2D& TargetWorldPosition,
		bool bTargetIsDropoff);
	void SetMinimapTargetVisible(bool bVisible);
	void SetResourceState(
		float FuelPercent,
		float HullPercent,
		int32 Credits,
		int32 ActiveFare,
		bool bRefuelAvailable,
		int32 RefuelPricePerUnit,
		bool bRepairAvailable,
		int32 RepairPricePerHullUnit,
		bool bVehicleDestroyed);
	void ReleaseAllInputs();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

private:
	void BuildWidgetTree();
	UButton* AddControlButton(
		UCanvasPanel* RootCanvas,
		FName WidgetName,
		const FString& LabelText,
		const FAnchors& Anchors,
		const FVector2D& Alignment,
		const FVector2D& Position,
		const FVector2D& Size,
		const FLinearColor& Color);
	AFlyingCabPawn* GetFlyingCabPawn() const;
	void UpdateHorizontalInput();
	FVector2D WorldToMinimap(const FVector2D& WorldPosition) const;
	void UpdateMinimapMarkers();
	UBorder* AddMinimapPoint(
		UCanvasPanel* Canvas,
		FName WidgetName,
		const FVector2D& WorldPosition,
		const FVector2D& Size,
		const FLinearColor& Color);

	UFUNCTION()
	void HandleLeftPressed();

	UFUNCTION()
	void HandleLeftReleased();

	UFUNCTION()
	void HandleRightPressed();

	UFUNCTION()
	void HandleRightReleased();

	UFUNCTION()
	void HandleThrustPressed();

	UFUNCTION()
	void HandleThrustReleased();

	UFUNCTION()
	void HandleResetPressed();

	UFUNCTION()
	void HandleRefuelPressed();

	UFUNCTION()
	void HandleRefuelReleased();

	UPROPERTY(Transient)
	TObjectPtr<UButton> LeftButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RightButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ThrustButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ResetButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RefuelButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ServiceButtonText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ObjectiveText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TrafficAlertText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ResourceText;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> MinimapCanvas;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> CabMarker;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> TargetMarker;

	FText PendingObjectiveText = FText::FromString(TEXT("FLIGHT LAB"));
	FText PendingTrafficAlertText;
	FLinearColor PendingTrafficAlertColor = FLinearColor::Transparent;
	FVector2D PendingCabWorldPosition = FVector2D::ZeroVector;
	FVector2D PendingTargetWorldPosition = FVector2D::ZeroVector;
	bool bPendingTargetIsDropoff = false;
	bool bPendingTargetVisible = false;
	bool bHasMinimapState = false;
	float PendingFuelPercent = 1.0f;
	float PendingHullPercent = 1.0f;
	int32 PendingCredits = 0;
	int32 PendingActiveFare = 0;
	int32 PendingRefuelPricePerUnit = 0;
	bool bPendingRefuelAvailable = false;
	int32 PendingRepairPricePerHullUnit = 0;
	bool bPendingRepairAvailable = false;
	bool bPendingVehicleDestroyed = false;

	bool bLeftPressed = false;
	bool bRightPressed = false;
	bool bThrustPressed = false;
	bool bRefuelPressed = false;
};
