// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlyingCabDeliveryZone.generated.h"

class AFlyingCabPawn;
class UBoxComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UENUM()
enum class EFlyingCabDeliveryZoneType : uint8
{
	Pickup,
	Dropoff
};

class AFlyingCabDeliveryZone;
DECLARE_MULTICAST_DELEGATE_OneParam(FOnFlyingCabZoneReady, AFlyingCabDeliveryZone*);

/** Runtime prototype marker that accepts the cab only after it slows down inside the zone. */
UCLASS()
class FLYINGCABFLIGHTLAB_API AFlyingCabDeliveryZone : public AActor
{
	GENERATED_BODY()

public:
	AFlyingCabDeliveryZone();

	virtual void Tick(float DeltaSeconds) override;

	void Configure(
		EFlyingCabDeliveryZoneType InZoneType,
		float InArrivalMaxPlanarSpeed,
		float InConfirmationDuration);
	void ConfigurePassengerOffer(const FString& DestinationName, int32 EstimatedFareCredits);
	void SetOfferRemainingSeconds(float RemainingSeconds);
	void SetAcceptanceEnabled(bool bEnabled);
	void SetZoneActive(bool bNewActive);
	bool IsZoneActive() const { return bZoneActive; }
	bool IsPawnInside(const AFlyingCabPawn* Pawn) const;
	bool IsConfirmationInProgress() const { return bConfirmationInProgress; }
	float GetConfirmationAlpha() const;
	float GetArrivalMaxPlanarSpeed() const { return ArrivalMaxPlanarSpeed; }
	EFlyingCabDeliveryZoneType GetZoneType() const { return ZoneType; }

	FOnFlyingCabZoneReady OnCabReady;

private:
	void ApplyZoneAppearance();
	void ResetConfirmation();
	void UpdateConfirmationVisuals();
	void CapturePassengerPath(const AFlyingCabPawn* Pawn);

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Delivery")
	TObjectPtr<UBoxComponent> TriggerBox;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Delivery")
	TObjectPtr<UStaticMeshComponent> MarkerBase;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Delivery")
	TObjectPtr<UStaticMeshComponent> MarkerLeft;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Delivery")
	TObjectPtr<UStaticMeshComponent> MarkerRight;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Delivery")
	TObjectPtr<UTextRenderComponent> ZoneLabel;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Delivery")
	TObjectPtr<UPointLightComponent> ZoneLight;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Delivery")
	TObjectPtr<UStaticMeshComponent> PassengerBody;

	UPROPERTY(VisibleAnywhere, Category = "Flying Cab|Delivery")
	TObjectPtr<UStaticMeshComponent> PassengerHead;

	EFlyingCabDeliveryZoneType ZoneType = EFlyingCabDeliveryZoneType::Pickup;
	float ArrivalMaxPlanarSpeed = 180.0f;
	float ConfirmationDuration = 0.0f;
	float ConfirmationElapsed = 0.0f;
	float PassengerCabX = 0.0f;
	float PassengerExitX = 0.0f;
	FString PassengerDestinationName;
	int32 PassengerEstimatedFareCredits = 0;
	int32 PassengerRemainingSeconds = 0;
	bool bZoneActive = false;
	bool bAcceptanceEnabled = true;
	bool bTriggered = false;
	bool bConfirmationInProgress = false;
};
