// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlyingCabInteractable.h"
#include "GameFramework/Actor.h"
#include "FlyingCabQuestInteractable.generated.h"

class UBoxComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UFlyingCabQuestEventComponent;

/** Placeable no-code interaction node that can satisfy any event-based objective. */
UCLASS(Blueprintable)
class FLYINGCABFLIGHTLAB_API AFlyingCabQuestInteractable
	: public AActor
	, public IFlyingCabInteractable
{
	GENERATED_BODY()

public:
	AFlyingCabQuestInteractable();

	virtual bool Interact(class AFlyingCabCharacter* Character, FText& OutMessage) override;
	virtual FText GetInteractionPrompt(const AFlyingCabCharacter* Character) const override;
	virtual FName GetQuestTargetId() const override { return InteractableId; }

	void Configure(
		FName InInteractableId,
		FName InEventId,
		const FText& InPrompt,
		const FText& InSuccessMessage);

private:
	void RefreshAppearance();

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Interactable")
	TObjectPtr<UBoxComponent> InteractionZone;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Interactable")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Interactable")
	TObjectPtr<UTextRenderComponent> Label;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Interactable")
	TObjectPtr<UPointLightComponent> Light;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flying Cab|Interactable", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFlyingCabQuestEventComponent> QuestEvent;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Interactable")
	FName InteractableId = TEXT("Interactable.WorldNode");

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Interactable")
	FText InteractionPrompt = FText::FromString(TEXT("Q // INTERACT"));

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Interactable")
	FText SuccessMessage = FText::FromString(TEXT("INTERACTION COMPLETE"));

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Interactable")
	FText ConsumedMessage = FText::FromString(TEXT("ALREADY COMPLETE"));
};
