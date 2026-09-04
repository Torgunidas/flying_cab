# Living World authoring

## Runtime evaluation

The previous point-to-point traffic prototype is disabled by default. Runtime traffic now comes only from `AFlyingCabLivingWorldManager`, so every ambient vehicle visible in the level belongs to the new route-driven system.

During PIE, press `O` to toggle the developer observer (`F8` remains available to Unreal Editor for its native Eject command):

- `WASD` or arrow keys: pan across the X/Z gameplay plane,
- `Page Up` / `Page Down` or numpad `+` / `-`: zoom,
- mouse wheel: optional zoom,
- hold `Shift`: fast pan,
- `Home`: recenter on the controlled pawn,
- `O`: return to the normal follow camera and restore gameplay input.

The observer intentionally keeps the controlled pawn possessed but suspends its gameplay input. A controlled cab is frozen for the duration and resumes with its previous velocity when the observer closes. Ambient traffic and pedestrians continue simulating while the camera moves independently.

The current vertical slice supports two authoring paths. Both use the same route and population data.

## Manual routes in the level

1. Place `FlyingCabLivingRoute` from **Place Actors > All Classes**.
2. Set a unique `RouteId`, `AgentKind`, `RouteClass` and population values in Details.
3. Add entries to `RouteNodes` and move their `LocalLocation` handles in the viewport.
4. Use semantic actions for stops, take-off, landing, buildings and passenger exchange.
5. Give matching vehicle and pedestrian nodes the same `StopId`.

Vehicle routes expose `Smooth Vehicle Corners` and `Vehicle Corner Smoothing Distance`. Smoothing uses bounded custom tangents, so it rounds abrupt direction changes while continuing to pass through every authored node. Disable it for a deliberately rigid lane or reduce the distance in a tight corridor. Pedestrian paths remain linear.

When at least one valid route actor exists in the level, the manager uses the manually authored route network.

## Generated population from a Data Asset

1. Create a Data Asset of class `FlyingCabLivingWorldProfile`.
2. Save it as `/Game/Data/DA_FlyingCabLivingWorldProfile`.
3. Add route definitions, nodes, movement parameters, spawn counts and vehicle colors.
4. Remove manually placed route actors if the profile should own the complete network.

If the default asset does not exist, the Flight Lab uses the built-in three-route prototype.

## Semantic actions

- `PassThrough`: ordinary path point.
- `Stop`: timed stop.
- `TakeOff`: route marker used for authoring and future animation.
- `Land`: vehicle brakes to the point, stops and services its `StopId`.
- `BoardVehicle`: pedestrian waits for a vehicle serving the same `StopId`.
- `ExitVehicle`: destination of the preceding boarding action.
- `EnterBuilding`: pedestrian disappears for `WaitDuration`.
- `ExitBuilding`: pedestrian reappears at this point.
- `Park`: reserved for parked and mission-controlled vehicles.

## Current prototype

- one Glassward–Rainline shuttle with take-off, cruise, approach, landing and dwell;
- one two-vehicle east express loop;
- two pedestrians cycling through buildings and both shuttle stops;
- obstacle sensors ignore triggers but react to blocking world geometry, player pawns, pedestrians and traffic vehicles.
- vehicle bodies add bounded acceleration lag and asynchronous hover motion without moving their collision hulls away from the route.

The previous eight straight-line traffic vehicles are disabled; only Living World vehicles are active.
