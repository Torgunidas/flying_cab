// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabQuestInteractable.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "FlyingCabCharacter.h"
#include "FlyingCabQuestEventComponent.h"
#include "FlyingCabQuestTypes.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AFlyingCabQuestInteractable::AFlyingCabQuestInteractable()
{
	PrimaryActorTick.bCanEverTick = false;
	InteractionZone = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionZone"));
	SetRootComponent(InteractionZone);
	InteractionZone->InitBoxExtent(FVector(90.0f, 80.0f, 90.0f));
	InteractionZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(InteractionZone);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetRelativeScale3D(FVector(0.8f, 0.45f, 1.1f));
	if (CubeMesh.Succeeded())
	{
		Body->SetStaticMesh(CubeMesh.Object);
	}
	if (BasicMaterial.Succeeded())
	{
		Body->SetMaterial(0, BasicMaterial.Object);
	}

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(InteractionZone);
	Label->SetRelativeLocation(FVector(0.0f, 55.0f, 115.0f));
	Label->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	Label->SetWorldSize(24.0f);

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(InteractionZone);
	Light->SetRelativeLocation(FVector(0.0f, 70.0f, 35.0f));
	Light->SetIntensity(1100.0f);
	Light->SetAttenuationRadius(320.0f);
	Light->SetCastShadows(false);

	QuestEvent = CreateDefaultSubobject<UFlyingCabQuestEventComponent>(TEXT("QuestEvent"));
	QuestEvent->EventId = FlyingCabQuestEvents::InteractionCompleted;
	QuestEvent->TargetId = InteractableId;
	QuestEvent->bEmitOnce = true;
	RefreshAppearance();
}

bool AFlyingCabQuestInteractable::Interact(AFlyingCabCharacter* Character, FText& OutMessage)
{
	if (!Character || Character->IsDead() || !QuestEvent)
	{
		OutMessage = FText::FromString(TEXT("INTERACTION UNAVAILABLE"));
		return false;
	}
	if (QuestEvent->bEmitOnce && QuestEvent->HasEmitted())
	{
		OutMessage = ConsumedMessage;
		return false;
	}
	QuestEvent->TargetId = InteractableId;
	QuestEvent->EmitQuestEvent();
	OutMessage = SuccessMessage;
	RefreshAppearance();
	return true;
}

FText AFlyingCabQuestInteractable::GetInteractionPrompt(
	const AFlyingCabCharacter* Character) const
{
	return QuestEvent && QuestEvent->bEmitOnce && QuestEvent->HasEmitted()
		? ConsumedMessage
		: InteractionPrompt;
}

void AFlyingCabQuestInteractable::Configure(
	FName InInteractableId,
	FName InEventId,
	const FText& InPrompt,
	const FText& InSuccessMessage)
{
	InteractableId = InInteractableId;
	InteractionPrompt = InPrompt;
	SuccessMessage = InSuccessMessage;
	if (QuestEvent)
	{
		QuestEvent->Configure(InEventId, InteractableId);
	}
	RefreshAppearance();
}

void AFlyingCabQuestInteractable::RefreshAppearance()
{
	const bool bConsumed = QuestEvent && QuestEvent->bEmitOnce && QuestEvent->HasEmitted();
	const FLinearColor Color = bConsumed
		? FLinearColor(0.15f, 0.85f, 0.32f)
		: FLinearColor(0.10f, 0.70f, 1.0f);
	Body->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(Color.R, Color.G, Color.B));
	Light->SetLightColor(Color);
	Label->SetTextRenderColor(Color.ToFColor(true));
	Label->SetText(FText::FromName(InteractableId));
}
