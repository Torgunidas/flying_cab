// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabTouchControls.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "FlyingCabPawn.h"

namespace
{
	constexpr float MinimapLeft = 10.0f;
	constexpr float MinimapRight = 122.0f;
	constexpr float MinimapTop = 24.0f;
	constexpr float MinimapBottom = 168.0f;
	const FVector2D MinimapWorldMin(-4950.0f, 0.0f);
	const FVector2D MinimapWorldMax(4950.0f, 6500.0f);
}

TSharedRef<SWidget> UFlyingCabTouchControls::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}

	return Super::RebuildWidget();
}

void UFlyingCabTouchControls::NativeDestruct()
{
	ReleaseAllInputs();
	Super::NativeDestruct();
}

void UFlyingCabTouchControls::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	ReleaseAllInputs();
	Super::NativeOnFocusLost(InFocusEvent);
}

void UFlyingCabTouchControls::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	ReleaseAllInputs();
	Super::NativeOnMouseLeave(InMouseEvent);
}

void UFlyingCabTouchControls::SetControlsVisible(bool bVisible)
{
	if (!bVisible)
	{
		ReleaseAllInputs();
	}

	SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UFlyingCabTouchControls::SetObjectiveText(const FText& Text)
{
	PendingObjectiveText = Text;
	if (ObjectiveText)
	{
		ObjectiveText->SetText(PendingObjectiveText);
	}
}

void UFlyingCabTouchControls::SetTrafficAlert(const FText& Text, const FLinearColor& Color)
{
	PendingTrafficAlertText = Text;
	PendingTrafficAlertColor = Color;
	if (TrafficAlertText)
	{
		TrafficAlertText->SetText(PendingTrafficAlertText);
		TrafficAlertText->SetColorAndOpacity(FSlateColor(PendingTrafficAlertColor));
		TrafficAlertText->SetVisibility(
			PendingTrafficAlertText.IsEmpty()
				? ESlateVisibility::Collapsed
				: ESlateVisibility::HitTestInvisible);
	}
}

void UFlyingCabTouchControls::SetMinimapState(
	const FVector2D& CabWorldPosition,
	const FVector2D& TargetWorldPosition,
	bool bTargetIsDropoff)
{
	PendingCabWorldPosition = CabWorldPosition;
	PendingTargetWorldPosition = TargetWorldPosition;
	bPendingTargetIsDropoff = bTargetIsDropoff;
	bPendingTargetVisible = true;
	bHasMinimapState = true;
	UpdateMinimapMarkers();
}

void UFlyingCabTouchControls::SetMinimapTargetVisible(bool bVisible)
{
	bPendingTargetVisible = bVisible;
	UpdateMinimapMarkers();
}

void UFlyingCabTouchControls::SetResourceState(
	float FuelPercent,
	float HullPercent,
	int32 Credits,
	int32 ActiveFare,
	bool bRefuelAvailable,
	int32 RefuelPricePerUnit,
	bool bRepairAvailable,
	int32 RepairPricePerHullUnit,
	bool bVehicleDestroyed)
{
	PendingFuelPercent = FMath::Clamp(FuelPercent, 0.0f, 1.0f);
	PendingHullPercent = FMath::Clamp(HullPercent, 0.0f, 1.0f);
	PendingCredits = FMath::Max(0, Credits);
	PendingActiveFare = FMath::Max(0, ActiveFare);
	bPendingRefuelAvailable = bRefuelAvailable;
	PendingRefuelPricePerUnit = FMath::Max(0, RefuelPricePerUnit);
	bPendingRepairAvailable = bRepairAvailable;
	PendingRepairPricePerHullUnit = FMath::Max(0, RepairPricePerHullUnit);
	bPendingVehicleDestroyed = bVehicleDestroyed;

	if (ResourceText)
	{
		const FString FareText = PendingActiveFare > 0
			? FString::Printf(TEXT("FARE  +%d CR"), PendingActiveFare)
			: FString(TEXT("FARE  --"));
		const FString ServiceText = bPendingRepairAvailable
			? FString::Printf(TEXT("REPAIR  %d CR/HP"), PendingRepairPricePerHullUnit)
			: (bPendingRefuelAvailable
				? FString::Printf(TEXT("FUEL SERVICE  %d CR/U"), PendingRefuelPricePerUnit)
				: FString());
		ResourceText->SetText(FText::FromString(FString::Printf(
			TEXT("CREDITS  %d\nFUEL %3.0f%%  |  HULL %3.0f%%\n%s%s%s"),
			PendingCredits,
			PendingFuelPercent * 100.0f,
			PendingHullPercent * 100.0f,
			*FareText,
			ServiceText.IsEmpty() ? TEXT("") : TEXT("\n"),
			*ServiceText)));

		const bool bCritical = bPendingVehicleDestroyed
			|| PendingFuelPercent <= 0.15f
			|| PendingHullPercent <= 0.25f;
		ResourceText->SetColorAndOpacity(FSlateColor(
			bCritical
				? FLinearColor(1.0f, 0.18f, 0.04f)
				: FLinearColor(0.20f, 0.92f, 0.72f)));
	}

	if (RefuelButton)
	{
		RefuelButton->SetVisibility(
			bPendingRefuelAvailable || bPendingRepairAvailable
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed);
		RefuelButton->SetBackgroundColor(
			bPendingRepairAvailable
				? FLinearColor(0.62f, 0.08f, 0.88f, 0.90f)
				: FLinearColor(0.05f, 0.65f, 0.28f, 0.88f));
	}
	if (ServiceButtonText)
	{
		ServiceButtonText->SetText(FText::FromString(
			bPendingRepairAvailable ? TEXT("REPAIR") : TEXT("REFUEL")));
	}
}

void UFlyingCabTouchControls::ReleaseAllInputs()
{
	bLeftPressed = false;
	bRightPressed = false;
	bThrustPressed = false;
	bRefuelPressed = false;

	if (AFlyingCabPawn* Pawn = GetFlyingCabPawn())
	{
		Pawn->SetTouchHorizontalInput(0.0f);
		Pawn->SetTouchThrustPressed(false);
		Pawn->SetTouchRefuelPressed(false);
	}
}

void UFlyingCabTouchControls::BuildWidgetTree()
{
	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(),
		TEXT("TouchControlsRoot"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* MinimapFrame = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("MinimapFrame"));
	MinimapFrame->SetBrushColor(FLinearColor(0.01f, 0.025f, 0.05f, 0.88f));
	MinimapFrame->SetPadding(FMargin(0.0f));
	MinimapCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(),
		TEXT("MinimapCanvas"));
	MinimapFrame->AddChild(MinimapCanvas);

	UCanvasPanelSlot* MinimapSlot = RootCanvas->AddChildToCanvas(MinimapFrame);
	MinimapSlot->SetAnchors(FAnchors(0.0f, 0.0f));
	MinimapSlot->SetAlignment(FVector2D(0.0f, 0.0f));
	MinimapSlot->SetPosition(FVector2D(12.0f, 12.0f));
	MinimapSlot->SetSize(FVector2D(132.0f, 180.0f));
	MinimapSlot->SetZOrder(20);

	UTextBlock* MapTitle = WidgetTree->ConstructWidget<UTextBlock>();
	MapTitle->SetText(FText::FromString(TEXT("CITY GRID")));
	MapTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.15f, 0.75f, 0.85f)));
	FSlateFontInfo MapTitleFont = MapTitle->GetFont();
	MapTitleFont.Size = 11;
	MapTitle->SetFont(MapTitleFont);
	UCanvasPanelSlot* MapTitleSlot = MinimapCanvas->AddChildToCanvas(MapTitle);
	MapTitleSlot->SetPosition(FVector2D(8.0f, 3.0f));
	MapTitleSlot->SetSize(FVector2D(120.0f, 18.0f));

	struct FStopPin
	{
		FVector2D WorldPosition;
		const TCHAR* Code;
	};
	const FStopPin StopPins[] = {
		{FVector2D(-900.0f, 1150.0f), TEXT("YP")},
		{FVector2D(850.0f, 2050.0f), TEXT("ME")},
		{FVector2D(-750.0f, 3150.0f), TEXT("ST")},
		{FVector2D(-3800.0f, 2500.0f), TEXT("AM")},
		{FVector2D(3650.0f, 1150.0f), TEXT("ND")},
		{FVector2D(3350.0f, 5200.0f), TEXT("ZS")}};

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(StopPins); ++Index)
	{
		const FVector2D PinPosition = WorldToMinimap(StopPins[Index].WorldPosition);
		AddMinimapPoint(
			MinimapCanvas,
			FName(*FString::Printf(TEXT("StopPin%d"), Index)),
			StopPins[Index].WorldPosition,
			FVector2D(7.0f, 7.0f),
			FLinearColor(0.25f, 0.32f, 0.38f, 0.9f));

		UTextBlock* StopCode = WidgetTree->ConstructWidget<UTextBlock>();
		StopCode->SetText(FText::FromString(FString(StopPins[Index].Code)));
		StopCode->SetColorAndOpacity(FSlateColor(FLinearColor(0.42f, 0.55f, 0.62f)));
		FSlateFontInfo StopFont = StopCode->GetFont();
		StopFont.Size = 9;
		StopCode->SetFont(StopFont);
		UCanvasPanelSlot* StopCodeSlot = MinimapCanvas->AddChildToCanvas(StopCode);
		StopCodeSlot->SetPosition(PinPosition + FVector2D(6.0f, -7.0f));
		StopCodeSlot->SetSize(FVector2D(30.0f, 16.0f));
	}

	const FVector2D FuelStationWorldPositions[] = {
		FVector2D(850.0f, 2050.0f),
		FVector2D(-3800.0f, 2500.0f)};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(FuelStationWorldPositions); ++Index)
	{
		const FVector2D FuelStationMapPosition = WorldToMinimap(FuelStationWorldPositions[Index]);
		AddMinimapPoint(
			MinimapCanvas,
			FName(*FString::Printf(TEXT("FuelStationPin%d"), Index)),
			FuelStationWorldPositions[Index],
			FVector2D(9.0f, 9.0f),
			FLinearColor(0.15f, 1.0f, 0.45f, 0.95f));
		UTextBlock* FuelCode = WidgetTree->ConstructWidget<UTextBlock>();
		FuelCode->SetText(FText::FromString(TEXT("F")));
		FuelCode->SetColorAndOpacity(FSlateColor(FLinearColor(0.15f, 1.0f, 0.45f)));
		FSlateFontInfo FuelFont = FuelCode->GetFont();
		FuelFont.Size = 9;
		FuelCode->SetFont(FuelFont);
		UCanvasPanelSlot* FuelCodeSlot = MinimapCanvas->AddChildToCanvas(FuelCode);
		FuelCodeSlot->SetPosition(FuelStationMapPosition + FVector2D(6.0f, 2.0f));
		FuelCodeSlot->SetSize(FVector2D(18.0f, 16.0f));
	}

	const FVector2D RepairStationWorldPosition(0.0f, 4200.0f);
	const FVector2D RepairStationMapPosition = WorldToMinimap(RepairStationWorldPosition);
	AddMinimapPoint(
		MinimapCanvas,
		TEXT("RepairStationPin"),
		RepairStationWorldPosition,
		FVector2D(10.0f, 10.0f),
		FLinearColor(0.78f, 0.12f, 1.0f, 0.95f));
	UTextBlock* RepairCode = WidgetTree->ConstructWidget<UTextBlock>();
	RepairCode->SetText(FText::FromString(TEXT("R")));
	RepairCode->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.12f, 1.0f)));
	FSlateFontInfo RepairFont = RepairCode->GetFont();
	RepairFont.Size = 9;
	RepairCode->SetFont(RepairFont);
	UCanvasPanelSlot* RepairCodeSlot = MinimapCanvas->AddChildToCanvas(RepairCode);
	RepairCodeSlot->SetPosition(RepairStationMapPosition + FVector2D(6.0f, 2.0f));
	RepairCodeSlot->SetSize(FVector2D(18.0f, 16.0f));

	CabMarker = AddMinimapPoint(
		MinimapCanvas,
		TEXT("CabMarker"),
		PendingCabWorldPosition,
		FVector2D(10.0f, 10.0f),
		FLinearColor::White);
	TargetMarker = AddMinimapPoint(
		MinimapCanvas,
		TEXT("TargetMarker"),
		PendingTargetWorldPosition,
		FVector2D(15.0f, 15.0f),
		FLinearColor(0.0f, 0.9f, 1.0f));

	ObjectiveText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("ObjectiveText"));
	ObjectiveText->SetText(PendingObjectiveText);
	ObjectiveText->SetJustification(ETextJustify::Center);
	ObjectiveText->SetAutoWrapText(true);
	ObjectiveText->SetColorAndOpacity(FSlateColor(FLinearColor(0.15f, 0.92f, 1.0f)));
	ObjectiveText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
	ObjectiveText->SetShadowOffset(FVector2D(2.0f, 2.0f));
	FSlateFontInfo ObjectiveFont = ObjectiveText->GetFont();
	ObjectiveFont.Size = 22;
	ObjectiveText->SetFont(ObjectiveFont);

	UCanvasPanelSlot* ObjectiveSlot = RootCanvas->AddChildToCanvas(ObjectiveText);
	ObjectiveSlot->SetAnchors(FAnchors(1.0f, 0.0f));
	ObjectiveSlot->SetAlignment(FVector2D(1.0f, 0.0f));
	ObjectiveSlot->SetPosition(FVector2D(-12.0f, 88.0f));
	ObjectiveSlot->SetSize(FVector2D(195.0f, 112.0f));
	ObjectiveSlot->SetZOrder(20);

	TrafficAlertText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("TrafficAlertText"));
	TrafficAlertText->SetText(PendingTrafficAlertText);
	TrafficAlertText->SetJustification(ETextJustify::Center);
	TrafficAlertText->SetColorAndOpacity(FSlateColor(PendingTrafficAlertColor));
	TrafficAlertText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
	TrafficAlertText->SetShadowOffset(FVector2D(2.0f, 2.0f));
	TrafficAlertText->SetVisibility(
		PendingTrafficAlertText.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	FSlateFontInfo TrafficAlertFont = TrafficAlertText->GetFont();
	TrafficAlertFont.Size = 19;
	TrafficAlertText->SetFont(TrafficAlertFont);
	UCanvasPanelSlot* TrafficAlertSlot = RootCanvas->AddChildToCanvas(TrafficAlertText);
	TrafficAlertSlot->SetAnchors(FAnchors(0.5f, 0.0f));
	TrafficAlertSlot->SetAlignment(FVector2D(0.5f, 0.0f));
	TrafficAlertSlot->SetPosition(FVector2D(0.0f, 18.0f));
	TrafficAlertSlot->SetSize(FVector2D(280.0f, 54.0f));
	TrafficAlertSlot->SetZOrder(30);

	UBorder* ResourcePanel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("ResourcePanel"));
	ResourcePanel->SetBrushColor(FLinearColor(0.005f, 0.015f, 0.03f, 0.92f));
	ResourcePanel->SetPadding(FMargin(12.0f, 10.0f));

	ResourceText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("ResourceText"));
	ResourceText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.95f));
	ResourceText->SetShadowOffset(FVector2D(2.0f, 2.0f));
	FSlateFontInfo ResourceFont = ResourceText->GetFont();
	ResourceFont.Size = 16;
	ResourceText->SetFont(ResourceFont);
	ResourcePanel->AddChild(ResourceText);
	UCanvasPanelSlot* ResourceSlot = RootCanvas->AddChildToCanvas(ResourcePanel);
	ResourceSlot->SetAnchors(FAnchors(0.0f, 0.0f));
	ResourceSlot->SetPosition(FVector2D(12.0f, 200.0f));
	ResourceSlot->SetSize(FVector2D(260.0f, 126.0f));
	ResourceSlot->SetZOrder(20);

	LeftButton = AddControlButton(
		RootCanvas,
		TEXT("LeftButton"),
		TEXT("LEFT"),
		FAnchors(0.0f, 1.0f),
		FVector2D(0.0f, 1.0f),
		FVector2D(28.0f, -28.0f),
		FVector2D(132.0f, 132.0f),
		FLinearColor(0.02f, 0.45f, 0.70f, 0.82f));
	RightButton = AddControlButton(
		RootCanvas,
		TEXT("RightButton"),
		TEXT("RIGHT"),
		FAnchors(0.0f, 1.0f),
		FVector2D(0.0f, 1.0f),
		FVector2D(176.0f, -28.0f),
		FVector2D(132.0f, 132.0f),
		FLinearColor(0.02f, 0.45f, 0.70f, 0.82f));
	ThrustButton = AddControlButton(
		RootCanvas,
		TEXT("ThrustButton"),
		TEXT("THRUST"),
		FAnchors(1.0f, 1.0f),
		FVector2D(1.0f, 1.0f),
		FVector2D(-28.0f, -28.0f),
		FVector2D(176.0f, 176.0f),
		FLinearColor(0.95f, 0.22f, 0.03f, 0.86f));
	ResetButton = AddControlButton(
		RootCanvas,
		TEXT("ResetButton"),
		TEXT("RESET"),
		FAnchors(1.0f, 0.0f),
		FVector2D(1.0f, 0.0f),
		FVector2D(-24.0f, 24.0f),
		FVector2D(104.0f, 56.0f),
		FLinearColor(0.12f, 0.14f, 0.18f, 0.78f));
	RefuelButton = AddControlButton(
		RootCanvas,
		TEXT("RefuelButton"),
		TEXT("REFUEL"),
		FAnchors(1.0f, 0.0f),
		FVector2D(1.0f, 0.0f),
		FVector2D(-144.0f, 24.0f),
		FVector2D(116.0f, 56.0f),
		FLinearColor(0.05f, 0.65f, 0.28f, 0.88f));
	ServiceButtonText = Cast<UTextBlock>(RefuelButton->GetContent());

	LeftButton->OnPressed.AddDynamic(this, &UFlyingCabTouchControls::HandleLeftPressed);
	LeftButton->OnReleased.AddDynamic(this, &UFlyingCabTouchControls::HandleLeftReleased);
	LeftButton->OnUnhovered.AddDynamic(this, &UFlyingCabTouchControls::HandleLeftReleased);
	RightButton->OnPressed.AddDynamic(this, &UFlyingCabTouchControls::HandleRightPressed);
	RightButton->OnReleased.AddDynamic(this, &UFlyingCabTouchControls::HandleRightReleased);
	RightButton->OnUnhovered.AddDynamic(this, &UFlyingCabTouchControls::HandleRightReleased);
	ThrustButton->OnPressed.AddDynamic(this, &UFlyingCabTouchControls::HandleThrustPressed);
	ThrustButton->OnReleased.AddDynamic(this, &UFlyingCabTouchControls::HandleThrustReleased);
	ThrustButton->OnUnhovered.AddDynamic(this, &UFlyingCabTouchControls::HandleThrustReleased);
	ResetButton->OnPressed.AddDynamic(this, &UFlyingCabTouchControls::HandleResetPressed);
	RefuelButton->OnPressed.AddDynamic(this, &UFlyingCabTouchControls::HandleRefuelPressed);
	RefuelButton->OnReleased.AddDynamic(this, &UFlyingCabTouchControls::HandleRefuelReleased);
	RefuelButton->OnUnhovered.AddDynamic(this, &UFlyingCabTouchControls::HandleRefuelReleased);
	UpdateMinimapMarkers();
	SetResourceState(
		PendingFuelPercent,
		PendingHullPercent,
		PendingCredits,
		PendingActiveFare,
		bPendingRefuelAvailable,
		PendingRefuelPricePerUnit,
		bPendingRepairAvailable,
		PendingRepairPricePerHullUnit,
		bPendingVehicleDestroyed);
}

FVector2D UFlyingCabTouchControls::WorldToMinimap(const FVector2D& WorldPosition) const
{
	const float NormalizedX = FMath::GetMappedRangeValueClamped(
		FVector2D(MinimapWorldMin.X, MinimapWorldMax.X),
		FVector2D(0.0f, 1.0f),
		WorldPosition.X);
	const float NormalizedZ = FMath::GetMappedRangeValueClamped(
		FVector2D(MinimapWorldMin.Y, MinimapWorldMax.Y),
		FVector2D(0.0f, 1.0f),
		WorldPosition.Y);

	return FVector2D(
		FMath::Lerp(MinimapLeft, MinimapRight, NormalizedX),
		FMath::Lerp(MinimapBottom, MinimapTop, NormalizedZ));
}

void UFlyingCabTouchControls::UpdateMinimapMarkers()
{
	if (!CabMarker || !TargetMarker)
	{
		return;
	}

	TargetMarker->SetVisibility(
		bHasMinimapState && bPendingTargetVisible
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	if (!bHasMinimapState)
	{
		return;
	}

	if (UCanvasPanelSlot* CabSlot = Cast<UCanvasPanelSlot>(CabMarker->Slot))
	{
		CabSlot->SetPosition(WorldToMinimap(PendingCabWorldPosition));
	}
	if (UCanvasPanelSlot* TargetSlot = Cast<UCanvasPanelSlot>(TargetMarker->Slot))
	{
		TargetSlot->SetPosition(WorldToMinimap(PendingTargetWorldPosition));
	}

	TargetMarker->SetBrushColor(
		bPendingTargetIsDropoff
			? FLinearColor(1.0f, 0.18f, 0.04f)
			: FLinearColor(0.0f, 0.9f, 1.0f));
}

UBorder* UFlyingCabTouchControls::AddMinimapPoint(
	UCanvasPanel* Canvas,
	FName WidgetName,
	const FVector2D& WorldPosition,
	const FVector2D& Size,
	const FLinearColor& Color)
{
	UBorder* Point = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), WidgetName);
	Point->SetBrushColor(Color);
	UCanvasPanelSlot* PointSlot = Canvas->AddChildToCanvas(Point);
	PointSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	PointSlot->SetPosition(WorldToMinimap(WorldPosition));
	PointSlot->SetSize(Size);
	PointSlot->SetZOrder(5);
	return Point;
}

UButton* UFlyingCabTouchControls::AddControlButton(
	UCanvasPanel* RootCanvas,
	FName WidgetName,
	const FString& LabelText,
	const FAnchors& Anchors,
	const FVector2D& Alignment,
	const FVector2D& Position,
	const FVector2D& Size,
	const FLinearColor& Color)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), WidgetName);
	Button->SetBackgroundColor(Color);
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	Button->IsFocusable = false;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
	Label->SetText(FText::FromString(LabelText));
	Label->SetJustification(ETextJustify::Center);
	Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	FSlateFontInfo Font = Label->GetFont();
	Font.Size = LabelText == TEXT("RESET") ? 18 : 24;
	Label->SetFont(Font);
	Button->AddChild(Label);

	UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(Button);
	CanvasSlot->SetAnchors(Anchors);
	CanvasSlot->SetAlignment(Alignment);
	CanvasSlot->SetPosition(Position);
	CanvasSlot->SetSize(Size);
	CanvasSlot->SetZOrder(10);

	return Button;
}

AFlyingCabPawn* UFlyingCabTouchControls::GetFlyingCabPawn() const
{
	return Cast<AFlyingCabPawn>(GetOwningPlayerPawn());
}

void UFlyingCabTouchControls::UpdateHorizontalInput()
{
	if (AFlyingCabPawn* Pawn = GetFlyingCabPawn())
	{
		const float HorizontalInput = static_cast<float>(bRightPressed) - static_cast<float>(bLeftPressed);
		Pawn->SetTouchHorizontalInput(HorizontalInput);
	}
}

void UFlyingCabTouchControls::HandleLeftPressed()
{
	bLeftPressed = true;
	UpdateHorizontalInput();
}

void UFlyingCabTouchControls::HandleLeftReleased()
{
	bLeftPressed = false;
	UpdateHorizontalInput();
}

void UFlyingCabTouchControls::HandleRightPressed()
{
	bRightPressed = true;
	UpdateHorizontalInput();
}

void UFlyingCabTouchControls::HandleRightReleased()
{
	bRightPressed = false;
	UpdateHorizontalInput();
}

void UFlyingCabTouchControls::HandleThrustPressed()
{
	bThrustPressed = true;
	if (AFlyingCabPawn* Pawn = GetFlyingCabPawn())
	{
		Pawn->SetTouchThrustPressed(true);
	}
}

void UFlyingCabTouchControls::HandleThrustReleased()
{
	bThrustPressed = false;
	if (AFlyingCabPawn* Pawn = GetFlyingCabPawn())
	{
		Pawn->SetTouchThrustPressed(false);
	}
}

void UFlyingCabTouchControls::HandleResetPressed()
{
	if (AFlyingCabPawn* Pawn = GetFlyingCabPawn())
	{
		Pawn->ResetVehicle();
	}
}

void UFlyingCabTouchControls::HandleRefuelPressed()
{
	bRefuelPressed = true;
	if (AFlyingCabPawn* Pawn = GetFlyingCabPawn())
	{
		Pawn->SetTouchRefuelPressed(true);
	}
}

void UFlyingCabTouchControls::HandleRefuelReleased()
{
	bRefuelPressed = false;
	if (AFlyingCabPawn* Pawn = GetFlyingCabPawn())
	{
		Pawn->SetTouchRefuelPressed(false);
	}
}
