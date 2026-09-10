# A_R7 Flying Cab

Modified copy of the downloaded CC0 model in `../A_R7`. Originals are unchanged.

- `A_R7_FlyingCab.blend`: editable Blender 5.2 scene, body and four separate wheel/nozzle meshes. Preview camera and lights are in `Preview_Studio`.
- `A_R7_FlyingCab.fbx`: body and four nozzles only, with original material slots and UVs. The game-ready derivative lives in `Content/Vehicles/A_R7`; integration is documented in `docs/A_R7_SUPERCAR.md` relative to the Unreal project.
- `preview.png` / `preview_underside.png`: rendered checks of the assembly.
- `build_flying_cab.py`: reproducible build; run with Blender in background mode.

Uses `Body/A_R7_Body_1.fbx` without changing its geometry. The package supplies separate wheels, not an assembled vehicle. Two copies each of `Rims_1_R.fbx` and `Rims_2_L.fbx` are positioned at the wheel arches. Right wheels rotate +90 degrees and left wheels -90 degrees about the car's longitudinal X axis; all exterior rim faces point down (-Z). No new nozzle geometry, paint or effects were added.

Source units are retained (body length approximately 3.39 m); FBX uses unit conversion metadata. Car front is +X and up is +Z. Check import scale and materials when integrating into UE 5.8. This asset does not alter game code or controls.

Source license: CC0, see `../A_R7/licence.txt`.
