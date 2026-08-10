// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlyingCabFuelStation.generated.h"

class AFlyingCabPawn;
class UBoxComponent;
class UPointLightComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** Lightweight service zone that sells fuel while the cab holds the refuel input. */
UCLASS()
class FLYINGCABFLIGHTLAB_API AFlyingCabFuelStation : public AActor
{
	GENERATED_BODY()

public:
	AFlyingCabFuelStation();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void Configure(const FString& InServiceName);

	int32 GetFuelPricePerUnit() const { return FuelPricePerUnit; }

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

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Fuel Station")
	TObjectPtr<UBoxComponent> ServiceZone;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Fuel Station")
	TObjectPtr<UStaticMeshComponent> ServiceBase;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Fuel Station")
	TObjectPtr<UStaticMeshComponent> ServicePillar;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Fuel Station")
	TObjectPtr<UTextRenderComponent> ServiceLabel;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Fuel Station")
	TObjectPtr<UPointLightComponent> ServiceLight;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Fuel Station", meta = (ClampMin = "1"))
	int32 FuelPricePerUnit = 2;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Fuel Station", meta = (ClampMin = "0.0"))
	float RefuelUnitsPerSecond = 30.0f;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Fuel Station", meta = (ClampMin = "0.0"))
	float RefuelMaxPlanarSpeed = 140.0f;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Fuel Station")
	FString ServiceName = TEXT("FUEL SERVICE");

	TSet<TWeakObjectPtr<AFlyingCabPawn>> OverlappingPawns;
	TWeakObjectPtr<AFlyingCabPawn> ContextPawn;
	float RefuelUnitAccumulator = 0.0f;
};
