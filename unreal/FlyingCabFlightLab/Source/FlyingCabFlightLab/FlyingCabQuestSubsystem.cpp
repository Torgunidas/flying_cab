// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabQuestSubsystem.h"

#include "FlyingCabQuestCatalog.h"
#include "FlyingCabQuestDefinition.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabQuests, Log, All);

bool UFlyingCabQuestSubsystem::ConfigureCatalog(UFlyingCabQuestCatalog* InCatalog)
{
	FString ValidationError;
	if (!InCatalog || !InCatalog->IsConfigurationValid(ValidationError))
	{
		UE_LOG(LogFlyingCabQuests, Error, TEXT("Quest catalog rejected: %s"), *ValidationError);
		return false;
	}

	Catalog = InCatalog;
	Definitions.Reset();
	for (UFlyingCabQuestDefinition* Quest : Catalog->Quests)
	{
		Definitions.Add(Quest->QuestId, Quest);
	}

	TArray<FName> UnknownStateIds;
	for (const TPair<FName, FFlyingCabQuestRuntimeState>& Entry : States)
	{
		if (!Definitions.Contains(Entry.Key))
		{
			UnknownStateIds.Add(Entry.Key);
		}
	}
	for (const FName UnknownStateId : UnknownStateIds)
	{
		States.Remove(UnknownStateId);
	}
	if (!Definitions.Contains(TrackedQuestId))
	{
		SelectNextTrackedQuest();
	}
	return true;
}

void UFlyingCabQuestSubsystem::SetGameplayEventsEnabled(bool bEnabled)
{
	bGameplayEventsEnabled = bEnabled;
	if (!bGameplayEventsEnabled)
	{
		TrackedQuestId = NAME_None;
	}
	else
	{
		SelectNextTrackedQuest();
	}
}

void UFlyingCabQuestSubsystem::StartAutoQuests()
{
	if (!bGameplayEventsEnabled)
	{
		return;
	}
	for (const TPair<FName, TObjectPtr<UFlyingCabQuestDefinition>>& Entry : Definitions)
	{
		if (Entry.Value && Entry.Value->bAutoStartInFreeroam)
		{
			StartQuest(Entry.Key);
		}
	}
}

bool UFlyingCabQuestSubsystem::StartQuest(FName QuestId)
{
	UFlyingCabQuestDefinition* Definition = GetQuestDefinition(QuestId);
	if (!bGameplayEventsEnabled || !Definition)
	{
		return false;
	}

	FFlyingCabQuestRuntimeState& State = States.FindOrAdd(QuestId);
	if (State.Status != EFlyingCabQuestStatus::Inactive)
	{
		return false;
	}
	State.QuestId = QuestId;
	State.Status = EFlyingCabQuestStatus::Active;
	State.ActiveObjectiveIndex = 0;
	State.ObjectiveProgress.Init(0, Definition->Objectives.Num());
	TrackedQuestId = QuestId;
	BroadcastState(QuestId);
	UE_LOG(LogFlyingCabQuests, Display, TEXT("Quest started: %s"), *QuestId.ToString());
	return true;
}

int32 UFlyingCabQuestSubsystem::RecordEvent(FName EventId, FName TargetId, int32 Amount)
{
	if (!bGameplayEventsEnabled || EventId.IsNone() || Amount <= 0)
	{
		return 0;
	}

	TArray<FName> ActiveQuestIds;
	for (const TPair<FName, FFlyingCabQuestRuntimeState>& Entry : States)
	{
		if (Entry.Value.Status == EFlyingCabQuestStatus::Active)
		{
			ActiveQuestIds.Add(Entry.Key);
		}
	}

	int32 AdvancedQuestCount = 0;
	for (const FName QuestId : ActiveQuestIds)
	{
		FFlyingCabQuestRuntimeState* State = States.Find(QuestId);
		UFlyingCabQuestDefinition* Definition = GetQuestDefinition(QuestId);
		if (!State || !Definition || !Definition->Objectives.IsValidIndex(State->ActiveObjectiveIndex))
		{
			continue;
		}

		const FFlyingCabQuestObjectiveDefinition& Objective =
			Definition->Objectives[State->ActiveObjectiveIndex];
		if (Objective.EventId != EventId
			|| (!Objective.TargetId.IsNone() && Objective.TargetId != TargetId))
		{
			continue;
		}

		const int32 ObjectiveIndex = State->ActiveObjectiveIndex;
		if (!State->ObjectiveProgress.IsValidIndex(ObjectiveIndex))
		{
			State->ObjectiveProgress.SetNumZeroed(Definition->Objectives.Num());
		}
		State->ObjectiveProgress[ObjectiveIndex] = FMath::Clamp(
			State->ObjectiveProgress[ObjectiveIndex] + Amount,
			0,
			Objective.RequiredCount);
		++AdvancedQuestCount;

		if (State->ObjectiveProgress[ObjectiveIndex] >= Objective.RequiredCount)
		{
			++State->ActiveObjectiveIndex;
			if (State->ActiveObjectiveIndex >= Definition->Objectives.Num())
			{
				if (Definition->bRequiresTurnIn)
				{
					State->Status = EFlyingCabQuestStatus::ReadyToTurnIn;
					BroadcastState(QuestId);
				}
				else
				{
					CompleteQuest(QuestId);
				}
				continue;
			}
		}
		BroadcastState(QuestId);
	}
	return AdvancedQuestCount;
}

bool UFlyingCabQuestSubsystem::TurnInQuest(FName QuestId)
{
	const FFlyingCabQuestRuntimeState* State = States.Find(QuestId);
	return State && State->Status == EFlyingCabQuestStatus::ReadyToTurnIn
		&& CompleteQuest(QuestId);
}

bool UFlyingCabQuestSubsystem::SetTrackedQuest(FName QuestId)
{
	const FFlyingCabQuestRuntimeState* State = States.Find(QuestId);
	if (!bGameplayEventsEnabled || !State
		|| (State->Status != EFlyingCabQuestStatus::Active
			&& State->Status != EFlyingCabQuestStatus::ReadyToTurnIn))
	{
		return false;
	}
	TrackedQuestId = QuestId;
	return true;
}

void UFlyingCabQuestSubsystem::ResetAllQuests()
{
	States.Reset();
	TrackedQuestId = NAME_None;
	OnQuestStateChanged.Broadcast(NAME_None, EFlyingCabQuestStatus::Inactive);
}

EFlyingCabQuestStatus UFlyingCabQuestSubsystem::GetQuestStatus(FName QuestId) const
{
	const FFlyingCabQuestRuntimeState* State = States.Find(QuestId);
	return State ? State->Status : EFlyingCabQuestStatus::Inactive;
}

UFlyingCabQuestDefinition* UFlyingCabQuestSubsystem::GetQuestDefinition(FName QuestId) const
{
	const TObjectPtr<UFlyingCabQuestDefinition>* Found = Definitions.Find(QuestId);
	return Found ? Found->Get() : nullptr;
}

FText UFlyingCabQuestSubsystem::GetTrackerText() const
{
	if (!bGameplayEventsEnabled)
	{
		return FText::GetEmpty();
	}
	const FFlyingCabQuestRuntimeState* State = States.Find(TrackedQuestId);
	const UFlyingCabQuestDefinition* Definition = GetQuestDefinition(TrackedQuestId);
	if (!State || !Definition)
	{
		return FText::GetEmpty();
	}
	if (State->Status == EFlyingCabQuestStatus::ReadyToTurnIn)
	{
		return FText::Format(
			NSLOCTEXT("FlyingCab", "QuestReady", "QUEST // {0}\nRETURN TO QUEST GIVER"),
			Definition->Title);
	}
	if (State->Status != EFlyingCabQuestStatus::Active
		|| !Definition->Objectives.IsValidIndex(State->ActiveObjectiveIndex))
	{
		return FText::GetEmpty();
	}

	const FFlyingCabQuestObjectiveDefinition& Objective =
		Definition->Objectives[State->ActiveObjectiveIndex];
	const int32 Progress = State->ObjectiveProgress.IsValidIndex(State->ActiveObjectiveIndex)
		? State->ObjectiveProgress[State->ActiveObjectiveIndex]
		: 0;
	const FText ProgressText = Objective.RequiredCount > 1
		? FText::Format(
			NSLOCTEXT("FlyingCab", "QuestProgress", "{0}  {1}/{2}"),
			Objective.Description,
			FText::AsNumber(Progress),
			FText::AsNumber(Objective.RequiredCount))
		: Objective.Description;
	return FText::Format(
		NSLOCTEXT("FlyingCab", "QuestTracker", "QUEST // {0}\n{1}"),
		Definition->Title,
		ProgressText);
}

const FFlyingCabQuestRuntimeState* UFlyingCabQuestSubsystem::FindState(FName QuestId) const
{
	return States.Find(QuestId);
}

bool UFlyingCabQuestSubsystem::CompleteQuest(FName QuestId)
{
	FFlyingCabQuestRuntimeState* State = States.Find(QuestId);
	UFlyingCabQuestDefinition* Definition = GetQuestDefinition(QuestId);
	if (!State || !Definition || State->Status == EFlyingCabQuestStatus::Completed)
	{
		return false;
	}
	State->Status = EFlyingCabQuestStatus::Completed;
	BroadcastState(QuestId);
	OnQuestCompleted.Broadcast(Definition);
	UE_LOG(LogFlyingCabQuests, Display, TEXT("Quest completed: %s"), *QuestId.ToString());

	UFlyingCabQuestDefinition* NextDefinition = Definition->NextQuest.LoadSynchronous();
	if (NextDefinition)
	{
		if (!Definitions.Contains(NextDefinition->QuestId))
		{
			Definitions.Add(NextDefinition->QuestId, NextDefinition);
		}
		StartQuest(NextDefinition->QuestId);
	}
	else
	{
		SelectNextTrackedQuest();
	}
	return true;
}

void UFlyingCabQuestSubsystem::SelectNextTrackedQuest()
{
	TrackedQuestId = NAME_None;
	if (!bGameplayEventsEnabled)
	{
		return;
	}
	for (const TPair<FName, FFlyingCabQuestRuntimeState>& Entry : States)
	{
		if (Entry.Value.Status == EFlyingCabQuestStatus::Active
			|| Entry.Value.Status == EFlyingCabQuestStatus::ReadyToTurnIn)
		{
			TrackedQuestId = Entry.Key;
			return;
		}
	}
}

void UFlyingCabQuestSubsystem::BroadcastState(FName QuestId)
{
	OnQuestStateChanged.Broadcast(QuestId, GetQuestStatus(QuestId));
}
