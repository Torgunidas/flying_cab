// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabQuestGiver.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/GameInstance.h"
#include "FlyingCabCharacter.h"
#include "FlyingCabQuestDefinition.h"
#include "FlyingCabQuestSubsystem.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AFlyingCabQuestGiver::AFlyingCabQuestGiver()
{
	PrimaryActorTick.bCanEverTick = false;
	InteractionZone = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionZone"));
	SetRootComponent(InteractionZone);
	InteractionZone->InitBoxExtent(FVector(85.0f, 75.0f, 120.0f));
	InteractionZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(InteractionZone);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetRelativeLocation(FVector(0.0f, 0.0f, -25.0f));
	Body->SetRelativeScale3D(FVector(0.32f, 0.32f, 0.9f));
	Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
	Head->SetupAttachment(InteractionZone);
	Head->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Head->SetRelativeLocation(FVector(0.0f, 0.0f, 85.0f));
	Head->SetRelativeScale3D(FVector(0.34f));
	if (CylinderMesh.Succeeded())
	{
		Body->SetStaticMesh(CylinderMesh.Object);
	}
	if (SphereMesh.Succeeded())
	{
		Head->SetStaticMesh(SphereMesh.Object);
	}
	for (UStaticMeshComponent* Part : {Body, Head})
	{
		if (BasicMaterial.Succeeded())
		{
			Part->SetMaterial(0, BasicMaterial.Object);
		}
	}

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(InteractionZone);
	Label->SetRelativeLocation(FVector(0.0f, 45.0f, 155.0f));
	Label->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	Label->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	Label->SetWorldSize(25.0f);

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(InteractionZone);
	Light->SetRelativeLocation(FVector(0.0f, 65.0f, 80.0f));
	Light->SetIntensity(1300.0f);
	Light->SetAttenuationRadius(380.0f);
	Light->SetCastShadows(false);
	RefreshAppearance();
}

void AFlyingCabQuestGiver::BeginPlay()
{
	Super::BeginPlay();
	if (UFlyingCabQuestSubsystem* Quests = GetQuestSubsystem())
	{
		Quests->OnQuestStateChanged.AddDynamic(this, &AFlyingCabQuestGiver::HandleQuestStateChanged);
	}
	RefreshAppearance();
}

void AFlyingCabQuestGiver::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UFlyingCabQuestSubsystem* Quests = GetQuestSubsystem())
	{
		Quests->OnQuestStateChanged.RemoveDynamic(this, &AFlyingCabQuestGiver::HandleQuestStateChanged);
	}
	Super::EndPlay(EndPlayReason);
}

bool AFlyingCabQuestGiver::Interact(AFlyingCabCharacter* Character, FText& OutMessage)
{
	if (!Character || Character->IsDead() || !QuestDefinition)
	{
		OutMessage = FText::FromString(TEXT("NO ASSIGNMENT AVAILABLE"));
		return false;
	}
	UFlyingCabQuestSubsystem* Quests = GetQuestSubsystem();
	if (!Quests)
	{
		OutMessage = FText::FromString(TEXT("QUEST NETWORK UNAVAILABLE"));
		return false;
	}

	Quests->RecordEvent(FlyingCabQuestEvents::QuestGiverInteracted, QuestGiverId);
	EFlyingCabQuestStatus Status = Quests->GetQuestStatus(QuestDefinition->QuestId);
	if (Status == EFlyingCabQuestStatus::Inactive)
	{
		if (!Quests->StartQuest(QuestDefinition->QuestId))
		{
			OutMessage = FText::FromString(TEXT("ASSIGNMENT UNAVAILABLE"));
			return false;
		}
		OutMessage = FText::GetEmpty();
		RefreshAppearance();
		return true;
	}

	Status = Quests->GetQuestStatus(QuestDefinition->QuestId);
	if (Status == EFlyingCabQuestStatus::ReadyToTurnIn)
	{
		const bool bCompleted = Quests->TurnInQuest(QuestDefinition->QuestId);
		OutMessage = bCompleted
			? FText::GetEmpty()
			: FText::FromString(TEXT("ASSIGNMENT TURN-IN FAILED"));
		RefreshAppearance();
		return bCompleted;
	}
	if (Status == EFlyingCabQuestStatus::Active)
	{
		Quests->SetTrackedQuest(QuestDefinition->QuestId);
		OutMessage = Quests->GetTrackerText();
		return true;
	}

	OutMessage = FText::Format(
		NSLOCTEXT("FlyingCab", "QuestAlreadyCompleted", "ASSIGNMENT ALREADY COMPLETE // {0}"),
		QuestDefinition->Title);
	return true;
}

FText AFlyingCabQuestGiver::GetInteractionPrompt(const AFlyingCabCharacter* Character) const
{
	if (!QuestDefinition)
	{
		return FText::FromString(TEXT("Q // NO ASSIGNMENT"));
	}
	const UFlyingCabQuestSubsystem* Quests = GetQuestSubsystem();
	const EFlyingCabQuestStatus Status = Quests
		? Quests->GetQuestStatus(QuestDefinition->QuestId)
		: EFlyingCabQuestStatus::Inactive;
	switch (Status)
	{
	case EFlyingCabQuestStatus::Inactive:
		return FText::Format(
			NSLOCTEXT("FlyingCab", "QuestOfferPrompt", "Q // ACCEPT {0}"),
			QuestDefinition->Title);
	case EFlyingCabQuestStatus::ReadyToTurnIn:
		return FText::Format(
			NSLOCTEXT("FlyingCab", "QuestTurnInPrompt", "Q // COMPLETE {0}"),
			QuestDefinition->Title);
	case EFlyingCabQuestStatus::Active:
		return FText::Format(
			NSLOCTEXT("FlyingCab", "QuestActivePrompt", "Q // REVIEW {0}"),
			QuestDefinition->Title);
	case EFlyingCabQuestStatus::Completed:
	default:
		return FText::FromString(TEXT("Q // TALK"));
	}
}

void AFlyingCabQuestGiver::Configure(
	FName InQuestGiverId,
	const FText& InDisplayName,
	UFlyingCabQuestDefinition* InQuestDefinition)
{
	QuestGiverId = InQuestGiverId;
	DisplayName = InDisplayName;
	QuestDefinition = InQuestDefinition;
	RefreshAppearance();
}

UFlyingCabQuestSubsystem* AFlyingCabQuestGiver::GetQuestSubsystem() const
{
	UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UFlyingCabQuestSubsystem>() : nullptr;
}

void AFlyingCabQuestGiver::RefreshAppearance()
{
	const UFlyingCabQuestSubsystem* Quests = GetQuestSubsystem();
	const EFlyingCabQuestStatus Status = Quests && QuestDefinition
		? Quests->GetQuestStatus(QuestDefinition->QuestId)
		: EFlyingCabQuestStatus::Inactive;
	FLinearColor Color(0.10f, 0.72f, 1.0f);
	FString Marker(TEXT("!"));
	if (Status == EFlyingCabQuestStatus::Active)
	{
		Color = FLinearColor(1.0f, 0.72f, 0.08f);
		Marker = TEXT("...");
	}
	else if (Status == EFlyingCabQuestStatus::ReadyToTurnIn)
	{
		Color = FLinearColor(0.18f, 1.0f, 0.42f);
		Marker = TEXT("?");
	}
	else if (Status == EFlyingCabQuestStatus::Completed)
	{
		Color = FLinearColor(0.45f, 0.50f, 0.55f);
		Marker.Reset();
	}
	for (UStaticMeshComponent* Part : {Body, Head})
	{
		Part->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(Color.R, Color.G, Color.B));
	}
	Light->SetLightColor(Color);
	Label->SetTextRenderColor(Color.ToFColor(true));
	Label->SetText(FText::FromString(FString::Printf(
		TEXT("%s\n%s"),
		*DisplayName.ToString(),
		*Marker)));
}

void AFlyingCabQuestGiver::HandleQuestStateChanged(
	FName QuestId,
	EFlyingCabQuestStatus Status)
{
	if (QuestDefinition && QuestDefinition->QuestId == QuestId)
	{
		RefreshAppearance();
	}
}
