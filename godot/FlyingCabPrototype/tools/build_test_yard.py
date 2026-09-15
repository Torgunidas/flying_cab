#!/usr/bin/env python3
"""Explicitly bake the Foundry platform and its playable fleet; never runs at Play.

Geometry belongs to the editable city, placed at (34.5, 149.6, 0). The physical
fleet stores world poses in an untransformed level instance, outside compilation.
"""
from pathlib import Path
import re

PROJECT = Path(__file__).resolve().parents[1]
MODELS = ["basic_cab", "heavy_shuttle", "lorry", "supercar", "limousine",
          "luxury_car", "police_car", "poor_car", "tow_car",
          "normal_car_1", "normal_car_2", "normal_car_3"]
NAMES = ["CAB", "SHUTTLE", "LORRY", "SUPERCAR", "LIMOUSINE", "LUXURY",
         "POLICE", "POOR", "TOW CAR", "VIOLET", "GRAPHITE", "COPPER"]

def vector(value):
    return "Vector3(" + ", ".join(f"{part:.4f}" for part in value) + ")"

resources, nodes = [], []

def material(name, rgb, emission=0):
    color = "Color(" + ", ".join(str(v) for v in (*rgb, 1)) + ")"
    resources.append(f'[sub_resource type="StandardMaterial3D" id="{name}"]\nalbedo_color = {color}\nmetallic = 0.5\nroughness = 0.65\nemission_enabled = {str(emission > 0).lower()}\nemission = {color}\nemission_energy_multiplier = {emission}\n')

def box(name, at, size, mat, rotation=0):
    resources.append(f'[sub_resource type="BoxMesh" id="{name}"]\nsize = {vector(size)}\n')
    nodes.append(f'[node name="{name}" type="MeshInstance3D" parent="."]\nposition = {vector(at)}\nrotation_degrees = Vector3({rotation}, 0, 0)\nmesh = SubResource("{name}")\nmaterial_override = SubResource("{mat}")\ncast_shadow = {1 if name == "Deck" else 0}\n')

def label(name, at, text, pixel=0.007, font=44, color="Color(0.8, 0.96, 0.94, 1)"):
    nodes.append(f'[node name="{name}" type="Label3D" parent="."]\nposition = {vector(at)}\ntext = "{text}"\npixel_size = {pixel}\nfont_size = {font}\nmodulate = {color}\noutline_size = 0\nshaded = false\ndouble_sided = false\ncast_shadow = 0\n')

material("steel", (0.13, 0.19, 0.23))
material("dark", (0.055, 0.09, 0.115))
material("teal", (0.24, 0.87, 0.79), 1.1)
material("amber", (1.0, 0.61, 0.26), 0.8)
material("paint", (0.62, 0.77, 0.76), 0.2)
resources.append('[sub_resource type="BoxShape3D" id="deck_shape"]\nsize = Vector3(53, 0.8, 8.8)\n')
nodes.append('[node name="FoundryTestPlatform" type="StaticBody3D" groups=["vehicle_test_platform"]]\nmetadata/entity_id = "city_02/test_platform/foundry"\nmetadata/building = NodePath("../Buildings/EastArcology2")\n')
nodes.append('[node name="Collision" type="CollisionShape3D" parent="."]\nposition = Vector3(0, 0, -2.2)\nshape = SubResource("deck_shape")\n')
box("Deck", (0, 0, -2.2), (53, 0.8, 8.8), "steel")
box("FrontFascia", (0, 0.04, 2.25), (53, 0.5, 0.12), "dark")
box("FrontEdge", (0, 0.32, 2.32), (53, 0.06, 0.06), "teal")
box("PedestrianStripe", (0, 0.405, 1.85), (52, 0.012, 0.06), "paint")
for i, x in enumerate([-12.5, 5.5, 20.5]):
    box(f"Bracket{i}", (x, -1.5, -4.5), (0.35, 4.6, 0.35), "steel", 42)
    box(f"Mount{i}", (x, -1.9, -6.35), (1.2, 4.3, 0.4), "dark")
box("EntrySign", (-21, 2.65, -3.6), (10, 2.4, 0.18), "dark")
box("EntryNeon", (-21, 3.85, -3.45), (10, 0.07, 0.05), "amber")
label("Title", (-21, 3.2, -3.45), "FOUNDRY", 0.012, 52)
label("Subtitle", (-21, 2.15, -3.45), "TEST FLEET", 0.013, 64)
label("LandingLabel", (-23, 0, 2.35), "LAND HERE", 0.008, 42)
for i in range(5):
    box(f"LandingStripe{i}", (-25 + i, 0.414, 0), (0.5, 0.02, 1.65), "paint")
label("BayDirection", (-17.6, 0.01, 2.35), ">>", 0.007, 44)

fleet = ['[gd_scene format=3]\n']
for i, model in enumerate(MODELS):
    path = "scenes/cab.tscn" if model == "basic_cab" else f"scenes/vehicles/{model}.tscn"
    fleet.append(f'[ext_resource type="PackedScene" path="res://{path}" id="car{i}"]\n')
fleet.append('[node name="FoundryTestFleet" type="Node3D"]\n')
cursor = -18.5  # X=16 in the city: leave the western end for the player's arrival.
for i, model in enumerate(MODELS):
    content = (PROJECT / f"resources/vehicles/{model}.tres").read_text()
    def read_vector(key, default):
        match = re.search(rf"^{key} = Vector3\(([^)]+)\)", content, re.M)
        return tuple(map(float, match[1].split(","))) if match else default
    size = read_vector("collision_size", (2.2, 0.7, 0.9))
    offset = read_vector("collision_offset", (0, 0, 0))
    x = cursor + size[0] * 0.5
    y = 0.4 + size[1] * 0.5 - offset[1] + 0.035
    fleet.append(f'[node name="{model}" parent="." groups=["test_fleet"] instance=ExtResource("car{i}")]\nposition = {vector((x + 34.5, y + 149.6, 0))}\nentity_id = &"test_fleet/{model}"\ninitial_owner_id = &""\n')
    label(f"BayLabel{i:02d}", (x, 0.01, 2.36), f"{i+1:02d} / {NAMES[i]}", 0.008, 44)
    for side in [-1, 1]:
        box(f"BayMark{i}_{side+1}", (x + side * (size[0] * 0.5 + 0.24), 0.413, -0.2), (0.06, 0.02, 2.0), "paint")
    cursor += size[0] + 1.2
assert cursor - 1.2 < 25.5, "Fleet exceeds platform edge"
(PROJECT / "scenes/foundry_test_platform.tscn").write_text('[gd_scene format=3]\n\n' + '\n'.join(resources + nodes))
(PROJECT / "scenes/foundry_test_fleet.tscn").write_text('\n'.join(fleet))
print("FOUNDRY_TEST_YARD: baked 53 m platform, clear arrival bay and 12 playable cars")
