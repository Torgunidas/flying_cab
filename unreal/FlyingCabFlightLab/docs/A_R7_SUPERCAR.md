# A_R7 Supersport

Optional, freely accessible car assembled from the CC0 A_R7 download. The default player pawn remains the existing cab.

## Where to find it

Four parked cars spawn in bays connected to district platforms: Yellow Projects, Ashline Market, Neon Docks and Glassward Transit. Bays are 1330 cm from the district center, to the right at Ashline Market and to the left elsewhere. Their positions keep passenger pickup/dropoff approaches clear. Spawn height is traced against the actual map floor. Walk up and use the existing Q interaction to enter. No purchase, quest, license or progression flag is required. Normal fuel, damage and destruction recovery still apply; each car starts with a full tank and is registered with the fleet for recovery.

One NPC per Express route uses the same model (six on the default Metro routes). These cars follow existing routes, signals, stops and obstacle detection, with a 1.6x speed/acceleration/deceleration profile. Other NPC cars retain their current behavior. Moving traffic remains ambient traffic; the player drives the four parked copies.

## Performance and presentation

- Horizontal speed limit: 3150 cm/s, compared with 1050 for the standard cab; existing highway bonus raises it to 4725.
- Horizontal acceleration: 4200 cm/s², compared with 1400.
- Climb limit: 1725 cm/s; vertical thrust: 3000 cm/s².
- Original input, damping, reset and possession logic is reused unchanged.
- CCD is enabled on the optional player's hull. Collider half extents are 110 × 62 × 29 cm.
- Four plume mounts follow the rotated rim faces, including mirrored travel direction.

`Content/Vehicles/A_R7/` contains the imported static mesh and materials. `Build/A_R7/` contains its reproducible OBJ, material palette and MTL. The Blender source remains in `assets/cars/A_R7_FlyingCab/`; the original download remains unchanged in `assets/cars/A_R7/`.

Run `scripts/Export-A_R7.py` with Blender, then `scripts/Import-A_R7.py` with the UE 5.8 Python commandlet to rebuild the asset. The mesh is uniformly scaled to 220 cm length and centered without reshaping the body. Material slots and UVs are retained; only the paint material responds to the game's Color parameter. `/Game/Vehicles/A_R7` is explicitly included in cooked builds on both Windows and macOS. No platform-specific engine paths are stored in game configuration.

## Verification

The importer checks mesh bounds and material assignments. `FlyingCab.Functional.PIE.Supercar` checks four parked instances, public entry, a nearby solid floor, full starting fuel, the imported mesh, NPC instances, possession and return to the original cab, and the 3150 cm/s player speed cap.

Verified on Windows with UE 5.8 on 2026-09-10: Editor Development build succeeded; Core + Functional.PIE passed 46/46 tests (`Saved/Logs/A_R7Tests.log`, 10 tests include expected warning logs from failure/recovery scenarios). After adding the parking extensions, ResidentialAccess detected an overlap with Ashline Court's approach; the Ashline Market bay was moved east. Final rendered Supercar, ResidentialAccess and WorldStartup checks all pass (`Saved/Logs/A_R7Final.log`). The in-game image is `Saved/Automation/A_R7_InGame.png`. macOS compilation and a user's manual driving session have not been performed in this task.
