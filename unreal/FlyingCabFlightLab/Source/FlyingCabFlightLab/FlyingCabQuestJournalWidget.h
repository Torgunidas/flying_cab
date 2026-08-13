// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "CoreMinimal.h"
#include "FlyingCabQuestTypes.h"
#include "FlyingCabQuestJournalWidget.generated.h"

class UBorder;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class UFlyingCabQuestJournalWidget;

/** Self-routing list button used by the native journal fallback layout. */
UCLASS()
class FLYINGCABFLIGHTLAB_API UFlyingCabQuestJournalEntryButton : public UButton
{
	GENERATED_BODY()

public:
	void InitializeEntry(UFlyingCabQuestJournalWidget* InOwner, FName InQuestId);

private:
	UFUNCTION()
	void HandleEntryClicked();

	TWeakObjectPtr<UFlyingCabQuestJournalWidget> JournalOwner;
	FName QuestId = NAME_None;
};

/** Modal player-facing journal. Native layout works immediately and can be skinned later in UMG. */
UCLASS(Blueprintable)
class FLYINGCABFLIGHTLAB_API UFlyingCabQuestJournalWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowJournal();
	void HideJournal();
	void RefreshJournal();
	void SelectQuest(FName QuestId);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(
		const FGeometry& InGeometry,
		const FKeyEvent& InKeyEvent) override;

private:
	void BuildWidgetTree();
	void RefreshQuestList(const TArray<FFlyingCabQuestJournalEntry>& Entries);
	void RefreshQuestDetails(const TArray<FFlyingCabQuestJournalEntry>& Entries);
	class UFlyingCabQuestSubsystem* GetQuestSubsystem() const;
	void SetButtonLabel(UButton* Button, const FText& Label) const;

	UFUNCTION()
	void HandleActiveTabClicked();

	UFUNCTION()
	void HandleCompletedTabClicked();

	UFUNCTION()
	void HandleTrackClicked();

	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleQuestStateChanged(FName QuestId, EFlyingCabQuestStatus Status);

	UFUNCTION()
	void HandleTrackedQuestChanged(FName QuestId);

	UPROPERTY(Transient)
	TObjectPtr<UButton> ActiveTabButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CompletedTabButton;

	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> QuestList;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailTitle;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailStatus;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailDescription;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailObjective;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailReward;

	UPROPERTY(Transient)
	TObjectPtr<UButton> TrackButton;

	FName SelectedQuestId = NAME_None;
	bool bShowingCompleted = false;
};
