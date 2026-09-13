# Ari — pierwszy grywalny tryb pieszy

Wdrożenie 2026-09-13, Godot 4.7.2. Zakres: zaparkuj → wysiądź → chodź i skacz po tarasie → wróć do tego samego auta → odleć. Działa w domyślnej grze przez `scenes/game.tscn` oraz w próbie F6 `scenes/flight_lab.tscn`.

## Sterowanie

| Działanie | Klawiatura | Dotyk |
| --- | --- | --- |
| Wejście / wyjście | Q | Mały przycisk WSIĄDŹ / WYSIĄDŹ nad sterowaniem |
| Chodzenie | A/D lub ←/→ | Dotychczasowe dwie strzałki |
| Skok | W, ↑ lub spacja | Prawy przycisk SKOK |
| Powrót do miejsca wysiadania | R | Zębatka → Wróć do miejsca wysiadania |
| Zapis | F5 | Zębatka → Zapisz grę |

Najpierw zatrzymaj auto i puść ciąg. Przycisk wysiadania pojawia się, kiedy podłoże i wolna przestrzeń pozwalają postawić Ariego obok auta. Podczas wsiadania lub wysiadania pasażera trzeba poczekać na zakończenie tej czynności. Pasażer już siedzący w aucie nie blokuje opuszczenia pojazdu przez Ariego; kurs i pasażer zostają przy zaparkowanym aucie.

Wejście wymaga stania na podłożu blisko wolnego punktu przy aucie. Dostępne są własne auta oraz pojazdy bez właściciela i bez kierowcy. To jeszcze nie kradzież pojazdów NPC. Naprawa pod E i tankowanie pod F zachowują dotychczasowe działanie podczas sterowania autem.

Skok reaguje na nowe naciśnięcie. Przytrzymanie przycisku przez całe lądowanie nie powoduje kolejnego skoku. Przy zmianie trybu i zamykaniu menu trzeba puścić wcześniej trzymane klawisze. Dotykowe palce są śledzone niezależnie; zmiana trybu usuwa ich wcześniejsze polecenia.

## Ruch i prezentacja

`scenes/people/ari.tscn` zawiera `WalkingActor`, kapsułę 1,9 m wysokości i 0,28 m promienia oraz wariant obecnego szkieletu człowieka z żółtym pasem na ubraniu. Parametry można edytować na postaci w Inspectorze:

| Parametr | Wartość robocza |
| --- | --- |
| Prędkość chodu | 4 m/s |
| Przyspieszenie / hamowanie na podłożu | 18 m/s² |
| Początkowa prędkość skoku | 4,4 m/s |
| Grawitacja | 12,25 m/s² |
| Sterowanie w powietrzu | 45% przyspieszenia poziomego |
| Płaszczyzna ruchu | XY, Z = 0 |

Przeniesiono wspólne przyciski i stany animacji ze starego Godota, a impuls skoku, ograniczone sterowanie w powietrzu i sprawdzanie wysiadania z Unreal. Zachowano obecne 4 m/s chodzenia. Animacje rozróżniają stanie, chód, skok, spadanie i krótkie ugięcie przy lądowaniu. Postać zachowuje kierunek patrzenia po puszczeniu kierunku.

Jedna kamera śledzi aktualnego aktora. Pieszo płynnie zbliża się z 19 do 10 m, zmniejsza wyprzedzenie ruchu i podnosi cel o 0,25 m względem środka kapsuły. Ustawienia są w `FlightCameraTuning`. HUD pieszy pokazuje Ariego i portfel, ukrywa paliwo, stan auta i pasażerów. Nie dodano minimapy ani stałego panelu instrukcji do domyślnej gry.

## Przejmowanie sterowania i zapis

`OnFootInteraction` wykonuje zapytania fizyczne i przekazuje sterowanie przez `PlayerSession`. Jeden węzeł Ariego pozostaje w mapie przez kolejne wejścia i wyjścia; w aucie jest niewidoczny, bez aktywnej fizyki i kolizji. Auto nadal istnieje i zachowuje swój model, identyfikator, właściciela, paliwo, HP oraz pasażerów.

Wyjście wymaga kontaktu auta z podłożem, prędkości nie większej niż 0,25 m/s, neutralnego polecenia i wygaszonego ciągu. Autopilot i reset wykluczają interakcję. Po obu stronach bryły kolizji danego modelu sprawdzane są podłoże statyczne, nachylenie i pełna kapsuła postaci. Gdy jedna strona jest zasłonięta, używana jest druga; gdy obie są zablokowane, gracz pozostaje w aucie. Modele mogą nadpisać boczne pozycje markerami `Visual/EntryLeft` i `Visual/EntryRight`; domyślne pozycje wynikają z indywidualnej bryły kolizji. Nie ma generatora uruchamianego przy Play.

Wejście sprawdza odległość do dostępnego punktu (1,35 m), różnicę wysokości, wolnego kierowcę i przeszkody między postacią a punktem. Wskazówka i wykonana akcja używają tego samego pojazdu; jego dostępność jest ponownie sprawdzana przed przekazaniem sterowania.

`PlayerState` jest niezależny od węzła postaci i auta. Schemat zapisu 3 przechowuje tryb, mapę, auto, pozycję, prędkość, kierunek patrzenia i miejsce ostatniego wysiadania. Schematy 1 i 2 nadal są wczytywane jako jazda autem. Walidacja danych Ariego odbywa się przed zmianą sesji. Autosave i zapis w opcjach działają również pieszo. Testy używają osobnych plików w `build/`, aby nie zastępować zapisu użytkownika.

Przygotowanie grafiki renderuje także Ariego i odwiedza pełną trasę próbek miasta również po wczytaniu zapisu pieszego. Przywraca potem jego pozycję, widoczność, kamerę i stan zaparkowanego auta.

## Granice tego etapu

Nie dodano jeszcze wnętrz, drzwi, interakcji dialogowych, kradzieży, obrażeń Ariego, konsekwencji śmierci ani wyskakiwania z lecącego auta. Dotychczasowy `MapRouter` pozostaje podstawą przyszłych pomieszczeń. R / opcja powrotu przenosi Ariego do wolnego miejsca ostatniego wysiadania bez zmiany kredytów i stanu auta; to pomoc prototypowa, nie ustalony system zdrowia lub ratunku. Przekroczenie dolnego marginesu świata również próbuje tego powrotu.

## Weryfikacja

Windows / Godot 4.7.2 / GeForce RTX 3080 Ti: **603/603 kontroli** na tym etapie:

| Zestaw | Wynik |
| --- | --- |
| Tryb pieszy: rzeczywiste podłoże, skok, blokady, wejście/wyjście, zapis i 12 modeli | 45/45 |
| Tryb pieszy z rendererem, kamera, warmup i pełny start z zapisu | 10/10 |
| Architektura | 41/41 |
| Lot i reakcja ciągu | 23/23 + 28/28 |
| Pojazdy i biblioteka modeli | 45/45 + 178/178 |
| Taxi, UI, interakcje, strzałka, populacja | 39/39 + 21/21 + 52/52 + 48/48 + 61/61 |
| Start gry z grafiką | 8/8 |
| Płynność kamery lotu i reset | 4/4 |

Obejrzano obrazy depotu i skoku. Test kamery lotu utrzymał 10,5 m/s, odchylenie kroku obrazu 0,00022 px. To test poprawności śledzenia, nie pomiar wydajności telefonu. macOS, przeglądarka i fizyczny telefon wymagają sprawdzenia tej zmiany; test dotyku na Windows wykorzystuje zdarzenia ekranowe. Nie wykonano eksportu ani paczki itch.io.

Z katalogu prototypu:

```bash
bash tools/verify.sh on-foot
bash tools/verify.sh on-foot-render
```

Windows z lokalnym plikiem ścieżki silnika:

```powershell
& ./tools/run-godot.ps1 --headless --fixed-fps 120 --script tests/on_foot_tests.gd
& ./tools/run-godot.ps1 --resolution 540x960 --disable-vsync --script tests/on_foot_render_tests.gd
```

Obrazy lokalne: `build/on-foot-depot.png`, `build/on-foot-jump.png`, `build/on-foot-back-in-cab.png`, `build/on-foot-restored.png`.
