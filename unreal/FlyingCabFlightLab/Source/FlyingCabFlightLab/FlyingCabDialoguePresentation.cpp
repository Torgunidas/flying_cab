#include "FlyingCabDialoguePresentation.h"

namespace
{
	/** Guards against a float division remainder eating a whole character of budget. */
	constexpr double TypewriterBudgetEpsilon = 1.0e-4;

	constexpr TCHAR EllipsisCharacter = static_cast<TCHAR>(0x2026);
}

float FFlyingCabTypewriter::PauseAfter(TCHAR Character, const FFlyingCabDialoguePacing& Pacing)
{
	if (Character == TEXT('.') || Character == TEXT('!') || Character == TEXT('?')
		|| Character == EllipsisCharacter)
	{
		return FMath::Max(0.0f, Pacing.SentencePauseSeconds);
	}
	if (Character == TEXT(',') || Character == TEXT(';') || Character == TEXT(':'))
	{
		return FMath::Max(0.0f, Pacing.ClausePauseSeconds);
	}
	if (Character == TEXT('\n'))
	{
		return FMath::Max(0.0f, Pacing.LineBreakPauseSeconds);
	}
	return 0.0f;
}

void FFlyingCabTypewriter::Start(const FString& InText, const FFlyingCabDialoguePacing& InPacing)
{
	Text = InText;
	Pacing = InPacing;
	Reset();
}

void FFlyingCabTypewriter::Reset()
{
	Budget = 0.0;
	HoldSeconds = 0.0;
	VisibleCharacters = 0;
	CharactersSinceBlip = 0;
	PendingBlips = 0;
}

void FFlyingCabTypewriter::Advance(float DeltaSeconds)
{
	if (DeltaSeconds <= 0.0f || IsFinished())
	{
		return;
	}
	Budget += DeltaSeconds;
	const double SecondsPerCharacter = 1.0 / FMath::Max(1.0f, Pacing.CharactersPerSecond);
	while (!IsFinished())
	{
		const double Cost = SecondsPerCharacter + HoldSeconds;
		if (Budget + TypewriterBudgetEpsilon < Cost)
		{
			break;
		}
		Budget -= Cost;
		HoldSeconds = 0.0;
		const TCHAR Revealed = Text[VisibleCharacters];
		++VisibleCharacters;
		if (Pacing.BlipEveryCharacters > 0 && !FChar::IsWhitespace(Revealed))
		{
			++CharactersSinceBlip;
			if (CharactersSinceBlip >= Pacing.BlipEveryCharacters)
			{
				CharactersSinceBlip = 0;
				++PendingBlips;
			}
		}
		HoldSeconds = PauseAfter(Revealed, Pacing);
	}
	if (IsFinished())
	{
		Budget = 0.0;
		HoldSeconds = 0.0;
	}
}

void FFlyingCabTypewriter::SkipToEnd()
{
	VisibleCharacters = Text.Len();
	Budget = 0.0;
	HoldSeconds = 0.0;
	CharactersSinceBlip = 0;
}

int32 FFlyingCabTypewriter::ConsumeBlips()
{
	const int32 Blips = PendingBlips;
	PendingBlips = 0;
	return Blips;
}

void FFlyingCabDialogueStage::BeginLine(
	int32 InOptionCount,
	bool bPlayPanelIntro,
	const FFlyingCabDialoguePacing& InPacing)
{
	Pacing = InPacing;
	OptionCount = FMath::Max(0, InOptionCount);
	PanelElapsed = bPlayPanelIntro ? 0.0f : FMath::Max(Pacing.PanelIntroSeconds, 1.0f);
	StaggerElapsed = 0.0f;
	AnnouncedOptions = 0;
	PendingOptionReveals = 0;
	bSkipped = false;
	Phase = bPlayPanelIntro
		? EFlyingCabDialoguePhase::PanelIntro
		: EFlyingCabDialoguePhase::Measuring;
}

void FFlyingCabDialogueStage::MarkMeasured()
{
	if (Phase == EFlyingCabDialoguePhase::Measuring)
	{
		Phase = EFlyingCabDialoguePhase::Typing;
	}
}

void FFlyingCabDialogueStage::MarkTypingFinished()
{
	if (Phase != EFlyingCabDialoguePhase::Typing)
	{
		return;
	}
	StaggerElapsed = 0.0f;
	Phase = OptionCount > 0
		? EFlyingCabDialoguePhase::Staggering
		: EFlyingCabDialoguePhase::Ready;
}

void FFlyingCabDialogueStage::SkipPacing()
{
	bSkipped = true;
	PanelElapsed = FMath::Max(PanelElapsed, Pacing.PanelIntroSeconds);
	if (Phase == EFlyingCabDialoguePhase::PanelIntro)
	{
		Phase = EFlyingCabDialoguePhase::Measuring;
	}
}

void FFlyingCabDialogueStage::Advance(float DeltaSeconds)
{
	const float Delta = FMath::Max(0.0f, DeltaSeconds);
	if (Phase == EFlyingCabDialoguePhase::PanelIntro)
	{
		PanelElapsed += Delta;
		if (PanelElapsed >= Pacing.PanelIntroSeconds)
		{
			Phase = EFlyingCabDialoguePhase::Measuring;
		}
		return;
	}
	if (Phase != EFlyingCabDialoguePhase::Staggering)
	{
		return;
	}
	StaggerElapsed += Delta;
	int32 Started = 0;
	for (int32 Index = 0; Index < OptionCount; ++Index)
	{
		if (GetOptionProgress(Index) > 0.0f)
		{
			++Started;
		}
	}
	if (Started > AnnouncedOptions)
	{
		PendingOptionReveals += Started - AnnouncedOptions;
		AnnouncedOptions = Started;
	}
	if (OptionCount == 0 || GetOptionProgress(OptionCount - 1) >= 1.0f)
	{
		Phase = EFlyingCabDialoguePhase::Ready;
	}
}

float FFlyingCabDialogueStage::GetPanelAlpha() const
{
	if (Pacing.PanelIntroSeconds <= KINDA_SMALL_NUMBER)
	{
		return 1.0f;
	}
	return FlyingCabEase::OutCubic(PanelElapsed / Pacing.PanelIntroSeconds);
}

float FFlyingCabDialogueStage::GetPanelScale() const
{
	return FMath::Lerp(Pacing.PanelIntroStartScale, 1.0f, GetPanelAlpha());
}

float FFlyingCabDialogueStage::GetOptionStart(int32 Index) const
{
	const float Stagger = bSkipped ? Pacing.SkippedOptionStaggerSeconds : Pacing.OptionStaggerSeconds;
	const float Delay = bSkipped ? 0.0f : Pacing.OptionsDelaySeconds;
	return FMath::Max(0.0f, Delay) + FMath::Max(0.0f, Stagger) * Index;
}

float FFlyingCabDialogueStage::GetOptionProgress(int32 Index) const
{
	if (Index < 0 || Index >= OptionCount)
	{
		return 0.0f;
	}
	if (Phase == EFlyingCabDialoguePhase::Ready)
	{
		return 1.0f;
	}
	if (Phase != EFlyingCabDialoguePhase::Staggering)
	{
		return 0.0f;
	}
	const float Reveal = FMath::Max(KINDA_SMALL_NUMBER, Pacing.OptionRevealSeconds);
	return FMath::Clamp((StaggerElapsed - GetOptionStart(Index)) / Reveal, 0.0f, 1.0f);
}

bool FFlyingCabDialogueStage::IsOptionRevealed(int32 Index) const
{
	return GetOptionProgress(Index) > 0.0f;
}

float FFlyingCabDialogueStage::GetOptionAlpha(int32 Index) const
{
	return FlyingCabEase::OutCubic(GetOptionProgress(Index));
}

float FFlyingCabDialogueStage::GetOptionOffset(int32 Index) const
{
	return Pacing.OptionSlideDistance * (1.0f - FlyingCabEase::OutCubic(GetOptionProgress(Index)));
}

int32 FFlyingCabDialogueStage::GetRevealedOptionCount() const
{
	int32 Revealed = 0;
	for (int32 Index = 0; Index < OptionCount; ++Index)
	{
		if (IsOptionRevealed(Index))
		{
			++Revealed;
		}
	}
	return Revealed;
}

int32 FFlyingCabDialogueStage::ConsumeOptionReveals()
{
	const int32 Reveals = PendingOptionReveals;
	PendingOptionReveals = 0;
	return Reveals;
}

namespace
{
	void WrapParagraph(
		const FString& Paragraph,
		float MaxWidth,
		const TFunctionRef<float(const FString&)>& MeasureWidth,
		TArray<FString>& OutLines)
	{
		TArray<FString> Words;
		Paragraph.ParseIntoArrayWS(Words);
		if (Words.IsEmpty())
		{
			OutLines.Add(FString());
			return;
		}
		FString Current;
		for (const FString& Word : Words)
		{
			const FString Candidate = Current.IsEmpty() ? Word : Current + TEXT(" ") + Word;
			if (MaxWidth <= 0.0f || MeasureWidth(Candidate) <= MaxWidth)
			{
				Current = Candidate;
				continue;
			}
			if (!Current.IsEmpty())
			{
				OutLines.Add(Current);
				Current.Reset();
			}
			// A word wider than the line is split by characters, so the loop always makes progress.
			FString Rest = Word;
			while (Rest.Len() > 1 && MeasureWidth(Rest) > MaxWidth)
			{
				int32 Fit = 1;
				while (Fit < Rest.Len() && MeasureWidth(Rest.Left(Fit + 1)) <= MaxWidth)
				{
					++Fit;
				}
				OutLines.Add(Rest.Left(Fit));
				Rest = Rest.RightChop(Fit);
			}
			Current = Rest;
		}
		if (!Current.IsEmpty())
		{
			OutLines.Add(Current);
		}
	}
}

FString FFlyingCabTextWrapper::Wrap(
	const FString& Text,
	float MaxWidth,
	const TFunctionRef<float(const FString&)>& MeasureWidth,
	int32* OutLineCount)
{
	FString Normalized = Text;
	Normalized.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
	Normalized.ReplaceInline(TEXT("\r"), TEXT("\n"));
	TArray<FString> Paragraphs;
	Normalized.ParseIntoArray(Paragraphs, TEXT("\n"), false);
	TArray<FString> Lines;
	for (const FString& Paragraph : Paragraphs)
	{
		WrapParagraph(Paragraph, MaxWidth, MeasureWidth, Lines);
	}
	if (OutLineCount)
	{
		*OutLineCount = Lines.Num();
	}
	return FString::Join(Lines, TEXT("\n"));
}

int32 FFlyingCabTextWrapper::CountLines(const FString& WrappedText)
{
	if (WrappedText.IsEmpty())
	{
		return 0;
	}
	int32 Lines = 1;
	for (const TCHAR Character : WrappedText)
	{
		if (Character == TEXT('\n'))
		{
			++Lines;
		}
	}
	return Lines;
}
