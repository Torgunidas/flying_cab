#include "FlyingCabNarrativeSettings.h"

UFlyingCabNarrativeSettings::UFlyingCabNarrativeSettings()
{
	QuestCatalog = FSoftObjectPath(TEXT("/Game/Data/Quests/DA_FlyingCabQuestCatalog.DA_FlyingCabQuestCatalog"));
	NpcRoster = FSoftObjectPath(TEXT("/Game/Data/Narrative/DA_FlyingCabNpcRoster.DA_FlyingCabNpcRoster"));
}
