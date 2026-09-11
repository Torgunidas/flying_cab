#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FlyingCabHighwayAssistComponent.generated.h"

class UStaticMesh;
class UMaterialInterface;
class UStaticMeshComponent;
class UTextRenderComponent;

/** Location-only gameplay modifier. Never reads, stores or synthesizes input commands. */
UCLASS(ClassGroup = "Flying Cab", meta = (BlueprintSpawnableComponent))
class FLYINGCABFLIGHTLAB_API UFlyingCabHighwayAssistComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UFlyingCabHighwayAssistComponent();
	void Advance(float DeltaSeconds);
	void ResetAssist();
	float GetSpeedMultiplier() const;
	float GetFuelMultiplier() const;
	float GetBlend() const { return Blend; }

protected:
	virtual void BeginPlay() override;

private:
	void UpdateAppearance(const FVector& Velocity);

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Highway Turbo", meta = (ClampMin = "1.0", ClampMax = "2.0"))
	float SpeedMultiplier = 1.5f;
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Highway Turbo", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float FuelConsumptionMultiplier = 0.5f;
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Highway Turbo", meta = (ClampMin = "0.1"))
	float EntrySeconds = 0.5f;
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Highway Turbo", meta = (ClampMin = "0.1"))
	float ExitSeconds = 0.8f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Streaks;
	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> Label;
	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> Material;
	float Blend = 0.0f;
	float ActiveSpeedMultiplier = 1.f;
	float ActiveFuelMultiplier = 1.f;
	float VisualTime = 0.0f;
};
