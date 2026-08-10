// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlyingCabNightshiftOffice.generated.h"

class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** Compact runtime-built interior used to validate the first on-foot location. */
UCLASS()
class FLYINGCABFLIGHTLAB_API AFlyingCabNightshiftOffice : public AActor
{
	GENERATED_BODY()

public:
	AFlyingCabNightshiftOffice();

	FVector GetEntryLocation() const;
	FVector GetExitPortalLocation() const;
	FVector GetTerminalLocation() const;

private:
	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Nightshift Office")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Nightshift Office")
	TObjectPtr<UStaticMeshComponent> Floor;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Nightshift Office")
	TObjectPtr<UStaticMeshComponent> Ceiling;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Nightshift Office")
	TObjectPtr<UStaticMeshComponent> LeftWall;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Nightshift Office")
	TObjectPtr<UStaticMeshComponent> RightWall;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Nightshift Office")
	TObjectPtr<UStaticMeshComponent> BackWall;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Nightshift Office")
	TObjectPtr<UStaticMeshComponent> Counter;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Nightshift Office")
	TObjectPtr<UTextRenderComponent> OfficeLabel;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Nightshift Office")
	TObjectPtr<UPointLightComponent> CyanLight;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Nightshift Office")
	TObjectPtr<UPointLightComponent> MagentaLight;
};
