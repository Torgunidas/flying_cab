// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FlyingCabLivingWorldTypes.h"
#include "FlyingCabLivingWorldProfile.generated.h"

/** Population and route data used by the living-world manager. */
UCLASS(BlueprintType)
class FLYINGCABFLIGHTLAB_API UFlyingCabLivingWorldProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying Cab|Living World")
	TArray<FFlyingCabLivingRouteDefinition> Routes;

	bool IsConfigurationValid(FString& OutError) const;
	static UFlyingCabLivingWorldProfile* LoadDefaultAsset();
	static TArray<FFlyingCabLivingRouteDefinition> BuildPrototypeRoutes();
	static int32 CountAgents(
		TConstArrayView<FFlyingCabLivingRouteDefinition> Definitions,
		EFlyingCabLivingAgentKind AgentKind);
};
