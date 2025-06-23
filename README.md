# flying_cab
# FlyingCab

FlyingCab to prototypowa gra 2D tworzona w Godot 4. Repozytorium zawiera
sceny, skrypty i zasoby potrzebne do uruchomienia projektu.

## Wymagania

- Godot Engine 4.x (projekt korzysta z `features = "4.4, Mobile"` w pliku
  `project.godot`)
- System Windows lub przeglądarka (przygotowane presety eksportu)

## Uruchomienie

1. Otwórz `project.godot` w Godot 4.
2. Główną sceną startową jest `scenes/ui_scenes/MainMenu.tscn`
   (UID `uid://dy0b0lt64gpho`).
3. Uruchom projekt z poziomu edytora (`F5`) lub skorzystaj z jednego z
   presetów eksportu:
   - **Web** – plik wynikowy `exports/index.html`
   - **Windows Desktop** – plik `FlyingCab.exe`

## Struktura katalogów

Assets/ – grafika i dźwięki (wiele paczek ZIP i plików PSD/SVG)
dialogues/ – zasoby dialogów (*.tres, *.tscn)
scenes/ – sceny gry (poziomy, UI, pojazdy, questy)
scripts/ – skrypty GDScript (logika gracza, NPC, system questów, UI)

markdown
Kopiuj

Najważniejsze skrypty:

- `GameState.gd` – przechowuje stan globalny (np. pieniądze, wybrany poziom)
- `GameRoot.gd` – ładuje poziom i gracza po uruchomieniu
- `QuestSys.gd` – zarządza misjami/questami
- `car_neo.gd` – model pojazdu/taksówki
- `ui_logic/` – logika menu, dialogów, mapy i innych elementów interfejsu

Autoloady definiowane są w `project.godot` (GameState, QuestSys, QuestLog itp.).

## Eksport

Plik `export_presets.cfg` zawiera dwa presety:

- **Web** – eksport do folderu `exports/`
- **Windows Desktop** – eksport do `FlyingCab.exe`

Aby zbudować grę, w edytorze Godot wybierz `Project > Export...` i uruchom
eksport zgodnie z wybranym presetem.

## Licencja

Projekt udostępniany jest na licencji MIT. Szczegóły znajdują się w pliku
`LICENSE`.