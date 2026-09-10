// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabQuestHubData.h"
#include "FlyingCabAuthoredWorld.h"
#include "FlyingCabNpcDefinition.h"
#include "FlyingCabQuestDefinition.h"
#include "FlyingCabQuestGiver.h"
#include "EngineUtils.h"

namespace
{
	const TArray<FFlyingCabQuestHubDefinition> QuestHubs = {
		{
			TEXT("QuestGiver.Mike"),
			TEXT("MIKE"),
			TEXT("M"),
			FVector(-10000.0f, 0.0f, 10060.0f),
			FVector2D(-10000.0f, 10060.0f),
			TEXT("Quest.NightshiftContract")},
		{
			TEXT("QuestGiver.Jack"),
			TEXT("JACK"),
			TEXT("J"),
			FVector(20500.0f, 0.0f, 4860.0f),
			FVector2D(20500.0f, 4860.0f),
			TEXT("Get_Money")}};
}

TArray<FFlyingCabQuestHubDefinition> FlyingCabQuestHubData::GetQuestHubs(UWorld* World)
{
	TArray<FFlyingCabQuestHubDefinition> Result;
	auto AddProfile = [&Result](UFlyingCabNpcDefinition* Profile, const FVector& Location)
	{
		if (!Profile) return;
		FFlyingCabQuestHubDefinition Hub;
		Hub.HubId = Profile->NpcId; Hub.DisplayName = Profile->DisplayName.ToString();
		Hub.MinimapInitial = Profile->MinimapInitial; Hub.WorldLocation = Location;
		Hub.MinimapWorldPosition = FVector2D(Location.X, Location.Z); Hub.Profile = Profile;
		for (const auto& Topic : Profile->Topics) if (Topic.Quest) { Hub.QuestId = Topic.Quest->QuestId; break; }
		const int32 Existing = Result.IndexOfByPredicate([&Hub](const auto& Item) { return Item.HubId == Hub.HubId; });
		if (Existing == INDEX_NONE) Result.Add(Hub); else Result[Existing] = Hub;
	};
	if (!AFlyingCabAuthoredWorld::Find(World))
 {
	if (auto* Roster = UFlyingCabNpcRoster::LoadDefaultAsset())
	{
		for (const auto& Spawn : Roster->Npcs) AddProfile(Spawn.Profile, Spawn.WorldLocation);
	}
	else Result = QuestHubs; // Recovery only when the shipped editable roster is missing.
 }
	if (World)
		for (TActorIterator<AFlyingCabQuestGiver> It(World); It; ++It)
			AddProfile(It->GetNpcProfile(), It->GetActorLocation());
	return Result;
}
