#include "FlyingCabDialogueWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "FlyingCabNarrativeSettings.h"
#include "FlyingCabPlayerController.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace
{
	// The stage is a band across the lower part of the screen; the world stays visible above it.
	constexpr float StageLeft = 0.04f;
	constexpr float StageTop = 0.46f;
	constexpr float StageRight = 0.96f;
	constexpr float StageBottom = 0.94f;
	constexpr float NpcColumnFraction = 0.58f;
	constexpr float AnswerColumnFraction = 0.42f;
	constexpr float ColumnGap = 18.0f;
	constexpr float BubblePaddingX = 22.0f;
	constexpr float BubblePaddingY = 18.0f;
	/** Used before the widget has a real geometry, so a line never waits forever to be wrapped. */
	constexpr float FallbackViewportWidth = 1280.0f;
	constexpr int32 MaxLineCount = 6;
	constexpr int32 CompactAnswerThreshold = 5;

	const FLinearColor ScrimColor(0.002f, 0.006f, 0.016f, 0.30f);
	const FLinearColor NameplateColor(1.0f, 0.72f, 0.10f, 0.96f);
	const FLinearColor NameTextColor(0.04f, 0.03f, 0.01f);
	const FLinearColor BubbleColor(0.012f, 0.030f, 0.055f, 0.97f);
	const FLinearColor LineTextColor(0.93f, 0.96f, 1.0f);
	const FLinearColor HintColor(0.42f, 0.56f, 0.66f);
	const FLinearColor FeedbackColor(1.0f, 0.64f, 0.30f);
	const FLinearColor OptionIdleColor(0.030f, 0.075f, 0.115f, 0.96f);
	const FLinearColor OptionSelectedColor(0.10f, 0.40f, 0.58f, 0.98f);
	const FLinearColor OptionDisabledColor(0.05f, 0.06f, 0.08f, 0.92f);
	const FLinearColor OptionNumberColor(0.10f, 0.93f, 1.0f);
	const FLinearColor OptionTextColor(0.90f, 0.95f, 1.0f);
	const FLinearColor CaretColor(1.0f, 0.72f, 0.10f);
	const FLinearColor CloseButtonColor(0.10f, 0.14f, 0.19f, 0.90f);

	/** Tried in order; the first size that keeps the line within MaxLineCount wins. */
	const int32 LineFontSizes[] = { 22, 20, 18, 16 };

	int32 NumberFromKey(const FKey& Key)
	{
		static const FKey Digits[] = {
			EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five,
			EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine };
		static const FKey NumPadDigits[] = {
			EKeys::NumPadOne, EKeys::NumPadTwo, EKeys::NumPadThree, EKeys::NumPadFour, EKeys::NumPadFive,
			EKeys::NumPadSix, EKeys::NumPadSeven, EKeys::NumPadEight, EKeys::NumPadNine };
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Digits); ++Index)
		{
			if (Key == Digits[Index] || Key == NumPadDigits[Index])
			{
				return Index + 1;
			}
		}
		return 0;
	}
}

UTextBlock* UFlyingCabDialogueWidget::MakeText(int32 FontSize, const FLinearColor& Color, bool bWrap)
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = FontSize;
	Text->SetFont(Font);
	Text->SetColorAndOpacity(FSlateColor(Color));
	Text->SetAutoWrapText(bWrap);
	Text->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
	Text->SetShadowOffset(FVector2D(2.0f, 2.0f));
	return Text;
}

UFlyingCabDialogueOptionButton::UFlyingCabDialogueOptionButton()
{
	InitIsFocusable(false);
}

void UFlyingCabDialogueOptionButton::InitializeOption(
	UFlyingCabDialogueWidget* InOwner,
	int32 InIndex,
	int32 InRevision)
{
	OwnerWidget = InOwner;
	Index = InIndex;
	Revision = InRevision;
	OnClicked.AddDynamic(this, &UFlyingCabDialogueOptionButton::Choose);
	OnHovered.AddDynamic(this, &UFlyingCabDialogueOptionButton::Highlight);
}

void UFlyingCabDialogueOptionButton::Choose()
{
	if (OwnerWidget.IsValid())
	{
		OwnerWidget->ChooseOption(Index, Revision);
	}
}

void UFlyingCabDialogueOptionButton::Highlight()
{
	// Mouse and keyboard must agree on what is selected.
	if (OwnerWidget.IsValid())
	{
		OwnerWidget->SetSelectedOption(Index);
	}
}

TSharedRef<SWidget> UFlyingCabDialogueWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UFlyingCabDialogueWidget::BuildLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(),
		TEXT("DialogueRoot"));
	WidgetTree->RootWidget = Root;

	Scrim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Scrim"));
	Scrim->SetBrushColor(ScrimColor);
	UCanvasPanelSlot* ScrimSlot = Root->AddChildToCanvas(Scrim);
	ScrimSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	ScrimSlot->SetOffsets(FMargin(0.0f));
	ScrimSlot->SetZOrder(0);

	UHorizontalBox* StageRow = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(),
		TEXT("Stage"));
	UCanvasPanelSlot* StageSlot = Root->AddChildToCanvas(StageRow);
	StageSlot->SetAnchors(FAnchors(StageLeft, StageTop, StageRight, StageBottom));
	StageSlot->SetOffsets(FMargin(0.0f));
	StageSlot->SetZOrder(10);

	NpcSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("NpcSizeBox"));
	NpcColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("NpcColumn"));
	NpcSizeBox->AddChild(NpcColumn);
	UHorizontalBoxSlot* NpcSlot = StageRow->AddChildToHorizontalBox(NpcSizeBox);
	NpcSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	NpcSlot->SetVerticalAlignment(VAlign_Bottom);
	NpcSlot->SetPadding(FMargin(0.0f, 0.0f, ColumnGap, 0.0f));
	// The bubble grows upward out of its lower-left corner.
	NpcColumn->SetRenderTransformPivot(FVector2D(0.12f, 0.9f));

	Nameplate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Nameplate"));
	Nameplate->SetBrushColor(NameplateColor);
	Nameplate->SetPadding(FMargin(12.0f, 6.0f));
	UHorizontalBox* NameRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Nameplate->SetContent(NameRow);
	PortraitBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PortraitBox"));
	PortraitBox->SetWidthOverride(64.0f);
	PortraitBox->SetHeightOverride(64.0f);
	PortraitImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Portrait"));
	PortraitBox->AddChild(PortraitImage);
	PortraitBox->SetVisibility(ESlateVisibility::Collapsed);
	UHorizontalBoxSlot* PortraitSlot = NameRow->AddChildToHorizontalBox(PortraitBox);
	PortraitSlot->SetVerticalAlignment(VAlign_Center);
	PortraitSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
	Speaker = MakeText(22, NameTextColor, false);
	Speaker->SetShadowOffset(FVector2D(0.0f, 0.0f));
	UHorizontalBoxSlot* SpeakerSlot = NameRow->AddChildToHorizontalBox(Speaker);
	SpeakerSlot->SetVerticalAlignment(VAlign_Center);
	UVerticalBoxSlot* NameplateSlot = NpcColumn->AddChildToVerticalBox(Nameplate);
	NameplateSlot->SetHorizontalAlignment(HAlign_Left);

	UBorder* Bubble = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Bubble"));
	Bubble->SetBrushColor(BubbleColor);
	Bubble->SetPadding(FMargin(BubblePaddingX, BubblePaddingY));
	UVerticalBox* BubbleStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Bubble->SetContent(BubbleStack);

	UOverlay* LineOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("LineOverlay"));
	// The ghost carries the whole wrapped line and reserves its height without drawing anything.
	GhostLine = MakeText(LineFontSizes[0], LineTextColor, false);
	GhostLine->SetVisibility(ESlateVisibility::Hidden);
	VisibleLine = MakeText(LineFontSizes[0], LineTextColor, false);
	UOverlaySlot* GhostSlot = LineOverlay->AddChildToOverlay(GhostLine);
	GhostSlot->SetHorizontalAlignment(HAlign_Left);
	GhostSlot->SetVerticalAlignment(VAlign_Top);
	UOverlaySlot* VisibleSlot = LineOverlay->AddChildToOverlay(VisibleLine);
	VisibleSlot->SetHorizontalAlignment(HAlign_Left);
	VisibleSlot->SetVerticalAlignment(VAlign_Top);
	BubbleStack->AddChildToVerticalBox(LineOverlay);

	SkipHint = MakeText(11, HintColor, false);
	SkipHint->SetText(NSLOCTEXT("FlyingCab", "DialogueSkipHint", "SPACE  skip"));
	SkipHint->SetVisibility(ESlateVisibility::Hidden);
	UVerticalBoxSlot* SkipSlot = BubbleStack->AddChildToVerticalBox(SkipHint);
	SkipSlot->SetHorizontalAlignment(HAlign_Right);
	SkipSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 0.0f));

	UVerticalBoxSlot* BubbleSlot = NpcColumn->AddChildToVerticalBox(Bubble);
	BubbleSlot->SetHorizontalAlignment(HAlign_Fill);

	Feedback = MakeText(14, FeedbackColor, true);
	Feedback->SetVisibility(ESlateVisibility::Collapsed);
	UVerticalBoxSlot* FeedbackSlot = NpcColumn->AddChildToVerticalBox(Feedback);
	FeedbackSlot->SetPadding(FMargin(4.0f, 8.0f, 0.0f, 0.0f));

	AnswerSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("AnswerSizeBox"));
	Options = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Answers"));
	AnswerSizeBox->AddChild(Options);
	UHorizontalBoxSlot* AnswerSlot = StageRow->AddChildToHorizontalBox(AnswerSizeBox);
	AnswerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	AnswerSlot->SetVerticalAlignment(VAlign_Bottom);

	UTextBlock* Footer = MakeText(12, HintColor, false);
	Footer->SetJustification(ETextJustify::Center);
	Footer->SetText(NSLOCTEXT(
		"FlyingCab",
		"DialogueKeys",
		"UP / DOWN  select     1-9  pick     ENTER  confirm     ESC  leave"));
	UCanvasPanelSlot* FooterSlot = Root->AddChildToCanvas(Footer);
	FooterSlot->SetAnchors(FAnchors(0.5f, 1.0f));
	FooterSlot->SetAlignment(FVector2D(0.5f, 1.0f));
	FooterSlot->SetPosition(FVector2D(0.0f, -14.0f));
	FooterSlot->SetSize(FVector2D(680.0f, 26.0f));
	FooterSlot->SetZOrder(20);

	// Touch builds have no Escape key, so the modal keeps an explicit way out.
	UFlyingCabDialogueOptionButton* Close = WidgetTree->ConstructWidget<UFlyingCabDialogueOptionButton>(
		UFlyingCabDialogueOptionButton::StaticClass(),
		TEXT("CloseConversation"));
	Close->SetBackgroundColor(CloseButtonColor);
	Close->OnClicked.AddDynamic(this, &UFlyingCabDialogueWidget::CloseDialogue);
	UTextBlock* CloseLabel = MakeText(18, LineTextColor, false);
	CloseLabel->SetText(NSLOCTEXT("FlyingCab", "DialogueClose", "X"));
	CloseLabel->SetJustification(ETextJustify::Center);
	Close->SetContent(CloseLabel);
	UCanvasPanelSlot* CloseSlot = Root->AddChildToCanvas(Close);
	CloseSlot->SetAnchors(FAnchors(1.0f, 0.0f));
	CloseSlot->SetAlignment(FVector2D(1.0f, 0.0f));
	CloseSlot->SetPosition(FVector2D(-18.0f, 18.0f));
	CloseSlot->SetSize(FVector2D(46.0f, 46.0f));
	CloseSlot->SetZOrder(20);
}

void UFlyingCabDialogueWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	// The root must be hit-testable so a click anywhere can skip the typing.
	SetVisibility(ESlateVisibility::Visible);
	const UFlyingCabNarrativeSettings* Settings = GetDefault<UFlyingCabNarrativeSettings>();
	Pacing = Settings->GetPacing();
	TypingSound = Settings->TypingSound;
	AnswerAppearSound = Settings->AnswerAppearSound;
	AnswerConfirmSound = Settings->AnswerConfirmSound;
	SoundVolume = Settings->ConversationSoundVolume;
	// SetSession already ran once with default pacing; this is the first line the player actually sees.
	bHasPresentedLine = false;
	LastTickSeconds = 0.0;
	RefreshView();
}

void UFlyingCabDialogueWidget::NativeDestruct()
{
	if (Session)
	{
		Session->OnChanged.RemoveDynamic(this, &UFlyingCabDialogueWidget::RefreshView);
	}
	Super::NativeDestruct();
}

void UFlyingCabDialogueWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	CachedViewportWidth = MyGeometry.GetLocalSize().X;
	// A conversation pauses the world, so the reveal runs on wall-clock time rather than game time.
	const double Now = FPlatformTime::Seconds();
	const float Delta = LastTickSeconds > 0.0
		? static_cast<float>(FMath::Clamp(Now - LastTickSeconds, 0.0, 0.25))
		: InDeltaTime;
	LastTickSeconds = Now;
	AdvancePresentation(Delta);
}

void UFlyingCabDialogueWidget::SetSession(UFlyingCabDialogueSession* InSession)
{
	if (Session)
	{
		Session->OnChanged.RemoveDynamic(this, &UFlyingCabDialogueWidget::RefreshView);
	}
	Session = InSession;
	if (Session)
	{
		Session->OnChanged.AddDynamic(this, &UFlyingCabDialogueWidget::RefreshView);
	}
	bHasPresentedLine = false;
	RefreshView();
}

void UFlyingCabDialogueWidget::RefreshView()
{
	if (!Session)
	{
		return;
	}
	CurrentView = Session->GetView();
	SelectedOption = 0;
	if (Speaker)
	{
		Speaker->SetText(CurrentView.Speaker);
	}
	if (PortraitBox && PortraitImage)
	{
		UTexture2D* Face = CurrentView.Portrait.IsNull() ? nullptr : CurrentView.Portrait.LoadSynchronous();
		if (Face)
		{
			PortraitImage->SetBrushFromTexture(Face, false);
			PortraitBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			PortraitBox->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	if (Feedback)
	{
		Feedback->SetText(CurrentView.Feedback);
		Feedback->SetVisibility(CurrentView.Feedback.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	}
	RebuildOptions();

	FullLine = CurrentView.Text.ToString();
	Typewriter.Start(FString(), Pacing);
	AppliedCharacters = -1;
	LaidOutWidth = 0.0f;
	if (VisibleLine)
	{
		VisibleLine->SetText(FText::GetEmpty());
	}
	Stage.BeginLine(CurrentView.Options.Num(), !bHasPresentedLine, Pacing);
	bHasPresentedLine = true;
	ApplyPresentation();
	PresentDialogue(CurrentView);
}

void UFlyingCabDialogueWidget::RebuildOptions()
{
	OptionEnabled.Reset();
	OptionButtons.Reset();
	OptionCarets.Reset();
	if (!Options)
	{
		return;
	}
	Options->ClearChildren();
	const int32 FontSize = CurrentView.Options.Num() > CompactAnswerThreshold ? 15 : 18;
	for (int32 Index = 0; Index < CurrentView.Options.Num(); ++Index)
	{
		const FFlyingCabDialogueOptionView& Option = CurrentView.Options[Index];
		UFlyingCabDialogueOptionButton* Button = WidgetTree->ConstructWidget<UFlyingCabDialogueOptionButton>(
			UFlyingCabDialogueOptionButton::StaticClass());
		Button->InitializeOption(this, Index, CurrentView.Revision);
		Button->SetIsEnabled(Option.bEnabled);

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UTextBlock* Caret = MakeText(FontSize, CaretColor, false);
		Caret->SetText(FText::FromString(FString::Chr(static_cast<TCHAR>(0x25B8))));
		Caret->SetVisibility(ESlateVisibility::Hidden);
		UHorizontalBoxSlot* CaretSlot = Row->AddChildToHorizontalBox(Caret);
		CaretSlot->SetVerticalAlignment(VAlign_Center);
		CaretSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));

		if (Index < 9)
		{
			UTextBlock* Number = MakeText(FontSize, OptionNumberColor, false);
			Number->SetText(FText::AsNumber(Index + 1));
			UHorizontalBoxSlot* NumberSlot = Row->AddChildToHorizontalBox(Number);
			NumberSlot->SetVerticalAlignment(VAlign_Center);
			NumberSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
		}

		UTextBlock* Label = MakeText(FontSize, OptionTextColor, true);
		Label->SetText(Option.bEnabled
			? Option.Text
			: FText::Format(
				NSLOCTEXT("FlyingCab", "DisabledChoice", "{0}\n{1}"),
				Option.Text,
				Option.UnavailableReason));
		UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Label);
		LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		LabelSlot->SetVerticalAlignment(VAlign_Center);

		UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Frame->SetPadding(FMargin(14.0f, 10.0f));
		Frame->SetBrushColor(FLinearColor::Transparent);
		Frame->SetContent(Row);
		Button->SetContent(Frame);
		// Hidden still occupies its slot, so the column never jumps while answers arrive.
		Button->SetVisibility(ESlateVisibility::Hidden);

		UVerticalBoxSlot* ButtonSlot = Options->AddChildToVerticalBox(Button);
		ButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		ButtonSlot->SetHorizontalAlignment(HAlign_Fill);

		OptionButtons.Add(Button);
		OptionCarets.Add(Caret);
		OptionEnabled.Add(Option.bEnabled);
	}
}

float UFlyingCabDialogueWidget::MeasureTextWidth(const FString& Line, const FSlateFontInfo& Font) const
{
	if (FSlateApplication::IsInitialized() && FSlateApplication::Get().GetRenderer())
	{
		const TSharedRef<FSlateFontMeasure> Measure =
			FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		return static_cast<float>(Measure->Measure(Line, Font).X);
	}
	// Headless fallback: an average glyph width keeps the wrap deterministic instead of stalling the line.
	return Line.Len() * Font.Size * 0.55f;
}

bool UFlyingCabDialogueWidget::TryLayoutLine()
{
	if (!VisibleLine || !GhostLine)
	{
		return false;
	}
	const float Viewport = CachedViewportWidth > 1.0f ? CachedViewportWidth : FallbackViewportWidth;
	const float StageWidth = Viewport * (StageRight - StageLeft);
	const float NpcWidth = FMath::Max(200.0f, StageWidth * NpcColumnFraction - ColumnGap);
	const float AnswerWidth = FMath::Max(160.0f, StageWidth * AnswerColumnFraction);
	if (NpcSizeBox)
	{
		NpcSizeBox->SetWidthOverride(NpcWidth);
	}
	if (AnswerSizeBox)
	{
		AnswerSizeBox->SetWidthOverride(AnswerWidth);
	}
	const float TextWidth = NpcWidth - BubblePaddingX * 2.0f - 4.0f;
	if (TextWidth <= 0.0f)
	{
		return false;
	}

	FSlateFontInfo Font = VisibleLine->GetFont();
	FString Wrapped;
	for (int32 SizeIndex = 0; SizeIndex < UE_ARRAY_COUNT(LineFontSizes); ++SizeIndex)
	{
		Font.Size = LineFontSizes[SizeIndex];
		int32 LineCount = 0;
		Wrapped = FFlyingCabTextWrapper::Wrap(
			FullLine,
			TextWidth,
			[this, &Font](const FString& Candidate) { return MeasureTextWidth(Candidate, Font); },
			&LineCount);
		if (LineCount <= MaxLineCount)
		{
			break;
		}
	}
	VisibleLine->SetFont(Font);
	GhostLine->SetFont(Font);
	GhostLine->SetText(FText::FromString(Wrapped));
	VisibleLine->SetText(FText::GetEmpty());
	AppliedCharacters = 0;
	Typewriter.Start(Wrapped, Pacing);
	LaidOutWidth = Viewport;
	return true;
}

void UFlyingCabDialogueWidget::AdvancePresentation(float DeltaSeconds)
{
	// Wrap as early as the first tick, so the bubble already has its final size while it fades in.
	if (LaidOutWidth <= 0.0f)
	{
		TryLayoutLine();
	}
	if (Stage.GetPhase() == EFlyingCabDialoguePhase::Measuring)
	{
		if (LaidOutWidth > 0.0f)
		{
			Stage.MarkMeasured();
		}
	}
	else
	{
		Stage.Advance(DeltaSeconds);
	}
	if (Stage.IsTyping())
	{
		Typewriter.Advance(DeltaSeconds);
		if (Typewriter.ConsumeBlips() > 0)
		{
			PlayConversationSound(TypingSound);
		}
		if (Typewriter.IsFinished())
		{
			Stage.MarkTypingFinished();
		}
	}
	else if (Stage.IsReady() && LaidOutWidth > 0.0f
		&& CachedViewportWidth > 1.0f
		&& !FMath::IsNearlyEqual(CachedViewportWidth, LaidOutWidth, 1.0f))
	{
		// The window changed size after the line was wrapped; re-wrap and show it whole.
		TryLayoutLine();
		Typewriter.SkipToEnd();
		AppliedCharacters = -1;
	}
	ApplyPresentation();
}

void UFlyingCabDialogueWidget::ApplyPresentation()
{
	if (NpcColumn)
	{
		NpcColumn->SetRenderOpacity(Stage.GetPanelAlpha());
		NpcColumn->SetRenderScale(FVector2D(Stage.GetPanelScale()));
	}
	if (VisibleLine)
	{
		const int32 Visible = Typewriter.GetVisibleCharacters();
		if (Visible != AppliedCharacters)
		{
			AppliedCharacters = Visible;
			VisibleLine->SetText(FText::FromString(Typewriter.GetText().Left(Visible)));
		}
	}
	if (SkipHint)
	{
		SkipHint->SetVisibility(Stage.IsTyping()
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Hidden);
	}
	if (Stage.ConsumeOptionReveals() > 0)
	{
		PlayConversationSound(AnswerAppearSound);
	}
	for (int32 Index = 0; Index < OptionButtons.Num(); ++Index)
	{
		UFlyingCabDialogueOptionButton* Button = OptionButtons[Index];
		if (!Button)
		{
			continue;
		}
		const bool bRevealed = Stage.IsOptionRevealed(Index);
		Button->SetVisibility(bRevealed ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		if (!bRevealed)
		{
			continue;
		}
		Button->SetRenderOpacity(Stage.GetOptionAlpha(Index));
		Button->SetRenderTranslation(FVector2D(Stage.GetOptionOffset(Index), 0.0f));
	}
	ApplySelectionHighlight();
}

void UFlyingCabDialogueWidget::ApplySelectionHighlight()
{
	for (int32 Index = 0; Index < OptionButtons.Num(); ++Index)
	{
		const bool bSelected = Index == SelectedOption && Stage.IsOptionRevealed(Index);
		const bool bEnabled = OptionEnabled.IsValidIndex(Index) ? OptionEnabled[Index] : true;
		if (UFlyingCabDialogueOptionButton* Button = OptionButtons[Index])
		{
			Button->SetBackgroundColor(bSelected
				? OptionSelectedColor
				: (bEnabled ? OptionIdleColor : OptionDisabledColor));
		}
		if (OptionCarets.IsValidIndex(Index) && OptionCarets[Index])
		{
			OptionCarets[Index]->SetVisibility(
				bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
		}
	}
}

void UFlyingCabDialogueWidget::SetSelectedOption(int32 Index)
{
	if (!OptionButtons.IsValidIndex(Index))
	{
		return;
	}
	SelectedOption = Index;
	ApplySelectionHighlight();
}

void UFlyingCabDialogueWidget::MoveSelection(int32 Delta)
{
	const int32 Revealed = Stage.GetRevealedOptionCount();
	if (Revealed <= 0)
	{
		return;
	}
	SetSelectedOption((SelectedOption + Delta + Revealed) % Revealed);
}

void UFlyingCabDialogueWidget::SkipReveal()
{
	Stage.SkipPacing();
	if (Stage.GetPhase() == EFlyingCabDialoguePhase::Measuring && TryLayoutLine())
	{
		Stage.MarkMeasured();
	}
	Typewriter.SkipToEnd();
	if (Stage.IsTyping())
	{
		Stage.MarkTypingFinished();
	}
	ApplyPresentation();
}

bool UFlyingCabDialogueWidget::IsRevealComplete() const
{
	return Stage.IsReady() && Typewriter.IsFinished();
}

void UFlyingCabDialogueWidget::ChooseOption(int32 Index, int32 Revision)
{
	// An answer that has not arrived yet cannot be taken, by mouse, number key or Blueprint.
	if (!Stage.IsOptionRevealed(Index))
	{
		return;
	}
	PlayConversationSound(AnswerConfirmSound);
	if (Session)
	{
		Session->ChooseOption(Index, Revision);
	}
	if (Session && Session->IsActive())
	{
		SetKeyboardFocus();
	}
}

void UFlyingCabDialogueWidget::CloseDialogue()
{
	if (AFlyingCabPlayerController* Controller = Cast<AFlyingCabPlayerController>(GetOwningPlayer()))
	{
		Controller->CloseDialogue();
	}
}

void UFlyingCabDialogueWidget::PlayConversationSound(const TSoftObjectPtr<USoundBase>& Sound)
{
	if (Sound.IsNull())
	{
		return;
	}
	USoundBase* Loaded = Sound.LoadSynchronous();
	if (!Loaded)
	{
		return;
	}
	// The world is paused during a conversation, so these have to be UI sounds.
	UGameplayStatics::PlaySound2D(this, Loaded, SoundVolume, 1.0f, 0.0f, nullptr, nullptr, true);
}

bool UFlyingCabDialogueWidget::HandleDialogueKey(const FKey& Key)
{
	if (Key == EKeys::Escape)
	{
		CloseDialogue();
		return true;
	}
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar)
	{
		if (!IsRevealComplete())
		{
			SkipReveal();
			return true;
		}
		ChooseOption(SelectedOption, CurrentView.Revision);
		return true;
	}
	if (Key == EKeys::Up || Key == EKeys::W)
	{
		MoveSelection(-1);
		return true;
	}
	if (Key == EKeys::Down || Key == EKeys::S)
	{
		MoveSelection(1);
		return true;
	}
	const int32 Number = NumberFromKey(Key);
	if (Number > 0)
	{
		const int32 Index = Number - 1;
		if (OptionButtons.IsValidIndex(Index) && Stage.IsOptionRevealed(Index))
		{
			SetSelectedOption(Index);
			ChooseOption(Index, CurrentView.Revision);
		}
		return true;
	}
	return false;
}

FReply UFlyingCabDialogueWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	const FKey Key = Event.GetKey();
	const bool bCommits = Key == EKeys::Enter || Key == EKeys::SpaceBar || NumberFromKey(Key) > 0;
	if (!Event.IsRepeat() || !bCommits)
	{
		HandleDialogueKey(Key);
	}
	// Q/J/R belong to gameplay; they must never leak through the modal.
	return FReply::Handled();
}

FReply UFlyingCabDialogueWidget::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (!IsRevealComplete())
	{
		SkipReveal();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}
