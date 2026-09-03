// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabQuestCatalog.h"

#include "FlyingCabCityData.h"
#include "FlyingCabQuestDefinition.h"
#include "Misc/DataValidation.h"

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
		FirstShift->Category = EFlyingCabQuestCategory::Main;
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
		NightshiftContract->Category = EFlyingCabQuestCategory::Main;
		NightshiftContract->bRequiresTurnIn = true;
		NightshiftContract->Objectives = {
			MakeObjective(
				TEXT("CompleteTwoFares"),
				TEXT("Complete passenger fares"),
				FlyingCabQuestEvents::PassengerDelivered,
				2)};
		NightshiftContract->Reward.Credits = 200;

		UFlyingCabQuestDefinition* Money = NewObject<UFlyingCabQuestDefinition>(
			Catalog,
			TEXT("Quest_Money"));
		Money->QuestId = TEXT("Get_Money");
		Money->Title = FText::FromString(TEXT("GET MONEY"));
		Money->Description = FText::FromString(
			TEXT("Run fares and earn 1000 credits across the city."));
		Money->Category = EFlyingCabQuestCategory::Side;
		Money->Objectives = {
			MakeObjective(
				TEXT("EarnCredits"),
				TEXT("Earn credits from fares"),
				FlyingCabQuestEvents::CreditsEarned,
				1000)};
		Money->Reward.Credits = 250;

		Catalog->Quests = {FirstShift, NightshiftContract, Money};
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

#if WITH_EDITOR
EDataValidationResult UFlyingCabQuestCatalog::IsDataValid(
	FDataValidationContext& Context) const
{
	const EDataValidationResult ParentResult = Super::IsDataValid(Context);
	FString ValidationError;
	if (!IsConfigurationValid(ValidationError))
	{
		Context.AddError(FText::FromString(ValidationError));
		return EDataValidationResult::Invalid;
	}
	return ParentResult;
}
#endif

bool UFlyingCabQuestCatalog::IsConfigurationValid(FString& OutError) const
{
	if (Quests.IsEmpty())
	{
		OutError = TEXT("The catalog must contain at least one quest.");
		return false;
	}

	TSet<FName> CustomEventIds;
	for (const FName EventId : AllowedCustomEventIds)
	{
		if (EventId.IsNone() || FlyingCabQuestEvents::IsKnownEvent(EventId)
			|| CustomEventIds.Contains(EventId))
		{
			OutError = TEXT("Custom event IDs must be non-empty, unique and distinct from native events.");
			return false;
		}
		CustomEventIds.Add(EventId);
	}

	TSet<FName> QuestIds;
	for (const UFlyingCabQuestDefinition* Quest : Quests)
	{
		FString QuestError;
		if (!IsQuestEntryValid(Quest, QuestError))
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

bool UFlyingCabQuestCatalog::IsQuestEntryValid(
	const UFlyingCabQuestDefinition* Quest,
	FString& OutError) const
{
	if (!Quest)
	{
		OutError = TEXT("The catalog contains an empty quest reference.");
		return false;
	}
	if (!Quest->IsConfigurationValid(OutError))
	{
		return false;
	}
	for (const FFlyingCabQuestObjectiveDefinition& Objective : Quest->Objectives)
	{
		if (!FlyingCabQuestEvents::IsKnownEvent(Objective.EventId)
			&& !AllowedCustomEventIds.Contains(Objective.EventId))
		{
			OutError = FString::Printf(
				TEXT("Objective %s uses unknown event ID %s. Add intentional Blueprint events to AllowedCustomEventIds."),
				*Objective.ObjectiveId.ToString(),
				*Objective.EventId.ToString());
			return false;
		}
		const bool bPassengerDistrictEvent =
			Objective.EventId == FlyingCabQuestEvents::PassengerPickedUp
			|| Objective.EventId == FlyingCabQuestEvents::PassengerDelivered;
		if (bPassengerDistrictEvent && !Objective.TargetId.IsNone())
		{
			const bool bKnownDistrict = FlyingCabCityData::GetDistricts().ContainsByPredicate(
				[&Objective](const FFlyingCabDistrictDefinition& District)
				{
					return District.DistrictId == Objective.TargetId;
				});
			if (!bKnownDistrict)
			{
				OutError = FString::Printf(
					TEXT("Objective %s uses unknown passenger district TargetId %s."),
					*Objective.ObjectiveId.ToString(),
					*Objective.TargetId.ToString());
				return false;
			}
		}
	}
	OutError.Reset();
	return true;
}

UFlyingCabQuestDefinition* UFlyingCabQuestCatalog::FindQuest(FName QuestId) const
{
	const TObjectPtr<UFlyingCabQuestDefinition>* Found = Quests.FindByPredicate(
		[this, QuestId](const UFlyingCabQuestDefinition* Quest)
		{
			FString ValidationError;
			return Quest && Quest->QuestId == QuestId
				&& IsQuestEntryValid(Quest, ValidationError);
		});
	return Found ? Found->Get() : nullptr;
}

UFlyingCabQuestCatalog* UFlyingCabQuestCatalog::LoadDefaultAsset()
{
	UFlyingCabQuestCatalog* Loaded = LoadObject<UFlyingCabQuestCatalog>(
		nullptr,
		QuestCatalogPath);
	if (Loaded)
	{
		for (const UFlyingCabQuestDefinition* Quest : Loaded->Quests)
		{
			FString ValidationError;
			if (Loaded->IsQuestEntryValid(Quest, ValidationError))
			{
				return Loaded;
			}
		}
	}

	UE_LOG(
		LogFlyingCabQuestCatalog,
		Warning,
		TEXT("Using built-in quest definitions because %s has no usable quests."),
		QuestCatalogPath);
	return CreateFallbackCatalog();
}

const TCHAR* UFlyingCabQuestCatalog::GetDefaultAssetPath()
{
	return QuestCatalogPath;
}
