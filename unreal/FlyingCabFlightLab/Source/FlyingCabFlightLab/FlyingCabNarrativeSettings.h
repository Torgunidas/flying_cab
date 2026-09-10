#pragma once

#include "Engine/DeveloperSettings.h"
#include "FlyingCabNarrativeSettings.generated.h"

class UFlyingCabQuestCatalog;
class UFlyingCabNpcRoster;
class UFlyingCabDialogueWidget;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Flying Cab Narrative"))
class FLYINGCABFLIGHTLAB_API UFlyingCabNarrativeSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UFlyingCabNarrativeSettings();
	virtual FName GetCategoryName() const override { return TEXT("Game"); }
	UPROPERTY(Config, EditAnywhere, Category = "Content")
	TSoftObjectPtr<UFlyingCabQuestCatalog> QuestCatalog;
	UPROPERTY(Config, EditAnywhere, Category = "Content")
	TSoftObjectPtr<UFlyingCabNpcRoster> NpcRoster;
	/** Optional Widget Blueprint subclass. Empty uses the native conversation panel. */
	UPROPERTY(Config, EditAnywhere, Category = "Presentation")
	TSoftClassPtr<UFlyingCabDialogueWidget> DialogueWidgetClass;
};
