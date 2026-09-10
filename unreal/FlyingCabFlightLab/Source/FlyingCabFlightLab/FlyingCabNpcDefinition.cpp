#include "FlyingCabNpcDefinition.h"
#include "FlyingCabDialogueDefinition.h"
#include "FlyingCabQuestCatalog.h"
#include "FlyingCabNarrativeSettings.h"
#include "FlyingCabQuestDefinition.h"
#include "Misc/DataValidation.h"

bool UFlyingCabNpcDefinition::IsConfigurationValid(FString& OutError) const
{
	TArray<FString> Errors;
	if (NpcId.IsNone() || DisplayName.IsEmpty()) Errors.Add(TEXT("NPC identity and display name are required."));
	for (int32 I = 0; I < Topics.Num(); ++I)
	{
		const auto& Topic = Topics[I];
		if (!Topic.Quest && (!Topic.Dialogue || Topic.Title.IsEmpty()))
			Errors.Add(FString::Printf(TEXT("Topic %d: choose a quest, or supply a title and conversation."), I + 1));
		FString Error;
		if (Topic.Dialogue && !Topic.Dialogue->IsConfigurationValid(Error))
			Errors.Add(FString::Printf(TEXT("Topic %d: %s"), I + 1, *Error));
	}
	OutError = FString::Join(Errors, TEXT("\n"));
	return Errors.IsEmpty();
}

UFlyingCabNpcRoster* UFlyingCabNpcRoster::LoadDefaultAsset()
{
	return GetDefault<UFlyingCabNarrativeSettings>()->NpcRoster.LoadSynchronous();
}

bool UFlyingCabNpcRoster::IsConfigurationValid(FString& OutError) const
{
	TArray<FString> Errors;
	TSet<FName> Ids;
	for (int32 I = 0; I < Npcs.Num(); ++I)
	{
		const auto& Spawn = Npcs[I];
		FString Error;
		if (!Spawn.Profile || !Spawn.Profile->IsConfigurationValid(Error))
			Errors.Add(FString::Printf(TEXT("NPC %d: %s"), I + 1, Spawn.Profile ? *Error : TEXT("choose a profile.")));
		else if (Ids.Contains(Spawn.Profile->NpcId)) Errors.Add(FString::Printf(TEXT("Duplicate NPC ID: %s"), *Spawn.Profile->NpcId.ToString()));
		if (Spawn.Profile) Ids.Add(Spawn.Profile->NpcId);
		if (Spawn.WorldLocation.ContainsNaN()) Errors.Add(TEXT("NPC position must be finite."));
	}
	OutError = FString::Join(Errors, TEXT("\n"));
	return Errors.IsEmpty();
}

#if WITH_EDITOR
void UFlyingCabNpcDefinition::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	if (NpcId.IsNone()) NpcId = FName(*FString::Printf(TEXT("Npc.%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
	Super::PostEditChangeProperty(Event);
}
void UFlyingCabNpcDefinition::PostDuplicate(EDuplicateMode::Type Mode)
{
	Super::PostDuplicate(Mode);
	if (Mode == EDuplicateMode::Normal) NpcId = FName(*FString::Printf(TEXT("Npc.%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
}
EDataValidationResult UFlyingCabNpcDefinition::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);
	FString Error;
	bool bValid = IsConfigurationValid(Error);
	if (!bValid) Context.AddError(FText::FromString(Error));
	const auto* Catalog = UFlyingCabQuestCatalog::LoadDefaultAsset();
	for (const auto& Topic : Topics)
	{
		if (Topic.Quest && (!Catalog || !Catalog->Quests.Contains(Topic.Quest)))
		{
			Context.AddError(FText::Format(NSLOCTEXT("FlyingCab", "TopicUnregistered", "Quest {0} is not in the quest catalog. Open it and use Add to catalog."), Topic.Quest->Title));
			bValid = false;
		}
	}
	return bValid ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
EDataValidationResult UFlyingCabNpcRoster::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);
	FString Error;
	if (!IsConfigurationValid(Error)) { Context.AddError(FText::FromString(Error)); return EDataValidationResult::Invalid; }
	return EDataValidationResult::Valid;
}
#endif
