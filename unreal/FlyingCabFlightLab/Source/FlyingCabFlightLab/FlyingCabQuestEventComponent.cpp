// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabQuestEventComponent.h"

#include "Engine/GameInstance.h"
#include "FlyingCabQuestSubsystem.h"

UFlyingCabQuestEventComponent::UFlyingCabQuestEventComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

int32 UFlyingCabQuestEventComponent::EmitQuestEvent()
{
	return EmitQuestEventOverride(EventId, TargetId, Amount);
}

int32 UFlyingCabQuestEventComponent::EmitQuestEventOverride(
	FName InEventId,
	FName InTargetId,
	int32 InAmount)
{
	if ((bEmitOnce && bHasEmitted) || InEventId.IsNone() || InAmount <= 0)
	{
		return 0;
	}
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UFlyingCabQuestSubsystem* Quests = GameInstance
		? GameInstance->GetSubsystem<UFlyingCabQuestSubsystem>()
		: nullptr;
	const int32 AdvancedQuestCount = Quests
		? Quests->RecordEvent(InEventId, InTargetId, InAmount)
		: 0;
	if (bEmitOnce && AdvancedQuestCount > 0)
	{
		bHasEmitted = true;
	}
	return AdvancedQuestCount;
}

void UFlyingCabQuestEventComponent::Configure(
	FName InEventId,
	FName InTargetId,
	int32 InAmount)
{
	EventId = InEventId;
	TargetId = InTargetId;
	Amount = FMath::Max(1, InAmount);
	bHasEmitted = false;
}
