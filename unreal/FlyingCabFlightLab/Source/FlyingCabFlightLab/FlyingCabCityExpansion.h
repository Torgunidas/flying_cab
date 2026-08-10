// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlyingCabCityExpansion.generated.h"

class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * Runtime-built east side of the flight lab. Keeping the prototype geometry in
 * code makes it portable and avoids introducing editor-generated map changes.
 */
UCLASS()
class FLYINGCABFLIGHTLAB_API AFlyingCabCityExpansion : public AActor
{
	GENERATED_BODY()

public:
	AFlyingCabCityExpansion();

protected:
	virtual void BeginPlay() override;

private:
	void OpenExistingEasternBoundary();
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

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BasicMaterial;
};
