#pragma once

#include "Engine/DataAsset.h"
#include "FlyingCabQuestTypes.h"
#include "FlyingCabDialogueDefinition.generated.h"

class UFlyingCabQuestDefinition;

UENUM(BlueprintType)
enum class EFlyingCabDialogueAction : uint8
{
	Continue,
	StartQuest UMETA(DisplayName = "Accept quest"),
	TurnInQuest UMETA(DisplayName = "Turn in quest"),
	TrackQuest UMETA(DisplayName = "Track quest"),
	Topics UMETA(DisplayName = "Back to topics")
};

/** An empty Quest reference uses the NPC topic's quest. Conditions are ANDed. */
USTRUCT(BlueprintType)
struct FLYINGCABFLIGHTLAB_API FFlyingCabDialogueCondition
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition")
	TObjectPtr<UFlyingCabQuestDefinition> Quest;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition")
	EFlyingCabQuestStatus RequiredStatus = EFlyingCabQuestStatus::Inactive;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition")
	bool bInvert = false;
};

USTRUCT(BlueprintType)
struct FLYINGCABFLIGHTLAB_API FFlyingCabDialogueChoice
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Choice", meta = (MultiLine = "true"))
	FText Text;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Choice")
	EFlyingCabDialogueAction Action = EFlyingCabDialogueAction::Continue;
	/** Leave empty to use the quest selected once on the NPC topic. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Choice", meta = (EditCondition = "Action != EFlyingCabDialogueAction::Continue && Action != EFlyingCabDialogueAction::Topics", EditConditionHides))
	TObjectPtr<UFlyingCabQuestDefinition> Quest;
	/** None finishes the conversation. Back to topics ignores this field. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Choice", meta = (EditCondition = "Action != EFlyingCabDialogueAction::Topics", EditConditionHides))
	FName NextNodeId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Choice", meta = (TitleProperty = "RequiredStatus"))
	TArray<FFlyingCabDialogueCondition> Conditions;
	/** Otherwise unavailable choices are shown disabled with an explanation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Choice")
	bool bHideWhenUnavailable = false;
};

USTRUCT(BlueprintType)
struct FLYINGCABFLIGHTLAB_API FFlyingCabDialogueNode
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Node")
	FName NodeId;
	/** Empty uses the NPC display name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Node")
	FText Speaker;
	/** Supports {QuestTitle}, {QuestDescription}, {Objective}, {Progress}, {Required}, {RewardCredits}. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Node", meta = (MultiLine = "true"))
	FText Text;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Node", meta = (TitleProperty = "Text"))
	TArray<FFlyingCabDialogueChoice> Choices;
};

/** The first matching rule selects a start node. No match uses EntryNodeId. */
USTRUCT(BlueprintType)
struct FLYINGCABFLIGHTLAB_API FFlyingCabDialogueEntry
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Entry")
	FFlyingCabDialogueCondition Condition;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Entry")
	FName NodeId;
};

UCLASS(BlueprintType)
class FLYINGCABFLIGHTLAB_API UFlyingCabDialogueDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName DialogueId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName EntryNodeId = TEXT("Offer");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue", meta = (TitleProperty = "NodeId"))
	TArray<FFlyingCabDialogueEntry> EntryRules;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue", meta = (TitleProperty = "NodeId"))
	TArray<FFlyingCabDialogueNode> Nodes;
	const FFlyingCabDialogueNode* FindNode(FName NodeId) const;
	bool IsConfigurationValid(FString& OutError) const;
	static UFlyingCabDialogueDefinition* CreateQuestTemplate(UObject* Outer);
	void InitializeQuestTemplate();
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
