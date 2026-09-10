#include "FlyingCabDialogueDefinition.h"
#include "Misc/DataValidation.h"

const FFlyingCabDialogueNode* UFlyingCabDialogueDefinition::FindNode(FName NodeId) const
{
	return Nodes.FindByPredicate([NodeId](const auto& Node) { return Node.NodeId == NodeId; });
}

bool UFlyingCabDialogueDefinition::IsConfigurationValid(FString& OutError) const
{
	TArray<FString> Errors;
	TSet<FName> Ids;
	for (int32 I = 0; I < Nodes.Num(); ++I)
	{
		const auto& Node = Nodes[I];
		if (Node.NodeId.IsNone() || Ids.Contains(Node.NodeId))
			Errors.Add(FString::Printf(TEXT("Node %d: ID is empty or duplicated."), I + 1));
		Ids.Add(Node.NodeId);
		if (Node.Text.IsEmpty()) Errors.Add(FString::Printf(TEXT("Node %s: text is empty."), *Node.NodeId.ToString()));
		for (int32 C = 0; C < Node.Choices.Num(); ++C)
		{
			const auto& Choice = Node.Choices[C];
			if (Choice.Text.IsEmpty()) Errors.Add(FString::Printf(TEXT("Node %s, choice %d: text is empty."), *Node.NodeId.ToString(), C + 1));
			if (Choice.Action != EFlyingCabDialogueAction::Topics && !Choice.NextNodeId.IsNone() && !FindNode(Choice.NextNodeId))
				Errors.Add(FString::Printf(TEXT("Node %s, choice %d: missing destination %s."), *Node.NodeId.ToString(), C + 1, *Choice.NextNodeId.ToString()));
		}
	}
	if (EntryNodeId.IsNone() || !FindNode(EntryNodeId)) Errors.Add(TEXT("The default entry node does not exist."));
	for (const auto& Rule : EntryRules)
		if (Rule.NodeId.IsNone() || !FindNode(Rule.NodeId)) Errors.Add(TEXT("An entry rule references a missing node."));
	OutError = FString::Join(Errors, TEXT("\n"));
	return Errors.IsEmpty();
}

UFlyingCabDialogueDefinition* UFlyingCabDialogueDefinition::CreateQuestTemplate(UObject* Outer)
{
	auto* Result = NewObject<UFlyingCabDialogueDefinition>(Outer);
	Result->InitializeQuestTemplate();
	return Result;
}

void UFlyingCabDialogueDefinition::InitializeQuestTemplate()
{
	DialogueId = FName(*FString::Printf(TEXT("Dialogue.%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
	EntryNodeId = TEXT("Offer");
	Nodes.Reset();
	EntryRules.Reset();
	auto AddChoice = [](FFlyingCabDialogueNode& Node, const FText& Text, EFlyingCabDialogueAction Action, FName Next = NAME_None)
	{
		FFlyingCabDialogueChoice Choice;
		Choice.Text = Text; Choice.Action = Action; Choice.NextNodeId = Next;
		Node.Choices.Add(Choice);
	};
	auto AddNode = [this](FName Id, const FText& Text) -> FFlyingCabDialogueNode&
	{
		auto& Node = Nodes.AddDefaulted_GetRef(); Node.NodeId = Id; Node.Text = Text; return Node;
	};
	auto& Offer = AddNode(TEXT("Offer"), NSLOCTEXT("FlyingCab", "DialogOffer", "{QuestDescription}\n\nPayment: {RewardCredits} CR."));
	AddChoice(Offer, NSLOCTEXT("FlyingCab", "DialogAccept", "I'll take the job."), EFlyingCabDialogueAction::StartQuest, TEXT("Accepted"));
	AddChoice(Offer, NSLOCTEXT("FlyingCab", "DialogDecline", "Not right now."), EFlyingCabDialogueAction::Topics);
	auto& Active = AddNode(TEXT("Active"), NSLOCTEXT("FlyingCab", "DialogActive", "Your assignment: {Objective}\nProgress: {Progress}/{Required}."));
	AddChoice(Active, NSLOCTEXT("FlyingCab", "DialogTrack", "Track this assignment."), EFlyingCabDialogueAction::TrackQuest, TEXT("Accepted"));
	AddChoice(Active, NSLOCTEXT("FlyingCab", "DialogBack", "Let's talk about something else."), EFlyingCabDialogueAction::Topics);
	auto& Ready = AddNode(TEXT("Ready"), NSLOCTEXT("FlyingCab", "DialogReady", "Everything done? Your payment is {RewardCredits} CR."));
	AddChoice(Ready, NSLOCTEXT("FlyingCab", "DialogTurnIn", "Yes. Let's settle up."), EFlyingCabDialogueAction::TurnInQuest, TEXT("Thanks"));
	AddChoice(Ready, NSLOCTEXT("FlyingCab", "DialogLater", "I'll come back later."), EFlyingCabDialogueAction::Topics);
	auto& Done = AddNode(TEXT("Done"), NSLOCTEXT("FlyingCab", "DialogDone", "Thanks for your help with {QuestTitle}."));
	AddChoice(Done, NSLOCTEXT("FlyingCab", "DialogOther", "Anything else?"), EFlyingCabDialogueAction::Topics);
	auto& Accepted = AddNode(TEXT("Accepted"), NSLOCTEXT("FlyingCab", "DialogAccepted", "All set. You can find the assignment in your shift log."));
	AddChoice(Accepted, NSLOCTEXT("FlyingCab", "DialogGoodbye", "See you."), EFlyingCabDialogueAction::Continue);
	auto& Thanks = AddNode(TEXT("Thanks"), NSLOCTEXT("FlyingCab", "DialogThanks", "We're square. Good work."));
	AddChoice(Thanks, NSLOCTEXT("FlyingCab", "DialogOther", "Anything else?"), EFlyingCabDialogueAction::Topics);
	for (auto Pair : {TPair<EFlyingCabQuestStatus, FName>(EFlyingCabQuestStatus::Active, TEXT("Active")),
		TPair<EFlyingCabQuestStatus, FName>(EFlyingCabQuestStatus::ReadyToTurnIn, TEXT("Ready")),
		TPair<EFlyingCabQuestStatus, FName>(EFlyingCabQuestStatus::Completed, TEXT("Done"))})
	{
		FFlyingCabDialogueEntry Rule; Rule.Condition.RequiredStatus = Pair.Key; Rule.NodeId = Pair.Value; EntryRules.Add(Rule);
	}
}

#if WITH_EDITOR
void UFlyingCabDialogueDefinition::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	if (DialogueId.IsNone()) DialogueId = FName(*FString::Printf(TEXT("Dialogue.%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
	TSet<FName> Used;
	for (auto& Node : Nodes)
	{
		if (Node.NodeId.IsNone() || Used.Contains(Node.NodeId))
			Node.NodeId = FName(*FString::Printf(TEXT("Line_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8)));
		Used.Add(Node.NodeId);
	}
	Super::PostEditChangeProperty(Event);
}
void UFlyingCabDialogueDefinition::PostDuplicate(EDuplicateMode::Type Mode)
{
	Super::PostDuplicate(Mode);
	if (Mode == EDuplicateMode::Normal) DialogueId = FName(*FString::Printf(TEXT("Dialogue.%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
}
EDataValidationResult UFlyingCabDialogueDefinition::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);
	FString Error;
	if (!IsConfigurationValid(Error)) { Context.AddError(FText::FromString(Error)); return EDataValidationResult::Invalid; }
	return EDataValidationResult::Valid;
}
#endif
