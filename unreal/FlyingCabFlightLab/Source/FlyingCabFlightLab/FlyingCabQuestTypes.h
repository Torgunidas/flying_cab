// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlyingCabQuestTypes.generated.h"

/** Stable lifecycle used by gameplay, UI, save data and future dialogue conditions. */
UENUM(BlueprintType)
enum class EFlyingCabQuestStatus : uint8
{
	Inactive,
	Active,
	ReadyToTurnIn,
	Completed
};

/** One sequential, event-driven objective configured inside a quest data asset. */
USTRUCT(BlueprintType)
struct FLYINGCABFLIGHTLAB_API FFlyingCabQuestObjectiveDefinition
{
	GENERATED_BODY()

	/** Stable key within this quest. It is persisted instead of localized text. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName ObjectiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FText Description;

	/** Generic gameplay event, for example Passenger.Delivered or Interaction.Completed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName EventId = NAME_None;

	/** Optional event context filter. None accepts the event from every source. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName TargetId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest", meta = (ClampMin = "1"))
	int32 RequiredCount = 1;
};

/** Rewards remain data. Their owners apply them when the quest-completed event fires. */
USTRUCT(BlueprintType)
struct FLYINGCABFLIGHTLAB_API FFlyingCabQuestReward
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward", meta = (ClampMin = "0"))
	int32 Credits = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	TArray<FName> GrantedAccessIds;
};

/** Runtime-only for now, already shaped so a future SaveGame can serialize it directly. */
USTRUCT(BlueprintType)
struct FLYINGCABFLIGHTLAB_API FFlyingCabQuestRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Quest")
	FName QuestId = NAME_None;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Quest")
	EFlyingCabQuestStatus Status = EFlyingCabQuestStatus::Inactive;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Quest")
	int32 ActiveObjectiveIndex = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Quest")
	TArray<int32> ObjectiveProgress;
};

/** Canonical event identifiers shared by native systems and Blueprint content. */
namespace FlyingCabQuestEvents
{
	FLYINGCABFLIGHTLAB_API extern const FName VehicleEntered;
	FLYINGCABFLIGHTLAB_API extern const FName VehicleExited;
	FLYINGCABFLIGHTLAB_API extern const FName PassengerPickedUp;
	FLYINGCABFLIGHTLAB_API extern const FName PassengerDelivered;
	FLYINGCABFLIGHTLAB_API extern const FName FuelPurchased;
	FLYINGCABFLIGHTLAB_API extern const FName RepairPurchased;
	FLYINGCABFLIGHTLAB_API extern const FName NearMiss;
	FLYINGCABFLIGHTLAB_API extern const FName InteractionCompleted;
	FLYINGCABFLIGHTLAB_API extern const FName QuestGiverInteracted;
	FLYINGCABFLIGHTLAB_API extern const FName AccessGranted;
}
