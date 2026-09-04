// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlyingCabInteractable.h"
#include "FlyingCabQuestTypes.h"
#include "GameFramework/Actor.h"
#include "FlyingCabQuestGiver.generated.h"

class UBoxComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UFlyingCabQuestDefinition;

/** Placeable quest endpoint. Assign one quest asset in Details; no Blueprint graph is required. */
UCLASS(Blueprintable)
class FLYINGCABFLIGHTLAB_API AFlyingCabQuestGiver
	: public AActor
	, public IFlyingCabInteractable
{
	GENERATED_BODY()

public:
	AFlyingCabQuestGiver();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual bool Interact(class AFlyingCabCharacter* Character, FText& OutMessage) override;
	virtual FText GetInteractionPrompt(const AFlyingCabCharacter* Character) const override;
	virtual FName GetQuestTargetId() const override { return QuestGiverId; }

	void Configure(
		FName InQuestGiverId,
		const FText& InDisplayName,
		UFlyingCabQuestDefinition* InQuestDefinition);

	UFlyingCabQuestDefinition* GetQuestDefinition() const { return QuestDefinition; }

private:
	class UFlyingCabQuestSubsystem* GetQuestSubsystem() const;
	void RefreshAppearance();

	UFUNCTION()
	void HandleQuestStateChanged(FName QuestId, EFlyingCabQuestStatus Status);

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Quest Giver")
	TObjectPtr<UBoxComponent> InteractionZone;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Quest Giver")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Quest Giver")
	TObjectPtr<UStaticMeshComponent> Head;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Quest Giver")
	TObjectPtr<UTextRenderComponent> Label;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Quest Giver")
	TObjectPtr<UPointLightComponent> Light;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Flying Cab|Quest Giver", meta = (AllowPrivateAccess = "true"))
	FName QuestGiverId = TEXT("QuestGiver.Dispatcher");

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Flying Cab|Quest Giver", meta = (AllowPrivateAccess = "true"))
	FText DisplayName = FText::FromString(TEXT("DISPATCHER"));

	/** Drag a UFlyingCabQuestDefinition asset here in the level Details panel. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Flying Cab|Quest Giver", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFlyingCabQuestDefinition> QuestDefinition;
};
