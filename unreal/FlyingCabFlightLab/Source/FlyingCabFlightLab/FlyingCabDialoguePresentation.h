#pragma once

#include "CoreMinimal.h"

/** Presentation-only helpers for the conversation modal. No UObjects, no Slate: unit tests drive them directly. */
namespace FlyingCabEase
{
	inline float OutCubic(float Alpha)
	{
		const float Clamped = FMath::Clamp(Alpha, 0.0f, 1.0f);
		const float Inverse = 1.0f - Clamped;
		return 1.0f - Inverse * Inverse * Inverse;
	}
}

/** Designer-facing timings, copied out of the narrative settings so this layer stays free of engine config. */
struct FLYINGCABFLIGHTLAB_API FFlyingCabDialoguePacing
{
	float CharactersPerSecond = 55.0f;
	/** Extra hold after , ; : */
	float ClausePauseSeconds = 0.05f;
	/** Extra hold after . ! ? and the ellipsis. */
	float SentencePauseSeconds = 0.13f;
	float LineBreakPauseSeconds = 0.09f;
	float PanelIntroSeconds = 0.16f;
	float PanelIntroStartScale = 0.92f;
	/** Quiet beat between the finished line and the first answer. */
	float OptionsDelaySeconds = 0.10f;
	float OptionStaggerSeconds = 0.085f;
	float OptionRevealSeconds = 0.14f;
	float OptionSlideDistance = 30.0f;
	/** Pacing used once the player has skipped the typing. */
	float SkippedOptionStaggerSeconds = 0.02f;
	int32 BlipEveryCharacters = 3;
};

/** Reveals a line one character at a time, holding briefly on punctuation. */
struct FLYINGCABFLIGHTLAB_API FFlyingCabTypewriter
{
	void Start(const FString& InText, const FFlyingCabDialoguePacing& InPacing);
	void Advance(float DeltaSeconds);
	void SkipToEnd();
	void Reset();

	const FString& GetText() const { return Text; }
	int32 GetVisibleCharacters() const { return VisibleCharacters; }
	int32 GetTotalCharacters() const { return Text.Len(); }
	bool IsFinished() const { return VisibleCharacters >= Text.Len(); }
	/** Returns how many blips are owed since the last call and clears the counter. */
	int32 ConsumeBlips();

	static float PauseAfter(TCHAR Character, const FFlyingCabDialoguePacing& Pacing);

private:
	FString Text;
	FFlyingCabDialoguePacing Pacing;
	double Budget = 0.0;
	double HoldSeconds = 0.0;
	int32 VisibleCharacters = 0;
	int32 CharactersSinceBlip = 0;
	int32 PendingBlips = 0;
};

enum class EFlyingCabDialoguePhase : uint8
{
	/** The panel itself is fading and scaling in. Only the first line of a conversation uses it. */
	PanelIntro,
	/** Waiting for a real widget width so the line can be wrapped once, before any character shows. */
	Measuring,
	Typing,
	/** Answers arrive one after another. */
	Staggering,
	Ready
};

/** Phase machine and per-answer motion for a single line. Advance() is the only clock it has. */
struct FLYINGCABFLIGHTLAB_API FFlyingCabDialogueStage
{
	void BeginLine(int32 InOptionCount, bool bPlayPanelIntro, const FFlyingCabDialoguePacing& InPacing);
	/** Called once the widget knows its width and has wrapped the line. */
	void MarkMeasured();
	void MarkTypingFinished();
	/** Fast-forwards the panel intro and tightens the answer stagger. */
	void SkipPacing();
	void Advance(float DeltaSeconds);

	EFlyingCabDialoguePhase GetPhase() const { return Phase; }
	bool IsTyping() const { return Phase == EFlyingCabDialoguePhase::Typing; }
	bool IsReady() const { return Phase == EFlyingCabDialoguePhase::Ready; }
	float GetPanelAlpha() const;
	float GetPanelScale() const;
	int32 GetOptionCount() const { return OptionCount; }
	/** True once the answer has started sliding in; before that it must not be visible or selectable. */
	bool IsOptionRevealed(int32 Index) const;
	float GetOptionAlpha(int32 Index) const;
	float GetOptionOffset(int32 Index) const;
	int32 GetRevealedOptionCount() const;
	/** Answers that started arriving since the last call, for the knock sound. */
	int32 ConsumeOptionReveals();

private:
	float GetOptionProgress(int32 Index) const;
	float GetOptionStart(int32 Index) const;

	FFlyingCabDialoguePacing Pacing;
	EFlyingCabDialoguePhase Phase = EFlyingCabDialoguePhase::Ready;
	float PanelElapsed = 0.0f;
	float StaggerElapsed = 0.0f;
	int32 OptionCount = 0;
	int32 AnnouncedOptions = 0;
	int32 PendingOptionReveals = 0;
	bool bSkipped = false;
};

/** Wraps a line with hard breaks so a partially revealed string can never reflow while it types. */
struct FLYINGCABFLIGHTLAB_API FFlyingCabTextWrapper
{
	/** MeasureWidth reports the rendered width of a candidate line; tests supply a synthetic one. */
	static FString Wrap(
		const FString& Text,
		float MaxWidth,
		const TFunctionRef<float(const FString&)>& MeasureWidth,
		int32* OutLineCount = nullptr);

	static int32 CountLines(const FString& WrappedText);
};
