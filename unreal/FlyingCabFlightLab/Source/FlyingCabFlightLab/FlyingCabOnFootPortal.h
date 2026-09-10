// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlyingCabInteractable.h"
#include "GameFramework/Actor.h"
#include "FlyingCabOnFootPortal.generated.h"

class UBoxComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** Foot-only doorway that teleports within the persistent gameplay world. */
UCLASS()
class FLYINGCABFLIGHTLAB_API AFlyingCabOnFootPortal
	: public AActor
	, public IFlyingCabInteractable
{
	GENERATED_BODY()

public:
	AFlyingCabOnFootPortal();

	virtual void OnConstruction(const FTransform& Transform) override;
 UPROPERTY(EditInstanceOnly, Category="Flying Cab|On Foot Portal") TObjectPtr<AActor> DestinationActor;
 UPROPERTY(EditAnywhere, Category="Flying Cab|On Foot Portal") FVector DestinationOffset = FVector::ZeroVector;
 void Configure(
		const FString& InPortalName,
		const FText& InPrompt,
		const FVector& InDestination,
		const FLinearColor& InColor);

	virtual bool Interact(class AFlyingCabCharacter* Character, FText& OutMessage) override;
	virtual FText GetInteractionPrompt(const AFlyingCabCharacter* Character) const override;
	virtual FName GetQuestTargetId() const override { return FName(*PortalName); }

private:
	void ApplyAppearance();

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|On Foot Portal")
	TObjectPtr<UBoxComponent> InteractionZone;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|On Foot Portal")
	TObjectPtr<UStaticMeshComponent> LeftFrame;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|On Foot Portal")
	TObjectPtr<UStaticMeshComponent> RightFrame;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|On Foot Portal")
	TObjectPtr<UStaticMeshComponent> TopFrame;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|On Foot Portal")
	TObjectPtr<UTextRenderComponent> PortalLabel;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|On Foot Portal")
	TObjectPtr<UPointLightComponent> PortalLight;

	UPROPERTY(EditAnywhere, Category="Flying Cab|On Foot Portal")
	FString PortalName = TEXT("DOOR");
	UPROPERTY(EditAnywhere, Category="Flying Cab|On Foot Portal")
	FText InteractionPrompt = FText::FromString(TEXT("Q // ENTER"));
	UPROPERTY(EditAnywhere, Category="Flying Cab|On Foot Portal")
	FVector Destination = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, Category="Flying Cab|On Foot Portal")
	FLinearColor PortalColor = FLinearColor(0.05f, 0.75f, 1.0f);
};
