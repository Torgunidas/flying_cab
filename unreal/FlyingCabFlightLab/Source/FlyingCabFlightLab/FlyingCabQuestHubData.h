// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/ArrayView.h"
#include "CoreMinimal.h"

class UFlyingCabNpcDefinition;
class UWorld;

/** Projection of the NPC roster and optional placed actors, shared by spawn, platforms and minimap. */
struct FFlyingCabQuestHubDefinition
{
	FName HubId = NAME_None;
	FString DisplayName;
	FString MinimapInitial;
	FVector WorldLocation = FVector::ZeroVector;
	FVector2D MinimapWorldPosition = FVector2D::ZeroVector;
	FName QuestId = NAME_None;
	UFlyingCabNpcDefinition* Profile = nullptr;
};

namespace FlyingCabQuestHubData
{
	FLYINGCABFLIGHTLAB_API TArray<FFlyingCabQuestHubDefinition> GetQuestHubs(UWorld* World = nullptr);
}
