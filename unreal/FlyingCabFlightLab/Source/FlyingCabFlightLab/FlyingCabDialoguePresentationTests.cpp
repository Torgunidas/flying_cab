#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "FlyingCabDialoguePresentation.h"

namespace
{
	FFlyingCabDialoguePacing MakeTestPacing()
	{
		FFlyingCabDialoguePacing Pacing;
		Pacing.CharactersPerSecond = 50.0f;
		Pacing.ClausePauseSeconds = 0.0f;
		Pacing.SentencePauseSeconds = 0.0f;
		Pacing.LineBreakPauseSeconds = 0.0f;
		Pacing.PanelIntroSeconds = 0.2f;
		Pacing.OptionsDelaySeconds = 0.1f;
		Pacing.OptionStaggerSeconds = 0.1f;
		Pacing.OptionRevealSeconds = 0.1f;
		Pacing.SkippedOptionStaggerSeconds = 0.02f;
		Pacing.BlipEveryCharacters = 0;
		return Pacing;
	}

	/** Every glyph is ten units wide, so wrapping assertions stay exact. */
	float TenPerCharacter(const FString& Candidate)
	{
		return Candidate.Len() * 10.0f;
	}

	TArray<FString> SplitWords(const FString& Text)
	{
		TArray<FString> Words;
		Text.ParseIntoArrayWS(Words);
		return Words;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlyingCabDialogueTypewriterTest,
	"FlyingCab.Core.Dialogue.Typewriter",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FFlyingCabDialogueTypewriterTest::RunTest(const FString&)
{
	const FFlyingCabDialoguePacing Pacing = MakeTestPacing();
	FFlyingCabTypewriter Typewriter;

	const FString Plain = FString::ChrN(100, TEXT('a'));
	Typewriter.Start(Plain, Pacing);
	TestEqual(TEXT("A fresh line shows nothing"), Typewriter.GetVisibleCharacters(), 0);
	Typewriter.Advance(1.0f);
	TestEqual(TEXT("One second at 50 per second reveals half the line"), Typewriter.GetVisibleCharacters(), 50);
	TestFalse(TEXT("Half a line is not finished"), Typewriter.IsFinished());
	Typewriter.Advance(1.0f);
	TestEqual(TEXT("The second second finishes the line"), Typewriter.GetVisibleCharacters(), 100);
	TestTrue(TEXT("A fully revealed line reports finished"), Typewriter.IsFinished());
	Typewriter.Advance(100.0f);
	TestEqual(TEXT("Extra time never runs past the end"), Typewriter.GetVisibleCharacters(), 100);

	// A sentence break holds before the next character, without losing the ones already revealed.
	FFlyingCabDialoguePacing Punctuated = Pacing;
	Punctuated.CharactersPerSecond = 100.0f;
	Punctuated.SentencePauseSeconds = 0.5f;
	Typewriter.Start(TEXT("ab.cd"), Punctuated);
	Typewriter.Advance(0.03f);
	TestEqual(TEXT("Three characters arrive before the full stop holds"), Typewriter.GetVisibleCharacters(), 3);
	Typewriter.Advance(0.05f);
	TestEqual(TEXT("The sentence pause blocks the next character"), Typewriter.GetVisibleCharacters(), 3);
	Typewriter.Advance(0.5f);
	TestTrue(TEXT("The line resumes once the pause is served"), Typewriter.GetVisibleCharacters() >= 4);

	Typewriter.Start(TEXT("abcdefghij"), Pacing);
	Typewriter.Advance(0.02f);
	Typewriter.SkipToEnd();
	TestTrue(TEXT("Skipping completes the line"), Typewriter.IsFinished());
	TestEqual(TEXT("Skipping shows every character"), Typewriter.GetVisibleCharacters(), 10);

	FFlyingCabDialoguePacing Noisy = Pacing;
	Noisy.BlipEveryCharacters = 3;
	Typewriter.Start(TEXT("abcdefghi"), Noisy);
	Typewriter.Advance(10.0f);
	TestEqual(TEXT("Nine characters owe three blips"), Typewriter.ConsumeBlips(), 3);
	TestEqual(TEXT("Blips are consumed exactly once"), Typewriter.ConsumeBlips(), 0);

	// Whitespace must not trigger the typing sound.
	Typewriter.Start(TEXT("a  b  c"), Noisy);
	Typewriter.Advance(10.0f);
	TestEqual(TEXT("Spaces do not count towards blips"), Typewriter.ConsumeBlips(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlyingCabDialogueRevealTimelineTest,
	"FlyingCab.Core.Dialogue.RevealTimeline",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FFlyingCabDialogueRevealTimelineTest::RunTest(const FString&)
{
	const FFlyingCabDialoguePacing Pacing = MakeTestPacing();
	FFlyingCabDialogueStage Stage;

	Stage.BeginLine(3, true, Pacing);
	TestTrue(TEXT("A new conversation opens with the panel intro"),
		Stage.GetPhase() == EFlyingCabDialoguePhase::PanelIntro);
	TestTrue(TEXT("The panel starts transparent"), Stage.GetPanelAlpha() < 0.01f);
	TestTrue(TEXT("The panel starts scaled down"), Stage.GetPanelScale() < 1.0f);
	Stage.Advance(0.25f);
	TestTrue(TEXT("The intro hands over to measuring"),
		Stage.GetPhase() == EFlyingCabDialoguePhase::Measuring);
	TestTrue(TEXT("The panel ends fully opaque"), FMath::IsNearlyEqual(Stage.GetPanelAlpha(), 1.0f));

	Stage.MarkMeasured();
	TestTrue(TEXT("Measuring hands over to typing"), Stage.IsTyping());
	TestEqual(TEXT("No answer is shown while the line types"), Stage.GetRevealedOptionCount(), 0);
	TestFalse(TEXT("An answer that has not arrived is not revealed"), Stage.IsOptionRevealed(0));

	Stage.MarkTypingFinished();
	TestEqual(TEXT("The quiet beat still shows no answer"), Stage.GetRevealedOptionCount(), 0);
	Stage.Advance(0.11f);
	TestEqual(TEXT("The first answer arrives after the delay"), Stage.GetRevealedOptionCount(), 1);
	TestEqual(TEXT("One arrival is reported"), Stage.ConsumeOptionReveals(), 1);
	Stage.Advance(0.1f);
	TestEqual(TEXT("The second answer follows one stagger later"), Stage.GetRevealedOptionCount(), 2);
	Stage.Advance(0.1f);
	TestEqual(TEXT("The third answer follows"), Stage.GetRevealedOptionCount(), 3);
	TestTrue(TEXT("An arriving answer slides in from the right"), Stage.GetOptionOffset(2) > 0.0f);
	Stage.Advance(0.2f);
	TestTrue(TEXT("The stage settles once the last answer lands"), Stage.IsReady());
	TestTrue(TEXT("A settled answer is fully opaque"), FMath::IsNearlyEqual(Stage.GetOptionAlpha(2), 1.0f));
	TestTrue(TEXT("A settled answer sits at its resting place"),
		FMath::IsNearlyZero(Stage.GetOptionOffset(2)));

	// Skipping keeps the same order but collapses the wait.
	Stage.BeginLine(3, true, Pacing);
	Stage.SkipPacing();
	TestTrue(TEXT("Skipping leaves the panel intro immediately"),
		Stage.GetPhase() == EFlyingCabDialoguePhase::Measuring);
	Stage.MarkMeasured();
	Stage.MarkTypingFinished();
	Stage.Advance(0.05f);
	TestEqual(TEXT("A skipped line brings every answer at once"), Stage.GetRevealedOptionCount(), 3);

	// A line with no answers must not stall waiting for a stagger that never runs.
	Stage.BeginLine(0, false, Pacing);
	Stage.MarkMeasured();
	Stage.MarkTypingFinished();
	TestTrue(TEXT("A line without answers is ready at once"), Stage.IsReady());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlyingCabDialogueTextWrapTest,
	"FlyingCab.Core.Dialogue.TextWrap",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FFlyingCabDialogueTextWrapTest::RunTest(const FString&)
{
	auto Measure = [](const FString& Candidate) { return TenPerCharacter(Candidate); };

	const FString Source = TEXT("aaa bbb ccc ddd");
	int32 LineCount = 0;
	const FString Wrapped = FFlyingCabTextWrapper::Wrap(Source, 70.0f, Measure, &LineCount);
	TestEqual(TEXT("Seventy units fit two three-letter words per line"), LineCount, 2);
	TestEqual(TEXT("The reported line count matches the breaks"),
		FFlyingCabTextWrapper::CountLines(Wrapped), LineCount);
	TestEqual(TEXT("Wrapping keeps every word in order"),
		FString::Join(SplitWords(Wrapped), TEXT(" ")),
		FString::Join(SplitWords(Source), TEXT(" ")));

	TArray<FString> Lines;
	Wrapped.ParseIntoArray(Lines, TEXT("\n"), false);
	for (const FString& Line : Lines)
	{
		TestTrue(TEXT("No wrapped line is wider than the limit"), TenPerCharacter(Line) <= 70.0f);
		TestFalse(TEXT("No wrapped line is empty"), Line.IsEmpty());
	}

	// A single word wider than the line is split by characters instead of looping forever.
	const FString LongWord = TEXT("aaaaaaaaaa");
	const FString SplitWord = FFlyingCabTextWrapper::Wrap(LongWord, 35.0f, Measure, &LineCount);
	TestEqual(TEXT("A word wider than the line is split"), LineCount, 4);
	TestEqual(TEXT("Splitting a long word loses no character"),
		SplitWord.Replace(TEXT("\n"), TEXT("")), LongWord);

	// Authored line breaks survive.
	const FString Authored = FFlyingCabTextWrapper::Wrap(TEXT("aaa\nbbb"), 100.0f, Measure, &LineCount);
	TestEqual(TEXT("An authored break stays a break"), LineCount, 2);
	TestEqual(TEXT("An authored break is preserved verbatim"), Authored, FString(TEXT("aaa\nbbb")));

	const FString Unbounded = FFlyingCabTextWrapper::Wrap(Source, 0.0f, Measure, &LineCount);
	TestEqual(TEXT("A zero width falls back to a single line"), LineCount, 1);
	TestEqual(TEXT("A zero width keeps the text intact"), Unbounded, Source);
	return true;
}
#endif
