// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabQuestDefinition.h"

#include "Misc/DataValidation.h"
#include "FlyingCabQuestCatalog.h"
#include "FlyingCabNarrativeSettings.h"

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

#if WITH_EDITOR
EDataValidationResult UFlyingCabQuestDefinition::IsDataValid(
	FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);
	FString ValidationError;
	if (!IsConfigurationValid(ValidationError))
	{
		Context.AddError(FText::FromString(ValidationError));
		return EDataValidationResult::Invalid;
	}
	const auto* Catalog = GetDefault<UFlyingCabNarrativeSettings>()->QuestCatalog.LoadSynchronous();
	if (Catalog)
	{
		const bool bRegistered = Catalog->Quests.Contains(this);
		const bool bValid = bRegistered ? Catalog->IsQuestEntryValid(this, ValidationError) : Catalog->IsQuestContentValid(this, ValidationError);
		if (!bValid) { Context.AddError(FText::FromString(ValidationError)); return EDataValidationResult::Invalid; }
		if (!bRegistered) Context.AddWarning(NSLOCTEXT("FlyingCab", "DraftQuest", "Draft quest: use Add to catalog before assigning it to an NPC."));
	}
	return EDataValidationResult::Valid;
}
#endif

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

#if WITH_EDITOR
void UFlyingCabQuestDefinition::EnsureAuthoringIds()
{
	if (QuestId.IsNone()) QuestId = FName(*FString::Printf(TEXT("Quest.%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
	TSet<FName> Used;
	for (auto& Objective : Objectives)
	{
		if (Objective.ObjectiveId.IsNone() || Used.Contains(Objective.ObjectiveId))
			Objective.ObjectiveId = FName(*FString::Printf(TEXT("Objective_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
		Used.Add(Objective.ObjectiveId);
	}
}
void UFlyingCabQuestDefinition::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	EnsureAuthoringIds();
	Super::PostEditChangeProperty(Event);
}
void UFlyingCabQuestDefinition::PostDuplicate(EDuplicateMode::Type Mode)
{
	Super::PostDuplicate(Mode);
	if (Mode == EDuplicateMode::Normal)
	{
		QuestId = NAME_None;
		EnsureAuthoringIds();
	}
}
#endif
