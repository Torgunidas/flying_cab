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
		SetTrackedQuestInternal(NAME_None);
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
	State.ActivationOrder = NextActivationOrder++;
	State.CompletionOrder = 0;
	SetTrackedQuestInternal(QuestId);
	BroadcastUpdate(QuestId, EFlyingCabQuestChangeType::Started);
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
		const int32 CurrentProgress = State->ObjectiveProgress[ObjectiveIndex];

		if (CurrentProgress >= Objective.RequiredCount)
		{
			++State->ActiveObjectiveIndex;
			if (State->ActiveObjectiveIndex >= Definition->Objectives.Num())
			{
				if (Definition->bRequiresTurnIn)
				{
					State->Status = EFlyingCabQuestStatus::ReadyToTurnIn;
					BroadcastUpdate(
						QuestId,
						EFlyingCabQuestChangeType::ReadyToTurnIn,
						ObjectiveIndex,
						CurrentProgress,
						Objective.RequiredCount);
				}
				else
				{
					CompleteQuest(QuestId);
				}
				continue;
			}
			BroadcastUpdate(
				QuestId,
				EFlyingCabQuestChangeType::ObjectiveCompleted,
				ObjectiveIndex,
				CurrentProgress,
				Objective.RequiredCount);
			continue;
		}
		BroadcastUpdate(
			QuestId,
			EFlyingCabQuestChangeType::Progressed,
			ObjectiveIndex,
			CurrentProgress,
			Objective.RequiredCount);
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
	SetTrackedQuestInternal(QuestId);
	return true;
}

bool UFlyingCabQuestSubsystem::ClearTrackedQuest()
{
	if (TrackedQuestId.IsNone())
	{
		return false;
	}
	SetTrackedQuestInternal(NAME_None);
	return true;
}

void UFlyingCabQuestSubsystem::ResetAllQuests()
{
	States.Reset();
	NextActivationOrder = 1;
	NextCompletionOrder = 1;
	SetTrackedQuestInternal(NAME_None);
	OnQuestStateChanged.Broadcast(NAME_None, EFlyingCabQuestStatus::Inactive);
	FFlyingCabQuestUpdate Update;
	Update.ChangeType = EFlyingCabQuestChangeType::Reset;
	OnQuestUpdated.Broadcast(Update);
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

TArray<FFlyingCabQuestJournalEntry> UFlyingCabQuestSubsystem::GetJournalEntries() const
{
	TArray<FFlyingCabQuestJournalEntry> Entries;
	Entries.Reserve(States.Num());
	for (const TPair<FName, FFlyingCabQuestRuntimeState>& Pair : States)
	{
		const FFlyingCabQuestRuntimeState& State = Pair.Value;
		const UFlyingCabQuestDefinition* Definition = GetQuestDefinition(Pair.Key);
		if (!Definition || State.Status == EFlyingCabQuestStatus::Inactive)
		{
			continue;
		}

		FFlyingCabQuestJournalEntry& Entry = Entries.AddDefaulted_GetRef();
		Entry.QuestId = Pair.Key;
		Entry.Title = Definition->Title;
		Entry.Description = Definition->Description;
		Entry.Status = State.Status;
		Entry.RewardCredits = Definition->Reward.Credits;
		Entry.RewardAccessIds = Definition->Reward.GrantedAccessIds;
		Entry.bTracked = Pair.Key == TrackedQuestId;
		Entry.ActivationOrder = State.ActivationOrder;
		Entry.CompletionOrder = State.CompletionOrder;
		if (State.Status == EFlyingCabQuestStatus::ReadyToTurnIn)
		{
			Entry.CurrentObjective = NSLOCTEXT(
				"FlyingCab",
				"QuestJournalReturn",
				"Return to the quest giver");
		}
		else if (State.Status == EFlyingCabQuestStatus::Active
			&& Definition->Objectives.IsValidIndex(State.ActiveObjectiveIndex))
		{
			const FFlyingCabQuestObjectiveDefinition& Objective =
				Definition->Objectives[State.ActiveObjectiveIndex];
			Entry.CurrentObjective = Objective.Description;
			Entry.CurrentProgress = State.ObjectiveProgress.IsValidIndex(State.ActiveObjectiveIndex)
				? State.ObjectiveProgress[State.ActiveObjectiveIndex]
				: 0;
			Entry.RequiredProgress = Objective.RequiredCount;
		}
	}

	Entries.Sort([](
		const FFlyingCabQuestJournalEntry& A,
		const FFlyingCabQuestJournalEntry& B)
	{
		if (A.Status == EFlyingCabQuestStatus::Completed
			&& B.Status == EFlyingCabQuestStatus::Completed
			&& A.CompletionOrder != B.CompletionOrder)
		{
			return A.CompletionOrder > B.CompletionOrder;
		}
		if (A.ActivationOrder != B.ActivationOrder)
		{
			return A.ActivationOrder < B.ActivationOrder;
		}
		return A.QuestId.LexicalLess(B.QuestId);
	});
	return Entries;
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
	State->CompletionOrder = NextCompletionOrder++;
	BroadcastUpdate(QuestId, EFlyingCabQuestChangeType::Completed);
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
	if (!bGameplayEventsEnabled)
	{
		SetTrackedQuestInternal(NAME_None);
		return;
	}

	FName NextQuestId = NAME_None;
	int32 LowestActivationOrder = MAX_int32;
	for (const TPair<FName, FFlyingCabQuestRuntimeState>& Entry : States)
	{
		if (Entry.Value.Status == EFlyingCabQuestStatus::Active
			|| Entry.Value.Status == EFlyingCabQuestStatus::ReadyToTurnIn)
		{
			const int32 Order = Entry.Value.ActivationOrder > 0
				? Entry.Value.ActivationOrder
				: MAX_int32 - 1;
			if (Order < LowestActivationOrder
				|| (Order == LowestActivationOrder
					&& (NextQuestId.IsNone() || Entry.Key.LexicalLess(NextQuestId))))
			{
				LowestActivationOrder = Order;
				NextQuestId = Entry.Key;
			}
		}
	}
	SetTrackedQuestInternal(NextQuestId);
}

void UFlyingCabQuestSubsystem::BroadcastState(FName QuestId)
{
	OnQuestStateChanged.Broadcast(QuestId, GetQuestStatus(QuestId));
}

void UFlyingCabQuestSubsystem::BroadcastUpdate(
	FName QuestId,
	EFlyingCabQuestChangeType ChangeType,
	int32 ObjectiveIndex,
	int32 CurrentProgress,
	int32 RequiredProgress)
{
	BroadcastState(QuestId);
	FFlyingCabQuestUpdate Update;
	Update.QuestId = QuestId;
	Update.Status = GetQuestStatus(QuestId);
	Update.ChangeType = ChangeType;
	Update.ObjectiveIndex = ObjectiveIndex;
	Update.CurrentProgress = CurrentProgress;
	Update.RequiredProgress = RequiredProgress;
	OnQuestUpdated.Broadcast(Update);
}

void UFlyingCabQuestSubsystem::SetTrackedQuestInternal(FName QuestId)
{
	if (TrackedQuestId == QuestId)
	{
		return;
	}
	TrackedQuestId = QuestId;
	OnTrackedQuestChanged.Broadcast(TrackedQuestId);
}
