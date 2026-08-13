// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "FlyingCabQuestEventComponent.generated.h"

/** Drop this onto any Blueprint and call EmitQuestEvent from its existing gameplay logic. */
UCLASS(ClassGroup = "Flying Cab", meta = (BlueprintSpawnableComponent))
class FLYINGCABFLIGHTLAB_API UFlyingCabQuestEventComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFlyingCabQuestEventComponent();

	UFUNCTION(BlueprintCallable, Category = "Flying Cab|Quests")
	int32 EmitQuestEvent();

	UFUNCTION(BlueprintCallable, Category = "Flying Cab|Quests")
	int32 EmitQuestEventOverride(FName InEventId, FName InTargetId, int32 InAmount = 1);

	void Configure(FName InEventId, FName InTargetId, int32 InAmount = 1);
	bool HasEmitted() const { return bHasEmitted; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flying Cab|Quest Event")
	FName EventId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flying Cab|Quest Event")
	FName TargetId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flying Cab|Quest Event", meta = (ClampMin = "1"))
	int32 Amount = 1;

	/** Locks only after an active quest actually consumes the event. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flying Cab|Quest Event")
	bool bEmitOnce = false;

private:
	bool bHasEmitted = false;
};
