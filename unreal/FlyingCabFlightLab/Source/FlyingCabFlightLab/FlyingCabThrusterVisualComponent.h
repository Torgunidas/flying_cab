#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FlyingCabThrusterVisualComponent.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** A presentation sample of already applied acceleration. Never fed back into physics. */
struct FThrusterVisualDemand
{
	FVector Acceleration = FVector::ZeroVector;
	FVector ExhaustDirection = FVector(0, 0, -1);
	float Power = 0.f;
	static FThrusterVisualDemand Solve(const FVector& ControlAcceleration, const FVector& CoastAcceleration);
};

USTRUCT()
struct FThrusterNozzleVisual
{
	GENERATED_BODY()
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Joint;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Nozzle;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Core;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Plume;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Heat;
	UPROPERTY(Transient) TObjectPtr<UPointLightComponent> Light;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> PlumeMaterial;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> HeatMaterial;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> CoreMaterial;
	FVector Mouth = FVector::ZeroVector;
	FVector SurfacePoint = FVector::ZeroVector;
	FVector SurfaceNormal = FVector::UpVector;
	float SurfaceStrength = 0.f;
	bool bWetSurface = false;
};

USTRUCT()
struct FThrusterSurfaceParticle
{
	GENERATED_BODY()
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> Material;
	FVector Position = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	float Age = 0.f;
	float Lifetime = 0.f;
	float Size = 0.f;
	float Strength = 0.f;
	bool bWet = false;
};

/** Two gimballed jets, thermal refraction and pooled surface outwash. No input ownership. */
UCLASS(ClassGroup = (FlyingCab), meta = (BlueprintSpawnableComponent))
class FLYINGCABFLIGHTLAB_API UFlyingCabThrusterVisualComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UFlyingCabThrusterVisualComponent();
	virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void SubmitAcceleration(const FVector& ControlAcceleration, const FVector& CoastAcceleration);
	void ResetVisuals();
	const FThrusterVisualDemand& GetDemand() const { return Demand; }
	float GetDisplayedPower() const { return DisplayedPower; }
	FVector GetExhaustDirection() const;

	UPROPERTY(EditAnywhere, Category = "Flying Cab|Thrusters", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float PlumeLengthScale = 1.f;
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Thrusters")
	bool bEnableHeatDistortion = true;
	UPROPERTY(EditAnywhere, Category = "Flying Cab|Thrusters")
	bool bEnableSurfaceOutwash = true;

protected:
	virtual void BeginPlay() override;
private:
	UStaticMeshComponent* CreateMesh(FName Name, UStaticMesh* Mesh, UMaterialInterface* Material);
	void UpdateNozzles(float DeltaSeconds);
	void UpdateSurface(float DeltaSeconds);
	void CreateSurfacePool();
	void SpawnSurfaceParticle(const FThrusterNozzleVisual& Nozzle, float Side);
	UPROPERTY() TSoftObjectPtr<UStaticMesh> NozzleMesh;
	UPROPERTY() TSoftObjectPtr<UStaticMesh> CardMesh;
	UPROPERTY() TSoftObjectPtr<UStaticMesh> SphereMesh;
	UPROPERTY() TSoftObjectPtr<UMaterialInterface> PlumeBase;
	UPROPERTY() TSoftObjectPtr<UMaterialInterface> HeatBase;
	UPROPERTY() TSoftObjectPtr<UMaterialInterface> GlowBase;
	UPROPERTY() TSoftObjectPtr<UMaterialInterface> MetalBase;
	UPROPERTY() TSoftObjectPtr<UMaterialInterface> DustBase;
	UPROPERTY(Transient) TArray<FThrusterNozzleVisual> Nozzles;
	UPROPERTY(Transient) TArray<FThrusterSurfaceParticle> Particles;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> BodyVisual;
	FThrusterVisualDemand PendingDemand;
	FThrusterVisualDemand Demand;
	bool bHasSample = false;
	float DisplayedPower = 0.f;
	float NozzlePitch = -90.f;
	float MetalHeat = 0.f;
	float VisualTime = 0.f;
	float SurfaceTime = 0.f;
	int32 NextParticle = 0;
	FRandomStream SurfaceRandom{7341};
};
