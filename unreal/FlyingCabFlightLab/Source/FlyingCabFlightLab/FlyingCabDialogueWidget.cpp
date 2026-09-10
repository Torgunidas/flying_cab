#include "FlyingCabDialogueWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "FlyingCabPlayerController.h"
#include "InputCoreTypes.h"

UFlyingCabDialogueOptionButton::UFlyingCabDialogueOptionButton()
{
	InitIsFocusable(false);
}

void UFlyingCabDialogueOptionButton::InitializeOption(UFlyingCabDialogueWidget* InOwner, int32 InIndex, int32 InRevision)
{
	OwnerWidget = InOwner; Index = InIndex; Revision = InRevision;
	OnClicked.AddDynamic(this, &UFlyingCabDialogueOptionButton::Choose);
}
void UFlyingCabDialogueOptionButton::Choose()
{
	if (OwnerWidget.IsValid()) OwnerWidget->ChooseOption(Index, Revision);
}

TSharedRef<SWidget> UFlyingCabDialogueWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget) BuildLayout();
	return Super::RebuildWidget();
}

void UFlyingCabDialogueWidget::BuildLayout()
{
	auto* Root = WidgetTree->ConstructWidget<UCanvasPanel>(); WidgetTree->RootWidget = Root;
	auto* Shade = WidgetTree->ConstructWidget<UBorder>(); Shade->SetBrushColor(FLinearColor(0.005f, 0.009f, 0.018f, 0.78f));
	auto* ShadeSlot = Root->AddChildToCanvas(Shade); ShadeSlot->SetAnchors(FAnchors(0, 0, 1, 1)); ShadeSlot->SetOffsets(FMargin(0));
	auto* Panel = WidgetTree->ConstructWidget<UBorder>(); Panel->SetBrushColor(FLinearColor(0.028f, 0.041f, 0.063f)); Panel->SetPadding(FMargin(28));
	auto* PanelSlot = Root->AddChildToCanvas(Panel); PanelSlot->SetAnchors(FAnchors(0.15f, 0.12f, 0.85f, 0.88f)); PanelSlot->SetOffsets(FMargin(0));
	auto* Scroll = WidgetTree->ConstructWidget<UScrollBox>(); Panel->SetContent(Scroll);
	auto* Stack = WidgetTree->ConstructWidget<UVerticalBox>(); Scroll->AddChild(Stack);
	auto AddText = [this, Stack](int32 Size, FLinearColor Color)
	{
		auto* Text = WidgetTree->ConstructWidget<UTextBlock>(); auto Font = Text->GetFont(); Font.Size = Size; Text->SetFont(Font);
		Text->SetAutoWrapText(true); Text->SetColorAndOpacity(Color);
		Stack->AddChildToVerticalBox(Text)->SetPadding(FMargin(0, 0, 0, 18)); return Text;
	};
	auto* Header = AddText(12, FLinearColor(0.55f, 0.67f, 0.76f)); Header->SetText(NSLOCTEXT("FlyingCab", "DialogueHeader", "CITY CONNECTIONS  /  CONVERSATION"));
	Speaker = AddText(28, FLinearColor(1.0f, 0.74f, 0.30f));
	Line = AddText(20, FLinearColor(0.93f, 0.96f, 1.0f));
	Feedback = AddText(14, FLinearColor(1.0f, 0.64f, 0.30f));
	Options = WidgetTree->ConstructWidget<UVerticalBox>(); Stack->AddChildToVerticalBox(Options)->SetPadding(FMargin(0, 8, 0, 20));
	auto* Footer = AddText(12, FLinearColor(0.55f, 0.67f, 0.76f)); Footer->SetText(NSLOCTEXT("FlyingCab", "DialogueKeys", "UP / DOWN  select     ENTER  choose     ESC  leave"));
	auto* Close = WidgetTree->ConstructWidget<UFlyingCabDialogueOptionButton>(); Close->OnClicked.AddDynamic(this, &UFlyingCabDialogueWidget::CloseDialogue);
	auto* CloseText = WidgetTree->ConstructWidget<UTextBlock>(); CloseText->SetText(NSLOCTEXT("FlyingCab", "LeaveConversation", "LEAVE CONVERSATION")); CloseText->SetJustification(ETextJustify::Center); Close->SetContent(CloseText);
	Stack->AddChildToVerticalBox(Close)->SetPadding(FMargin(0, 4));
}

void UFlyingCabDialogueWidget::NativeConstruct()
{
	Super::NativeConstruct(); SetIsFocusable(true); RefreshView();
}
void UFlyingCabDialogueWidget::NativeDestruct()
{
	if (Session) Session->OnChanged.RemoveDynamic(this, &UFlyingCabDialogueWidget::RefreshView);
	Super::NativeDestruct();
}
void UFlyingCabDialogueWidget::SetSession(UFlyingCabDialogueSession* InSession)
{
	if (Session) Session->OnChanged.RemoveDynamic(this, &UFlyingCabDialogueWidget::RefreshView);
	Session = InSession;
	if (Session) Session->OnChanged.AddDynamic(this, &UFlyingCabDialogueWidget::RefreshView);
	RefreshView();
}
void UFlyingCabDialogueWidget::RefreshView()
{
	if (!Session) return;
	CurrentView = Session->GetView(); SelectedOption = 0;
	if (Speaker) Speaker->SetText(CurrentView.Speaker);
	if (Line) Line->SetText(CurrentView.Text);
	if (Feedback) { Feedback->SetText(CurrentView.Feedback); Feedback->SetVisibility(CurrentView.Feedback.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible); }
	if (Options)
	{
		Options->ClearChildren(); OptionButtons.Reset();
		for (int32 I = 0; I < CurrentView.Options.Num(); ++I)
		{
			const auto& Option = CurrentView.Options[I];
			auto* Button = WidgetTree->ConstructWidget<UFlyingCabDialogueOptionButton>(); Button->InitializeOption(this, I, CurrentView.Revision);
			Button->SetIsEnabled(Option.bEnabled); Button->SetBackgroundColor(FLinearColor(0.10f, 0.17f, 0.24f));
			auto* Padding = WidgetTree->ConstructWidget<UBorder>(); Padding->SetPadding(FMargin(16, 12)); Padding->SetBrushColor(FLinearColor::Transparent);
			auto* Text = WidgetTree->ConstructWidget<UTextBlock>(); Text->SetAutoWrapText(true); auto Font = Text->GetFont(); Font.Size = 17; Text->SetFont(Font);
			Text->SetText(Option.bEnabled ? Option.Text : FText::Format(NSLOCTEXT("FlyingCab", "DisabledChoice", "{0}\n{1}"), Option.Text, Option.UnavailableReason));
			Padding->SetContent(Text); Button->SetContent(Padding);
			Options->AddChildToVerticalBox(Button)->SetPadding(FMargin(0, 0, 0, 8)); OptionButtons.Add(Button);
		}
		if (!OptionButtons.IsEmpty()) OptionButtons[0]->SetBackgroundColor(FLinearColor(0.22f, 0.30f, 0.37f));
	}
	PresentDialogue(CurrentView);
}
void UFlyingCabDialogueWidget::ChooseOption(int32 Index, int32 Revision)
{
	if (Session) Session->ChooseOption(Index, Revision);
	if (Session && Session->IsActive()) SetKeyboardFocus();
}
void UFlyingCabDialogueWidget::CloseDialogue()
{
	if (auto* Controller = Cast<AFlyingCabPlayerController>(GetOwningPlayer())) Controller->CloseDialogue();
}
FReply UFlyingCabDialogueWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	const auto Key = Event.GetKey();
	if (Key == EKeys::Escape) { CloseDialogue(); return FReply::Handled(); }
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar)
	{
		if (!Event.IsRepeat()) ChooseOption(SelectedOption, CurrentView.Revision);
		return FReply::Handled();
	}
	if ((Key == EKeys::Up || Key == EKeys::W || Key == EKeys::Down || Key == EKeys::S) && !CurrentView.Options.IsEmpty())
	{
		const int32 Delta = Key == EKeys::Up || Key == EKeys::W ? -1 : 1;
		SelectedOption = (SelectedOption + Delta + CurrentView.Options.Num()) % CurrentView.Options.Num();
		for (int32 I = 0; I < OptionButtons.Num(); ++I) OptionButtons[I]->SetBackgroundColor(I == SelectedOption ? FLinearColor(0.22f, 0.30f, 0.37f) : FLinearColor(0.10f, 0.17f, 0.24f));
		return FReply::Handled();
	}
	// Q/J/R belong to gameplay; they must never leak through the modal.
	return FReply::Handled();
}
