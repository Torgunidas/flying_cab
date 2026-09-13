# Biblioteka pojazdów — 2026-09-13

Ukończono zadanie „różne pojazdy” rozpoczęte na Macu i przeniesione na Windows w commicie `ffe9915`. Katalog ma 12 modeli: dotychczasowe `basic_cab` i `heavy_shuttle` oraz dziesięć nowych wariantów. To biblioteka dla przyszłego living world. Domyślna gra nadal zaczyna się taksówką; nie dodano ruchu NPC, kradzieży, pościgów ani mechaniki transportu ładunku.

## Oglądanie i uruchamianie

Otwórz `scenes/vehicle_showroom.tscn` i uruchom F6. Galeria pokazuje wszystkie modele w tej samej skali; strzałki lewo/prawo wybierają pojedynczy model, a `0` przywraca porównanie. Scena ma zapisane modele, oświetlenie i kamerę, można ją edytować w Godocie. Przy uruchamianiu przygotowuje materiały, płomienie, smugi i światła za jednolitym ekranem.

Na Windows, z katalogu prototypu:

```powershell
& '.\Open Vehicle Library.ps1'
& '.\Open Editor.ps1'
& '.\Run Flight.ps1'
```

Skrypty korzystają z `tools/run-godot.ps1`: najpierw `FC_GODOT_BIN`, potem ścieżka z ignorowanego `build/godot-path.txt`, lokalny `build/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe`, następnie `godot` w PATH. Plik `build/godot-path.txt` zawiera jedną linię z pełną ścieżką do pliku wykonywalnego, bez cudzysłowów; odczytuje go również `tools/godot-env.sh`. Po przeniesieniu instalacji wystarczy zaktualizować ten lokalny plik. Godot 4.7.2 dla Windows został pobrany i potwierdzony jako `4.7.2.stable.official.ed1daf0bf`. Binariów i danych edytora nie przechowujemy w Git. Nie zmieniono polityki wykonywania PowerShell ani ustawień systemowych.

Na macOS pozostają dotychczasowe launchery. Galerię można też uruchomić po `source tools/godot-env.sh` przez `"$FC_GODOT_BIN" --path "$FC_PROJECT_DIR" res://scenes/vehicle_showroom.tscn`. Jest to ten sam projekt i zestaw scen na obu systemach.

## Modele i parametry próbne

Jednostkę potwierdzono pomiarem AABB wszystkich nieruchomych siatek bazowego cab: **2,2 m długości, 1,14 m wysokości**, od Y = −0,38 do 0,76. Pomiar obejmuje obudowy dysz i szyld, pomija płomienie i tekst etykiety. Głębokość wraz z bocznymi obrzeżami dysz wynosi 1,185 m. Bazowy kolider zachowuje dokładnie 2,2 × 0,7 × 0,9 m i zerowe przesunięcie.

| ID | Wygląd | Długość × wysokość cab | Masa kg | Ciąg pion/poziom N | Paliwo | HP | Odporność | Miejsca pasażerów |
| --- | --- | --- | ---: | --- | ---: | ---: | ---: | ---: |
| basic_cab | Oryginalna żółta taksówka | 1 × 1 | 100 | 2350 / 1400 | 100 | 100 | 0% | 3 |
| heavy_shuttle | Zachowany wygląd cab, zgodność zapisów | 1 × 1 | 180 | 3500 / 1900 | 180 | 220 | 0% | 6 |
| lorry | Bladoniebieska ciężarówka, wysoka skrzynia | 1,5 × 2 | 320 | 5900 / 2900 | 280 | 400 | 40% | 1 |
| supercar | Czerwony, klinowa bryła, niski kokpit, skrzydło | 1 × 1 | 80 | 2250 / 1560 | 90 | 85 | 5% | 1 |
| limousine | Długa biała karoseria, złote listwy | 2 × 1 | 190 | 4000 / 2280 | 190 | 180 | 15% | 5 |
| luxury_car | Perłowy lakier, złota krata i intarsja dachu | 1 × 1 | 120 | 2820 / 1740 | 130 | 150 | 20% | 3 |
| police_car | Niebieski, bladoniebieskie drzwi, zielone odblaski | 1 × 1 | 125 | 3100 / 2000 | 140 | 180 | 30% | 3 |
| poor_car | Krótki zielony, kanciasty, plamy rdzy | 0,5 × 1 | 60 | 1170 / 630 | 55 | 55 | 0% | 1 |
| tow_car | Pomarańczowa laweta z otwartą platformą | 1,5 × 1,5 | 240 | 4700 / 2400 | 220 | 300 | 35% | 1 |
| normal_car_1 | Fioletowy | 1 × 1 | 100 | 2350 / 1400 | 100 | 100 | 5% | 3 |
| normal_car_2 | Grafitowy | 1 × 1 | 110 | 2475 / 1430 | 115 | 120 | 10% | 3 |
| normal_car_3 | Miedziany | 1 × 1 | 95 | 2280 / 1378 | 95 | 95 | 5% | 3 |

To wartości prototypu, nie masy rzeczywistych samochodów ani finalny balans. Nowe auta zaczynają z pełnym zbiornikiem; zachowano 150/180 paliwa początkowego starszego shuttle. Supercar ma limit poziomy 14 m/s; lorry i tow 8,5 m/s; pozostałe nowe auta 10,5 m/s. Wszystkie zachowują wspólny `ThrustResponse` 0,18/0,12 s. Spalanie nowych modeli skaluje się z pierwiastkiem z masy względem cab; postój bez ciągu nadal nie zużywa paliwa.

## Inspector, geometria i stan

Wybierz aktora i rozwiń **Definition**, albo otwórz jego `.tres` w `resources/vehicles/`. Masa, obie siły ciągu, pojemność i spalanie, `max_hull` oraz `damage_resistance` są osobnymi edytowalnymi polami. Odporność 0–0,95 redukuje otrzymywane obrażenia: `amount * (1 - damage_resistance)`. Zwiększenie HP nie zwiększa obrażeń. Pola ciała obejmują `visual_scene`, kolizję, przesunięcie, początek reflektorów i prześwit nawigacji. `size_units` jest zapisaną metadaną autorowania; jej zmiana sama nie skaluje geometrii.

Zapisane sceny wyglądu znajdują się w `scenes/vehicles/visuals/`, gotowi aktorzy w `scenes/vehicles/`. Wszystkie używają jednego `scripts/cab.gd`; mają własną definicję i prywatny stan. Sceny aktorów odwołują się do scen wyglądu i wspólnych komponentów świateł/efektów, nie zawierają kopii fizyki. Wstawiając kolejne egzemplarze, nadaj unikalne `entity_id`.

Nowe kolidery obejmują nieruchomą sylwetkę i mają 1,2 m głębokości; ich dół leży na Y = −0,38. Prześwit nawigacji uwzględnia niesymetryczną wysokość względem początku auta oraz margines. To konserwatywne pojedyncze bryły, także nad pustą częścią lawety. Modelowanie kontaktu z ładunkiem należy do przyszłego zadania. Dysze są rozstawione stosownie do długości; początek świateł i odległość początku smugi zależą od modelu.

`_enter_tree()` wybiera ciało przed wiązaniem komponentów w `_ready()`. `apply_model()` waliduje definicję, odtwarza wygląd i kolider, ustala masę/ID, zeruje polecenia i przepina światła oraz dysze. `RuntimeContext.bind_vehicle()` rozwiązuje ID zapisanego modelu przed odtworzeniem stanu. Wczytanie zachowuje paliwo, procent uszkodzenia/HP, pasażerów i pozycję; modele oraz egzemplarze nie współdzielą stanu. Stare zapisy `heavy_shuttle` i brak pola modelu nadal działają.

## Policja i laweta

`Visual/Beacons` radiowozu ma skrypt `VehicleBeacons`. Domyślnie co 8 s występuje 2,4 s serii naprzemiennych błysków z krokiem 0,12 s. Parametry można edytować w scenie wyglądu. Każdy egzemplarz ma własne materiały i zegar. Lampy są emisyjnymi siatkami, bez dodatkowych źródeł światła/cieni, syreny i logiki policyjnej. `show_warmup_effects()` pokazuje obie lampy, a `reset_visuals()` przywraca aktualną fazę natychmiast.

Laweta ma wyłącznie model i pusty **`Visual/CargoMount`** (`Marker3D`) na `(-1.65, 0.41, 0)`. Przód pojazdu wskazuje lokalne +X. Środek ustawionego tam cab leży na tylnej krawędzi platformy, więc jego tył wystaje o 1,1 m, dokładnie połowę długości. Y dopasowuje spód głównego podwozia cab do platformy; obudowy dysz pozostają po bokach. Statyczną przymiarkę tworzy wyłącznie test renderowania i usuwa ją po zdjęciu. Nie ma załadunku, przewozu, rozładunku, fizycznego mocowania ani wliczania masy ładunku.

## Jawne autorowanie

`tools/build_vehicles.gd` tworzy tylko dziesięć nowych modeli, ich definicje/aktorów, katalog i galerię. Wymaga jawnej flagi; nie jest wywoływany przez Play ani scenę miasta:

```powershell
& '.\tools\run-godot.ps1' --headless --script tools/build_vehicles.gd -- --write-models
```

Narzędzie nadpisuje wymienione wygenerowane zasoby — zmiany ręczne w nich trzeba przenieść do generatora przed jego ponownym użyciem. Nie przebudowuje miasta ani oryginalnej grafiki cab. Siatki klinowe są zapisanymi `ArrayMesh`, reszta prostymi `BoxMesh`/`CylinderMesh`; brak CSG i tworzenia geometrii w grze. Nowe auto ma 24–35 siatek (oryginalny cab: 26), z prostymi współdzielonymi materiałami i ograniczonymi cieniami. To budżet małej biblioteki, nie potwierdzenie wydajności dużego ruchu NPC na telefonie. Przyszły system wprowadzania aut do świata musi wywołać istniejące przygotowanie ich efektów przed pokazaniem.

## Weryfikacja Windows

Godot **4.7.2 stable**, Windows, OpenGL Compatibility, NVIDIA RTX 3080 Ti. **651/651 kontroli**: modele 178, pojazdy 45, reakcja ciągu 28, lot 23, postój 16, efekty 18, smog 24, architektura 41, taxi 39, trasa 6, interakcje 52, UI 21, populacja przystanków 61, strzałka celu 48, przestrzeń/paliwo 43, start z rendererem 8. Końcowe logi nie zawierają błędów ani ostrzeżeń. Są zapisane lokalnie w `build/windows-*.log` i `build/windows-verification.json`.

Nowy zestaw obejmuje pomiar rzeczywistych siatek, rozmiary kolizji/prześwitów, start i zmianę modelu, zwolnienie poprzedniego wyglądu, przepięcie efektów, redukcję obrażeń, pełny zapis JSON wszystkich modeli, odtwarzanie ich wyglądu i stanu, osobne egzemplarze, fazy radiowozu, wymiary lawety oraz rzeczywiste lądowanie i start każdego modelu. Aktualizowano stare założenie katalogu ograniczonego do dwóch aut.

`tools/check_clean_import.py --engine <Godot.exe>` został uruchomiony na Windows: **PASS, exit=0, errors=0**. Działa z pełną ścieżką do silnika i z natywnymi ścieżkami Windows; nie wymagał zmiany. Import w ograniczonej piaskownicy zgłaszał problemy dostępu do certyfikatów/danych użytkownika; końcowy import i testy wykonano z normalnym dostępem lokalnym.

Render galerii, wszystkich 12 modeli osobno i statycznej przymiarki lawety: PASS. Obejrzano obrazy, poprawiono nakładanie dachu na górną powierzchnię kabiny. Wyniki: `build/vehicle-library.png`, `build/vehicle-<id>.png`, `build/vehicle-tow-fit.png`. Start normalnej gry: 8/8, przygotowanie grafiki około 6,8 s w tej próbie; to nie benchmark FPS.

Powtórzenie: `bash tools/verify.sh models`, `bash tools/verify.sh models-render` lub na Windows `tools/run-godot.ps1` z `--headless --fixed-fps 120 --script tests/vehicle_models_tests.gd`. Render: bez `--headless`, z `--script tests/vehicle_showroom_render_tests.gd`. Nowa zawartość wymaga jeszcze weryfikacji macOS oraz telefonu. Nie wykonano eksportu ani pakowania na itch.io.
