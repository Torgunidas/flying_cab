#pragma once

#include "Engine/DeveloperSettings.h"
#include "FlyingCabDialoguePresentation.h"
#include "FlyingCabNarrativeSettings.generated.h"

class UFlyingCabQuestCatalog;
class UFlyingCabNpcRoster;
class UFlyingCabDialogueWidget;
class USoundBase;

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

	/** How fast the NPC line types itself in. */
	UPROPERTY(Config, EditAnywhere, Category = "Presentation|Pacing", meta = (ClampMin = "1.0", ClampMax = "400.0"))
	float TypedCharactersPerSecond = 55.0f;
	/** Extra hold after a comma, semicolon or colon. */
	UPROPERTY(Config, EditAnywhere, Category = "Presentation|Pacing", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ClausePauseSeconds = 0.05f;
	/** Extra hold after a full stop, exclamation mark, question mark or ellipsis. */
	UPROPERTY(Config, EditAnywhere, Category = "Presentation|Pacing", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float SentencePauseSeconds = 0.13f;
	UPROPERTY(Config, EditAnywhere, Category = "Presentation|Pacing", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float LineBreakPauseSeconds = 0.09f;
	/** Fade and scale of the panel when the conversation opens. */
	UPROPERTY(Config, EditAnywhere, Category = "Presentation|Pacing", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float PanelIntroSeconds = 0.16f;
	UPROPERTY(Config, EditAnywhere, Category = "Presentation|Pacing", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float PanelIntroStartScale = 0.92f;
	/** Quiet beat between the finished line and the first answer. */
	UPROPERTY(Config, EditAnywhere, Category = "Presentation|Pacing", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float AnswersDelaySeconds = 0.10f;
	/** Gap between one answer arriving and the next. This is the knock-knock-knock rhythm. */
	UPROPERTY(Config, EditAnywhere, Category = "Presentation|Pacing", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AnswerStaggerSeconds = 0.085f;
	UPROPERTY(Config, EditAnywhere, Category = "Presentation|Pacing", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float AnswerRevealSeconds = 0.14f;
	/** How far an answer slides in from the right, in pixels. */
	UPROPERTY(Config, EditAnywhere, Category = "Presentation|Pacing", meta = (ClampMin = "0.0", ClampMax = "200.0"))
	float AnswerSlideDistance = 30.0f;
	/** Stagger used once the player has skipped the typing. */
	UPROPERTY(Config, EditAnywhere, Category = "Presentation|Pacing", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float SkippedAnswerStaggerSeconds = 0.02f;

	/** Zero disables the typing sound without clearing the asset. */
	UPROPERTY(Config, EditAnywhere, Category = "Presentation|Sound", meta = (ClampMin = "0", ClampMax = "20"))
	int32 TypingSoundEveryCharacters = 3;
	UPROPERTY(Config, EditAnywhere, Category = "Presentation|Sound")
	TSoftObjectPtr<USoundBase> TypingSound;
	UPROPERTY(Config, EditAnywhere, Category = "Presentation|Sound")
	TSoftObjectPtr<USoundBase> AnswerAppearSound;
	UPROPERTY(Config, EditAnywhere, Category = "Presentation|Sound")
	TSoftObjectPtr<USoundBase> AnswerConfirmSound;
	UPROPERTY(Config, EditAnywhere, Category = "Presentation|Sound", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float ConversationSoundVolume = 0.6f;

	/** Collects the tuning above into the plain struct the presentation layer works with. */
	FFlyingCabDialoguePacing GetPacing() const;
};
