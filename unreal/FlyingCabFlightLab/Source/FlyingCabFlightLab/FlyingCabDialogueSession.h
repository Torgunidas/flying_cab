#pragma once

#include "CoreMinimal.h"
#include "FlyingCabDialogueDefinition.h"
#include "FlyingCabDialogueSession.generated.h"

class UFlyingCabNpcDefinition;
class UFlyingCabQuestSubsystem;

USTRUCT(BlueprintType)
struct FLYINGCABFLIGHTLAB_API FFlyingCabDialogueOptionView
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FText Text;
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	bool bEnabled = true;
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FText UnavailableReason;
};

USTRUCT(BlueprintType)
struct FLYINGCABFLIGHTLAB_API FFlyingCabDialogueView
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FText Speaker;
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FText Text;
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	TArray<FFlyingCabDialogueOptionView> Options;
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	int32 Revision = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FText Feedback;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFlyingCabDialogueChanged);

/** A controller-owned conversation. Preview uses isolated status overrides and executes no gameplay API. */
UCLASS(BlueprintType)
class FLYINGCABFLIGHTLAB_API UFlyingCabDialogueSession : public UObject
{
	GENERATED_BODY()
public:
	bool Start(UFlyingCabNpcDefinition* InProfile, UFlyingCabQuestSubsystem* InQuests);
	bool StartPreview(UFlyingCabNpcDefinition* InProfile, EFlyingCabQuestStatus Status);
	UFUNCTION(BlueprintCallable, Category = "Flying Cab|Dialogue")
	bool ChooseOption(int32 Index, int32 ExpectedRevision);
	UFUNCTION(BlueprintCallable, Category = "Flying Cab|Dialogue")
	void Cancel();
	UFUNCTION(BlueprintPure, Category = "Flying Cab|Dialogue")
	FFlyingCabDialogueView GetView() const { return View; }
	UFUNCTION(BlueprintPure, Category = "Flying Cab|Dialogue")
	bool IsActive() const { return bActive; }
	UPROPERTY(BlueprintAssignable, Category = "Flying Cab|Dialogue")
	FFlyingCabDialogueChanged OnChanged;
	void Refresh();
private:
	bool Begin(UFlyingCabNpcDefinition* InProfile);
	bool OpenTopic(int32 Index);
	void ShowTopics();
	bool GoToNode(FName NodeId);
	EFlyingCabQuestStatus GetStatus(const UFlyingCabQuestDefinition* Quest) const;
	bool MeetsCondition(const FFlyingCabDialogueCondition& Condition, FText& Reason) const;
	bool CanChoose(const FFlyingCabDialogueChoice& Choice, FText& Reason) const;
	FText FormatText(const FText& Text) const;
	UFlyingCabQuestDefinition* ResolveQuest(UFlyingCabQuestDefinition* ExplicitQuest) const;
	void Finish(bool bCompletedConversation);
	UPROPERTY(Transient)
	TObjectPtr<UFlyingCabNpcDefinition> Profile;
	UPROPERTY(Transient)
	TObjectPtr<UFlyingCabQuestSubsystem> Quests;
	UPROPERTY(Transient)
	TObjectPtr<UFlyingCabDialogueDefinition> Dialogue;
	UPROPERTY(Transient)
	TObjectPtr<UFlyingCabQuestDefinition> ContextQuest;
	UPROPERTY(Transient)
	FFlyingCabDialogueView View;
	TArray<int32> ChoiceIndices;
	TMap<FName, EFlyingCabQuestStatus> PreviewStates;
	EFlyingCabQuestStatus PreviewDefaultStatus = EFlyingCabQuestStatus::Inactive;
	FName CurrentNode;
	bool bActive = false;
	bool bTopics = true;
	bool bChoosing = false;
	bool bPreview = false;
	bool bVisitedTopic = false;
};
