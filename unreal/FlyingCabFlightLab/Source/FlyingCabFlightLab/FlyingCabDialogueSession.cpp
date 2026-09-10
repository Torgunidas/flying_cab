#include "FlyingCabDialogueSession.h"
#include "FlyingCabNpcDefinition.h"
#include "FlyingCabQuestDefinition.h"
#include "FlyingCabQuestSubsystem.h"

bool UFlyingCabDialogueSession::Start(UFlyingCabNpcDefinition* InProfile, UFlyingCabQuestSubsystem* InQuests)
{
	bPreview = false; Quests = InQuests; PreviewStates.Reset();
	if (!Quests || !Quests->AreGameplayEventsEnabled()) return false;
	return Begin(InProfile);
}

bool UFlyingCabDialogueSession::StartPreview(UFlyingCabNpcDefinition* InProfile, EFlyingCabQuestStatus Status)
{
	bPreview = true; Quests = nullptr; PreviewStates.Reset(); PreviewDefaultStatus = Status;
	return Begin(InProfile);
}

bool UFlyingCabDialogueSession::Begin(UFlyingCabNpcDefinition* InProfile)
{
	FString Error;
	if (!InProfile || !InProfile->IsConfigurationValid(Error)) return false;
	Profile = InProfile; bActive = true; bVisitedTopic = false; View.Feedback = FText::GetEmpty();
	ShowTopics();
	return true;
}

UFlyingCabQuestDefinition* UFlyingCabDialogueSession::ResolveQuest(UFlyingCabQuestDefinition* ExplicitQuest) const
{
	return ExplicitQuest ? ExplicitQuest : ContextQuest.Get();
}

EFlyingCabQuestStatus UFlyingCabDialogueSession::GetStatus(const UFlyingCabQuestDefinition* Quest) const
{
	if (!Quest) return EFlyingCabQuestStatus::Inactive;
	if (bPreview)
	{
		const auto* Status = PreviewStates.Find(Quest->QuestId);
		return Status ? *Status : PreviewDefaultStatus;
	}
	return Quests ? Quests->GetQuestStatus(Quest->QuestId) : EFlyingCabQuestStatus::Inactive;
}

bool UFlyingCabDialogueSession::MeetsCondition(const FFlyingCabDialogueCondition& Condition, FText& Reason) const
{
	auto* Quest = ResolveQuest(Condition.Quest);
	if (!Quest || (!bPreview && (!Quests || Quests->GetQuestDefinition(Quest->QuestId) != Quest)))
	{
		Reason = NSLOCTEXT("FlyingCab", "DialogMissingQuest", "This topic needs a quest registered in the catalog.");
		return false;
	}
	const bool bMatches = GetStatus(Quest) == Condition.RequiredStatus;
	if (bMatches != Condition.bInvert) return true;
	Reason = FText::Format(NSLOCTEXT("FlyingCab", "DialogConditionFailed", "{0}: required quest state is {1}{2}."), Quest->Title,
		Condition.bInvert ? FText::FromString(TEXT("not ")) : FText::GetEmpty(),
		StaticEnum<EFlyingCabQuestStatus>()->GetDisplayNameTextByValue(static_cast<int64>(Condition.RequiredStatus)));
	return false;
}

bool UFlyingCabDialogueSession::CanChoose(const FFlyingCabDialogueChoice& Choice, FText& Reason) const
{
	for (const auto& Condition : Choice.Conditions) if (!MeetsCondition(Condition, Reason)) return false;
	if (Choice.Action == EFlyingCabDialogueAction::Continue || Choice.Action == EFlyingCabDialogueAction::Topics) return true;
	auto* Quest = ResolveQuest(Choice.Quest);
	if (!Quest || (!bPreview && (!Quests || Quests->GetQuestDefinition(Quest->QuestId) != Quest)))
	{
		Reason = NSLOCTEXT("FlyingCab", "DialogMissingQuest", "This topic needs a quest registered in the catalog."); return false;
	}
	const auto Status = GetStatus(Quest);
	if (Choice.Action == EFlyingCabDialogueAction::StartQuest)
	{
		if (!bPreview) return Quests->CanStartQuest(Quest->QuestId, Reason);
		if (Status == EFlyingCabQuestStatus::Inactive)
		{
			for (const auto& Prerequisite : Quest->PrerequisiteQuests)
				if (!Prerequisite || GetStatus(Prerequisite) != EFlyingCabQuestStatus::Completed)
				{ Reason = NSLOCTEXT("FlyingCab", "PreviewPrerequisite", "A prerequisite quest is not completed in this preview state."); return false; }
			return true;
		}
	}
	if (Choice.Action == EFlyingCabDialogueAction::TurnInQuest && Status == EFlyingCabQuestStatus::ReadyToTurnIn)
	{
		if (!Quest->TurnInNpcId.IsNone() && Quest->TurnInNpcId != Profile->NpcId)
		{ Reason = NSLOCTEXT("FlyingCab", "WrongTurnInNpc", "Return to the NPC assigned to receive this quest."); return false; }
		return true;
	}
	if (Choice.Action == EFlyingCabDialogueAction::TrackQuest &&
		(Status == EFlyingCabQuestStatus::Active || Status == EFlyingCabQuestStatus::ReadyToTurnIn)) return true;
	Reason = NSLOCTEXT("FlyingCab", "DialogWrongState", "This action is unavailable in the current quest state.");
	return false;
}

FText UFlyingCabDialogueSession::FormatText(const FText& Text) const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("QuestTitle"), ContextQuest ? ContextQuest->Title : FText::GetEmpty());
	Args.Add(TEXT("QuestDescription"), ContextQuest ? ContextQuest->Description : FText::GetEmpty());
	Args.Add(TEXT("RewardCredits"), ContextQuest ? ContextQuest->Reward.Credits : 0);
	FText Objective; int32 Progress = 0, Required = 0;
	if (ContextQuest && !ContextQuest->Objectives.IsEmpty())
	{
		int32 Index = 0;
		if (!bPreview && Quests)
			if (const auto* State = Quests->FindState(ContextQuest->QuestId))
			{
				Index = FMath::Clamp(State->ActiveObjectiveIndex, 0, ContextQuest->Objectives.Num() - 1);
				if (State->ObjectiveProgress.IsValidIndex(Index)) Progress = State->ObjectiveProgress[Index];
			}
		Objective = ContextQuest->Objectives[Index].Description;
		Required = ContextQuest->Objectives[Index].RequiredCount;
		if (bPreview && (GetStatus(ContextQuest) == EFlyingCabQuestStatus::ReadyToTurnIn || GetStatus(ContextQuest) == EFlyingCabQuestStatus::Completed)) Progress = Required;
	}
	Args.Add(TEXT("Objective"), Objective); Args.Add(TEXT("Progress"), Progress); Args.Add(TEXT("Required"), Required);
	return FText::Format(Text, Args);
}

void UFlyingCabDialogueSession::ShowTopics()
{
	bTopics = true; Dialogue = nullptr; ContextQuest = nullptr; CurrentNode = NAME_None;
	Refresh();
}

bool UFlyingCabDialogueSession::OpenTopic(int32 Index)
{
	if (!Profile || !Profile->Topics.IsValidIndex(Index)) return false;
	const auto& Topic = Profile->Topics[Index];
	ContextQuest = Topic.Quest;
	Dialogue = Topic.Dialogue ? Topic.Dialogue.Get() : UFlyingCabDialogueDefinition::CreateQuestTemplate(this);
	FString Error;
	if (!Dialogue->IsConfigurationValid(Error)) { View.Feedback = FText::FromString(Error); Refresh(); return false; }
	FName Entry = Dialogue->EntryNodeId;
	for (const auto& Rule : Dialogue->EntryRules)
	{
		FText Reason;
		if (MeetsCondition(Rule.Condition, Reason)) { Entry = Rule.NodeId; break; }
	}
	bVisitedTopic = true;
	return GoToNode(Entry);
}

bool UFlyingCabDialogueSession::GoToNode(FName NodeId)
{
	if (NodeId.IsNone()) { Finish(true); return true; }
	if (!Dialogue || !Dialogue->FindNode(NodeId)) return false;
	CurrentNode = NodeId; bTopics = false; Refresh(); return true;
}

void UFlyingCabDialogueSession::Refresh()
{
	++View.Revision; View.Options.Reset(); ChoiceIndices.Reset();
	if (!bActive || !Profile) { OnChanged.Broadcast(); return; }
	View.Speaker = Profile->DisplayName;
	View.Portrait = Profile->Portrait;
	if (bTopics)
	{
		View.Text = Profile->Greeting;
		for (int32 I = 0; I < Profile->Topics.Num(); ++I)
		{
			const auto& Topic = Profile->Topics[I];
			auto& Option = View.Options.AddDefaulted_GetRef();
			Option.Text = !Topic.Title.IsEmpty() ? Topic.Title : Topic.Quest ? Topic.Quest->Title : NSLOCTEXT("FlyingCab", "Conversation", "Conversation");
			ChoiceIndices.Add(I);
		}
		auto& Exit = View.Options.AddDefaulted_GetRef(); Exit.Text = NSLOCTEXT("FlyingCab", "DialogGoodbye", "See you."); ChoiceIndices.Add(INDEX_NONE);
	}
	else if (const auto* Node = Dialogue ? Dialogue->FindNode(CurrentNode) : nullptr)
	{
		if (!Node->Speaker.IsEmpty()) View.Speaker = Node->Speaker;
		View.Text = FormatText(Node->Text);
		for (int32 I = 0; I < Node->Choices.Num(); ++I)
		{
			const auto& Choice = Node->Choices[I]; FText Reason;
			const bool bEnabled = CanChoose(Choice, Reason);
			if (!bEnabled && Choice.bHideWhenUnavailable) continue;
			auto& Option = View.Options.AddDefaulted_GetRef();
			Option.Text = FormatText(Choice.Text); Option.bEnabled = bEnabled; Option.UnavailableReason = Reason;
			ChoiceIndices.Add(I);
		}
		// A terminal line still has an explicit user-driven exit, never an automatic loop.
		if (View.Options.IsEmpty())
		{
			auto& Exit = View.Options.AddDefaulted_GetRef(); Exit.Text = NSLOCTEXT("FlyingCab", "DialogFinish", "Finish conversation"); ChoiceIndices.Add(INDEX_NONE);
		}
	}
	OnChanged.Broadcast();
}

bool UFlyingCabDialogueSession::ChooseOption(int32 Index, int32 ExpectedRevision)
{
	if (!bActive || bChoosing || ExpectedRevision != View.Revision || !ChoiceIndices.IsValidIndex(Index)) return false;
	if (!bPreview && (!Quests || !Quests->AreGameplayEventsEnabled())) { Cancel(); return false; }
	TGuardValue<bool> Guard(bChoosing, true);
	View.Feedback = FText::GetEmpty();
	const int32 ChoiceIndex = ChoiceIndices[Index];
	if (ChoiceIndex == INDEX_NONE) { Finish(true); return true; }
	if (bTopics) return OpenTopic(ChoiceIndex);
	const auto* Node = Dialogue ? Dialogue->FindNode(CurrentNode) : nullptr;
	if (!Node || !Node->Choices.IsValidIndex(ChoiceIndex)) return false;
	// Copy before commands broadcast arbitrary gameplay delegates.
	const FFlyingCabDialogueChoice Choice = Node->Choices[ChoiceIndex];
	FText Reason;
	if (!CanChoose(Choice, Reason)) { View.Feedback = Reason; Refresh(); return false; }
	auto* Quest = ResolveQuest(Choice.Quest);
	bool bSuccess = true;
	if (Choice.Action == EFlyingCabDialogueAction::Topics) { ShowTopics(); return true; }
	if (Choice.Action != EFlyingCabDialogueAction::Continue)
	{
		if (bPreview)
		{
			if (Choice.Action == EFlyingCabDialogueAction::StartQuest) PreviewStates.Add(Quest->QuestId, EFlyingCabQuestStatus::Active);
			if (Choice.Action == EFlyingCabDialogueAction::TurnInQuest) PreviewStates.Add(Quest->QuestId, EFlyingCabQuestStatus::Completed);
			View.Feedback = NSLOCTEXT("FlyingCab", "PreviewEffect", "Preview: action simulated; no game state or assets changed.");
		}
		else switch (Choice.Action)
		{
		case EFlyingCabDialogueAction::StartQuest: bSuccess = Quests->StartQuest(Quest->QuestId); break;
		case EFlyingCabDialogueAction::TurnInQuest: bSuccess = Quests->TurnInQuestAtNpc(Quest->QuestId, Profile->NpcId); break;
		case EFlyingCabDialogueAction::TrackQuest: bSuccess = Quests->SetTrackedQuest(Quest->QuestId); break;
		default: break;
		}
	}
	if (!bSuccess)
	{
		View.Feedback = NSLOCTEXT("FlyingCab", "DialogActionFailed", "The assignment changed. Please choose again."); Refresh(); return false;
	}
	if (!bActive) return true;
	return GoToNode(Choice.NextNodeId);
}

void UFlyingCabDialogueSession::Finish(bool bCompletedConversation)
{
	if (!bActive) return;
	bActive = false;
	// Existing event types, now emitted on finishing a topic rather than opening the modal.
	if (bCompletedConversation && bVisitedTopic && !bPreview && Quests && Profile)
	{
		Quests->RecordEvent(FlyingCabQuestEvents::QuestGiverInteracted, Profile->NpcId);
		Quests->RecordEvent(FlyingCabQuestEvents::InteractionCompleted, Profile->NpcId);
	}
	Refresh();
}

void UFlyingCabDialogueSession::Cancel()
{
	Finish(false);
}
