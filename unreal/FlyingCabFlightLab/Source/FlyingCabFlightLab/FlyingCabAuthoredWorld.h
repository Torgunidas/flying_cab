#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlyingCabCityData.h"
#include "FlyingCabAuthoredWorld.generated.h"

class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class AFlyingCabNightshiftOffice;
class AFlyingCabOnFootPortal;
class AFlyingCabAccessTerminal;

/** A saved, independently editable piece of city geometry. Colour survives map reloads. */
UCLASS()
class FLYINGCABFLIGHTLAB_API AFlyingCabWorldGeometry : public AActor
{
 GENERATED_BODY()
public:
 AFlyingCabWorldGeometry();
 virtual void OnConstruction(const FTransform& Transform) override;
 virtual void BeginPlay() override;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Geometry") TObjectPtr<UStaticMeshComponent> Mesh;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Geometry") TObjectPtr<UInstancedStaticMeshComponent> Windows;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Geometry") FLinearColor Color = FLinearColor::White;
 void ApplyColor();
};

/** Move this parent to move a complete stop, its building, services and passenger locations. */
UCLASS()
class FLYINGCABFLIGHTLAB_API AFlyingCabDistrictAnchor : public AActor
{
 GENERATED_BODY()
public:
 AFlyingCabDistrictAnchor();
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Taxi stop") FFlyingCabDistrictDefinition Definition;
 FFlyingCabDistrictDefinition GetWorldDefinition() const;
};

/** Presence of this actor makes the saved level authoritative. No runtime city regeneration. */
UCLASS()
class FLYINGCABFLIGHTLAB_API AFlyingCabAuthoredWorld : public AActor
{
 GENERATED_BODY()
public:
 AFlyingCabAuthoredWorld();
 static AFlyingCabAuthoredWorld* Find(const UWorld* World);
 UPROPERTY(EditInstanceOnly, Category="World") TObjectPtr<AFlyingCabNightshiftOffice> Office;
 UPROPERTY(EditInstanceOnly, Category="World") TObjectPtr<AFlyingCabOnFootPortal> Entrance;
 UPROPERTY(EditInstanceOnly, Category="World") TObjectPtr<AFlyingCabOnFootPortal> Exit;
 UPROPERTY(EditInstanceOnly, Category="World") TObjectPtr<AFlyingCabAccessTerminal> Terminal;
 UPROPERTY(EditInstanceOnly, Category="World") TObjectPtr<AActor> ServiceVehicleSpawn;
 UPROPERTY(EditInstanceOnly, Category="World") TArray<TObjectPtr<AActor>> SupercarSpawns;
};
