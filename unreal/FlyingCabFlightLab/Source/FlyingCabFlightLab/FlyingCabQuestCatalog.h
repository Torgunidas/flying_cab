// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "FlyingCabQuestCatalog.generated.h"

class UFlyingCabQuestDefinition;

/** Single editor-facing index of all quest assets used by this game. */
UCLASS(BlueprintType)
class FLYINGCABFLIGHTLAB_API UFlyingCabQuestCatalog : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual void PostLoad() override;
	bool IsConfigurationValid(FString& OutError) const;
	UFlyingCabQuestDefinition* FindQuest(FName QuestId) const;

	static UFlyingCabQuestCatalog* LoadDefaultAsset();
	static const TCHAR* GetDefaultAssetPath();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Cab|Quests")
	TArray<TObjectPtr<UFlyingCabQuestDefinition>> Quests;
};
