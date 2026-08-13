// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FlyingCabInteractable.generated.h"

class AFlyingCabCharacter;

UINTERFACE(MinimalAPI)
class UFlyingCabInteractable : public UInterface
{
	GENERATED_BODY()
};

/** Small native interaction contract shared by doors, terminals and future vehicles. */
class FLYINGCABFLIGHTLAB_API IFlyingCabInteractable
{
	GENERATED_BODY()

public:
	virtual bool Interact(AFlyingCabCharacter* Character, FText& OutMessage) = 0;
	virtual FText GetInteractionPrompt(const AFlyingCabCharacter* Character) const = 0;

	/** Optional stable context used by generic quest objectives and future dialogue conditions. */
	virtual FName GetQuestTargetId() const { return NAME_None; }
};
