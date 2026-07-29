// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlyingCabRepairStation.generated.h"

class AFlyingCabPawn;
class UBoxComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** The city's single paid hull-repair point. */
UCLASS()
class FLYINGCABFLIGHTLAB_API AFlyingCabRepairStation : public AActor
{
	GENERATED_BODY()

public:
	AFlyingCabRepairStation();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void ClearContextPawn();

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Repair Station")
	TObjectPtr<UBoxComponent> ServiceZone;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Repair Station")
	TObjectPtr<UStaticMeshComponent> ServiceBase;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Repair Station")
	TObjectPtr<UStaticMeshComponent> ServicePillar;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Repair Station")
	TObjectPtr<UTextRenderComponent> ServiceLabel;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Repair Station")
	TObjectPtr<UPointLightComponent> ServiceLight;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Repair Station", meta = (ClampMin = "1"))
	int32 RepairPricePerHullUnit = 1;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Repair Station", meta = (ClampMin = "0.0"))
	float RepairHullPerSecond = 20.0f;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Repair Station", meta = (ClampMin = "0.0"))
	float RepairMaxPlanarSpeed = 140.0f;

	TWeakObjectPtr<AFlyingCabPawn> ContextPawn;
	float RepairUnitAccumulator = 0.0f;
};
