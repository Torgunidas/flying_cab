// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlyingCabQuestTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FlyingCabQuestSubsystem.generated.h"

class UFlyingCabQuestCatalog;
class UFlyingCabQuestDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FFlyingCabQuestStateChanged,
	FName,
	QuestId,
	EFlyingCabQuestStatus,
	Status);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FFlyingCabQuestCompleted,
	UFlyingCabQuestDefinition*,
	Quest);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FFlyingCabQuestUpdated,
	FFlyingCabQuestUpdate,
	Update);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FFlyingCabTrackedQuestChanged,
	FName,
	QuestId);

/** Persistent runtime owner of quest state. Gameplay reports events; it never edits quest data. */
UCLASS()
class FLYINGCABFLIGHTLAB_API UFlyingCabQuestSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Flying Cab|Quests")
	bool ConfigureCatalog(UFlyingCabQuestCatalog* InCatalog);

	UFUNCTION(BlueprintCallable, Category = "Flying Cab|Quests")
	void SetGameplayEventsEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Flying Cab|Quests")
	void StartAutoQuests();

	UFUNCTION(BlueprintCallable, Category = "Flying Cab|Quests")
	bool StartQuest(FName QuestId);

	/** Returns how many active quests advanced. */
	UFUNCTION(BlueprintCallable, Category = "Flying Cab|Quests")
	int32 RecordEvent(FName EventId, FName TargetId = NAME_None, int32 Amount = 1);

	UFUNCTION(BlueprintCallable, Category = "Flying Cab|Quests")
	bool TurnInQuest(FName QuestId);

	UFUNCTION(BlueprintCallable, Category = "Flying Cab|Quests")
	bool SetTrackedQuest(FName QuestId);

	UFUNCTION(BlueprintCallable, Category = "Flying Cab|Quests")
	bool ClearTrackedQuest();

	UFUNCTION(BlueprintCallable, Category = "Flying Cab|Quests")
	void ResetAllQuests();

	UFUNCTION(BlueprintPure, Category = "Flying Cab|Quests")
	EFlyingCabQuestStatus GetQuestStatus(FName QuestId) const;

	UFUNCTION(BlueprintPure, Category = "Flying Cab|Quests")
	UFlyingCabQuestDefinition* GetQuestDefinition(FName QuestId) const;

	UFUNCTION(BlueprintPure, Category = "Flying Cab|Quests")
	FName GetTrackedQuestId() const { return TrackedQuestId; }

	UFUNCTION(BlueprintPure, Category = "Flying Cab|Quests")
	FText GetTrackerText() const;

	UFUNCTION(BlueprintPure, Category = "Flying Cab|Quests")
	TArray<FFlyingCabQuestJournalEntry> GetJournalEntries() const;

	UFUNCTION(BlueprintPure, Category = "Flying Cab|Quests")
	bool AreGameplayEventsEnabled() const { return bGameplayEventsEnabled; }

	const FFlyingCabQuestRuntimeState* FindState(FName QuestId) const;

	UPROPERTY(BlueprintAssignable, Category = "Flying Cab|Quests")
	FFlyingCabQuestStateChanged OnQuestStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Flying Cab|Quests")
	FFlyingCabQuestCompleted OnQuestCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Flying Cab|Quests")
	FFlyingCabQuestUpdated OnQuestUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Flying Cab|Quests")
	FFlyingCabTrackedQuestChanged OnTrackedQuestChanged;

private:
	bool CompleteQuest(FName QuestId);
	void SelectNextTrackedQuest();
	void BroadcastState(FName QuestId);
	void BroadcastUpdate(
		FName QuestId,
		EFlyingCabQuestChangeType ChangeType,
		int32 ObjectiveIndex = INDEX_NONE,
		int32 CurrentProgress = 0,
		int32 RequiredProgress = 0);
	void SetTrackedQuestInternal(FName QuestId);

	UPROPERTY(Transient)
	TObjectPtr<UFlyingCabQuestCatalog> Catalog;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UFlyingCabQuestDefinition>> Definitions;

	UPROPERTY(Transient)
	TMap<FName, FFlyingCabQuestRuntimeState> States;

	UPROPERTY(Transient)
	FName TrackedQuestId = NAME_None;

	int32 NextActivationOrder = 1;
	int32 NextCompletionOrder = 1;

	/** Explicitly enabled by Free Roam after mode selection; pre-run possession cannot advance quests. */
	bool bGameplayEventsEnabled = false;
};
