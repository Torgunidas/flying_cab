#!/usr/bin/env python3
"""Explicit scene authoring tool. Never runs at game startup.

Rebuilding overwrites scenes/city.tscn; retain manual editor changes before use.
Units are metres. Flight is XY at Z=0; grounded towers stand behind that plane.
"""
from pathlib import Path
import math
import random

PROJECT = Path(__file__).resolve().parents[1]
GROUND_Y = -48
rng = random.Random(240912)
resources, nodes, cache = [], [], {}

def num(x):
    return f'{x:.5f}'.rstrip('0').rstrip('.') if x else '0'

def vec(values):
    return f'Vector{len(values)}(' + ', '.join(map(num, values)) + ')'

def color(hex_color, alpha=1):
    return 'Color(' + ', '.join(num(int(hex_color[i:i+2], 16) / 255) for i in (0, 2, 4)) + f', {alpha})'

def sub(kind, properties):
    key = (kind, properties)
    if key not in cache:
        name = f'r{len(resources) + 1}'
        resources.append(f'[sub_resource type="{kind}" id="{name}"]\n{properties}\n')
        cache[key] = f'SubResource("{name}")'
    return cache[key]

def node(name, kind, parent='.', properties='', groups=()):
    group_text = ' groups=[' + ', '.join(f'"{g}"' for g in groups) + ']' if groups else ''
    parent_text = f' parent="{parent}"' if parent is not None else ''
    nodes.append(f'[node name="{name}" type="{kind}"{parent_text}{group_text}]\n{properties}\n')
    return name if parent == '.' else f'{parent}/{name}'

def material(hex_color, emission=0, metal=0.15, rough=0.55):
    props = f'albedo_color = {color(hex_color)}\nmetallic = {metal}\nroughness = {rough}'
    if emission:
        props += f'\nemission_enabled = true\nemission = {color(hex_color)}\nemission_energy_multiplier = {emission}'
    return sub('StandardMaterial3D', props)

def box(parent, name, pos, size, mat, collide=False, rotation=None):
    mesh = sub('BoxMesh', f'size = {vec(size)}')
    props = f'position = {vec(pos)}\nmesh = {mesh}\nmaterial_override = {mat}'
    if rotation:
        props += f'\nrotation = {vec(rotation)}'
    result = node(name, 'MeshInstance3D', parent, props)
    if collide:
        body = node(name + 'Solid', 'StaticBody3D', parent, f'position = {vec(pos)}')
        node('Collision', 'CollisionShape3D', body, 'shape = ' + sub('BoxShape3D', f'size = {vec(size)}'))
    return result

def cylinder(parent, name, pos, radius, height, mat, top=None):
    mesh = sub('CylinderMesh', f'top_radius = {num(radius if top is None else top)}\nbottom_radius = {num(radius)}\nheight = {num(height)}\nradial_segments = 16\nrings = 1')
    return node(name, 'MeshInstance3D', parent, f'position = {vec(pos)}\nmesh = {mesh}\nmaterial_override = {mat}')

def label(parent, name, pos, text, tint, pixel=.025, size=48):
    escaped = text.replace('\n', '\\n')
    return node(name, 'Label3D', parent, f'position = {vec(pos)}\ntext = "{escaped}"\nfont_size = {size}\npixel_size = {pixel}\nmodulate = {color(tint)}\noutline_size = 0\nshaded = false\ndouble_sided = false')

def light(parent, name, pos, tint, energy=2.0, reach=8):
    return node(name, 'OmniLight3D', parent, f'position = {vec(pos)}\nlight_color = {color(tint)}\nlight_energy = {energy}\nomni_range = {reach}\nomni_attenuation = 1.5\ndistance_fade_enabled = true\ndistance_fade_begin = 32.0\ndistance_fade_length = 12.0')

def facade(base, tint, width, height, seed, upper=False, degraded=False):
    return sub('ShaderMaterial', f'shader = ExtResource("facade")\nshader_parameter/base_color = {color(base)}\nshader_parameter/window_color = {color(tint)}\nshader_parameter/panel_count = {vec((max(3, int(width / 1.25)), max(2, int(height / 2.5))))}\nshader_parameter/seed = {seed}.0\nshader_parameter/lit_fraction = {0.16 if degraded else 0.68 if upper else 0.44}\nshader_parameter/window_energy = {0.22 if degraded else 0.42 if upper else 0.75}\nshader_parameter/glass = {0.85 if upper else 0.3}')

def beam(parent, name, start, end, mat, thick=.18):
    delta = [end[i] - start[i] for i in range(3)]
    length = math.sqrt(sum(v*v for v in delta))
    # All structural brackets lie in YZ; cylinders' axis is local Y.
    mesh = sub('BoxMesh', f'size = {vec((thick, length, thick))}')
    centre = [(a+b)*.5 for a,b in zip(start,end)]
    return node(name, 'MeshInstance3D', parent, f'position = {vec(centre)}\nrotation = {vec((math.atan2(delta[2], delta[1]), 0, 0))}\nmesh = {mesh}\nmaterial_override = {mat}')

INK = material('172131', metal=.55, rough=.25)
CONCRETE = material('343b4b', rough=.84)
STEEL = material('59697a', metal=.8, rough=.32)
IVORY = material('bbc8cb', metal=.3, rough=.27)
GOLD = material('d9af73', metal=.75, rough=.25)
PINK = material('ff60b3', 1.4)
CYAN = material('63e6dc', 1.2)
LILAC = material('9d86ff', 1.3)
AMBER = material('ffad62', 1.2)
WHITE = material('c2f5df', 1.2)
GREEN = material('4c8273', rough=.85)

node('City', 'Node3D', None)
node('Skyline', 'Node3D')
node('Buildings', 'Node3D')
node('LandingPads', 'Node3D')
node('Highways', 'Node3D')

# Distant silhouettes still rise from the ground, layered behind the full city.
for i in range(24):
    x = -118 + i * 10.2
    h = rng.uniform(175, 365)
    w = rng.uniform(5.5, 12)
    z = rng.uniform(-48, -36)
    box('Skyline', f'Tower{i}', (x, (GROUND_Y+h)/2, z), (w, h-GROUND_Y, 10), facade('172b3b', '506c88', w, h-GROUND_Y, i))
    box('Skyline', f'Crown{i}', (x, h+1.5, z), (w*.55, 3, 8), INK)
    box('Skyline', f'Beacon{i}', (x, h+3.3, z+5.1), (.18, .35, .15), LILAC)

TOWERS = {}
for side in (-1, 1):
    for index, distance in enumerate((22, 40, 55)):
        x = side * distance
        w = (14, 14, 11)[index]
        h = (297, 280, 250)[index]
        front = -4.8 - index * .7
        parent = node(('West' if side < 0 else 'East') + f'Arcology{index+1}', 'Node3D', 'Buildings', f'position = {vec((x,0,0))}', ('grounded_building',))
        TOWERS[(side,index)] = (parent, w, front, h)
        # A single collision-bearing spine joins the ground and upper terraces.
        spine = node('GroundedSpine', 'StaticBody3D', parent, f'position = {vec((0,(GROUND_Y+h)/2,front-6))}')
        node('Collision', 'CollisionShape3D', spine, 'shape = ' + sub('BoxShape3D', f'size = {vec((w,h-GROUND_Y,12))}'))
        low_base, low_tint = ('30283b','e7a2c9') if side < 0 else ('26373d','a1d8d3')
        upper_base, upper_tint = ('607e84','c4f8e6') if side < 0 else ('354d64','ffe0a3')
        for name, lo, hi, base, tint, upper in [('Sump',GROUND_Y,11,'17262a','8ea99e',False),('Undercity',11,151,low_base,low_tint,False),('ServiceBelt',151,170,'253749','6d889e',False),('UpperCity',170,h,upper_base,upper_tint,True)]:
            box(parent, name, (0,(lo+hi)/2,front-6), (w,hi-lo,12), facade(base,tint,w,hi-lo,index+50*(side+1)+int(lo),upper,name=='Sump'))
        # Chunked plinth, ribs and a setback crown make the silhouette architectural.
        box(parent, 'Foundation', (0,GROUND_Y+2.8,front-4.8), (w+1,5.6,14.4), CONCRETE)
        trim = LILAC if side < 0 else AMBER
        for edge in (-1,1):
            box(parent, f'LowRib{edge}', (edge*(w/2-.2),(GROUND_Y+1+151)/2,front+.1), (.34,150-GROUND_Y,.4), INK)
            box(parent, f'UpperRib{edge}', (edge*(w/2-.15),(170+h)/2,front+.2), (.38,h-170,.5), IVORY if side < 0 else GOLD)
            box(parent, f'CrownLight{edge}', (edge*(w/2-.16),h-8,front+.5), (.08,15,.08), WHITE if side < 0 else AMBER)
        box(parent,'SetbackCrown',(0,h+1.5,front-6),(w*.78,3,10),IVORY if side<0 else GOLD)
        box(parent,'RoofHalo',(0,h+3.1,front-6),(w*.82,.12,10.5),WHITE if side<0 else AMBER)
        for y in range(GROUND_Y+8, 149, 8):
            box(parent,f'FloorBand{y}',(0,y,front+.2),(w,.28,.55),INK)
            box(parent,f'UtilityBox{y}',(-w*.3 if y%16 else w*.3,y+1.1,front+.7),(1.8,1.2,1.1),STEEL)
            for slit in range(4):
                box(parent,f'Vent{y}_{slit}',(-w*.3 if y%16 else w*.3,y+.8+slit*.18,front+1.27),(1.5,.07,.02),INK)
        for y in range(176, h-4, 9):
            box(parent,f'UpperBand{y}',(0,y,front+.15),(w,.14,.3),IVORY if side<0 else GOLD)
        if side < 0:
            # Pearl ribs and rounded service pods give Eden a softer silhouette.
            for edge in (-1,1):
                cylinder(parent,f'PearlColumn{edge}',(edge*(w/2-.8),(171+h)/2,front+.05),.7,h-171,IVORY)
            for y in range(181,h-4,18):
                cylinder(parent,f'RoundedPod{y}',(0,y,front-3),w*.44,1.4,IVORY)
        else:
            # Corporate exoskeleton, narrow polished gold fins and dark glazing.
            for k in range(5):
                box(parent,f'GoldMullion{k}',((k-2)*w*.18,(174+h)/2,front+.18),(.11,h-174,.25),GOLD)
            for y in range(183,h-4,22):
                box(parent,f'CapitalBelt{y}',(0,y,front+.55),(w,.55,1.1),GOLD)
        # Exposed services stop where the prestigious upper cladding begins.
        for k in range(2):
            cylinder(parent,f'Pipe{k}',(w*.42-k*.4,(GROUND_Y+2+146)/2,front+.45),.12,144-GROUND_Y,STEEL)
        # Slender antennas have visible support on each roof.
        cylinder(parent,'Antenna',(w*.25,h+6,front-5),.10,6,STEEL)
        light(parent,'RoofWash',(0,h+2,front+2),'b6ffed' if side<0 else 'ffe1b6',2,10)

DISTRICTS = {
    'velvet': dict(title='VELVET // AFTER HOURS', tint='ff8bc3', accent=PINK, upper=False,
        venues=['VELVET','ROOM 09','SUGAR CLUB','LOVE HOTEL','LAST KISS','BLUE HOUR'], subs=['COCKTAILS / ALL NIGHT','PRIVATE ROOMS','LIVE SHOW / 24 H','ROOMS BY THE HOUR','BAR / NO QUESTIONS','LOUNGE / SINCE 2089']),
    'foundry': dict(title='FOUNDRY // WORK NEVER ENDS', tint='ffc58b', accent=AMBER, upper=False,
        venues=['OCTANE','NIGHT SHIFT','TORQUE','GRIDWORKS','EAST EXCHANGE','AFTERMARKET'], subs=['FUEL / SERVICE','OFFICES / OPEN 24 H','REPAIR / BODY SHOP','ENERGY SYSTEMS','LOGISTICS / OFFICES','PARTS / AUGMENT REPAIR']),
    'eden': dict(title='EDEN // A BETTER YOU', tint='b9ffe4', accent=WHITE, upper=True,
        venues=['EDEN CLINIC','SOMA','NEW SELF','LUCENT','BLOOM','HALO HEALTH'], subs=['REGENERATIVE MEDICINE','WELLBEING / PRIVATE SPA','AUGMENTATION ATELIER','LONGEVITY INSTITUTE','BODY / MIND / BALANCE','PRECISION CARE']),
    'aurelia': dict(title='AURELIA // ABOVE IT ALL', tint='ffe0a3', accent=AMBER, upper=True,
        venues=['AURELIA','CROWN BANK','MERIDIAN','HELIOS','VAULT 01','APEX CAPITAL'], subs=['PRIVATE WEALTH','BANKING / INVITATION ONLY','GLOBAL FINANCE','ASSET MANAGEMENT','TRUST / SECURITY','YOUR FUTURE. OWNED.'])
}

def terrace(name, parent, x, y, front, width, district, venue_index, start=False):
    d = DISTRICTS[district]
    # Parent at tower X, but all pad positions are authored in world XY.
    pad = node(name, 'StaticBody3D', 'LandingPads', f'position = {vec((x,y-.4,0))}\nmetadata/building = NodePath("../../{parent}")', ('refuel_pad',))
    if start or name == 'Foundry03':
        station_id = 'ari_depot' if start else 'foundry_body_shop'
        service_name = 'DEPOT / SERWIS' if start else 'FOUNDRY / BODY SHOP'
        nodes.append(f'[node name="Workshop" parent="{pad}" instance=ExtResource("workshop")]\nstation_id = &"{station_id}"\nservice_name = "{service_name}"\n')
    taxi_specs = {
        'Pad0': ('depot', 'DEPOT ARIEGO', True),
        'Velvet01': ('velvet_neon', 'VELVET / BAR', False),
        'Velvet02': ('velvet_club', 'VELVET / ROOM 09', False),
        'Foundry01': ('foundry_market', 'FOUNDRY / OCTANE', False),
        'Foundry02': ('foundry_docks', 'FOUNDRY / NIGHT SHIFT', False),
        'Foundry03': ('foundry_torque', 'FOUNDRY / TORQUE', True),
    }
    taxi_id, taxi_label, fuel_service = taxi_specs.get(name, (f'{district}_{name[-2:]}', f"{district.upper()} / {d['venues'][venue_index]}", False))
    node('TaxiStop', 'Node3D', pad, f'position = Vector3(0, 0.4, 0)\nscript = ExtResource("taxi_stop")\nstop_id = "{taxi_id}"\ndisplay_name = "{taxi_label}"\ndistrict = "{district.title()}"\nfuel_service = {str(fuel_service).lower()}\nhalf_width = {width / 2}\nwaiting_x = {-width / 2 + 1.2}\ndoor_x = {width / 2 - 1.0}')
    depth = 2.1 - front
    box(pad, 'Deck', (0,0,(front+2.1)/2), (width,.8,depth), IVORY if district=='eden' else STEEL if d['upper'] else CONCRETE)
    node('Collision','CollisionShape3D',pad, f'position = {vec((0,0,(front+2.1)/2))}\nshape = '+sub('BoxShape3D',f'size = {vec((width,.8,depth))}'))
    box(pad,'Fascia',(0,0,2.12),(width,.58,.12), INK if not d['upper'] else GOLD if district=='aurelia' else STEEL)
    box(pad,'EdgeLight',(0,.30,2.20),(width,.075,.075),d['accent'])
    label(pad,'BerthID',(0,-.04,2.21),name.upper()+'  /  FUEL'+(' + REPAIR' if name == 'Foundry03' else ''),d['tint'],.008,34)
    for edge in (-1,1):
        beam(pad,f'Cantilever{edge}',(edge*(width/2-.5),-3.5,front-.15),(edge*(width/2-.5),-.35,1.5),STEEL,.24)
        box(pad,f'PadBeacon{edge}',(edge*(width/2-.3),.7,1.6),(.15,.6,.15),d['accent'])
    for k in range(5):
        box(pad,f'LandingStripe{k}',((k-2)*.75,.408,.65),(.4,.016,.7),WHITE)
    # Door, sign and canopy all anchor directly to the arcology facade.
    box(parent,f'{name}Door',(0,y+1.7,front+.18),(2.6,3.4,.3),INK)
    box(parent,f'{name}DoorHeader',(0,y+3.5,front+.4),(2.8,.12,.12),d['accent'])
    sign_y = y+6.2
    box(parent,f'{name}Sign',(0,sign_y,front+.36),(width+1,3.7,.6),material('17222b') if d['upper'] else material('251726'))
    for dy in (-1.76,1.76):
        box(parent,f'{name}SignEdge{dy}',(0,sign_y+dy,front+.72),(width+1,.085,.09),d['accent'])
    venue = 'ARI / CAB DEPOT' if start else d['venues'][venue_index]
    subtitle = 'READY FOR THE NIGHT' if start else d['subs'][venue_index]
    label(parent,f'{name}Venue',(0,sign_y+.45,front+.72),venue,d['tint'],.022,48)
    label(parent,f'{name}Service',(0,sign_y-.8,front+.72),subtitle,'c8d9e1',.011,36)
    canopy = IVORY if d['upper'] else material('452139') if district=='velvet' else material('917552')
    box(parent,f'{name}Canopy',(0,y+4.4,front+1.6),(width*.8,.23,3.1),canopy)
    box(parent,f'{name}CanopyTube',(0,y+4.26,front+3.12),(width*.8,.055,.07),d['accent'])
    light(parent,f'{name}NeonWash',(0,y+2.8,front+3.8),d['tint'],2.4,8)
    # Additional asymmetrical signage and architectural detail distinguish quarters.
    if district=='velvet':
        box(parent,f'{name}Blade',(width*.46,y+8.9,front+1),(1.2,7.2,.5),INK)
        label(parent,f'{name}BladeText',(width*.46,y+8.9,front+1.3),'N\nI\nT\nE','ff9fcf',.022,40)
        light(parent,f'{name}BlueSpill',(-width*.38,y+1.8,front+2),'7a92ff',1.6,6)
        # Twin neon hoops and a glowing side advertisement, all mounted on brackets.
        ring = sub('TorusMesh','inner_radius = 1.0\nouter_radius = 1.1\nrings = 24\nring_segments = 8')
        node(f'{name}NeonHoop','MeshInstance3D',parent,f'position = {vec((-width*.38,y+10,front+.9))}\nrotation = {vec((math.pi/2,0,0))}\nmesh = {ring}\nmaterial_override = {PINK}')
        label(parent,f'{name}NightIcon',(-width*.38,y+10,front+1),'XO','ffa3d3',.024,42)
        box(parent,f'{name}Ad',(-width*.32,y+17,front+.55),(3.4,6.0,.5),material('471c40'))
        label(parent,f'{name}AdText',(-width*.32,y+17,front+.85),'FEEL\nREAL\n\\n24 / 7','e9a7d4',.018,44)
    elif district=='foundry':
        for k in range(3):
            box(parent,f'{name}Shutter{k}',(-width*.35,y+1+k*.45,front+.45),(1.7,.25,.3),STEEL)
        box(parent,f'{name}Pump',(width*.33,y+1.1,front+1.5),(.8,2.1,.75),material('bf8642',metal=.4))
        box(parent,f'{name}PumpScreen',(width*.33,y+1.55,front+1.9),(.58,.5,.03),CYAN)
        label(parent,f'{name}PumpMark',(width*.33,y+1.55,front+1.95),'F','172131',.008,44)
        box(parent,f'{name}ServiceUnit',(width*.20,y+12.5,front+.8),(5.6,3.3,1.6),material('697a79'))
        for k in range(6):
            box(parent,f'{name}ServiceGrille{k}',(width*.20,y+11.3+k*.42,front+1.63),(5.1,.18,.09),INK)
        label(parent,f'{name}ServiceID',(-width*.37,y+12.4,front+1),'24\nH','ffb880',.025,48)
    else:
        # Terraced planters are attached to the rear of the landing deck, away from the cab.
        for edge in (-1,1):
            cylinder(parent,f'{name}Planter{edge}',(edge*(width*.39),y+.38,front+1.1),.6,.7,IVORY if district=='eden' else GOLD)
            cylinder(parent,f'{name}Topiary{edge}',(edge*(width*.39),y+1.05,front+1.1),.62,.85,GREEN,top=.32)
        if district=='eden':
            box(parent,f'{name}CrossV',(-width*.42,y+9.3,front+.5),(.3,2,.2),WHITE)
            box(parent,f'{name}CrossH',(-width*.42,y+9.3,front+.5),(2,.3,.2),WHITE)
        else:
            for k in range(3):
                box(parent,f'{name}GoldFin{k}',(width*.40+k*.22,y+8.5,front+.65),(.09,4-k*.6,.3),GOLD)

# Ari's terrace sits on a real arcology, just above the contaminated layer.
depot,w,front,h = TOWERS[(-1,0)]
terrace('Pad0',depot,-22,54,front,10,'foundry',0,True)

for side in (-1,1):
    for index in range(3):
        parent,w,front,h = TOWERS[(side,index)]
        x = side*(22,40,55)[index]
        lower = ([28,86,130],[50,110],[70])[index]
        upper = ([186,246,286],[206,266],[226])[index]
        for upper_half, levels in ((False,lower),(True,upper)):
            district = ('eden' if side<0 else 'aurelia') if upper_half else ('velvet' if side<0 else 'foundry')
            for j,y in enumerate(levels):
                venue_index = (j if index==0 else 3+j if index==1 else 5)
                terrace(f'{district.title()}{venue_index+1:02}',parent,x,y+(4 if side>0 and not upper_half else 0),front,min(w-2,10),district,venue_index)

# An inhabited, neglected service undercroft beneath the neon districts.
# Every block starts at ground; all clutter stays behind the clear flight plane.
node('LowLife','Node3D',properties=f'position = {vec((0,GROUND_Y,0))}')
RUST = material('473c32',metal=.3,rough=.85)
DARK = material('19262a',metal=.4,rough=.65)
SICKLY = material('8ec2af',.75)
WARN = material('c97949',.9)
for i,x in enumerate((-62,-48,-32,-8,9,32,48,62)):
    w = (6,7,5,8,7,6,7,6)[i]
    h = (19,26,16,22,17,24,20,29)[i]
    front = -7.8 if abs(x)<15 else -3.2
    block = node(f'ServiceStack{i+1}','Node3D','LowLife',f'position = {vec((x,0,0))}')
    box(block,'GroundedHousing',(0,h/2,front-4),(w,h,8),facade('142327','a8b49a',w,h,700+i,degraded=True),True)
    box(block,'Roof',(0,h+.2,front-4),(w+.4,.4,8.4),RUST)
    for k in range(3):
        box(block,f'SealedWindow{k}',((-1+k)*w*.27,h*.64,front+.16),(w*.21,2.7,.3),DARK)
        box(block,f'WindowBrace{k}',((-1+k)*w*.27,h*.64,front+.37),(.12,3,.15),RUST,rotation=(0,0,.55))
    for k in range(4):
        box(block,f'Duct{k}',(0,3+k*3.4,front+.4),(w,.35,.8),DARK)
    cylinder(block,'Exhaust',(w*.28,h+1.7,front-3),.4,3,RUST)
    cylinder(block,'AirTank',(-w*.28,h+1.1,front-3),.85,1.8,DARK)
    sign = ['NO SIGNAL','CASH ONLY','LOST / FOUND','LOW LIFE','BLACK MARKET','DEBT OFFICE','NO CREDIT','NIGHT TRADE'][i]
    box(block,'AdBacking',(0,7,front+.65),(w*.92,2.0,.3),DARK)
    label(block,'UndercitySign',(0,7.05,front+.83),sign,'a8bdb0' if i%2 else 'd7996e',.014,38)
    box(block,'BrokenTube',(-w*.14,8.1,front+.9),(w*.56,.07,.08),SICKLY if i%2 else WARN)
    light(block,'StreetPool',(w*.25,6,front+2.0),'6eafa7' if i%2 else 'ca815e',1.6,10)
    label(block,'Warning',(w*.1,2.5,front+.5),'KEEP OUT' if i%2 else 'NO FUTURE','a88871',.012,36)
    for k in range(2):
        box(block,f'ScrapContainer{k}',(-w*.32+k*w*.6,1.3,front+.85),(w*.36,2.6,1.0),RUST)
        for seam in range(3):
            box(block,f'ContainerSeam{k}_{seam}',(-w*.32+k*w*.6+(seam-1)*.3,1.3,front+1.37),(.035,2.3,.02),DARK)

# Long conduits and sparse old streetlights make the ground legible through fog.
for i,x in enumerate(range(-60,70,10)):
    node_name = f'UtilityPost{i}'
    cylinder('LowLife',node_name,(x,3,-2.0),.10,6,STEEL)
    box('LowLife',f'OldLamp{i}',(x,6,-1.8),(.75,.18,.4),SICKLY if i%3 else WARN)
    light('LowLife',f'LampPool{i}',(x,5.8,-.5),'81bdb8' if i%3 else 'd39368',1.2,9)
    box('LowLife',f'GroundConduit{i}',(x,1.0,-3.2),(9.6,.22,.26),RUST)

# World-anchored animated layers complement height/depth fog in Compatibility.
node('Smog','Node3D')
for i,z in enumerate((-25,-3,5)):
    mat = sub('ShaderMaterial',f'shader = ExtResource("smog")\nshader_parameter/phase = {i*8}.0\nshader_parameter/opacity = {0.32 if i==0 else 0.22}')
    mesh = sub('QuadMesh',f'size = {vec((224,64))}')
    node(f'Bank{i+1}','MeshInstance3D','Smog',f'position = {vec((.5,GROUND_Y+18,z))}\nmesh = {mesh}\nmaterial_override = {mat}\ncast_shadow = 0',('smog_bank',))

# Six open air lanes: the Unreal cross and ring, with shared visible/gameplay extents.
lanes = [('Spine',.5,160,12,288),('Crosstown',.5,160,144,12),('WestRing',-66.5,160,10,288),('EastRing',67.5,160,10,288),('LowRing',.5,16,144,10),('SkyRing',.5,304,144,10)]
for name,x,y,w,h in lanes:
    lane = node(name,'Node3D','Highways',f'position = {vec((x,y,0))}\nscript = ExtResource("highway")\nsize = {vec((w,h))}',('highway',))
    shader = sub('ShaderMaterial',f'shader = ExtResource("lane")\nshader_parameter/color = {color("94dcca")}\nshader_parameter/length_m = {max(w,h)}.0\nshader_parameter/vertical = {str(h>w).lower()}')
    mesh = sub('QuadMesh',f'size = {vec((w,h))}')
    node('HolographicLane','MeshInstance3D',lane,f'position = {vec((0,0,-2.2))}\nmesh = {mesh}\nmaterial_override = {shader}\ncast_shadow = 0')
    # Sparse signs sit behind the flight plane and do not obstruct passage.
    label(lane,'RouteName',(0,0,-2.1),f'{name.upper()}\nEXPRESS  /  1.5x','a4dacd',.012,32)

# District gateways are facade-mounted, outside the clear crossing.
for side in (-1,1):
    for upper in (False,True):
        district = ('eden' if side<0 else 'aurelia') if upper else ('velvet' if side<0 else 'foundry')
        d = DISTRICTS[district]
        parent,w,front,h = TOWERS[(side,0)]
        y = 174 if upper else 144
        label(parent,f'{district}Gateway',(0,y,front+.7),district.upper(),d['tint'],.038,64)
        label(parent,f'{district}Motto',(0,y-2,front+.7),d['title'].split(' // ')[1],'c4d1db',.012,34)

for side,x in ((-1,-99.5),(1,100.5)):
    for y in range(GROUND_Y+8,321,16):
        box('Highways',f'GridBoundary{side}_{y}',(x,y,-2),(.10,1.0,.08),AMBER)

# Shared, baked signs hang behind the flight plane, 20 m before each boundary.
node('PerimeterWarnings', 'Node3D')
for side, x in (('West', -79.5), ('East', 80.5)):
    for i, y in enumerate(range(GROUND_Y+16, 321, 24)):
        nodes.append(f'[node name="{side}{i:02d}" parent="PerimeterWarnings" instance=ExtResource("perimeter_sign")]\nposition = {vec((x,y,-2.5))}\n')

nodes.append('[node name="FoundryTestPlatform" parent="." instance=ExtResource("test_platform")]\nposition = Vector3(34.5, 149.6, 0)\n')

header = '[gd_scene format=3]\n\n' + '\n'.join([
    '[ext_resource type="PackedScene" path="res://scenes/foundry_test_platform.tscn" id="test_platform"]',
    '[ext_resource type="Shader" path="res://shaders/facade.gdshader" id="facade"]',
    '[ext_resource type="Shader" path="res://shaders/air_lane.gdshader" id="lane"]',
    '[ext_resource type="Shader" path="res://shaders/smog_bank.gdshader" id="smog"]',
    '[ext_resource type="Script" path="res://scripts/highway.gd" id="highway"]',
    '[ext_resource type="Script" path="res://scripts/taxi/taxi_stop.gd" id="taxi_stop"]',
    '[ext_resource type="PackedScene" path="res://scenes/repair_station.tscn" id="workshop"]',
    '[ext_resource type="PackedScene" path="res://scenes/perimeter_sign.tscn" id="perimeter_sign"]']) + '\n\n'
(PROJECT/'scenes/city.tscn').write_text(header+'\n'.join(resources)+'\n'+'\n'.join(nodes))
print(f'Authored city.tscn: {len(nodes)} editable nodes, {len(resources)} shared resources, 25 attached fuel terraces, 6 air lanes')
