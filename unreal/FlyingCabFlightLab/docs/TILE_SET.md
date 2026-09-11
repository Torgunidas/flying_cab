# Tile_Set — gotowe elementy świata

Biblioteka: **Content → Tile_Set**. Pierwszy element: **Highway → BP_HighwayTile**.
Nowe rodzaje elementów (dźwignie, przyciski, sygnały, platformy, stacje) będziemy dodawać w osobnych podfolderach tej biblioteki. Na tym etapie gotowa jest autostrada.

## Układanie autostrady

1. Otwórz mapę FlightLab poza Play. W Content Browser znajdź `Tile_Set/Highway/BP_HighwayTile` i przeciągnij na mapę.
2. W Details → Transform ustaw **Location Y = 0**. Początek klocka oznacza środek strefy lotu. Widoczny pas jest automatycznie odsunięty w tło — nie kopiuj Y = -520 ze starego RingNorth.
3. Przesuwaj w osiach X/Z. W Details wyszukaj `Length` (długość w lokalnej osi X), `Width` (szerokość w lokalnej osi Z), `Depth` (głębokość w osi Y). Domyślnie 2800 × 900 × 400 cm.
4. Duplikuj cały aktor przez Ctrl+D. Dla domyślnego poziomego odcinka przesuń kopię o **2800 cm w X**, aby końce się zetknęły. Możesz lekko nakładać odcinki; premie się nie mnożą.
5. Obrót **Y = 90°** daje odcinek pionowy. Obracaj wokół osi Y, aby pozostać w płaszczyźnie gry X/Z. Skalowanie aktora zmienia pas i strefę razem; wygodniej zmieniać Length/Width, pozostawiając skalę 1.
6. W Details → Tile → Highway Turbo ustaw `Speed Multiplier` (domyślnie 1.5), `Fuel Consumption Multiplier` (0.5) i `Enabled`.
7. Zapisz mapę Ctrl+S, uruchom Play i przeleć przez pas. Pojazd pokazuje HWY TURBO i smugi. Po wyjściu premia płynnie zanika. Minimapę uzupełnia automatycznie linia odcinka, zgodna z jego położeniem, obrotem i długością.

1.5 oznacza limit prędkości wyższy o 50%, 0.5 oznacza zużycie połowy paliwa podczas napędu. Tak jak dotychczas Highway Turbo nie dodaje automatycznego ciągu. Czasy wejścia/wyjścia nadal należą do komponentu HighwayAssist pojazdu (0.5/0.8 s). Pusty bak, wrak, reset i opuszczenie pojazdu zachowują wcześniejsze zabezpieczenia.

## Istniejąca mapa

Nie podmieniono ani nie zapisano mapy użytkownika. Stare RingNorth/RingNorth2 są dekoracjami; nowy BP_HighwayTile jest niezależnym gotowym elementem. Można ustawić jego X/Z i długość według RingNorth2, pozostawiając Y=0, a następnie usunąć zbędną dekorację RingNorth2.

Sześć dawnych stref nadal działa według granic miasta. Usunięcie starej dekoracji nie usuwa dawnej strefy. Usunięcie lub przeniesienie nowego klocka usuwa/przenosi jego własną premię. Przy nałożeniu stref obowiązuje największy mnożnik prędkości i najmniejszy mnożnik spalania, bez mnożenia premii.

Nowe klocki automatycznie dodają linie minimapy. Linie aktualizują się także po przesunięciu, obrocie, skalowaniu, dodaniu i usunięciu klocka podczas gry. Części poza granicami miasta są przycinane; nie rozszerzają zasięgu minimapy. Wyłączenie `Enabled` wyłącza premię, ale pozostawia widoczną drogę i jej linię. Linie są pod znacznikami i nie przechwytują kliknięć.

Ruch uliczny jest osobnym systemem: klocki nie tworzą tras ani pojazdów NPC. Trasy edytuje się w folderze `Routes`, przez `Route Nodes` i `Spawn Count` (patrz `WORLD_EDITING.md`). Autostrady nie mają kolizji blokujących lot. Nie trzeba dodawać triggerów, tagów ani wpisów do listy stref.

## Implementacja i weryfikacja

`AFlyingCabHighwayTile` zawiera widoczny pas, instancjonowane oznaczenia i obrócony box strefy. HighwayAssist odpytuje aktorów w swoim świecie; kopie PIE nie współdzielą rejestru. Chronione pliki wejścia i manifest pozostają bez zmian.

`scripts/Create-TileSet.py` tworzy brakujący Blueprint bez nadpisywania istniejącego i bez zapisu mapy (commandlet Python). `scripts/Verify-TileSet.py` uruchamiaj przez `-ExecutePythonScript`, w pełnym edytorze: testuje duplikowanie, parametry, geometrię, zapis i ponowny odczyt na własnej mapie testowej.

Test `FlyingCab.Core.City.HighwayTile` obejmuje obrócone i skalowane granice, głębokość, nakładanie premii, wyłączenie, przesunięcie i usunięcie. Rozszerzony `FlyingCab.Functional.PIE.HighwayTurbo` ładuje gotowy Blueprint i sprawdza faktyczne limity prędkości oraz spalanie gracza.

macOS: build i pełny pakiet Automation dla Tile_Set **czekają na weryfikację**. Brak dostępu do Maca w tej sesji. Ta sama wersja UE 5.8 i te same źródła/assety dla obu systemów.

Windows UE 5.8 (2026-09-11): build i pełny pakiet Automation **51/51 zaliczone** (40 bez ostrzeżeń, 11 z ostrzeżeniami, 0 błędów), test zapisu i duplikowania zaliczony. Raport: `Saved/Automation/TileSetFull2/index.json`. Wygląd wymaga jeszcze ręcznej oceny w edytorze; testy uruchomiono bez renderowania.

Rozszerzenie o minimapę (2026-09-11): build Windows i pełny pakiet **52/52 zaliczone**. Dodatkowy renderowany test **1/1**; obejrzano podgląd ukośnego odcinka w `Saved/Automation/TileMinimap.png`. macOS nadal czeka na build i testy. Warstwa minimapy jest automatyczna także dla już zapisanych instancji BP_HighwayTile, bez migracji assetu.
