// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabOnFootPortal.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "FlyingCabCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabPortal, Log, All);

AFlyingCabOnFootPortal::AFlyingCabOnFootPortal()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractionZone = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionZone"));
	SetRootComponent(InteractionZone);
	InteractionZone->InitBoxExtent(FVector(105.0f, 100.0f, 125.0f));
	InteractionZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractionZone->SetGenerateOverlapEvents(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	LeftFrame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftFrame"));
	RightFrame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightFrame"));
	TopFrame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TopFrame"));
	for (UStaticMeshComponent* Frame : {LeftFrame, RightFrame, TopFrame})
	{
		Frame->SetupAttachment(InteractionZone);
		Frame->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Frame->SetMobility(EComponentMobility::Movable);
		if (CubeMesh.Succeeded())
		{
			Frame->SetStaticMesh(CubeMesh.Object);
		}
		if (BasicMaterial.Succeeded())
		{
			Frame->SetMaterial(0, BasicMaterial.Object);
		}
	}
	LeftFrame->SetRelativeLocation(FVector(-92.0f, 75.0f, 0.0f));
	LeftFrame->SetRelativeScale3D(FVector(0.16f, 0.18f, 2.5f));
	RightFrame->SetRelativeLocation(FVector(92.0f, 75.0f, 0.0f));
	RightFrame->SetRelativeScale3D(FVector(0.16f, 0.18f, 2.5f));
	TopFrame->SetRelativeLocation(FVector(0.0f, 75.0f, 118.0f));
	TopFrame->SetRelativeScale3D(FVector(2.0f, 0.18f, 0.16f));

	PortalLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PortalLabel"));
	PortalLabel->SetupAttachment(InteractionZone);
	PortalLabel->SetRelativeLocation(FVector(0.0f, 85.0f, 170.0f));
	PortalLabel->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	PortalLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	PortalLabel->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	PortalLabel->SetWorldSize(30.0f);

	PortalLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PortalLight"));
	PortalLight->SetupAttachment(InteractionZone);
	PortalLight->SetRelativeLocation(FVector(0.0f, 80.0f, 30.0f));
	PortalLight->SetIntensity(1600.0f);
	PortalLight->SetAttenuationRadius(420.0f);
	PortalLight->SetCastShadows(false);

	ApplyAppearance();
}

void AFlyingCabOnFootPortal::Configure(
	const FString& InPortalName,
	const FText& InPrompt,
	const FVector& InDestination,
	const FLinearColor& InColor)
{
	PortalName = InPortalName;
	InteractionPrompt = InPrompt;
	Destination = InDestination;
	PortalColor = InColor;
	ApplyAppearance();
}

bool AFlyingCabOnFootPortal::Interact(AFlyingCabCharacter* Character, FText& OutMessage)
{
	if (!Character || Character->IsDead())
	{
		OutMessage = FText::FromString(TEXT("DOOR CONTROL UNAVAILABLE"));
		return false;
	}

	Character->GetCharacterMovement()->StopMovementImmediately();
	Character->SetActorLocation(
		Destination,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	OutMessage = FText::FromString(FString::Printf(TEXT("ENTERED // %s"), *PortalName));
	UE_LOG(
		LogFlyingCabPortal,
		Display,
		TEXT("Character used portal %s to %s."),
		*PortalName,
		*Destination.ToCompactString());
	return true;
}

FText AFlyingCabOnFootPortal::GetInteractionPrompt(const AFlyingCabCharacter* Character) const
{
	return InteractionPrompt;
}

void AFlyingCabOnFootPortal::ApplyAppearance()
{
	for (UStaticMeshComponent* Frame : {LeftFrame, RightFrame, TopFrame})
	{
		Frame->SetVectorParameterValueOnMaterials(
			TEXT("Color"),
			FVector(PortalColor.R, PortalColor.G, PortalColor.B));
	}
	PortalLabel->SetText(FText::FromString(PortalName));
	PortalLabel->SetTextRenderColor(PortalColor.ToFColor(true));
	PortalLight->SetLightColor(PortalColor);
}
