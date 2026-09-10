#pragma once

#include "Engine/DataAsset.h"
#include "FlyingCabNpcDefinition.generated.h"

class UFlyingCabDialogueDefinition;
class UFlyingCabQuestDefinition;
class UTexture2D;

USTRUCT(BlueprintType)
struct FLYINGCABFLIGHTLAB_API FFlyingCabNpcTopic
{
	GENERATED_BODY()
	/** Empty uses the quest title. Required for topics without a quest. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Topic")
	FText Title;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Topic")
	TObjectPtr<UFlyingCabQuestDefinition> Quest;
	/** Empty uses the built-in quest conversation. Assign an asset for custom lines/branches. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Topic")
	TObjectPtr<UFlyingCabDialogueDefinition> Dialogue;
};

/** Reusable NPC identity and conversations; location belongs to the roster or placed actor. */
UCLASS(BlueprintType)
class FLYINGCABFLIGHTLAB_API UFlyingCabNpcDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC")
	FName NpcId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	FString MinimapInitial = TEXT("N");
	/** Optional face shown beside the name during a conversation. Empty leaves just the nameplate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	TSoftObjectPtr<UTexture2D> Portrait;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC", meta = (MultiLine = "true"))
	FText Greeting = NSLOCTEXT("FlyingCab", "NpcGreeting", "What can I do for you?");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC", meta = (TitleProperty = "Title"))
	TArray<FFlyingCabNpcTopic> Topics;
	bool IsConfigurationValid(FString& OutError) const;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};

USTRUCT(BlueprintType)
struct FLYINGCABFLIGHTLAB_API FFlyingCabNpcSpawn
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	TObjectPtr<UFlyingCabNpcDefinition> Profile;
	/** Used for spawn, generated platform and minimap. A placed actor with the same profile overrides this. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	FVector WorldLocation = FVector::ZeroVector;
};

UCLASS(BlueprintType)
class FLYINGCABFLIGHTLAB_API UFlyingCabNpcRoster : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPCs", meta = (TitleProperty = "Profile"))
	TArray<FFlyingCabNpcSpawn> Npcs;
	static UFlyingCabNpcRoster* LoadDefaultAsset();
	bool IsConfigurationValid(FString& OutError) const;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
