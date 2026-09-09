// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabQuestHubData.h"

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

TConstArrayView<FFlyingCabQuestHubDefinition> FlyingCabQuestHubData::GetQuestHubs()
{
	return MakeArrayView(QuestHubs);
}
