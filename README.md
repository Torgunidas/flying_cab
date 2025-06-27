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

Instrukcja dodawania nowego questa
Utwórz zasób QuestData

W Godot wybierz New Resource → QuestData (dzięki class_name QuestData w pliku QuestData.gd).

Wypełnij pola:

id – unikalny identyfikator.

title – krótka nazwa.

description – opis zadania.

objective – tekst pokazywany w okienku celu (QuestObjectiveUI).

reward – kwota wypłacana po ukończeniu.

next_quest_id – opcjonalnie id kolejnego questa w linii.

start_target domyślnie ustaw na "quest_giver".

Dodaj zasób do QuestSysRoot

Otwórz scenes/QuestSysRoot.tscn.

W Inspectorze w polu quests dodaj nowo utworzony .tres.
Dzięki temu autoload QuestSys zarejestruje questa w _ready() (linie 20‑23).

Umieść StartTrigger w poziomie

W scenie poziomu (np. scenes/level_1.tscn) dodaj węzeł Area2D z przypiętym skryptem StartTrigger.gd i wpisz quest_id.

Dodaj go do grupy quest_giver (wymagane do wyświetlenia znacznika startu, jeśli system markerów zostanie rozszerzony).

Gdy gracz wejdzie w obszar, quest zostanie aktywowany.

Zdefiniuj cel questa

W miejscu, do którego gracz ma dotrzeć, dodaj Area2D z przypiętym skryptem EndTrigger.gd (ustaw quest_id).

Ten węzeł powinien należeć do grupy quest_goal – dzięki temu MapOverlay wyświetli nad nim ikonę „!” (patrz funkcja _build_goal_markers).

Opcjonalnie możesz ustawić reward_scene (np. scenes/quest/RewardBox.tscn) i reward_texture, aby po ukończeniu pojawił się prosty obiekt nagrody.

(Opcjonalnie) Dodaj dialog

Jeśli quest rozpoczyna się rozmową z NPC, użyj węzła QuestGiver (scripts/info_board.gd).

W polu dialog_resource ustaw DialogBook lub DialogData. W dialogu można w akcjach wykorzystać start_quest i finish_quest.

Test

Uruchom poziom. Po wejściu w StartTrigger quest pojawi się w QuestLog, a aktualny cel w QuestObjectiveUI.

Marker „!” będzie widoczny nad węzłem z grupy quest_goal, jeżeli quest jest aktywny. Po wejściu w EndTrigger quest zostanie ukończony i (jeśli zadano) pojawi się RewardBox.

### Mini‑mapa we wnętrzach

Sceny należące do grupy `interiors` korzystają z innego powiększenia mini‑mapy.
Aby MapOverlay mógł prawidłowo ustawić limity kamery w takich poziomach,
należy dodać węzeł (np. `Sprite2D` lub `Node2D`) w grupie `level_bounds`
określający rozmiar planszy.

### Sterowanie mini‑mapą

Powiększenie mini‑mapy można zmieniać kółkiem myszy lub gestem pinch na
urządzeniach dotykowych. Zakres przybliżenia kontrolują zmienne eksportowane
`zoom_step`, `min_zoom` oraz `max_zoom` w skrypcie `MapOverlay.gd`.

## Licencja

Projekt udostępniany jest na licencji MIT. Szczegóły znajdują się w pliku
`LICENSE`.