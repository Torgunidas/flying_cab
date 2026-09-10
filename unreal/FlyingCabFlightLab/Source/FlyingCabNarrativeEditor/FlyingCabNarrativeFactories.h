#pragma once
#include "Factories/Factory.h"
#include "FlyingCabNarrativeFactories.generated.h"

UCLASS()
class UFlyingCabQuestFactory : public UFactory
{
	GENERATED_BODY()
public:
	UFlyingCabQuestFactory();
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* Parent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual FText GetDisplayName() const override;
	virtual uint32 GetMenuCategories() const override;
};
UCLASS()
class UFlyingCabDialogueFactory : public UFactory
{
	GENERATED_BODY()
public:
	UFlyingCabDialogueFactory();
	UPROPERTY(EditAnywhere, Category = "Template")
	bool bQuestTemplate = true;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* Parent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual FText GetDisplayName() const override;
	virtual uint32 GetMenuCategories() const override;
};
UCLASS()
class UFlyingCabNpcFactory : public UFactory
{
	GENERATED_BODY()
public:
	UFlyingCabNpcFactory();
	// Optional stable identity for migrating existing NPCs. Ordinary authoring generates an ID.
	UPROPERTY(EditAnywhere, Category = "Import")
	FName InitialNpcId;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* Parent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual FText GetDisplayName() const override;
	virtual uint32 GetMenuCategories() const override;
};

extern uint32 FlyingCabNarrativeAssetCategory;
