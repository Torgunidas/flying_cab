// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlyingCabRepairStation.generated.h"

class AFlyingCabPawn;
class UBoxComponent;
class UPointLightComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** Paid hull-repair point. */
UCLASS()
class FLYINGCABFLIGHTLAB_API AFlyingCabRepairStation : public AActor
{
	GENERATED_BODY()

public:
	AFlyingCabRepairStation();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void Configure(const FString& InServiceName, int32 InPricePerUnit = 1);
	int32 GetRepairPricePerHullUnit() const { return RepairPricePerHullUnit; }

private:
	void ClearContextPawn();
	void ResetServiceState();
	void RefreshTickState();

	UFUNCTION()
	void HandleServiceZoneBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleServiceZoneEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex);

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

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Repair Station")
	FString ServiceName = TEXT("NIGHTSHIFT REPAIR");

	TSet<TWeakObjectPtr<AFlyingCabPawn>> OverlappingPawns;
	TWeakObjectPtr<AFlyingCabPawn> ContextPawn;
	float RepairUnitAccumulator = 0.0f;
};
