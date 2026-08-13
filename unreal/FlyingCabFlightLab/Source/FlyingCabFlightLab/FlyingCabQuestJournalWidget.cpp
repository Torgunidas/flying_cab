// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabQuestJournalWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "FlyingCabPlayerController.h"
#include "FlyingCabQuestSubsystem.h"
#include "InputCoreTypes.h"

namespace
{
	UTextBlock* CreateJournalText(
		UWidgetTree* WidgetTree,
		const TCHAR* Name,
		int32 FontSize,
		const FLinearColor& Color,
		ETextJustify::Type Justification = ETextJustify::Left)
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			FName(Name));
		Text->SetJustification(Justification);
		Text->SetAutoWrapText(true);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
		Text->SetShadowOffset(FVector2D(1.0f, 1.0f));
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = FontSize;
		Text->SetFont(Font);
		return Text;
	}

	UButton* CreateJournalButton(
		UWidgetTree* WidgetTree,
		const TCHAR* Name,
		const FText& Label,
		const FLinearColor& Color,
		int32 FontSize = 17)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(),
			FName(Name));
		Button->SetBackgroundColor(Color);
		UTextBlock* Text = CreateJournalText(
			WidgetTree,
			*FString::Printf(TEXT("%sLabel"), Name),
			FontSize,
			FLinearColor::White,
			ETextJustify::Center);
		Text->SetText(Label);
		Button->AddChild(Text);
		return Button;
	}

	bool IsActiveJournalStatus(EFlyingCabQuestStatus Status)
	{
		return Status == EFlyingCabQuestStatus::Active
			|| Status == EFlyingCabQuestStatus::ReadyToTurnIn;
	}
}

void UFlyingCabQuestJournalEntryButton::InitializeEntry(
	UFlyingCabQuestJournalWidget* InOwner,
	FName InQuestId)
{
	JournalOwner = InOwner;
	QuestId = InQuestId;
	OnClicked.RemoveDynamic(this, &UFlyingCabQuestJournalEntryButton::HandleEntryClicked);
	OnClicked.AddDynamic(this, &UFlyingCabQuestJournalEntryButton::HandleEntryClicked);
}

void UFlyingCabQuestJournalEntryButton::HandleEntryClicked()
{
	if (JournalOwner.IsValid())
	{
		JournalOwner->SelectQuest(QuestId);
	}
}

TSharedRef<SWidget> UFlyingCabQuestJournalWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}
	return Super::RebuildWidget();
}

void UFlyingCabQuestJournalWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	if (UFlyingCabQuestSubsystem* Quests = GetQuestSubsystem())
	{
		Quests->OnQuestStateChanged.RemoveDynamic(
			this,
			&UFlyingCabQuestJournalWidget::HandleQuestStateChanged);
		Quests->OnQuestStateChanged.AddDynamic(
			this,
			&UFlyingCabQuestJournalWidget::HandleQuestStateChanged);
		Quests->OnTrackedQuestChanged.RemoveDynamic(
			this,
			&UFlyingCabQuestJournalWidget::HandleTrackedQuestChanged);
		Quests->OnTrackedQuestChanged.AddDynamic(
			this,
			&UFlyingCabQuestJournalWidget::HandleTrackedQuestChanged);
	}
	RefreshJournal();
}

void UFlyingCabQuestJournalWidget::NativeDestruct()
{
	if (UFlyingCabQuestSubsystem* Quests = GetQuestSubsystem())
	{
		Quests->OnQuestStateChanged.RemoveDynamic(
			this,
			&UFlyingCabQuestJournalWidget::HandleQuestStateChanged);
		Quests->OnTrackedQuestChanged.RemoveDynamic(
			this,
			&UFlyingCabQuestJournalWidget::HandleTrackedQuestChanged);
	}
	Super::NativeDestruct();
}

FReply UFlyingCabQuestJournalWidget::NativeOnKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::J)
	{
		if (AFlyingCabPlayerController* Controller =
			Cast<AFlyingCabPlayerController>(GetOwningPlayer()))
		{
			Controller->CloseQuestJournal();
		}
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UFlyingCabQuestJournalWidget::ShowJournal()
{
	SetVisibility(ESlateVisibility::Visible);
	RefreshJournal();
	SetKeyboardFocus();
}

void UFlyingCabQuestJournalWidget::HideJournal()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UFlyingCabQuestJournalWidget::RefreshJournal()
{
	const UFlyingCabQuestSubsystem* Quests = GetQuestSubsystem();
	const TArray<FFlyingCabQuestJournalEntry> Entries = Quests
		? Quests->GetJournalEntries()
		: TArray<FFlyingCabQuestJournalEntry>();
	RefreshQuestList(Entries);
	RefreshQuestDetails(Entries);
}

void UFlyingCabQuestJournalWidget::SelectQuest(FName QuestId)
{
	SelectedQuestId = QuestId;
	RefreshJournal();
}

void UFlyingCabQuestJournalWidget::BuildWidgetTree()
{
	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(),
		TEXT("QuestJournalRoot"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("QuestJournalBackdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.001f, 0.004f, 0.012f, 0.88f));
	UCanvasPanelSlot* BackdropSlot = RootCanvas->AddChildToCanvas(Backdrop);
	BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	BackdropSlot->SetOffsets(FMargin(0.0f));

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("QuestJournalPanel"));
	Panel->SetBrushColor(FLinearColor(0.012f, 0.030f, 0.055f, 0.99f));
	Panel->SetPadding(FMargin(28.0f, 22.0f));
	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(Panel);
	PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	PanelSlot->SetSize(FVector2D(980.0f, 680.0f));
	PanelSlot->SetZOrder(10);

	UVerticalBox* MainColumn = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(),
		TEXT("QuestJournalContent"));
	Panel->AddChild(MainColumn);

	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(),
		TEXT("QuestJournalHeader"));
	UVerticalBoxSlot* HeaderSlot = MainColumn->AddChildToVerticalBox(Header);
	HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));

	UTextBlock* HeaderText = CreateJournalText(
		WidgetTree,
		TEXT("QuestJournalTitle"),
		36,
		FLinearColor(0.10f, 0.93f, 1.0f));
	HeaderText->SetText(FText::FromString(TEXT("SHIFT LOG")));
	UHorizontalBoxSlot* HeaderTextSlot = Header->AddChildToHorizontalBox(HeaderText);
	HeaderTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	HeaderTextSlot->SetVerticalAlignment(VAlign_Center);

	UButton* CloseButton = CreateJournalButton(
		WidgetTree,
		TEXT("QuestJournalClose"),
		FText::FromString(TEXT("CLOSE  [J / ESC]")),
		FLinearColor(0.16f, 0.19f, 0.24f, 1.0f),
		15);
	CloseButton->OnClicked.AddDynamic(this, &UFlyingCabQuestJournalWidget::HandleCloseClicked);
	UHorizontalBoxSlot* CloseSlot = Header->AddChildToHorizontalBox(CloseButton);
	CloseSlot->SetPadding(FMargin(12.0f, 2.0f));
	CloseSlot->SetVerticalAlignment(VAlign_Fill);

	UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(),
		TEXT("QuestJournalTabs"));
	UVerticalBoxSlot* TabsSlot = MainColumn->AddChildToVerticalBox(Tabs);
	TabsSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));

	ActiveTabButton = CreateJournalButton(
		WidgetTree,
		TEXT("ActiveQuestTab"),
		FText::FromString(TEXT("ACTIVE")),
		FLinearColor(0.02f, 0.48f, 0.72f, 1.0f));
	CompletedTabButton = CreateJournalButton(
		WidgetTree,
		TEXT("CompletedQuestTab"),
		FText::FromString(TEXT("COMPLETED")),
		FLinearColor(0.10f, 0.14f, 0.19f, 1.0f));
	ActiveTabButton->OnClicked.AddDynamic(
		this,
		&UFlyingCabQuestJournalWidget::HandleActiveTabClicked);
	CompletedTabButton->OnClicked.AddDynamic(
		this,
		&UFlyingCabQuestJournalWidget::HandleCompletedTabClicked);
	for (UButton* Tab : {ActiveTabButton.Get(), CompletedTabButton.Get()})
	{
		UHorizontalBoxSlot* TabSlot = Tabs->AddChildToHorizontalBox(Tab);
		TabSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TabSlot->SetPadding(FMargin(4.0f, 0.0f));
	}

	UHorizontalBox* Body = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(),
		TEXT("QuestJournalBody"));
	UVerticalBoxSlot* BodySlot = MainColumn->AddChildToVerticalBox(Body);
	BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UBorder* ListPanel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("QuestListPanel"));
	ListPanel->SetBrushColor(FLinearColor(0.006f, 0.016f, 0.031f, 0.98f));
	ListPanel->SetPadding(FMargin(10.0f));
	QuestList = WidgetTree->ConstructWidget<UScrollBox>(
		UScrollBox::StaticClass(),
		TEXT("QuestList"));
	ListPanel->AddChild(QuestList);
	UHorizontalBoxSlot* ListSlot = Body->AddChildToHorizontalBox(ListPanel);
	ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ListSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));

	UBorder* DetailsPanel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("QuestDetailsPanel"));
	DetailsPanel->SetBrushColor(FLinearColor(0.018f, 0.045f, 0.075f, 0.98f));
	DetailsPanel->SetPadding(FMargin(24.0f, 20.0f));
	UHorizontalBoxSlot* DetailsPanelSlot = Body->AddChildToHorizontalBox(DetailsPanel);
	DetailsPanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	DetailsPanelSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));

	UVerticalBox* Details = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(),
		TEXT("QuestDetails"));
	DetailsPanel->AddChild(Details);

	DetailTitle = CreateJournalText(
		WidgetTree,
		TEXT("QuestDetailTitle"),
		30,
		FLinearColor(1.0f, 0.72f, 0.10f));
	DetailStatus = CreateJournalText(
		WidgetTree,
		TEXT("QuestDetailStatus"),
		15,
		FLinearColor(0.10f, 0.93f, 1.0f));
	DetailDescription = CreateJournalText(
		WidgetTree,
		TEXT("QuestDetailDescription"),
		18,
		FLinearColor(0.78f, 0.88f, 0.92f));
	DetailObjective = CreateJournalText(
		WidgetTree,
		TEXT("QuestDetailObjective"),
		21,
		FLinearColor::White);
	DetailReward = CreateJournalText(
		WidgetTree,
		TEXT("QuestDetailReward"),
		17,
		FLinearColor(0.20f, 1.0f, 0.58f));

	for (UTextBlock* Text : {
		DetailTitle.Get(),
		DetailStatus.Get(),
		DetailDescription.Get(),
		DetailObjective.Get(),
		DetailReward.Get()})
	{
		UVerticalBoxSlot* TextSlot = Details->AddChildToVerticalBox(Text);
		TextSlot->SetPadding(FMargin(2.0f, 2.0f, 2.0f, 14.0f));
		if (Text == DetailDescription)
		{
			TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
	}

	TrackButton = CreateJournalButton(
		WidgetTree,
		TEXT("TrackQuestButton"),
		FText::FromString(TEXT("TRACK")),
		FLinearColor(0.90f, 0.18f, 0.025f, 0.95f),
		19);
	TrackButton->OnClicked.AddDynamic(this, &UFlyingCabQuestJournalWidget::HandleTrackClicked);
	UVerticalBoxSlot* TrackSlot = Details->AddChildToVerticalBox(TrackButton);
	TrackSlot->SetPadding(FMargin(28.0f, 10.0f, 28.0f, 2.0f));
}

void UFlyingCabQuestJournalWidget::RefreshQuestList(
	const TArray<FFlyingCabQuestJournalEntry>& Entries)
{
	if (!QuestList)
	{
		return;
	}
	QuestList->ClearChildren();

	const auto IsVisibleInTab = [this](const FFlyingCabQuestJournalEntry& Entry)
	{
		return bShowingCompleted
			? Entry.Status == EFlyingCabQuestStatus::Completed
			: IsActiveJournalStatus(Entry.Status);
	};

	const bool bSelectionStillVisible = Entries.ContainsByPredicate(
		[this, &IsVisibleInTab](const FFlyingCabQuestJournalEntry& Entry)
		{
			return Entry.QuestId == SelectedQuestId && IsVisibleInTab(Entry);
		});
	if (!bSelectionStillVisible)
	{
		SelectedQuestId = NAME_None;
		for (const FFlyingCabQuestJournalEntry& Entry : Entries)
		{
			if (IsVisibleInTab(Entry) && (!bShowingCompleted && Entry.bTracked))
			{
				SelectedQuestId = Entry.QuestId;
				break;
			}
		}
		if (SelectedQuestId.IsNone())
		{
			for (const FFlyingCabQuestJournalEntry& Entry : Entries)
			{
				if (IsVisibleInTab(Entry))
				{
					SelectedQuestId = Entry.QuestId;
					break;
				}
			}
		}
	}

	int32 VisibleCount = 0;
	for (const FFlyingCabQuestJournalEntry& Entry : Entries)
	{
		if (!IsVisibleInTab(Entry))
		{
			continue;
		}
		++VisibleCount;
		UFlyingCabQuestJournalEntryButton* Button =
			WidgetTree->ConstructWidget<UFlyingCabQuestJournalEntryButton>(
				UFlyingCabQuestJournalEntryButton::StaticClass(),
				FName(*FString::Printf(TEXT("QuestEntry_%s"), *Entry.QuestId.ToString())));
		Button->InitializeEntry(this, Entry.QuestId);
		const bool bSelected = Entry.QuestId == SelectedQuestId;
		Button->SetBackgroundColor(
			bSelected
				? FLinearColor(0.03f, 0.55f, 0.78f, 1.0f)
				: (Entry.Status == EFlyingCabQuestStatus::Completed
					? FLinearColor(0.04f, 0.30f, 0.20f, 0.92f)
					: FLinearColor(0.04f, 0.10f, 0.17f, 0.95f)));

		FString RowText = Entry.bTracked ? TEXT("[TRACKED]  ") : FString();
		RowText += Entry.Title.ToString();
		if (Entry.Status == EFlyingCabQuestStatus::ReadyToTurnIn)
		{
			RowText += TEXT("\nRETURN TO QUEST GIVER");
		}
		else if (Entry.Status == EFlyingCabQuestStatus::Completed)
		{
			RowText += TEXT("\nCOMPLETE");
		}
		else if (!Entry.CurrentObjective.IsEmpty())
		{
			RowText += TEXT("\n") + Entry.CurrentObjective.ToString();
			if (Entry.RequiredProgress > 1)
			{
				RowText += FString::Printf(
					TEXT("  %d/%d"),
					Entry.CurrentProgress,
					Entry.RequiredProgress);
			}
		}
		UTextBlock* Label = CreateJournalText(
			WidgetTree,
			*FString::Printf(TEXT("QuestEntryLabel_%s"), *Entry.QuestId.ToString()),
			16,
			FLinearColor::White);
		Label->SetText(FText::FromString(RowText));
		Button->AddChild(Label);

		USizeBox* RowSize = WidgetTree->ConstructWidget<USizeBox>();
		RowSize->SetMinDesiredHeight(78.0f);
		RowSize->AddChild(Button);
		QuestList->AddChild(RowSize);
	}

	if (VisibleCount == 0)
	{
		UTextBlock* EmptyText = CreateJournalText(
			WidgetTree,
			TEXT("EmptyQuestList"),
			17,
			FLinearColor(0.55f, 0.65f, 0.70f),
			ETextJustify::Center);
		EmptyText->SetText(FText::FromString(
			bShowingCompleted
				? TEXT("NO COMPLETED ASSIGNMENTS")
				: TEXT("NO ACTIVE ASSIGNMENTS")));
		QuestList->AddChild(EmptyText);
	}
}

void UFlyingCabQuestJournalWidget::RefreshQuestDetails(
	const TArray<FFlyingCabQuestJournalEntry>& Entries)
{
	const FFlyingCabQuestJournalEntry* Selected = Entries.FindByPredicate(
		[this](const FFlyingCabQuestJournalEntry& Entry)
		{
			return Entry.QuestId == SelectedQuestId;
		});
	if (!Selected)
	{
		DetailTitle->SetText(FText::FromString(TEXT("NO ASSIGNMENT SELECTED")));
		DetailStatus->SetText(FText::GetEmpty());
		DetailDescription->SetText(FText::GetEmpty());
		DetailObjective->SetText(FText::GetEmpty());
		DetailReward->SetText(FText::GetEmpty());
		TrackButton->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	DetailTitle->SetText(Selected->Title);
	FString StatusText;
	switch (Selected->Status)
	{
	case EFlyingCabQuestStatus::Active:
		StatusText = Selected->bTracked ? TEXT("ACTIVE // TRACKED") : TEXT("ACTIVE");
		break;
	case EFlyingCabQuestStatus::ReadyToTurnIn:
		StatusText = Selected->bTracked
			? TEXT("READY TO TURN IN // TRACKED")
			: TEXT("READY TO TURN IN");
		break;
	case EFlyingCabQuestStatus::Completed:
		StatusText = TEXT("COMPLETED");
		break;
	default:
		break;
	}
	DetailStatus->SetText(FText::FromString(StatusText));
	DetailDescription->SetText(Selected->Description);

	FString ObjectiveText;
	if (!Selected->CurrentObjective.IsEmpty())
	{
		ObjectiveText = TEXT("CURRENT OBJECTIVE\n") + Selected->CurrentObjective.ToString();
		if (Selected->RequiredProgress > 1)
		{
			ObjectiveText += FString::Printf(
				TEXT("\nPROGRESS  %d / %d"),
				Selected->CurrentProgress,
				Selected->RequiredProgress);
		}
	}
	else if (Selected->Status == EFlyingCabQuestStatus::Completed)
	{
		ObjectiveText = TEXT("ALL OBJECTIVES COMPLETE");
	}
	DetailObjective->SetText(FText::FromString(ObjectiveText));

	FString RewardText = FString::Printf(TEXT("REWARD  //  %d CR"), Selected->RewardCredits);
	if (!Selected->RewardAccessIds.IsEmpty())
	{
		RewardText += FString::Printf(
			TEXT("  //  %d ACCESS UNLOCK%s"),
			Selected->RewardAccessIds.Num(),
			Selected->RewardAccessIds.Num() == 1 ? TEXT("") : TEXT("S"));
	}
	DetailReward->SetText(FText::FromString(RewardText));

	const bool bCanTrack = IsActiveJournalStatus(Selected->Status);
	TrackButton->SetVisibility(bCanTrack ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (bCanTrack)
	{
		SetButtonLabel(
			TrackButton,
			FText::FromString(Selected->bTracked ? TEXT("STOP TRACKING") : TEXT("TRACK")));
		TrackButton->SetBackgroundColor(
			Selected->bTracked
				? FLinearColor(0.16f, 0.19f, 0.24f, 1.0f)
				: FLinearColor(0.90f, 0.18f, 0.025f, 0.95f));
	}
}

UFlyingCabQuestSubsystem* UFlyingCabQuestJournalWidget::GetQuestSubsystem() const
{
	UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UFlyingCabQuestSubsystem>() : nullptr;
}

void UFlyingCabQuestJournalWidget::SetButtonLabel(UButton* Button, const FText& Label) const
{
	if (Button)
	{
		if (UTextBlock* Text = Cast<UTextBlock>(Button->GetContent()))
		{
			Text->SetText(Label);
		}
	}
}

void UFlyingCabQuestJournalWidget::HandleActiveTabClicked()
{
	bShowingCompleted = false;
	SelectedQuestId = NAME_None;
	ActiveTabButton->SetBackgroundColor(FLinearColor(0.02f, 0.48f, 0.72f, 1.0f));
	CompletedTabButton->SetBackgroundColor(FLinearColor(0.10f, 0.14f, 0.19f, 1.0f));
	RefreshJournal();
}

void UFlyingCabQuestJournalWidget::HandleCompletedTabClicked()
{
	bShowingCompleted = true;
	SelectedQuestId = NAME_None;
	ActiveTabButton->SetBackgroundColor(FLinearColor(0.10f, 0.14f, 0.19f, 1.0f));
	CompletedTabButton->SetBackgroundColor(FLinearColor(0.04f, 0.42f, 0.25f, 1.0f));
	RefreshJournal();
}

void UFlyingCabQuestJournalWidget::HandleTrackClicked()
{
	UFlyingCabQuestSubsystem* Quests = GetQuestSubsystem();
	if (!Quests || SelectedQuestId.IsNone())
	{
		return;
	}
	const bool bWasTracked = Quests->GetTrackedQuestId() == SelectedQuestId;
	const bool bChanged = bWasTracked
		? Quests->ClearTrackedQuest()
		: Quests->SetTrackedQuest(SelectedQuestId);
	if (bChanged)
	{
		if (AFlyingCabPlayerController* Controller =
			Cast<AFlyingCabPlayerController>(GetOwningPlayer()))
		{
			Controller->ShowEventMessage(
				bWasTracked
					? FText::FromString(TEXT("QUEST TRACKING DISABLED"))
					: FText::FromString(TEXT("QUEST TRACKING UPDATED")),
				bWasTracked
					? FLinearColor(0.60f, 0.70f, 0.75f)
					: FLinearColor(1.0f, 0.70f, 0.12f),
				1.5f);
		}
	}
	RefreshJournal();
}

void UFlyingCabQuestJournalWidget::HandleCloseClicked()
{
	if (AFlyingCabPlayerController* Controller =
		Cast<AFlyingCabPlayerController>(GetOwningPlayer()))
	{
		Controller->CloseQuestJournal();
	}
}

void UFlyingCabQuestJournalWidget::HandleQuestStateChanged(
	FName QuestId,
	EFlyingCabQuestStatus Status)
{
	RefreshJournal();
}

void UFlyingCabQuestJournalWidget::HandleTrackedQuestChanged(FName QuestId)
{
	RefreshJournal();
}
