// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlyingCabLivingWorldManager.generated.h"

class AFlyingCabLivingPedestrian;
class AFlyingCabLivingRoute;
class AFlyingCabTrafficVehicle;
class UFlyingCabLivingWorldProfile;
enum class EFlyingCabLivingRouteAction : uint8;

/** Owns route population and coordination between ambient vehicles and pedestrians. */
UCLASS()
class FLYINGCABFLIGHTLAB_API AFlyingCabLivingWorldManager : public AActor
{
	GENERATED_BODY()

public:
	AFlyingCabLivingWorldManager();
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool Initialize(UFlyingCabLivingWorldProfile* InProfile = nullptr);
	const TArray<TObjectPtr<AFlyingCabTrafficVehicle>>& GetTrafficVehicles() const
	{
		return TrafficVehicles;
	}
	const TArray<TObjectPtr<AFlyingCabLivingPedestrian>>& GetPedestrians() const
	{
		return Pedestrians;
	}
	const TArray<TObjectPtr<AFlyingCabLivingRoute>>& GetRoutes() const { return Routes; }
	static int32 GetPrototypeVehicleCount();
	static int32 GetPrototypePedestrianCount();
	int32 GetTotalBoardings() const { return TotalBoardings; }
	int32 GetTotalPassengerExits() const { return TotalPassengerExits; }

private:
	bool DiscoverOrGenerateRoutes(UFlyingCabLivingWorldProfile* InProfile);
	bool GenerateRoutes(TConstArrayView<struct FFlyingCabLivingRouteDefinition> Definitions);
	bool SpawnPopulation();
	void RegisterPedestrianWaiting(AFlyingCabLivingPedestrian* Pedestrian, FName StopId);
	void HandleVehicleStop(
		AFlyingCabTrafficVehicle* Vehicle,
		FName StopId,
		EFlyingCabLivingRouteAction Action);
	FActorSpawnParameters MakeSpawnParameters(AActor* SpawnOwner) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AFlyingCabLivingRoute>> Routes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AFlyingCabTrafficVehicle>> TrafficVehicles;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AFlyingCabLivingPedestrian>> Pedestrians;

	TMap<FName, TArray<TWeakObjectPtr<AFlyingCabLivingPedestrian>>> WaitingPedestrians;
	int32 TotalBoardings = 0;
	int32 TotalPassengerExits = 0;
	bool bInitialized = false;
};
