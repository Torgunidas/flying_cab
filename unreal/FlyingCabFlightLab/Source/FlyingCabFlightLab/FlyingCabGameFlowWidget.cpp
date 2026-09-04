// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabGameFlowWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "FlyingCabPlayerController.h"

namespace
{
	UTextBlock* CreateFlowText(
		UWidgetTree* WidgetTree,
		const TCHAR* Name,
		int32 FontSize,
		const FLinearColor& Color)
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			FName(Name));
		Text->SetJustification(ETextJustify::Center);
		Text->SetAutoWrapText(true);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
		Text->SetShadowOffset(FVector2D(2.0f, 2.0f));
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = FontSize;
		Text->SetFont(Font);
		return Text;
	}
}

TSharedRef<SWidget> UFlyingCabGameFlowWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}
	return Super::RebuildWidget();
}

void UFlyingCabGameFlowWidget::ShowModeSelection(
	const TArray<float>& BestTimes,
	int32 TargetCredits)
{
	bShowingResults = false;
	SetVisibility(ESlateVisibility::Visible);
	TitleText->SetText(FText::FromString(TEXT("FLYING CAB")));
	BodyText->SetText(FText::FromString(FString::Printf(
		TEXT("CHOOSE YOUR SHIFT\n\n")
		TEXT("TIME ATTACK\nReach a balance of %d CR as fast as possible.\n")
		TEXT("Fares and clean near misses earn credits. Fuel, repairs and towing cost time.\n\n")
		TEXT("FREE ROAM\nThe current open-ended city prototype."),
		FMath::Max(1, TargetCredits))));
	LeaderboardText->SetText(FText::FromString(BuildLeaderboardText(BestTimes)));
	SetButtonLabel(PrimaryButton, TEXT("TIME ATTACK"));
	SetButtonLabel(SecondaryButton, TEXT("FREE ROAM"));
	TertiaryButton->SetVisibility(ESlateVisibility::Collapsed);
}

void UFlyingCabGameFlowWidget::ShowTimeAttackResults(
	const FFlyingCabTimeAttackResult& Result,
	const TArray<float>& BestTimes)
{
	bShowingResults = true;
	SetVisibility(ESlateVisibility::Visible);
	TitleText->SetText(FText::FromString(FString::Printf(
		TEXT("SHIFT COMPLETE  //  %s"),
		*FormatTime(Result.ElapsedSeconds))));
	BodyText->SetText(FText::FromString(FString::Printf(
		TEXT("FINAL BALANCE  %d / %d CR\n")
		TEXT("DELIVERIES  %d  //  +%d CR\n")
		TEXT("CLEAN NEAR MISSES  %d  //  +%d CR\n")
		TEXT("FUEL  -%d CR  //  REPAIRS  -%d CR\n")
		TEXT("TOWS  %d  //  -%d CR"),
		Result.FinalCredits,
		Result.TargetCredits,
		Result.CompletedDeliveries,
		Result.DeliveryCreditsEarned,
		Result.NearMissCount,
		Result.NearMissCreditsEarned,
		Result.FuelCreditsSpent,
		Result.RepairCreditsSpent,
		Result.TowCount,
		Result.TowCreditsSpent)));
	LeaderboardText->SetText(FText::FromString(BuildLeaderboardText(BestTimes)));
	SetButtonLabel(PrimaryButton, TEXT("RETRY TIME ATTACK"));
	SetButtonLabel(SecondaryButton, TEXT("FREE ROAM"));
	SetButtonLabel(TertiaryButton, TEXT("MAIN MENU"));
	TertiaryButton->SetVisibility(ESlateVisibility::Visible);
}

void UFlyingCabGameFlowWidget::HideFlowScreen()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UFlyingCabGameFlowWidget::BuildWidgetTree()
{
	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(),
		TEXT("GameFlowRoot"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("Backdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.002f, 0.006f, 0.016f, 0.97f));
	UCanvasPanelSlot* BackdropSlot = RootCanvas->AddChildToCanvas(Backdrop);
	BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	BackdropSlot->SetOffsets(FMargin(0.0f));

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("FlowPanel"));
	Panel->SetBrushColor(FLinearColor(0.015f, 0.035f, 0.065f, 0.98f));
	Panel->SetPadding(FMargin(34.0f, 26.0f));
	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(Panel);
	PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	PanelSlot->SetSize(FVector2D(640.0f, 650.0f));
	PanelSlot->SetZOrder(10);

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(),
		TEXT("FlowContent"));
	Panel->AddChild(Content);

	TitleText = CreateFlowText(
		WidgetTree,
		TEXT("TitleText"),
		38,
		FLinearColor(0.10f, 0.93f, 1.0f));
	UVerticalBoxSlot* TitleSlot = Content->AddChildToVerticalBox(TitleText);
	TitleSlot->SetPadding(FMargin(4.0f, 0.0f, 4.0f, 16.0f));
	TitleSlot->SetHorizontalAlignment(HAlign_Fill);

	BodyText = CreateFlowText(
		WidgetTree,
		TEXT("BodyText"),
		17,
		FLinearColor(0.78f, 0.88f, 0.92f));
	UVerticalBoxSlot* BodySlot = Content->AddChildToVerticalBox(BodyText);
	BodySlot->SetPadding(FMargin(10.0f, 0.0f, 10.0f, 14.0f));
	BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	LeaderboardText = CreateFlowText(
		WidgetTree,
		TEXT("LeaderboardText"),
		16,
		FLinearColor(1.0f, 0.66f, 0.08f));
	UVerticalBoxSlot* LeaderboardSlot = Content->AddChildToVerticalBox(LeaderboardText);
	LeaderboardSlot->SetPadding(FMargin(8.0f, 4.0f, 8.0f, 16.0f));
	LeaderboardSlot->SetHorizontalAlignment(HAlign_Fill);

	auto AddFlowButton = [this, Content](
		const TCHAR* Name,
		const FLinearColor& Color) -> UButton*
	{
		USizeBox* ButtonSize = WidgetTree->ConstructWidget<USizeBox>();
		ButtonSize->SetHeightOverride(54.0f);
		UButton* Button = WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(),
			FName(Name));
		Button->SetBackgroundColor(Color);
		UTextBlock* Label = CreateFlowText(
			WidgetTree,
			*FString::Printf(TEXT("%sLabel"), Name),
			19,
			FLinearColor::White);
		Button->AddChild(Label);
		ButtonSize->AddChild(Button);
		UVerticalBoxSlot* Slot = Content->AddChildToVerticalBox(ButtonSize);
		Slot->SetPadding(FMargin(58.0f, 5.0f));
		Slot->SetHorizontalAlignment(HAlign_Fill);
		return Button;
	};

	PrimaryButton = AddFlowButton(
		TEXT("PrimaryButton"),
		FLinearColor(0.90f, 0.18f, 0.025f, 0.95f));
	SecondaryButton = AddFlowButton(
		TEXT("SecondaryButton"),
		FLinearColor(0.02f, 0.48f, 0.72f, 0.95f));
	TertiaryButton = AddFlowButton(
		TEXT("TertiaryButton"),
		FLinearColor(0.14f, 0.16f, 0.20f, 0.95f));

	PrimaryButton->OnClicked.AddDynamic(this, &UFlyingCabGameFlowWidget::HandlePrimaryClicked);
	SecondaryButton->OnClicked.AddDynamic(this, &UFlyingCabGameFlowWidget::HandleSecondaryClicked);
	TertiaryButton->OnClicked.AddDynamic(this, &UFlyingCabGameFlowWidget::HandleTertiaryClicked);
}

void UFlyingCabGameFlowWidget::SetButtonLabel(UButton* Button, const FString& Label) const
{
	if (Button)
	{
		if (UTextBlock* Text = Cast<UTextBlock>(Button->GetContent()))
		{
			Text->SetText(FText::FromString(Label));
		}
	}
}

FString UFlyingCabGameFlowWidget::BuildLeaderboardText(const TArray<float>& BestTimes) const
{
	FString Text(TEXT("LOCAL BEST TIMES"));
	if (BestTimes.IsEmpty())
	{
		return Text + TEXT("\nNO COMPLETED SHIFTS");
	}

	for (int32 Index = 0; Index < BestTimes.Num(); ++Index)
	{
		Text += FString::Printf(
			TEXT("\n%d.  %s"),
			Index + 1,
			*FormatTime(BestTimes[Index]));
	}
	return Text;
}

FString UFlyingCabGameFlowWidget::FormatTime(float Seconds)
{
	const float SafeSeconds = FMath::Max(0.0f, Seconds);
	const int32 Minutes = FMath::FloorToInt(SafeSeconds / 60.0f);
	const float RemainingSeconds = SafeSeconds - Minutes * 60.0f;
	return FString::Printf(TEXT("%02d:%04.1f"), Minutes, RemainingSeconds);
}

void UFlyingCabGameFlowWidget::HandlePrimaryClicked()
{
	if (AFlyingCabPlayerController* Controller = Cast<AFlyingCabPlayerController>(GetOwningPlayer()))
	{
		if (bShowingResults)
		{
			Controller->RestartWithRunMode(EFlyingCabRunMode::TimeAttack);
		}
		else
		{
			Controller->StartRunMode(EFlyingCabRunMode::TimeAttack);
		}
	}
}

void UFlyingCabGameFlowWidget::HandleSecondaryClicked()
{
	if (AFlyingCabPlayerController* Controller = Cast<AFlyingCabPlayerController>(GetOwningPlayer()))
	{
		if (bShowingResults)
		{
			Controller->RestartWithRunMode(EFlyingCabRunMode::Freeroam);
		}
		else
		{
			Controller->StartRunMode(EFlyingCabRunMode::Freeroam);
		}
	}
}

void UFlyingCabGameFlowWidget::HandleTertiaryClicked()
{
	if (AFlyingCabPlayerController* Controller = Cast<AFlyingCabPlayerController>(GetOwningPlayer()))
	{
		Controller->ReturnToModeSelection();
	}
}
