// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlyingCabInteractable.h"
#include "GameFramework/Actor.h"
#include "FlyingCabAccessTerminal.generated.h"

class UBoxComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** Grants one access right and visibly remembers it for the current game session. */
UCLASS()
class FLYINGCABFLIGHTLAB_API AFlyingCabAccessTerminal
	: public AActor
	, public IFlyingCabInteractable
{
	GENERATED_BODY()

public:
	AFlyingCabAccessTerminal();

	void Configure(FName InAccessId, const FString& InAccessDisplayName);

	virtual bool Interact(class AFlyingCabCharacter* Character, FText& OutMessage) override;
	virtual FText GetInteractionPrompt(const AFlyingCabCharacter* Character) const override;

private:
	bool IsAccessGranted() const;
	void RefreshAppearance();

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Access Terminal")
	TObjectPtr<UBoxComponent> InteractionZone;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Access Terminal")
	TObjectPtr<UStaticMeshComponent> TerminalBody;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Access Terminal")
	TObjectPtr<UStaticMeshComponent> TerminalScreen;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Access Terminal")
	TObjectPtr<UTextRenderComponent> TerminalLabel;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Access Terminal")
	TObjectPtr<UPointLightComponent> TerminalLight;

	FName AccessId = TEXT("Vehicle.Service");
	FString AccessDisplayName = TEXT("SERVICE VEHICLES");
};
