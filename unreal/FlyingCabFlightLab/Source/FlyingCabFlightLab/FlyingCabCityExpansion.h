// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlyingCabCityExpansion.generated.h"

class UMaterialInterface;
class UInstancedStaticMeshComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * Data-driven metro geometry. Collision-free express corridors separate four estates.
 */
UCLASS()
class FLYINGCABFLIGHTLAB_API AFlyingCabCityExpansion : public AActor
{
	GENERATED_BODY()

public:
	AFlyingCabCityExpansion();
	virtual void Tick(float DeltaSeconds) override;
#if WITH_EDITOR
 void BakeToLevel();
#endif
 UPROPERTY(VisibleAnywhere, Category="Authoring") bool bBakedToLevel = false;

protected:
	virtual void BeginPlay() override;

private:
	void BuildExpansionGeometry();
	void AddBlock(
		const FString& Name,
		const FVector& Location,
		const FVector& Scale,
		const FLinearColor& Color,
		bool bCollisionEnabled = true);
	void AddDistrictLabel(
		const FString& Name,
		const FVector& Location,
		const FLinearColor& Color);

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|City Expansion")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> RuntimeBlocks;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextRenderComponent>> RuntimeLabels;

	UPROPERTY()
	TObjectPtr<UTextRenderComponent> SignalLabel;
 bool bBaking = false;
 AActor* BakeParent = nullptr;

	/** All apartment windows share one collision-free instanced draw component. */
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> ResidentialWindows;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BasicMaterial;
};
