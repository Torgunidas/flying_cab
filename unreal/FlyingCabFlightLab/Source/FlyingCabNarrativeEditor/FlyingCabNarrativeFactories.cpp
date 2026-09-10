#include "FlyingCabNarrativeFactories.h"
#include "AssetTypeCategories.h"
#include "FlyingCabDialogueDefinition.h"
#include "FlyingCabNpcDefinition.h"
#include "FlyingCabQuestDefinition.h"

uint32 FlyingCabNarrativeAssetCategory = EAssetTypeCategories::Misc;

UFlyingCabQuestFactory::UFlyingCabQuestFactory()
{
	SupportedClass = UFlyingCabQuestDefinition::StaticClass(); bCreateNew = true; bEditAfterNew = true;
}
FText UFlyingCabQuestFactory::GetDisplayName() const { return NSLOCTEXT("FlyingCab", "QuestAsset", "Quest"); }
uint32 UFlyingCabQuestFactory::GetMenuCategories() const { return FlyingCabNarrativeAssetCategory; }
UObject* UFlyingCabQuestFactory::FactoryCreateNew(UClass* Class, UObject* Parent, FName Name, EObjectFlags Flags, UObject*, FFeedbackContext*)
{
	auto* Quest = NewObject<UFlyingCabQuestDefinition>(Parent, Class, Name, Flags);
	Quest->Title = FText::FromString(TEXT("New assignment"));
	Quest->Description = FText::FromString(TEXT("Describe the assignment for the player."));
	FFlyingCabQuestObjectiveDefinition Objective;
	Objective.Description = FText::FromString(TEXT("Deliver a passenger")); Objective.EventId = FlyingCabQuestEvents::PassengerDelivered;
	Quest->Objectives.Add(Objective); Quest->EnsureAuthoringIds(); return Quest;
}

UFlyingCabDialogueFactory::UFlyingCabDialogueFactory()
{
	SupportedClass = UFlyingCabDialogueDefinition::StaticClass(); bCreateNew = true; bEditAfterNew = true;
}
FText UFlyingCabDialogueFactory::GetDisplayName() const { return NSLOCTEXT("FlyingCab", "DialogueAsset", "Conversation (quest template)"); }
uint32 UFlyingCabDialogueFactory::GetMenuCategories() const { return FlyingCabNarrativeAssetCategory; }
UObject* UFlyingCabDialogueFactory::FactoryCreateNew(UClass* Class, UObject* Parent, FName Name, EObjectFlags Flags, UObject*, FFeedbackContext*)
{
	auto* Dialogue = NewObject<UFlyingCabDialogueDefinition>(Parent, Class, Name, Flags);
	Dialogue->InitializeQuestTemplate();
	if (!bQuestTemplate)
	{
		Dialogue->EntryRules.Reset(); Dialogue->Nodes.Reset(); Dialogue->EntryNodeId = TEXT("Hello");
		FFlyingCabDialogueNode Node; Node.NodeId = TEXT("Hello"); Node.Text = FText::FromString(TEXT("Welcome to the city."));
		FFlyingCabDialogueChoice Choice; Choice.Text = FText::FromString(TEXT("See you.")); Node.Choices.Add(Choice); Dialogue->Nodes.Add(Node);
	}
	return Dialogue;
}

UFlyingCabNpcFactory::UFlyingCabNpcFactory()
{
	SupportedClass = UFlyingCabNpcDefinition::StaticClass(); bCreateNew = true; bEditAfterNew = true;
}
FText UFlyingCabNpcFactory::GetDisplayName() const { return NSLOCTEXT("FlyingCab", "NpcAsset", "NPC profile"); }
uint32 UFlyingCabNpcFactory::GetMenuCategories() const { return FlyingCabNarrativeAssetCategory; }
UObject* UFlyingCabNpcFactory::FactoryCreateNew(UClass* Class, UObject* Parent, FName Name, EObjectFlags Flags, UObject*, FFeedbackContext*)
{
	auto* Profile = NewObject<UFlyingCabNpcDefinition>(Parent, Class, Name, Flags);
	Profile->NpcId = InitialNpcId.IsNone() ? FName(*FString::Printf(TEXT("Npc.%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))) : InitialNpcId;
	Profile->DisplayName = FText::FromString(TEXT("New NPC")); return Profile;
}
