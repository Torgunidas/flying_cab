// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabQuestHubData.h"

namespace
{
	const TArray<FFlyingCabQuestHubDefinition> QuestHubs = {
		{
			TEXT("QuestGiver.Mike"),
			TEXT("MIKE"),
			TEXT("M"),
			FVector(22990.0f, 0.0f, 760.0f),
			FVector2D(470.0f, 4120.0f),
			TEXT("Quest.NightshiftContract")},
		{
			TEXT("QuestGiver.Jack"),
			TEXT("JACK"),
			TEXT("J"),
			FVector(8300.0f, 0.0f, 2620.0f),
			FVector2D(8300.0f, 2620.0f),
			TEXT("Get_Money")}};
}

TConstArrayView<FFlyingCabQuestHubDefinition> FlyingCabQuestHubData::GetQuestHubs()
{
	return MakeArrayView(QuestHubs);
}
