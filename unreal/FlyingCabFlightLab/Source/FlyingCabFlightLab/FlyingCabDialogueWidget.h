#pragma once

#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "FlyingCabDialoguePresentation.h"
#include "FlyingCabDialogueSession.h"
#include "FlyingCabDialogueWidget.generated.h"

class UBorder;
class UImage;
class UOverlay;
class USizeBox;
class USoundBase;
class UTextBlock;
class UVerticalBox;
class UFlyingCabDialogueWidget;

UCLASS()
class UFlyingCabDialogueOptionButton : public UButton
{
	GENERATED_BODY()
public:
	UFlyingCabDialogueOptionButton();
	void InitializeOption(UFlyingCabDialogueWidget* InOwner, int32 InIndex, int32 InRevision);
private:
	UFUNCTION() void Choose();
	UFUNCTION() void Highlight();
	TWeakObjectPtr<UFlyingCabDialogueWidget> OwnerWidget;
	int32 Index = 0;
	int32 Revision = 0;
};

/**
 * Native conversation view. The line types itself in and the answers arrive one by one on the right.
 * Motion lives here; the session still owns every choice, condition and consequence.
 * A Widget Blueprint may replace this view by implementing PresentDialogue and calling ChooseOption/CloseDialogue.
 */
UCLASS(Blueprintable)
class FLYINGCABFLIGHTLAB_API UFlyingCabDialogueWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetSession(UFlyingCabDialogueSession* InSession);
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void ChooseOption(int32 Index, int32 Revision);
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void CloseDialogue();
	UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue")
	void PresentDialogue(const FFlyingCabDialogueView& View);
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	UFlyingCabDialogueSession* GetSession() const { return Session; }

	/** Finishes the typing at once and tightens the answer stagger. Never selects an answer. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void SkipReveal();
	/** True when the line is fully typed and every answer has arrived. */
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsRevealComplete() const;

	/** Deterministic clock for automation; NativeTick simply forwards its delta here. */
	void AdvancePresentation(float DeltaSeconds);
	int32 GetRevealedCharacterCount() const { return Typewriter.GetVisibleCharacters(); }
	int32 GetTotalCharacterCount() const { return Typewriter.GetTotalCharacters(); }
	int32 GetRevealedOptionCount() const { return Stage.GetRevealedOptionCount(); }
	int32 GetSelectedOption() const { return SelectedOption; }
	/** Same routing as the key handler, exposed so tests do not have to drive Slate. */
	bool HandleDialogueKey(const FKey& Key);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;

private:
	friend class UFlyingCabDialogueOptionButton;

	UTextBlock* MakeText(int32 FontSize, const FLinearColor& Color, bool bWrap);
	void BuildLayout();
	UFUNCTION() void RefreshView();
	void RebuildOptions();
	/** Measures the available width and wraps the line once, so revealed text can never reflow. */
	bool TryLayoutLine();
	void ApplyPresentation();
	void ApplySelectionHighlight();
	void MoveSelection(int32 Delta);
	void SetSelectedOption(int32 Index);
	void PlayConversationSound(const TSoftObjectPtr<USoundBase>& Sound);
	float MeasureTextWidth(const FString& Line, const FSlateFontInfo& Font) const;

	UPROPERTY(Transient) TObjectPtr<UFlyingCabDialogueSession> Session;
	UPROPERTY(Transient) TObjectPtr<UBorder> Scrim;
	UPROPERTY(Transient) TObjectPtr<USizeBox> NpcSizeBox;
	UPROPERTY(Transient) TObjectPtr<USizeBox> AnswerSizeBox;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> NpcColumn;
	UPROPERTY(Transient) TObjectPtr<UBorder> Nameplate;
	UPROPERTY(Transient) TObjectPtr<USizeBox> PortraitBox;
	UPROPERTY(Transient) TObjectPtr<UImage> PortraitImage;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Speaker;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> GhostLine;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> VisibleLine;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SkipHint;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Feedback;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> Options;
	UPROPERTY(Transient) TArray<TObjectPtr<UFlyingCabDialogueOptionButton>> OptionButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> OptionCarets;

	TArray<bool> OptionEnabled;
	FFlyingCabDialogueView CurrentView;
	FFlyingCabDialoguePacing Pacing;
	FFlyingCabTypewriter Typewriter;
	FFlyingCabDialogueStage Stage;
	FString FullLine;
	TSoftObjectPtr<USoundBase> TypingSound;
	TSoftObjectPtr<USoundBase> AnswerAppearSound;
	TSoftObjectPtr<USoundBase> AnswerConfirmSound;
	float SoundVolume = 0.6f;
	double LastTickSeconds = 0.0;
	float CachedViewportWidth = 0.0f;
	float LaidOutWidth = 0.0f;
	int32 SelectedOption = 0;
	int32 AppliedCharacters = -1;
	bool bHasPresentedLine = false;
};
