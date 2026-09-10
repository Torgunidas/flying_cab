#include "FlyingCabNarrativeSettings.h"

UFlyingCabNarrativeSettings::UFlyingCabNarrativeSettings()
{
	QuestCatalog = FSoftObjectPath(TEXT("/Game/Data/Quests/DA_FlyingCabQuestCatalog.DA_FlyingCabQuestCatalog"));
	NpcRoster = FSoftObjectPath(TEXT("/Game/Data/Narrative/DA_FlyingCabNpcRoster.DA_FlyingCabNpcRoster"));
}

FFlyingCabDialoguePacing UFlyingCabNarrativeSettings::GetPacing() const
{
	FFlyingCabDialoguePacing Pacing;
	Pacing.CharactersPerSecond = TypedCharactersPerSecond;
	Pacing.ClausePauseSeconds = ClausePauseSeconds;
	Pacing.SentencePauseSeconds = SentencePauseSeconds;
	Pacing.LineBreakPauseSeconds = LineBreakPauseSeconds;
	Pacing.PanelIntroSeconds = PanelIntroSeconds;
	Pacing.PanelIntroStartScale = PanelIntroStartScale;
	Pacing.OptionsDelaySeconds = AnswersDelaySeconds;
	Pacing.OptionStaggerSeconds = AnswerStaggerSeconds;
	Pacing.OptionRevealSeconds = AnswerRevealSeconds;
	Pacing.OptionSlideDistance = AnswerSlideDistance;
	Pacing.SkippedOptionStaggerSeconds = SkippedAnswerStaggerSeconds;
	Pacing.BlipEveryCharacters = TypingSoundEveryCharacters;
	return Pacing;
}
