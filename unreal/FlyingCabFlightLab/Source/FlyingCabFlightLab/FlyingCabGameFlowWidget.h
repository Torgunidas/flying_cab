// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FlyingCabRunTypes.h"
#include "FlyingCabGameFlowWidget.generated.h"

class UButton;
class UTextBlock;

/** Full-screen mode selection and Time Attack results overlay. */
UCLASS()
class FLYINGCABFLIGHTLAB_API UFlyingCabGameFlowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowModeSelection(const TArray<float>& BestTimes, int32 TargetCredits);
	void ShowTimeAttackResults(
		const FFlyingCabTimeAttackResult& Result,
		const TArray<float>& BestTimes);
	void HideFlowScreen();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildWidgetTree();
	void SetButtonLabel(UButton* Button, const FString& Label) const;
	FString BuildLeaderboardText(const TArray<float>& BestTimes) const;
	static FString FormatTime(float Seconds);

	UFUNCTION()
	void HandlePrimaryClicked();

	UFUNCTION()
	void HandleSecondaryClicked();

	UFUNCTION()
	void HandleTertiaryClicked();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BodyText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LeaderboardText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> PrimaryButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> SecondaryButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> TertiaryButton;

	bool bShowingResults = false;
};
