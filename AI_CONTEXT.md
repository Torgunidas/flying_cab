# AI CONTEXT - Flying Cab Project

## 1. Project Overview
- **Engine:** Godot 4.5.1 (Stable)
- **Language:** GDScript 2.0 (Strict typing preferred)
- **Type:** 2D Side-Scroller VTOL (Vehicle Take-Off and Landing) with narrative elements.
- **Visual Style:** Pixel art, cyberpunk/noir aesthetic.

## 2. Architecture & Core Systems

### Key Autoloads (Singletony)
- `GameState` (`GameState.gd`): Zarządza ekonomią (money), globalnym czasem (Maya Timer), odblokowanymi pojazdami i przedmiotami.
- `VehPers` (Vehicle Persistence): Kluczowy system zapisujący stan auta (paliwo, HP, pozycja) między zmianami scen.
- `QuestSys`: System zadań oparty na Resource (`QuestData`).
- `GameRoot` (`GameRoot.gd`): Zarządca sceny. Ładuje poziomy, usuwa domyślne auta i wstrzykuje auto gracza z `VehPers`.

### Physics & Movement (`Car.gd`)
- **Typ:** `CharacterBody2D` (nie RigidBody!).
- **Fizyka:** Customowa implementacja inercji.
  - `vertical_damping`: Tłumienie wznoszenia.
  - `soft_ceiling`: Mechanika "miękkiego sufitu" mapy (siła wypychająca w dół przy `max_altitude_y`).
  - **Balance System:** Raycasty (`rc_front`, `rc_rear`) wykrywają, czy auto stoi stabilnie, czy zsuwa się z krawędzi.
- **Paliwo:** Zużycie zależne od ciągu (`thrust_up` vs `hover`). Regeneracja podczas opadania.

### Player Logic (`player_character.gd`)
- Postać może swobodnie chodzić (`Platformer physics`) i wsiadać do pojazdów.
- **Enter Vehicle:** Przekazuje kontrolę (input, kamera, UI) do instancji `Car`.
- **UI Decoupling:** UI (`MobileControls`) jest luźno powiązane, znajdowane przez grupę lub nazwę.

## 3. Coding Guidelines
- **Class Names:** Używaj `class_name` dla wszystkich głównych skryptów (już zdefiniowane: `Car`, `PlayerCharacter`, `GameRoot`, `QuestData`).
- **State Management:** Nigdy nie modyfikuj stanu auta bezpośrednio przy ładowaniu poziomu – polegaj na `VehPers`.
- **Signal-Driven:** Preferuj sygnały do komunikacji z UI (np. `fuel_percent_changed`, `money_changed`).

## 4. Current Status (Last Updated: Session 1)
- **Analiza:** Przeanalizowano `car.gd`, `player_character.gd`, `GameState.gd`, `GameRoot.gd`.
- **Fizyka:** Działa poprawnie (VTOL + soft ceiling).
- **Zadania:** System Delivery z `TaxiMarker` i naliczaniem opłaty (`fare_accum`).
- **TODO / Next Steps:**
    1. Rozważamy system ulepszeń (Warsztat).
    2. Rozważamy refaktoryzację połączeń z UI (zamiast szukania węzłów po nazwie).