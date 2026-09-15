# Podejście do platformy i kamera piesza — 2026-09-14

Kamera zaczyna zbliżenie podczas zwalniania w pobliżu tarasu. Odległość jest liczona od najbliższego punktu górnej powierzchni bryły `Collision`, dzięki czemu działa po obu stronach zwykłej platformy i na całym długim tarasie Foundry / Test Fleet.

| Sytuacja | Kadr |
| --- | --- |
| Otwarta przestrzeń / szybki przelot | Dotychczasowy dystans 19 m, FOV 55°, kąt 5° |
| Podejście | Zbliżenie zaczyna się w promieniu 8 m od powierzchni tarasu; narasta przy zmniejszaniu odległości i prędkości |
| Lądowanie / zaparkowane auto | Dystans 6 m; pełna bliskość do 0,8 m od powierzchni i przy prędkości do 3 m/s |
| Pieszo | Dystans 4,2 m, poziomy widok z boku; Ari ma około 15% wysokości obrazu 540×960 |
| Powrót do auta i odlot | Najpierw kadr lądowania; płynne otwarcie przy oddalaniu i rozpędzaniu |

Przy 9 m/s lub szybciej kamera zachowuje szeroki kadr lotu. Pomiędzy 3 a 9 m/s wpływ zbliżenia zmienia się płynnie. Przelot pod platformą nie jest podejściem do jej nawierzchni. Kamera nie wymaga aktywnego kursu ani wybranej platformy docelowej.

Pieszo cel znajduje się 0,65 m nad środkiem postaci, a wyprzedzenie w kierunku ruchu dochodzi do 0,75 m. Krótki skok nie zmienia wysokości kadru — mieści się w pionowym obszarze swobodnego ruchu ±0,65 m. Nowe podłoże i dłuższe spadanie przesuwają kadr. Ograniczenie opóźnienia pionowego utrzymuje Ariego w obrazie również przy długim spadaniu. Przejście na poziomy kąt kamery i zmiana dystansu są płynne; teleport i wczytanie od razu ustawiają właściwe ujęcie.

## Edycja w Godocie

W `scenes/flight_lab.tscn` wybierz korzeń **FlightLab → Camera Tuning**. Zapisane ustawienia zasobu nadpisują wartości domyślne z `scripts/flight_camera_tuning.gd`. Można zapisać zasób jako `.tres` i współdzielić go pomiędzy mapami albo użyć osobnego profilu mapy.

- **Flight**: dotychczasowy dystans, FOV, kąt i wyprzedzenie lotu.
- **Platform approach**: promień początku/końca zbliżenia, dystans przy lądowaniu i progi prędkości. Utrzymuj `Approach Start Distance > Approach Close Distance` oraz `Approach Fast Speed > Approach Slow Speed`.
- **On foot**: dystans, poziome wyprzedzenie, wysokość celu, obszar skoku, szybkość śledzenia i kąt.
- **Transitions → Framing Speed**: tempo wygładzania dystansu i kąta.

### Strojenie pieszej kamery na żywo

1. Uruchom **F5**, wybierz Kontynuuj / Nowa gra i wysiądź z auta. Zamknij dialog, mapę oraz opcje — te widoki pauzują mapę, więc kamera nie odświeża w nich parametrów chodzenia.
2. W panelu **Scene** edytora przełącz **Local → Remote**. Rozwiń **root → Game → FlightLab** i wybierz **FlightLab**. To instancja aktualnej mapy utworzona po wyborze w menu.
3. W Inspectorze rozwiń **Camera Tuning → On foot**. **On Foot Distance** zmienia zbliżenie (mniej = bliżej), **On Foot Target Height** podnosi cel kadru (więcej = Ari niżej na ekranie), **On Foot Angle Degrees** zmienia pochylenie widoku. Pozostałe pola sterują wyprzedzeniem, tolerancją skoku i szybkością śledzenia. Zmiany działają w trakcie gry.
4. Zapisz wybrane liczby. Zmiany **Remote** dotyczą tylko uruchomionej instancji i znikną po zatrzymaniu gry. Po **F8** otwórz `scenes/flight_lab.tscn`, wybierz **Local → FlightLab → Camera Tuning**, przepisz wartości i zapisz scenę (**Ctrl+S / Cmd+S**). Zasób można też zapisać przez jego menu jako osobny `.tres`.

Edytuj **Camera Tuning**, nie transformację węzła `Camera3D`: skrypt aktualizuje transformację kamery w każdej klatce.

Nowa platforma korzysta z istniejącego kontraktu miasta: `StaticBody3D` w grupie `refuel_pad`, z aktywnym dzieckiem `Collision` typu `CollisionShape3D` i poziomym `BoxShape3D`. Grupa służy rejestracji lądowisk; dostępność płatnego paliwa nadal wynika z danych usług. Specjalny taras floty ma grupę `vehicle_test_platform`. Kamera czyta transformację i rozmiar kolizji, uwzględnia też dodanie/usunięcie platformy w działającej mapie. Nie ma listy współrzędnych platform w kodzie kamery. Po zmianie źródłowej sceny miasta wykonaj zwykłą kompilację `tools/compile_city.gd`.

`PlatformCameraFraming` wyłącznie oblicza wpływ bliskości platformy. `flight_lab.gd` ustawia ujęcie aktualnego `PlayerSession.focus`. `GraphicsWarmup` przygotowuje miasto szerokim kadrem lotu i przywraca właściwy kadr gracza przed oddaniem sterowania. Kamera nie zmienia fizyki, sterowania, kursów ani zapisu gry.

## Sprawdzenie

```bash
bash tools/verify.sh platform-camera
bash tools/verify.sh platform-camera-render
bash tools/verify.sh on-foot-render
bash tools/verify.sh presentation 120
bash tools/verify.sh boot
```

Testy graficzne uruchamiaj kolejno, z widocznym oknem Godota. Pozostałe procesy Godota mogą odebrać oknu fokus; zwykła gra celowo blokuje wtedy sterowanie. Próby używają osobnych zapisów w `build/`.

Weryfikacja lokalna na macOS / Godot 4.7.2: **144/144 kontroli** — `platform-camera` 15/15, `platform-camera-render` 15/15, `on-foot-render` 15/15, `on-foot` 46/46, `architecture` 41/41, `presentation` 4/4, `boot` 8/8. Próba jednostajnego lotu: 10,5 m/s, odchylenie kroku obrazu 0,00021 px, maksimum 0,00151 px przy renderowaniu 120 Hz i fizyce 60 Hz. Jest to kontrola płynności śledzenia, nie benchmark telefonu. Test lotu czeka na pełne przygotowanie populacji przed zadaniem ciągu i podaje go przez cały czas próby.

Import izolowanej kopii bez cache: **PASS, 0 błędów**, log `build/clean-import-679g1g3u/output.log`. Nowe kadrowanie wymaga jeszcze oceny na fizycznym telefonie i Windows. Nie wykonywano eksportu ani pakowania.

Obrazy podejścia: `build/platform-camera-flight.png`, `build/platform-camera-approach.png`, `build/platform-camera-landed.png`. Pieszo: `build/on-foot-depot.png`, `build/on-foot-walking.png`, `build/on-foot-jump.png`, `build/on-foot-restored.png`.
