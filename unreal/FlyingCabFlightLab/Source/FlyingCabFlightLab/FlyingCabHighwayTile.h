#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlyingCabHighwayTile.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;

/** Reusable road: its visible lane and oriented gameplay bounds share one transform. */
UCLASS(Blueprintable, meta=(DisplayName="Highway Tile"))
class FLYINGCABFLIGHTLAB_API AFlyingCabHighwayTile : public AActor
{
 GENERATED_BODY()
public:
 AFlyingCabHighwayTile();
 virtual void OnConstruction(const FTransform& Transform) override;
 bool ContainsLocation(const FVector& Location) const;
 // Tile bonuses combine by strongest value, never by multiplication. Legacy roads remain supported.
 static bool GetBonuses(UWorld* World, const FVector& Location, float LegacySpeed,
     float LegacyFuel, float& OutSpeed, float& OutFuel);

 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tile") TObjectPtr<UBoxComponent> Zone;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tile") TObjectPtr<UStaticMeshComponent> Lane;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tile") TObjectPtr<UInstancedStaticMeshComponent> Markings;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tile|Dimensions", meta=(ClampMin="100", Units="cm"))
 float Length = 2800.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tile|Dimensions", meta=(ClampMin="100", Units="cm"))
 float Width = 900.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tile|Dimensions", meta=(ClampMin="10", Units="cm"))
 float Depth = 400.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tile|Highway Turbo") bool bEnabled = true;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tile|Highway Turbo", meta=(ClampMin="1", ClampMax="2", ToolTip="Maximum flight speed multiplier; does not add thrust."))
 float SpeedMultiplier = 1.5f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tile|Highway Turbo", meta=(ClampMin="0.1", ClampMax="1", ToolTip="0.5 means half the fuel consumption while thrusting."))
 float FuelConsumptionMultiplier = 0.5f;
};
