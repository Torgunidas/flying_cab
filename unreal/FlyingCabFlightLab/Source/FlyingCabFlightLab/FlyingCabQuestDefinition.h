// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "FlyingCabQuestTypes.h"
#include "FlyingCabQuestDefinition.generated.h"

/** Author-facing quest definition. Runtime state never mutates this asset. */
UCLASS(BlueprintType)
class FLYINGCABFLIGHTLAB_API UFlyingCabQuestDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual void PostLoad() override;
	bool IsConfigurationValid(FString& OutError) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Identity")
	FName QuestId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Presentation")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Presentation", meta = (MultiLine = "true"))
	FText Description;

	/** Objectives advance in array order. Events cannot skip an earlier objective. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Flow")
	TArray<FFlyingCabQuestObjectiveDefinition> Objectives;

	/** If false, the quest completes as soon as its last objective is satisfied. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Flow")
	bool bRequiresTurnIn = false;

	/** Started automatically when Free Roam begins for the first time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Flow")
	bool bAutoStartInFreeroam = false;

	/** Optional next definition; drag another quest asset here to form a chain. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Flow")
	TSoftObjectPtr<UFlyingCabQuestDefinition> NextQuest;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Reward")
	FFlyingCabQuestReward Reward;

	/** Reserved stable hooks. A future dialogue system can resolve them without changing quest data. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Dialogue")
	FName OfferDialogueId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Dialogue")
	FName ActiveDialogueId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Dialogue")
	FName CompletionDialogueId = NAME_None;
};
