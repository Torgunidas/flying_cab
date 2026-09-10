"""Create the editable Mike/Jack profiles and conversations once, without overwriting authored content.

Run with UnrealEditor-Cmd <project> -run=pythonscript -script=<this file>.
Only new assets under /Game/Data/Narrative are saved. Existing quest definitions and the map are untouched.
"""
import unreal

ROOT = '/Game/Data/Narrative'
assets = unreal.AssetToolsHelpers.get_asset_tools()
unreal.EditorAssetLibrary.make_directory(ROOT)
created = []


def create(name, cls, factory):
    path = ROOT + '/' + name
    existing = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if existing:
        assert isinstance(existing, cls), path + ' has an unexpected class'
        return existing, False
    obj = assets.create_asset(name, ROOT, cls, factory)
    assert obj, 'Cannot create ' + path
    created.append(obj)
    return obj, True


def dialogue(name):
    return create(name, unreal.FlyingCabDialogueDefinition, unreal.FlyingCabDialogueFactory())[0]


nightshift = unreal.load_asset('/Game/Data/Quests/DA_Quest_NightshiftContract')
money = unreal.load_asset('/Game/Data/Quests/DA_Quest_Get_Money')
assert nightshift and money, 'Existing quest examples must be available'
nightshift_dialogue = dialogue('DA_Dialogue_Nightshift')
money_dialogue = dialogue('DA_Dialogue_GetMoney')

smalltalk_factory = unreal.FlyingCabDialogueFactory()
smalltalk_factory.set_editor_property('quest_template', False)
smalltalk, is_new = create('DA_Dialogue_CityTalk', unreal.FlyingCabDialogueDefinition, smalltalk_factory)
if is_new:
    node = smalltalk.get_editor_property('nodes')[0]
    node.set_editor_property('text', 'The city never really sleeps. Watch the traffic, keep fuel in the tank, and you will do fine.')
    choice = unreal.FlyingCabDialogueChoice()
    choice.set_editor_property('text', 'Thanks. What else can we talk about?')
    choice.set_editor_property('action', unreal.FlyingCabDialogueAction.TOPICS)
    node.set_editor_property('choices', [choice])
    smalltalk.set_editor_property('nodes', [node])


def profile(name, npc_id, display_name, greeting, quest, conversation):
    factory = unreal.FlyingCabNpcFactory()
    factory.set_editor_property('initial_npc_id', npc_id)
    obj, is_new = create(name, unreal.FlyingCabNpcDefinition, factory)
    if is_new:
        obj.set_editor_property('display_name', display_name)
        obj.set_editor_property('minimap_initial', display_name[0])
        obj.set_editor_property('greeting', greeting)
        assignment = unreal.FlyingCabNpcTopic()
        assignment.set_editor_property('quest', quest)
        assignment.set_editor_property('dialogue', conversation)
        talk = unreal.FlyingCabNpcTopic()
        talk.set_editor_property('title', 'Tell me about the city.')
        talk.set_editor_property('dialogue', smalltalk)
        obj.set_editor_property('topics', [assignment, talk])
    return obj


mike = profile('DA_NPC_Mike', 'QuestGiver.Mike', 'MIKE', 'Looking for work, driver?', nightshift, nightshift_dialogue)
jack = profile('DA_NPC_Jack', 'QuestGiver.Jack', 'JACK', 'How is the shift going?', money, money_dialogue)
roster, is_new = create('DA_FlyingCabNpcRoster', unreal.FlyingCabNpcRoster, unreal.DataAssetFactory())
if is_new:
    spawns = []
    for npc, position in [(mike, (-10000, 0, 10060)), (jack, (20500, 0, 4860))]:
        spawn = unreal.FlyingCabNpcSpawn()
        spawn.set_editor_property('profile', npc)
        spawn.set_editor_property('world_location', unreal.Vector(*position))
        spawns.append(spawn)
    roster.set_editor_property('npcs', spawns)

for obj in created:
    assert unreal.EditorAssetLibrary.save_loaded_asset(obj), obj.get_path_name()
unreal.log('NARRATIVE EXAMPLES: created %d assets; existing assets preserved.' % len(created))
