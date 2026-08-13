// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabQuestCatalog.h"

#include "FlyingCabQuestDefinition.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabQuestCatalog, Log, All);

namespace
{
	const TCHAR* QuestCatalogPath =
		TEXT("/Game/Data/Quests/DA_FlyingCabQuestCatalog.DA_FlyingCabQuestCatalog");

	FFlyingCabQuestObjectiveDefinition MakeObjective(
		const TCHAR* ObjectiveId,
		const TCHAR* Description,
		const FName EventId,
		int32 RequiredCount = 1)
	{
		FFlyingCabQuestObjectiveDefinition Objective;
		Objective.ObjectiveId = FName(ObjectiveId);
		Objective.Description = FText::FromString(Description);
		Objective.EventId = EventId;
		Objective.RequiredCount = RequiredCount;
		return Objective;
	}

	UFlyingCabQuestCatalog* CreateFallbackCatalog()
	{
		UFlyingCabQuestCatalog* Catalog = NewObject<UFlyingCabQuestCatalog>(
			GetTransientPackage(),
			TEXT("FlyingCabFallbackQuestCatalog"));

		UFlyingCabQuestDefinition* FirstShift = NewObject<UFlyingCabQuestDefinition>(
			Catalog,
			TEXT("Quest_FirstShift"));
		FirstShift->QuestId = TEXT("Quest.FirstShift");
		FirstShift->Title = FText::FromString(TEXT("FIRST SHIFT"));
		FirstShift->Description = FText::FromString(
			TEXT("Complete a passenger run and learn the city dispatch loop."));
		FirstShift->bAutoStartInFreeroam = true;
		FirstShift->Objectives = {
			MakeObjective(
				TEXT("BoardPassenger"),
				TEXT("Pick up a waiting passenger"),
				FlyingCabQuestEvents::PassengerPickedUp),
			MakeObjective(
				TEXT("CompleteFare"),
				TEXT("Deliver the passenger safely"),
				FlyingCabQuestEvents::PassengerDelivered)};
		FirstShift->Reward.Credits = 75;

		UFlyingCabQuestDefinition* NightshiftContract = NewObject<UFlyingCabQuestDefinition>(
			Catalog,
			TEXT("Quest_NightshiftContract"));
		NightshiftContract->QuestId = TEXT("Quest.NightshiftContract");
		NightshiftContract->Title = FText::FromString(TEXT("NIGHTSHIFT CONTRACT"));
		NightshiftContract->Description = FText::FromString(
			TEXT("Prove that you can handle two city fares, then report back."));
		NightshiftContract->bRequiresTurnIn = true;
		NightshiftContract->Objectives = {
			MakeObjective(
				TEXT("CompleteTwoFares"),
				TEXT("Complete passenger fares"),
				FlyingCabQuestEvents::PassengerDelivered,
				2)};
		NightshiftContract->Reward.Credits = 200;

		Catalog->Quests = {FirstShift, NightshiftContract};
		Catalog->AddToRoot();
		return Catalog;
	}
}

void UFlyingCabQuestCatalog::PostLoad()
{
	Super::PostLoad();
	FString ValidationError;
	if (!IsConfigurationValid(ValidationError))
	{
		UE_LOG(
			LogFlyingCabQuestCatalog,
			Error,
			TEXT("Invalid quest catalog %s: %s"),
			*GetPathName(),
			*ValidationError);
	}
}

bool UFlyingCabQuestCatalog::IsConfigurationValid(FString& OutError) const
{
	if (Quests.IsEmpty())
	{
		OutError = TEXT("The catalog must contain at least one quest.");
		return false;
	}

	TSet<FName> QuestIds;
	for (const UFlyingCabQuestDefinition* Quest : Quests)
	{
		FString QuestError;
		if (!Quest || !Quest->IsConfigurationValid(QuestError))
		{
			OutError = Quest
				? FString::Printf(TEXT("Quest %s is invalid: %s"), *Quest->GetName(), *QuestError)
				: FString(TEXT("The catalog contains an empty quest reference."));
			return false;
		}
		if (QuestIds.Contains(Quest->QuestId))
		{
			OutError = FString::Printf(
				TEXT("Quest ID %s is duplicated."),
				*Quest->QuestId.ToString());
			return false;
		}
		QuestIds.Add(Quest->QuestId);
	}

	OutError.Reset();
	return true;
}

UFlyingCabQuestDefinition* UFlyingCabQuestCatalog::FindQuest(FName QuestId) const
{
	const TObjectPtr<UFlyingCabQuestDefinition>* Found = Quests.FindByPredicate(
		[QuestId](const UFlyingCabQuestDefinition* Quest)
		{
			return Quest && Quest->QuestId == QuestId;
		});
	return Found ? Found->Get() : nullptr;
}

UFlyingCabQuestCatalog* UFlyingCabQuestCatalog::LoadDefaultAsset()
{
	static UFlyingCabQuestCatalog* Catalog = []()
	{
		UFlyingCabQuestCatalog* Loaded = LoadObject<UFlyingCabQuestCatalog>(
			nullptr,
			QuestCatalogPath);
		FString ValidationError;
		if (Loaded && Loaded->IsConfigurationValid(ValidationError))
		{
			Loaded->AddToRoot();
			return Loaded;
		}

		UE_LOG(
			LogFlyingCabQuestCatalog,
			Warning,
			TEXT("Using built-in quest definitions because %s is missing or invalid."),
			QuestCatalogPath);
		return CreateFallbackCatalog();
	}();
	return Catalog;
}

const TCHAR* UFlyingCabQuestCatalog::GetDefaultAssetPath()
{
	return QuestCatalogPath;
}
