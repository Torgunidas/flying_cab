// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabAccessTerminal.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/GameInstance.h"
#include "FlyingCabCharacter.h"
#include "FlyingCabProgressionSubsystem.h"
#include "FlyingCabQuestSubsystem.h"
#include "FlyingCabQuestTypes.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AFlyingCabAccessTerminal::AFlyingCabAccessTerminal()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractionZone = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionZone"));
	SetRootComponent(InteractionZone);
	InteractionZone->InitBoxExtent(FVector(115.0f, 100.0f, 105.0f));
	InteractionZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractionZone->SetGenerateOverlapEvents(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	TerminalBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TerminalBody"));
	TerminalBody->SetupAttachment(InteractionZone);
	TerminalBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TerminalBody->SetRelativeLocation(FVector(0.0f, 0.0f, -25.0f));
	TerminalBody->SetRelativeScale3D(FVector(1.25f, 0.75f, 1.7f));

	TerminalScreen = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TerminalScreen"));
	TerminalScreen->SetupAttachment(InteractionZone);
	TerminalScreen->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TerminalScreen->SetRelativeLocation(FVector(0.0f, 48.0f, 28.0f));
	TerminalScreen->SetRelativeScale3D(FVector(0.95f, 0.08f, 0.58f));

	for (UStaticMeshComponent* Mesh : {TerminalBody, TerminalScreen})
	{
		Mesh->SetMobility(EComponentMobility::Movable);
		if (CubeMesh.Succeeded())
		{
			Mesh->SetStaticMesh(CubeMesh.Object);
		}
		if (BasicMaterial.Succeeded())
		{
			Mesh->SetMaterial(0, BasicMaterial.Object);
		}
	}
	TerminalBody->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.08f, 0.02f, 0.12f));

	TerminalLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TerminalLabel"));
	TerminalLabel->SetupAttachment(InteractionZone);
	TerminalLabel->SetRelativeLocation(FVector(0.0f, 60.0f, 120.0f));
	TerminalLabel->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	TerminalLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	TerminalLabel->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	TerminalLabel->SetWorldSize(28.0f);

	TerminalLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("TerminalLight"));
	TerminalLight->SetupAttachment(InteractionZone);
	TerminalLight->SetRelativeLocation(FVector(0.0f, 80.0f, 40.0f));
	TerminalLight->SetIntensity(1700.0f);
	TerminalLight->SetAttenuationRadius(430.0f);
	TerminalLight->SetCastShadows(false);

	RefreshAppearance();
}

void AFlyingCabAccessTerminal::Configure(FName InAccessId, const FString& InAccessDisplayName)
{
	AccessId = InAccessId;
	AccessDisplayName = InAccessDisplayName;
	RefreshAppearance();
}

bool AFlyingCabAccessTerminal::Interact(AFlyingCabCharacter* Character, FText& OutMessage)
{
	if (!Character || Character->IsDead())
	{
		OutMessage = FText::FromString(TEXT("TERMINAL OFFLINE"));
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UFlyingCabProgressionSubsystem* Progression = GameInstance
		? GameInstance->GetSubsystem<UFlyingCabProgressionSubsystem>()
		: nullptr;
	if (!Progression)
	{
		OutMessage = FText::FromString(TEXT("ACCESS NETWORK UNAVAILABLE"));
		return false;
	}

	const bool bNewAccess = Progression->GrantAccess(AccessId);
	if (UFlyingCabQuestSubsystem* Quests =
		GameInstance->GetSubsystem<UFlyingCabQuestSubsystem>())
	{
		Quests->RecordEvent(FlyingCabQuestEvents::AccessGranted, AccessId);
	}
	OutMessage = FText::FromString(bNewAccess
		? FString::Printf(TEXT("ACCESS GRANTED // %s"), *AccessDisplayName)
		: FString::Printf(TEXT("ACCESS ALREADY ACTIVE // %s"), *AccessDisplayName));
	RefreshAppearance();
	return true;
}

FText AFlyingCabAccessTerminal::GetInteractionPrompt(const AFlyingCabCharacter* Character) const
{
	return FText::FromString(IsAccessGranted()
		? TEXT("Q // CHECK SERVICE LICENSE")
		: TEXT("Q // CLAIM SERVICE LICENSE"));
}

bool AFlyingCabAccessTerminal::IsAccessGranted() const
{
	UGameInstance* GameInstance = GetGameInstance();
	const UFlyingCabProgressionSubsystem* Progression = GameInstance
		? GameInstance->GetSubsystem<UFlyingCabProgressionSubsystem>()
		: nullptr;
	return Progression && Progression->HasAccess(AccessId);
}

void AFlyingCabAccessTerminal::RefreshAppearance()
{
	const bool bGranted = IsAccessGranted();
	const FLinearColor Color = bGranted
		? FLinearColor(0.12f, 1.0f, 0.42f)
		: FLinearColor(0.05f, 0.78f, 1.0f);
	TerminalScreen->SetVectorParameterValueOnMaterials(
		TEXT("Color"),
		FVector(Color.R, Color.G, Color.B));
	TerminalLight->SetLightColor(Color);
	TerminalLabel->SetTextRenderColor(Color.ToFColor(true));
	TerminalLabel->SetText(FText::FromString(bGranted
		? TEXT("LICENSE ACTIVE\nSERVICE VEHICLES")
		: TEXT("CITY ACCESS NODE\nSERVICE VEHICLES")));
}
