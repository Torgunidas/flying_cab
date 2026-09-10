#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "FlyingCabQuestCatalog.h"
#include "FlyingCabQuestDefinition.h"
#include "FlyingCabQuestSubsystem.h"
#include "FlyingCabDialogueDefinition.h"
#include "FlyingCabDialogueSession.h"
#include "FlyingCabNpcDefinition.h"

namespace
{
	UFlyingCabQuestDefinition* MakeNarrativeTestQuest(UObject* Outer, const TCHAR* Id)
	{
		auto* Quest = NewObject<UFlyingCabQuestDefinition>(Outer);
		Quest->QuestId = Id;
		Quest->Title = FText::FromString(Id);
		FFlyingCabQuestObjectiveDefinition Objective;
		Objective.ObjectiveId = TEXT("Deliver");
		Objective.Description = FText::FromString(TEXT("Deliver passengers"));
		Objective.EventId = FlyingCabQuestEvents::PassengerDelivered;
		Quest->Objectives.Add(Objective);
		return Quest;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyingCabQuestChainValidationTest,
	"FlyingCab.Core.Quests.ValidatedChains", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlyingCabQuestChainValidationTest::RunTest(const FString&)
{
	auto* GI = NewObject<UGameInstance>();
	auto* Catalog = NewObject<UFlyingCabQuestCatalog>(GI);
	auto* A = MakeNarrativeTestQuest(Catalog, TEXT("Quest.A"));
	auto* B = MakeNarrativeTestQuest(Catalog, TEXT("Quest.B"));
	Catalog->Quests = {A, B};
	A->NextQuest = B;
	FString Error;
	TestTrue(TEXT("Valid registered chain"), Catalog->IsConfigurationValid(Error));
	B->NextQuest = A;
	TestFalse(TEXT("Cycle rejected"), Catalog->IsConfigurationValid(Error));
	B->NextQuest.Reset();
	Catalog->Quests = {A};
	TestFalse(TEXT("Unregistered successor rejected"), Catalog->IsConfigurationValid(Error));
	Catalog->Quests = {A, B};
	B->Objectives.Reset();
	TestFalse(TEXT("Invalid successor rejected"), Catalog->IsConfigurationValid(Error));
	// A valid independent quest remains usable when another chain is broken.
	auto* C = MakeNarrativeTestQuest(Catalog, TEXT("Quest.C"));
	Catalog->Quests.Add(C);
	auto* Quests = NewObject<UFlyingCabQuestSubsystem>(GI);
	TestTrue(TEXT("Independent content survives"), Quests->ConfigureCatalog(Catalog));
	Quests->SetGameplayEventsEnabled(true);
	TestFalse(TEXT("Invalid chain cannot start"), Quests->StartQuest(A->QuestId));
	TestTrue(TEXT("Independent quest can start"), Quests->StartQuest(C->QuestId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyingCabDialogueLifecycleTest,
	"FlyingCab.Core.Dialogue.AcceptDeclineAndTurnIn", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlyingCabDialogueLifecycleTest::RunTest(const FString&)
{
	auto* GI = NewObject<UGameInstance>();
	auto* Quests = NewObject<UFlyingCabQuestSubsystem>(GI);
	auto* Catalog = NewObject<UFlyingCabQuestCatalog>(GI);
	auto* Quest = MakeNarrativeTestQuest(Catalog, TEXT("Quest.Dialogue"));
	Quest->bRequiresTurnIn = true;
	Catalog->Quests = {Quest};
	Quests->ConfigureCatalog(Catalog);
	Quests->SetGameplayEventsEnabled(true);
	auto* Profile = NewObject<UFlyingCabNpcDefinition>(GI);
	Profile->NpcId = TEXT("QuestGiver.Test");
	Profile->DisplayName = FText::FromString(TEXT("Test NPC"));
	FFlyingCabNpcTopic Topic;
	Topic.Quest = Quest;
	Topic.Dialogue = UFlyingCabDialogueDefinition::CreateQuestTemplate(Profile);
	Profile->Topics.Add(Topic);
	auto* Session = NewObject<UFlyingCabDialogueSession>(GI);
	TestTrue(TEXT("Open NPC"), Session->Start(Profile, Quests));
	TestEqual(TEXT("Opening is not acceptance"), Quests->GetQuestStatus(Quest->QuestId), EFlyingCabQuestStatus::Inactive);
	Session->ChooseOption(0, Session->GetView().Revision);
	Session->Cancel();
	TestEqual(TEXT("Cancel leaves quest inactive"), Quests->GetQuestStatus(Quest->QuestId), EFlyingCabQuestStatus::Inactive);
	Session->Start(Profile, Quests);
	Session->ChooseOption(0, Session->GetView().Revision);
	const int32 OfferRevision = Session->GetView().Revision;
	TestTrue(TEXT("Accept succeeds"), Session->ChooseOption(0, OfferRevision));
	TestEqual(TEXT("Quest accepted"), Quests->GetQuestStatus(Quest->QuestId), EFlyingCabQuestStatus::Active);
	TestFalse(TEXT("Stale double click is ignored"), Session->ChooseOption(0, OfferRevision));
	Session->Cancel();
	Quests->RecordEvent(FlyingCabQuestEvents::PassengerDelivered);
	Session->Start(Profile, Quests);
	Session->ChooseOption(0, Session->GetView().Revision);
	TestTrue(TEXT("Turn in succeeds"), Session->ChooseOption(0, Session->GetView().Revision));
	TestEqual(TEXT("Quest completed"), Quests->GetQuestStatus(Quest->QuestId), EFlyingCabQuestStatus::Completed);
	TestFalse(TEXT("Cannot turn in twice"), Quests->TurnInQuest(Quest->QuestId));
	Session->Cancel();
	Quests->SetGameplayEventsEnabled(false);
	TestFalse(TEXT("Dialogue cannot mutate competitive mode"), Session->Start(Profile, Quests));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyingCabDialoguePreviewTest,
	"FlyingCab.Core.Dialogue.PreviewAndValidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlyingCabDialoguePreviewTest::RunTest(const FString&)
{
	auto* Profile = NewObject<UFlyingCabNpcDefinition>();
	Profile->NpcId = TEXT("Npc.Preview");
	Profile->DisplayName = FText::FromString(TEXT("Preview"));
	auto* Quest = MakeNarrativeTestQuest(Profile, TEXT("Quest.Preview"));
	auto* Dialogue = UFlyingCabDialogueDefinition::CreateQuestTemplate(Profile);
	FFlyingCabNpcTopic Topic;
	Topic.Quest = Quest;
	Topic.Dialogue = Dialogue;
	Profile->Topics.Add(Topic);
	FString Error;
	TestTrue(TEXT("Template is valid"), Dialogue->IsConfigurationValid(Error));
	auto* Session = NewObject<UFlyingCabDialogueSession>();
	TestTrue(TEXT("Preview starts without a game"), Session->StartPreview(Profile, EFlyingCabQuestStatus::Inactive));
	Session->ChooseOption(0, Session->GetView().Revision);
	Session->ChooseOption(0, Session->GetView().Revision);
	TestTrue(TEXT("Preview executes simulated choices"), Session->IsActive());
	TestTrue(TEXT("Definition is not mutated by preview"), Quest->Objectives.Num() == 1);
	Dialogue->Nodes[0].Choices[0].NextNodeId = TEXT("MissingNode");
	TestFalse(TEXT("Broken transition rejected"), Dialogue->IsConfigurationValid(Error));
	return true;
}

#endif
