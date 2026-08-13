// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabQuestDefinition.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabQuestDefinition, Log, All);

void UFlyingCabQuestDefinition::PostLoad()
{
	Super::PostLoad();
	FString ValidationError;
	if (!IsConfigurationValid(ValidationError))
	{
		UE_LOG(
			LogFlyingCabQuestDefinition,
			Error,
			TEXT("Invalid quest asset %s: %s"),
			*GetPathName(),
			*ValidationError);
	}
}

bool UFlyingCabQuestDefinition::IsConfigurationValid(FString& OutError) const
{
	if (QuestId.IsNone() || Title.IsEmpty())
	{
		OutError = TEXT("QuestId and Title are required.");
		return false;
	}
	if (Objectives.IsEmpty())
	{
		OutError = TEXT("At least one objective is required.");
		return false;
	}

	TSet<FName> ObjectiveIds;
	for (const FFlyingCabQuestObjectiveDefinition& Objective : Objectives)
	{
		if (Objective.ObjectiveId.IsNone() || Objective.EventId.IsNone()
			|| Objective.Description.IsEmpty() || Objective.RequiredCount <= 0)
		{
			OutError = TEXT("Every objective needs a unique ID, description, event and positive count.");
			return false;
		}
		if (ObjectiveIds.Contains(Objective.ObjectiveId))
		{
			OutError = FString::Printf(
				TEXT("Objective ID %s is duplicated."),
				*Objective.ObjectiveId.ToString());
			return false;
		}
		ObjectiveIds.Add(Objective.ObjectiveId);
	}
	if (Reward.Credits < 0 || Reward.GrantedAccessIds.Contains(NAME_None))
	{
		OutError = TEXT("Rewards cannot be negative and access IDs cannot be None.");
		return false;
	}
	if (!NextQuest.IsNull() && NextQuest.ToSoftObjectPath() == FSoftObjectPath(this))
	{
		OutError = TEXT("A quest cannot point to itself as NextQuest.");
		return false;
	}

	OutError.Reset();
	return true;
}
