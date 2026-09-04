// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/ArrayView.h"
#include "CoreMinimal.h"

/** Shared, code-native placement for the first quest hubs and their minimap markers. */
struct FFlyingCabQuestHubDefinition
{
	FName HubId = NAME_None;
	FString DisplayName;
	FString MinimapInitial;
	FVector WorldLocation = FVector::ZeroVector;
	FVector2D MinimapWorldPosition = FVector2D::ZeroVector;
	FName QuestId = NAME_None;
};

namespace FlyingCabQuestHubData
{
	FLYINGCABFLIGHTLAB_API TConstArrayView<FFlyingCabQuestHubDefinition> GetQuestHubs();
}
