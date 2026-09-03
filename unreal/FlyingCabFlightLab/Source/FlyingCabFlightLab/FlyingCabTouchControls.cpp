// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabTouchControls.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "FlyingCabCharacter.h"
#include "FlyingCabCityData.h"
#include "FlyingCabPawn.h"
#include "FlyingCabPlayerController.h"
#include "FlyingCabQuestHubData.h"
#include "TimerManager.h"

namespace
{
	constexpr float MinimapLeft = 10.0f;
	constexpr float MinimapRight = 216.0f;
	constexpr float MinimapTop = 24.0f;
	constexpr float MinimapBottom = 168.0f;
	constexpr int32 MaxPassengerMinimapMarkers = 6;
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
	EventMessageQueue.Reset();
	MajorAnnouncementQueue.Reset();
	bEventMessageVisible = false;
	bMajorAnnouncementVisible = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EventMessageTimerHandle);
	}
	Super::NativeDestruct();
}

void UFlyingCabTouchControls::NativeTick(
	const FGeometry& MyGeometry,
	float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!bMajorAnnouncementVisible || !MajorAnnouncementPanel)
	{
		return;
	}

	MajorAnnouncementElapsed += FMath::Max(0.0f, InDeltaTime);
	const float Duration = FMath::Max(0.6f, ActiveMajorAnnouncement.DurationSeconds);
	const float FadeInDuration = FMath::Min(0.18f, Duration * 0.2f);
	const float FadeOutDuration = FMath::Min(0.28f, Duration * 0.25f);
	float Opacity = 1.0f;
	if (MajorAnnouncementElapsed < FadeInDuration)
	{
		Opacity = MajorAnnouncementElapsed / FMath::Max(KINDA_SMALL_NUMBER, FadeInDuration);
	}
	else if (MajorAnnouncementElapsed > Duration - FadeOutDuration)
	{
		Opacity = (Duration - MajorAnnouncementElapsed)
			/ FMath::Max(KINDA_SMALL_NUMBER, FadeOutDuration);
	}
	Opacity = FMath::Clamp(Opacity, 0.0f, 1.0f);
	MajorAnnouncementPanel->SetRenderOpacity(Opacity);
	const float Scale = FMath::Lerp(0.96f, 1.0f, FMath::Min(1.0f, Opacity * 1.4f));
	MajorAnnouncementPanel->SetRenderScale(FVector2D(Scale));

	if (MajorAnnouncementElapsed >= Duration)
	{
		bMajorAnnouncementVisible = false;
		MajorAnnouncementPanel->SetVisibility(ESlateVisibility::Collapsed);
		ShowNextMajorAnnouncement();
	}
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

void UFlyingCabTouchControls::SetOnFootMode(bool bOnFoot)
{
	ReleaseAllInputs();
	bOnFootMode = bOnFoot;
	RefreshControlMode();
}

void UFlyingCabTouchControls::SetObjectiveText(const FText& Text)
{
	PendingObjectiveText = Text;
	if (ObjectiveText && !ObjectiveText->GetText().EqualTo(PendingObjectiveText))
	{
		ObjectiveText->SetText(PendingObjectiveText);
	}
}

void UFlyingCabTouchControls::SetQuestText(const FText& Text)
{
	PendingQuestText = Text;
	if (QuestText && !QuestText->GetText().EqualTo(PendingQuestText))
	{
		QuestText->SetText(PendingQuestText);
	}
}

void UFlyingCabTouchControls::SetDeveloperObserverState(bool bActive, float ZoomMeters)
{
	if (bActive)
	{
		ReleaseAllInputs();
	}
	bDeveloperObserverMode = bActive;
	PendingDeveloperObserverZoomMeters = FMath::Max(0.0f, ZoomMeters);
	RefreshDeveloperObserverText();
	if (DeveloperObserverPanel)
	{
		DeveloperObserverPanel->SetVisibility(
			bDeveloperObserverMode
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
	}
	RefreshControlMode();
}

void UFlyingCabTouchControls::ShowEventMessage(
	const FText& Text,
	const FLinearColor& Color,
	float DurationSeconds)
{
	if (Text.IsEmpty())
	{
		return;
	}
	FFlyingCabQueuedHudMessage& Message = EventMessageQueue.AddDefaulted_GetRef();
	Message.Text = Text;
	Message.Color = Color;
	Message.DurationSeconds = FMath::Max(0.25f, DurationSeconds);
	if (!bEventMessageVisible)
	{
		ShowNextEventMessage();
	}
}

void UFlyingCabTouchControls::ShowMajorAnnouncement(
	const FText& Title,
	const FText& Detail,
	const FLinearColor& Color,
	float DurationSeconds,
	int32 InPriority)
{
	if (Title.IsEmpty())
	{
		return;
	}
	FFlyingCabQueuedAnnouncement& Announcement = MajorAnnouncementQueue.AddDefaulted_GetRef();
	Announcement.Title = Title;
	Announcement.Detail = Detail;
	Announcement.Color = Color;
	Announcement.DurationSeconds = FMath::Max(0.6f, DurationSeconds);
	Announcement.Priority = InPriority;
	MajorAnnouncementQueue.StableSort([](
		const FFlyingCabQueuedAnnouncement& A,
		const FFlyingCabQueuedAnnouncement& B)
	{
		return A.Priority > B.Priority;
	});
	if (!bMajorAnnouncementVisible)
	{
		ShowNextMajorAnnouncement();
	}
}

void UFlyingCabTouchControls::ClearEventMessage()
{
	bEventMessageVisible = false;
	PendingEventMessageText = FText::GetEmpty();
	if (EventMessageText)
	{
		EventMessageText->SetText(PendingEventMessageText);
	}
	if (EventMessagePanel)
	{
		EventMessagePanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	ShowNextEventMessage();
}

void UFlyingCabTouchControls::ShowNextEventMessage()
{
	if (EventMessageQueue.IsEmpty())
	{
		bEventMessageVisible = false;
		return;
	}
	const FFlyingCabQueuedHudMessage Message = EventMessageQueue[0];
	EventMessageQueue.RemoveAt(0);
	bEventMessageVisible = true;
	PendingEventMessageText = Message.Text;
	PendingEventMessageColor = Message.Color;
	if (EventMessageText)
	{
		EventMessageText->SetText(PendingEventMessageText);
		EventMessageText->SetColorAndOpacity(FSlateColor(PendingEventMessageColor));
	}
	if (EventMessagePanel)
	{
		EventMessagePanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EventMessageTimerHandle);
		World->GetTimerManager().SetTimer(
			EventMessageTimerHandle,
			this,
			&UFlyingCabTouchControls::ClearEventMessage,
			Message.DurationSeconds,
			false);
	}
}

void UFlyingCabTouchControls::ShowNextMajorAnnouncement()
{
	if (MajorAnnouncementQueue.IsEmpty())
	{
		bMajorAnnouncementVisible = false;
		if (MajorAnnouncementPanel)
		{
			MajorAnnouncementPanel->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}
	ActiveMajorAnnouncement = MajorAnnouncementQueue[0];
	MajorAnnouncementQueue.RemoveAt(0);
	MajorAnnouncementElapsed = 0.0f;
	bMajorAnnouncementVisible = true;
	if (MajorAnnouncementTitle)
	{
		MajorAnnouncementTitle->SetText(ActiveMajorAnnouncement.Title);
		MajorAnnouncementTitle->SetColorAndOpacity(
			FSlateColor(ActiveMajorAnnouncement.Color));
	}
	if (MajorAnnouncementDetail)
	{
		MajorAnnouncementDetail->SetText(ActiveMajorAnnouncement.Detail);
	}
	if (MajorAnnouncementPanel)
	{
		MajorAnnouncementPanel->SetRenderOpacity(0.0f);
		MajorAnnouncementPanel->SetRenderScale(FVector2D(0.96f));
		MajorAnnouncementPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UFlyingCabTouchControls::SetTrafficAlert(const FText& Text, const FLinearColor& Color)
{
	PendingTrafficAlertText = Text;
	PendingTrafficAlertColor = Color;
	if (TrafficAlertText)
	{
		if (!TrafficAlertText->GetText().EqualTo(PendingTrafficAlertText))
		{
			TrafficAlertText->SetText(PendingTrafficAlertText);
		}
		TrafficAlertText->SetColorAndOpacity(FSlateColor(PendingTrafficAlertColor));
		const ESlateVisibility DesiredVisibility =
			PendingTrafficAlertText.IsEmpty()
				? ESlateVisibility::Collapsed
				: ESlateVisibility::HitTestInvisible;
		if (TrafficAlertText->GetVisibility() != DesiredVisibility)
		{
			TrafficAlertText->SetVisibility(DesiredVisibility);
		}
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

void UFlyingCabTouchControls::SetPassengerOfferMarkers(
	const FVector2D& CabWorldPosition,
	const TArray<FVector2D>& OfferWorldPositions)
{
	PendingCabWorldPosition = CabWorldPosition;
	PendingPassengerOfferWorldPositions = OfferWorldPositions;
	bHasMinimapState = true;
	UpdateMinimapMarkers();
}

void UFlyingCabTouchControls::SetTimeAttackState(
	bool bActive,
	float ElapsedSeconds,
	int32 Credits,
	int32 TargetCredits)
{
	bPendingTimeAttackActive = bActive;
	PendingTimeAttackSeconds = FMath::Max(0.0f, ElapsedSeconds);
	PendingTimeAttackCredits = FMath::Max(0, Credits);
	PendingTimeAttackTargetCredits = FMath::Max(1, TargetCredits);
	if (TimeAttackPanel)
	{
		const ESlateVisibility DesiredVisibility =
			bPendingTimeAttackActive
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed;
		if (TimeAttackPanel->GetVisibility() != DesiredVisibility)
		{
			TimeAttackPanel->SetVisibility(DesiredVisibility);
		}
	}
	if (TimeAttackText)
	{
		const int32 Minutes = FMath::FloorToInt(PendingTimeAttackSeconds / 60.0f);
		const float Seconds = PendingTimeAttackSeconds - Minutes * 60.0f;
		const FText DisplayText = FText::FromString(FString::Printf(
			TEXT("TIME ATTACK  %02d:%04.1f\nBALANCE  %d / %d CR"),
			Minutes,
			Seconds,
			PendingTimeAttackCredits,
			PendingTimeAttackTargetCredits));
		if (!TimeAttackText->GetText().EqualTo(DisplayText))
		{
			TimeAttackText->SetText(DisplayText);
		}
		const float Progress = static_cast<float>(PendingTimeAttackCredits)
			/ static_cast<float>(PendingTimeAttackTargetCredits);
		TimeAttackText->SetColorAndOpacity(FSlateColor(
			Progress >= 0.9f
				? FLinearColor(0.25f, 1.0f, 0.42f)
				: FLinearColor(1.0f, 0.62f, 0.06f)));
	}
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
		const FString ServiceText = bOnFootMode
			? FString()
			: (bPendingRepairAvailable
				? FString::Printf(TEXT("REPAIR  %d CR/HP  // HOLD E"), PendingRepairPricePerHullUnit)
				: (bPendingRefuelAvailable
					? FString::Printf(TEXT("FUEL SERVICE  %d CR/U  // HOLD E"), PendingRefuelPricePerUnit)
					: FString()));
		const FString FuelCutoffText = !bOnFootMode
			&& !bPendingVehicleDestroyed
			&& PendingFuelPercent <= UE_SMALL_NUMBER
			? FString(TEXT("FUEL EMPTY // THRUST OFF"))
			: FString();
		const FText DisplayText = FText::FromString(FString::Printf(
			TEXT("CREDITS  %d\nFUEL %3.0f%%  |  HULL %3.0f%%\n%s%s%s%s%s"),
			PendingCredits,
			PendingFuelPercent * 100.0f,
			PendingHullPercent * 100.0f,
			*FareText,
			FuelCutoffText.IsEmpty() ? TEXT("") : TEXT("\n"),
			*FuelCutoffText,
			ServiceText.IsEmpty() ? TEXT("") : TEXT("\n"),
			*ServiceText));
		if (!ResourceText->GetText().EqualTo(DisplayText))
		{
			ResourceText->SetText(DisplayText);
		}

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
			!bOnFootMode && (bPendingRefuelAvailable || bPendingRepairAvailable)
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed);
		RefuelButton->SetBackgroundColor(
			bPendingRepairAvailable
				? FLinearColor(0.62f, 0.08f, 0.88f, 0.90f)
				: (bPendingRefuelAvailable
					? FLinearColor(0.05f, 0.65f, 0.28f, 0.88f)
					: FLinearColor(0.02f, 0.45f, 0.70f, 0.88f)));
	}
	RefreshControlMode();
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
	if (AFlyingCabCharacter* Character = GetFlyingCabCharacter())
	{
		Character->SetTouchHorizontalInput(0.0f);
		Character->SetTouchJumpPressed(false);
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
	MinimapSlot->SetSize(FVector2D(228.0f, 180.0f));
	MinimapSlot->SetZOrder(20);

	UTextBlock* MapTitle = WidgetTree->ConstructWidget<UTextBlock>();
	MapTitle->SetText(FText::FromString(TEXT("CITY GRID")));
	MapTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.15f, 0.75f, 0.85f)));
	FSlateFontInfo MapTitleFont = MapTitle->GetFont();
	MapTitleFont.Size = 11;
	MapTitle->SetFont(MapTitleFont);
	UCanvasPanelSlot* MapTitleSlot = MinimapCanvas->AddChildToCanvas(MapTitle);
	MapTitleSlot->SetPosition(FVector2D(8.0f, 3.0f));
	MapTitleSlot->SetSize(FVector2D(210.0f, 18.0f));

	for (const FFlyingCabQuestHubDefinition& Hub : FlyingCabQuestHubData::GetQuestHubs())
	{
		UOverlay* HubMarker = WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(),
			FName(*FString::Printf(TEXT("QuestHub_%s"), *Hub.DisplayName)));
		HubMarker->SetVisibility(ESlateVisibility::HitTestInvisible);

		UTextBlock* Outline = WidgetTree->ConstructWidget<UTextBlock>();
		Outline->SetText(FText::FromString(TEXT("●")));
		Outline->SetJustification(ETextJustify::Center);
		Outline->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.92f, 0.95f, 1.0f)));
		FSlateFontInfo OutlineFont = Outline->GetFont();
		OutlineFont.Size = 28;
		Outline->SetFont(OutlineFont);
		UOverlaySlot* OutlineSlot = HubMarker->AddChildToOverlay(Outline);
		OutlineSlot->SetHorizontalAlignment(HAlign_Center);
		OutlineSlot->SetVerticalAlignment(VAlign_Center);

		UTextBlock* Circle = WidgetTree->ConstructWidget<UTextBlock>();
		Circle->SetText(FText::FromString(TEXT("●")));
		Circle->SetJustification(ETextJustify::Center);
		Circle->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
		FSlateFontInfo CircleFont = Circle->GetFont();
		CircleFont.Size = 25;
		Circle->SetFont(CircleFont);
		UOverlaySlot* CircleSlot = HubMarker->AddChildToOverlay(Circle);
		CircleSlot->SetHorizontalAlignment(HAlign_Center);
		CircleSlot->SetVerticalAlignment(VAlign_Center);

		UTextBlock* Initial = WidgetTree->ConstructWidget<UTextBlock>();
		Initial->SetText(FText::FromString(Hub.MinimapInitial));
		Initial->SetJustification(ETextJustify::Center);
		Initial->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Initial->SetShadowColorAndOpacity(FLinearColor::Black);
		FSlateFontInfo InitialFont = Initial->GetFont();
		InitialFont.Size = 14;
		Initial->SetFont(InitialFont);
		UOverlaySlot* InitialSlot = HubMarker->AddChildToOverlay(Initial);
		InitialSlot->SetHorizontalAlignment(HAlign_Center);
		InitialSlot->SetVerticalAlignment(VAlign_Center);

		UCanvasPanelSlot* HubSlot = MinimapCanvas->AddChildToCanvas(HubMarker);
		HubSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		HubSlot->SetPosition(WorldToMinimap(Hub.MinimapWorldPosition));
		HubSlot->SetSize(FVector2D(30.0f, 30.0f));
		HubSlot->SetZOrder(8);
	}

	const TConstArrayView<FFlyingCabDistrictDefinition> Districts =
		FlyingCabCityData::GetDistricts();
	for (int32 Index = 0; Index < Districts.Num(); ++Index)
	{
		const FFlyingCabDistrictDefinition& District = Districts[Index];
		const FVector2D WorldPosition = District.GetMapPosition();
		const FVector2D PinPosition = WorldToMinimap(WorldPosition);
		AddMinimapPoint(
			MinimapCanvas,
			FName(*FString::Printf(TEXT("StopPin%d"), Index)),
			WorldPosition,
			FVector2D(7.0f, 7.0f),
			FLinearColor(0.25f, 0.32f, 0.38f, 0.9f));

		UTextBlock* StopCode = WidgetTree->ConstructWidget<UTextBlock>();
		StopCode->SetText(FText::FromString(FString(District.MinimapCode)));
		StopCode->SetColorAndOpacity(FSlateColor(FLinearColor(0.42f, 0.55f, 0.62f)));
		FSlateFontInfo StopFont = StopCode->GetFont();
		StopFont.Size = 9;
		StopCode->SetFont(StopFont);
		UCanvasPanelSlot* StopCodeSlot = MinimapCanvas->AddChildToCanvas(StopCode);
		StopCodeSlot->SetPosition(PinPosition + FVector2D(6.0f, -7.0f));
		StopCodeSlot->SetSize(FVector2D(30.0f, 16.0f));
	}

	const TArray<FFlyingCabServiceDefinition> FuelStations =
		FlyingCabCityData::GetFuelStations();
	for (int32 Index = 0; Index < FuelStations.Num(); ++Index)
	{
		const FVector2D WorldPosition = FuelStations[Index].GetMapPosition();
		const FVector2D FuelStationMapPosition = WorldToMinimap(WorldPosition);
		AddMinimapPoint(
			MinimapCanvas,
			FName(*FString::Printf(TEXT("FuelStationPin%d"), Index)),
			WorldPosition,
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

	const TArray<FFlyingCabServiceDefinition> RepairStations =
		FlyingCabCityData::GetRepairStations();
	for (int32 Index = 0; Index < RepairStations.Num(); ++Index)
	{
		const FVector2D WorldPosition = RepairStations[Index].GetMapPosition();
		const FVector2D RepairStationMapPosition = WorldToMinimap(WorldPosition);
		AddMinimapPoint(
			MinimapCanvas,
			FName(*FString::Printf(TEXT("RepairStationPin%d"), Index)),
			WorldPosition,
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
	}

	PassengerOfferMarkers.Reset();
	for (int32 Index = 0; Index < MaxPassengerMinimapMarkers; ++Index)
	{
		UBorder* Marker = AddMinimapPoint(
			MinimapCanvas,
			FName(*FString::Printf(TEXT("PassengerOfferMarker%d"), Index)),
			FVector2D::ZeroVector,
			FVector2D(12.0f, 12.0f),
			FLinearColor(1.0f, 0.78f, 0.05f, 1.0f));
		Marker->SetVisibility(ESlateVisibility::Collapsed);
		PassengerOfferMarkers.Add(Marker);
	}

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

	QuestText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("QuestText"));
	QuestText->SetText(PendingQuestText);
	QuestText->SetJustification(ETextJustify::Right);
	QuestText->SetAutoWrapText(true);
	QuestText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.70f, 0.12f)));
	QuestText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
	QuestText->SetShadowOffset(FVector2D(1.0f, 1.0f));
	FSlateFontInfo QuestFont = QuestText->GetFont();
	QuestFont.Size = 15;
	QuestText->SetFont(QuestFont);

	UCanvasPanelSlot* QuestSlot = RootCanvas->AddChildToCanvas(QuestText);
	QuestSlot->SetAnchors(FAnchors(1.0f, 0.0f));
	QuestSlot->SetAlignment(FVector2D(1.0f, 0.0f));
	QuestSlot->SetPosition(FVector2D(-12.0f, 12.0f));
	QuestSlot->SetSize(FVector2D(245.0f, 68.0f));
	QuestSlot->SetZOrder(20);

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

	EventMessagePanel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("EventMessagePanel"));
	EventMessagePanel->SetBrushColor(FLinearColor(0.005f, 0.012f, 0.025f, 0.94f));
	EventMessagePanel->SetPadding(FMargin(16.0f, 10.0f));
	EventMessageText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("EventMessageText"));
	EventMessageText->SetText(PendingEventMessageText);
	EventMessageText->SetJustification(ETextJustify::Center);
	EventMessageText->SetAutoWrapText(true);
	EventMessageText->SetColorAndOpacity(FSlateColor(PendingEventMessageColor));
	EventMessageText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
	EventMessageText->SetShadowOffset(FVector2D(2.0f, 2.0f));
	FSlateFontInfo EventMessageFont = EventMessageText->GetFont();
	EventMessageFont.Size = 20;
	EventMessageText->SetFont(EventMessageFont);
	EventMessagePanel->AddChild(EventMessageText);
	EventMessagePanel->SetVisibility(
		PendingEventMessageText.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* EventMessageSlot = RootCanvas->AddChildToCanvas(EventMessagePanel);
	EventMessageSlot->SetAnchors(FAnchors(0.5f, 0.0f));
	EventMessageSlot->SetAlignment(FVector2D(0.5f, 0.0f));
	EventMessageSlot->SetPosition(FVector2D(0.0f, 154.0f));
	EventMessageSlot->SetSize(FVector2D(520.0f, 72.0f));
	EventMessageSlot->SetZOrder(40);

	MajorAnnouncementPanel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("MajorAnnouncementPanel"));
	MajorAnnouncementPanel->SetBrushColor(FLinearColor(0.002f, 0.006f, 0.015f, 0.94f));
	MajorAnnouncementPanel->SetPadding(FMargin(28.0f, 18.0f));
	UVerticalBox* AnnouncementContent = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(),
		TEXT("MajorAnnouncementContent"));
	MajorAnnouncementPanel->AddChild(AnnouncementContent);
	MajorAnnouncementTitle = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("MajorAnnouncementTitle"));
	MajorAnnouncementTitle->SetJustification(ETextJustify::Center);
	MajorAnnouncementTitle->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	MajorAnnouncementTitle->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
	MajorAnnouncementTitle->SetShadowOffset(FVector2D(3.0f, 3.0f));
	FSlateFontInfo AnnouncementTitleFont = MajorAnnouncementTitle->GetFont();
	AnnouncementTitleFont.Size = 50;
	MajorAnnouncementTitle->SetFont(AnnouncementTitleFont);
	UVerticalBoxSlot* AnnouncementTitleSlot =
		AnnouncementContent->AddChildToVerticalBox(MajorAnnouncementTitle);
	AnnouncementTitleSlot->SetPadding(FMargin(4.0f, 0.0f, 4.0f, 8.0f));
	AnnouncementTitleSlot->SetHorizontalAlignment(HAlign_Fill);

	MajorAnnouncementDetail = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("MajorAnnouncementDetail"));
	MajorAnnouncementDetail->SetJustification(ETextJustify::Center);
	MajorAnnouncementDetail->SetAutoWrapText(true);
	MajorAnnouncementDetail->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	MajorAnnouncementDetail->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
	MajorAnnouncementDetail->SetShadowOffset(FVector2D(2.0f, 2.0f));
	FSlateFontInfo AnnouncementDetailFont = MajorAnnouncementDetail->GetFont();
	AnnouncementDetailFont.Size = 23;
	MajorAnnouncementDetail->SetFont(AnnouncementDetailFont);
	UVerticalBoxSlot* AnnouncementDetailSlot =
		AnnouncementContent->AddChildToVerticalBox(MajorAnnouncementDetail);
	AnnouncementDetailSlot->SetHorizontalAlignment(HAlign_Fill);

	MajorAnnouncementPanel->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* MajorAnnouncementSlot =
		RootCanvas->AddChildToCanvas(MajorAnnouncementPanel);
	MajorAnnouncementSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	MajorAnnouncementSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	MajorAnnouncementSlot->SetPosition(FVector2D(0.0f, -20.0f));
	MajorAnnouncementSlot->SetSize(FVector2D(940.0f, 178.0f));
	MajorAnnouncementSlot->SetZOrder(80);

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

	DeveloperObserverPanel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("DeveloperObserverPanel"));
	DeveloperObserverPanel->SetBrushColor(FLinearColor(0.005f, 0.02f, 0.035f, 0.95f));
	DeveloperObserverPanel->SetPadding(FMargin(18.0f, 10.0f));
	DeveloperObserverText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("DeveloperObserverText"));
	DeveloperObserverText->SetJustification(ETextJustify::Center);
	DeveloperObserverText->SetColorAndOpacity(
		FSlateColor(FLinearColor(0.10f, 0.95f, 1.0f)));
	DeveloperObserverText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
	DeveloperObserverText->SetShadowOffset(FVector2D(2.0f, 2.0f));
	FSlateFontInfo DeveloperObserverFont = DeveloperObserverText->GetFont();
	DeveloperObserverFont.Size = 18;
	DeveloperObserverText->SetFont(DeveloperObserverFont);
	DeveloperObserverPanel->AddChild(DeveloperObserverText);
	DeveloperObserverPanel->SetVisibility(
		bDeveloperObserverMode
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	UCanvasPanelSlot* DeveloperObserverSlot =
		RootCanvas->AddChildToCanvas(DeveloperObserverPanel);
	DeveloperObserverSlot->SetAnchors(FAnchors(0.5f, 0.0f));
	DeveloperObserverSlot->SetAlignment(FVector2D(0.5f, 0.0f));
	DeveloperObserverSlot->SetPosition(FVector2D(0.0f, 16.0f));
	DeveloperObserverSlot->SetSize(FVector2D(760.0f, 82.0f));
	DeveloperObserverSlot->SetZOrder(100);
	RefreshDeveloperObserverText();

	TimeAttackPanel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("TimeAttackPanel"));
	TimeAttackPanel->SetBrushColor(FLinearColor(0.015f, 0.025f, 0.055f, 0.93f));
	TimeAttackPanel->SetPadding(FMargin(10.0f, 7.0f));
	TimeAttackText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("TimeAttackText"));
	TimeAttackText->SetJustification(ETextJustify::Center);
	TimeAttackText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
	TimeAttackText->SetShadowOffset(FVector2D(2.0f, 2.0f));
	FSlateFontInfo TimeAttackFont = TimeAttackText->GetFont();
	TimeAttackFont.Size = 18;
	TimeAttackText->SetFont(TimeAttackFont);
	TimeAttackPanel->AddChild(TimeAttackText);
	UCanvasPanelSlot* TimeAttackSlot = RootCanvas->AddChildToCanvas(TimeAttackPanel);
	TimeAttackSlot->SetAnchors(FAnchors(0.5f, 0.0f));
	TimeAttackSlot->SetAlignment(FVector2D(0.5f, 0.0f));
	TimeAttackSlot->SetPosition(FVector2D(0.0f, 78.0f));
	TimeAttackSlot->SetSize(FVector2D(320.0f, 68.0f));
	TimeAttackSlot->SetZOrder(25);
	SetTimeAttackState(
		bPendingTimeAttackActive,
		PendingTimeAttackSeconds,
		PendingTimeAttackCredits,
		PendingTimeAttackTargetCredits);

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
	InteractButton = AddControlButton(
		RootCanvas,
		TEXT("InteractButton"),
		TEXT("EXIT"),
		FAnchors(1.0f, 0.0f),
		FVector2D(1.0f, 0.0f),
		FVector2D(-264.0f, 24.0f),
		FVector2D(104.0f, 56.0f),
		FLinearColor(0.02f, 0.45f, 0.70f, 0.88f));
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
	ThrustButtonText = Cast<UTextBlock>(ThrustButton->GetContent());
	InteractButtonText = Cast<UTextBlock>(InteractButton->GetContent());

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
	InteractButton->OnPressed.AddDynamic(this, &UFlyingCabTouchControls::HandleInteractPressed);
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
	RefreshControlMode();
}

FVector2D UFlyingCabTouchControls::WorldToMinimap(const FVector2D& WorldPosition) const
{
	const FVector2D MinimapWorldMin = FlyingCabCityData::GetMinimapWorldMin();
	const FVector2D MinimapWorldMax = FlyingCabCityData::GetMinimapWorldMax();
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
	for (int32 Index = 0; Index < PassengerOfferMarkers.Num(); ++Index)
	{
		UBorder* Marker = PassengerOfferMarkers[Index];
		const bool bMarkerVisible = PendingPassengerOfferWorldPositions.IsValidIndex(Index);
		Marker->SetVisibility(
			bMarkerVisible
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
		if (bMarkerVisible)
		{
			if (UCanvasPanelSlot* MarkerSlot = Cast<UCanvasPanelSlot>(Marker->Slot))
			{
				MarkerSlot->SetPosition(WorldToMinimap(PendingPassengerOfferWorldPositions[Index]));
			}
		}
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

AFlyingCabCharacter* UFlyingCabTouchControls::GetFlyingCabCharacter() const
{
	return Cast<AFlyingCabCharacter>(GetOwningPlayerPawn());
}

void UFlyingCabTouchControls::RefreshControlMode()
{
	const ESlateVisibility PrimaryControlVisibility = bDeveloperObserverMode
		? ESlateVisibility::Collapsed
		: ESlateVisibility::Visible;
	if (LeftButton)
	{
		LeftButton->SetVisibility(PrimaryControlVisibility);
	}
	if (RightButton)
	{
		RightButton->SetVisibility(PrimaryControlVisibility);
	}
	if (ThrustButton)
	{
		ThrustButton->SetVisibility(PrimaryControlVisibility);
	}
	if (InteractButton)
	{
		InteractButton->SetVisibility(PrimaryControlVisibility);
	}
	if (ThrustButtonText)
	{
		ThrustButtonText->SetText(FText::FromString(bOnFootMode ? TEXT("JUMP") : TEXT("THRUST")));
	}
	if (ResetButton)
	{
		ResetButton->SetVisibility(
			bDeveloperObserverMode || bOnFootMode
				? ESlateVisibility::Collapsed
				: ESlateVisibility::Visible);
	}
	if (RefuelButton)
	{
		RefuelButton->SetVisibility(
			!bDeveloperObserverMode && !bOnFootMode
				&& (bPendingRefuelAvailable || bPendingRepairAvailable)
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed);
	}
	if (ServiceButtonText)
	{
		const TCHAR* Label = bPendingRepairAvailable ? TEXT("REPAIR") : TEXT("REFUEL");
		ServiceButtonText->SetText(FText::FromString(Label));
	}
	if (InteractButtonText)
	{
		InteractButtonText->SetText(FText::FromString(bOnFootMode ? TEXT("ENTER") : TEXT("EXIT")));
	}
}

void UFlyingCabTouchControls::RefreshDeveloperObserverText()
{
	if (!DeveloperObserverText)
	{
		return;
	}
	DeveloperObserverText->SetText(FText::FromString(FString::Printf(
		TEXT("DEV OBSERVER  //  O RETURN  //  WASD / ARROWS PAN  //  PGUP-PGDN ZOOM  //  HOME RECENTER  //  SHIFT FAST\nZOOM %.0f M"),
		PendingDeveloperObserverZoomMeters)));
}

void UFlyingCabTouchControls::UpdateHorizontalInput()
{
	if (bDeveloperObserverMode)
	{
		bLeftPressed = false;
		bRightPressed = false;
	}
	if (AFlyingCabPawn* Pawn = GetFlyingCabPawn())
	{
		const float HorizontalInput = static_cast<float>(bRightPressed) - static_cast<float>(bLeftPressed);
		Pawn->SetTouchHorizontalInput(HorizontalInput);
	}
	if (AFlyingCabCharacter* Character = GetFlyingCabCharacter())
	{
		const float HorizontalInput = static_cast<float>(bRightPressed) - static_cast<float>(bLeftPressed);
		Character->SetTouchHorizontalInput(HorizontalInput);
	}
}

void UFlyingCabTouchControls::HandleLeftPressed()
{
	if (bDeveloperObserverMode)
	{
		return;
	}
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
	if (bDeveloperObserverMode)
	{
		return;
	}
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
	if (bDeveloperObserverMode)
	{
		return;
	}
	bThrustPressed = true;
	if (AFlyingCabPawn* Pawn = GetFlyingCabPawn())
	{
		Pawn->SetTouchThrustPressed(true);
	}
	if (AFlyingCabCharacter* Character = GetFlyingCabCharacter())
	{
		Character->SetTouchJumpPressed(true);
	}
}

void UFlyingCabTouchControls::HandleThrustReleased()
{
	bThrustPressed = false;
	if (AFlyingCabPawn* Pawn = GetFlyingCabPawn())
	{
		Pawn->SetTouchThrustPressed(false);
	}
	if (AFlyingCabCharacter* Character = GetFlyingCabCharacter())
	{
		Character->SetTouchJumpPressed(false);
	}
}

void UFlyingCabTouchControls::HandleResetPressed()
{
	if (bDeveloperObserverMode)
	{
		return;
	}
	if (AFlyingCabPawn* Pawn = GetFlyingCabPawn())
	{
		Pawn->ResetVehicle();
	}
}

void UFlyingCabTouchControls::HandleInteractPressed()
{
	if (bDeveloperObserverMode)
	{
		return;
	}
	if (AFlyingCabPlayerController* PlayerController =
		Cast<AFlyingCabPlayerController>(GetOwningPlayer()))
	{
		PlayerController->RequestContextInteraction();
	}
}

void UFlyingCabTouchControls::HandleRefuelPressed()
{
	if (bDeveloperObserverMode)
	{
		return;
	}
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
