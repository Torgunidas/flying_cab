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

Wejście wymaga stania na podłożu blisko wolnego punktu przy aucie. Dostępne są własne auta, pojazdy bez właściciela i zaparkowane auta NPC bez kierowcy. Przycisk PRZEJMIJ / Q przerywa plan właściciela, zachowując stan tego samego auta. [Living world i zasady kradzieży](LIVING_WORLD.md). Naprawa pod E i tankowanie pod F zachowują dotychczasowe działanie podczas sterowania autem.

Skok reaguje na nowe naciśnięcie. Przytrzymanie przycisku przez całe lądowanie nie powoduje kolejnego skoku. Przy zmianie trybu i zamykaniu menu trzeba puścić wcześniej trzymane klawisze. Dotykowe palce są śledzone niezależnie; zmiana trybu usuwa ich wcześniejsze polecenia.

## Ruch i prezentacja

`scenes/people/ari.tscn` zawiera `WalkingActor`, kapsułę 1,16 jednostki wysokości i 0,17 promienia oraz osobny wygląd Ariego z żółtym pasem na kurtce. Parametry można edytować na postaci w Inspectorze:

| Parametr | Wartość robocza |
| --- | --- |
| Prędkość ruchu (lekki trucht) | 2,5 jednostki/s |
| Przyspieszenie / hamowanie na podłożu | 11,25 jednostki/s² |
| Początkowa prędkość skoku | 3,5 jednostki/s |
| Grawitacja | 12,25 m/s² |
| Sterowanie w powietrzu | 45% przyspieszenia poziomego |
| Płaszczyzna ruchu | XY, Z = 1,2, wspólna z NPC |

Przeniesiono wspólne przyciski i stany animacji ze starego Godota, a impuls skoku, ograniczone sterowanie w powietrzu i sprawdzanie wysiadania z Unreal. Tempo, przyspieszenie i wysokość skoku dopasowano do mniejszej sylwetki; prędkość względem wzrostu pozostaje energiczna. Animacje rozróżniają stanie, chód, skok, spadanie i krótkie ugięcie przy lądowaniu. Postać zachowuje kierunek patrzenia po puszczeniu kierunku.

### Synchronizacja chodu — korekta 2026-09-13

Wspólna skala człowieka jest zapisana w `HumanRig`: **1,16 jednostki wysokości** od podeszwy do czubka głowy. To około 20% więcej niż dach podstawowego cab (około 0,975 jednostki od najniższego elementu auta), zamiast wcześniejszych 1,865 jednostki i prawie dwukrotnej wysokości dachu. Jednostka pojazdu nadal wynosi 2,2 długości × 1,14 wysokości wraz z szyldem. Sprawdzono sylwetkę obok wszystkich 12 modeli; proporcje aut między sobą pozostają zgodne z biblioteką. Kapsuła Ariego i zapytanie o wolne miejsce dla wysiadającego pasażera używają tej samej anatomii.

Ari i wszystkie cztery wyglądy pasażerów mają nowy, wspólny szkielet 13 kości: miednica, tułów, głowa oraz osobne uda, łydki, stopy, ramiona i przedramiona. Tułów i kończyny są zwężane, głowa ma prostą zaokrągloną bryłę, a buty płaską podeszwę i niższy nosek. Pas kurtki Ariego jest częścią jego skórkowanej siatki i podąża za tułowiem. `tools/build_passengers.gd` zapisuje cztery sceny pasażerów oraz `ari_visual.tscn`; przebudowa jest jawna, bez generowania przy Play. Każda osoba nadal używa jednej siatki i jednego materiału, bez dodatkowych cieni ani osobnego procesu NPC. Pula pasażerów ma wspólne pozycje spoczynkowe kości dla wszystkich wariantów.

Cykl kroku zależy od rzeczywistego dystansu, również przy rozpędzaniu, hamowaniu i na ukośnych odcinkach przy drzwiach. Podczas podparcia stopa przesuwa się względem ciała przeciwnie do ruchu, z tą samą prędkością, więc pozostaje nieruchoma na tarasie. Osobne obroty uda, kolana i kostki utrzymują jednocześnie piętę i palce na podłożu. W przenoszeniu stopa unosi się, kolano zgina, a przed kontaktem podeszwa wraca do poziomu. Miednica unosi się nad nogą podporową; przy lądowaniu ugięcie kolan amortyzuje ruch bez zagłębiania butów w platformie. To animacja dla poziomych tarasów, bez dopasowania stóp do dowolnego terenu.

| Parametr w Inspectorze | Pasażerowie — zdecydowany chód | Ari — lekki trucht |
| --- | --- | --- |
| Prędkość ruchu | `TaxiRules.walking_speed`: 1,5 | `WalkingActor.walking_speed`: 2,5 |
| Dystans pełnego cyklu obu nóg, `stride_length` | 0,95 | 1,25 |
| Udział podparcia jednej nogi, `stance_fraction` | 0,56 | 0,40 |
| Uniesienie stopy, `step_height` | 0,10 | 0,16 |
| Rytm przy pełnej prędkości | około 3,2 kroku/s | 4 kroki/s |

Nowe prędkości odpowiadają wcześniejszym 2,4 i 4 jednostki/s po przeskalowaniu wzrostu: postaci nie zwolniły względem długości własnego ciała. Czas dojścia, wsiadania, wysiadania, powrotu i odejścia wynika z długości odcinka. Starsze aktywne grupy zachowują wspólne tempo zakończenia etapu, dopasowane do najdłuższego odcinka w grupie. Parametry chodu tworzą wspólnie skalibrowany profil; po zmianie długości cyklu lub podparcia należy sprawdzić kontakt stóp przez `walking` i `human-render`.

Ari koliduje z podłożem i zabudową, a przechodzi przed samochodami oraz przez innych pieszych. Walka nie jest aktywna. Samochody zachowują wzajemne kolizje i obrażenia.

Ruch po kolizjach steruje fazą i kierunkiem: przy ścianie nogi zatrzymują się, a zmiana przycisku nie obraca Ariego tyłem do trwającego hamowania. Postój łagodnie wygasza krok. Skok i spadanie mają oddzielne pozy. Teleport, wczytanie, wejście do auta i ponowne użycie pasażera z puli resetują fazę.

Jedna kamera śledzi aktualnego aktora. Aktualizacja 2026-09-14 wprowadza [podejście do platformy i kadr platformówkowy](PLATFORM_CAMERA.md): odległość 19 m w locie, płynne zbliżenie do 6 m przy lądowaniu i 4,2 m pieszo. Ari zajmuje około 15% wysokości pionowego kadru, krótkie skoki mieszczą się w spokojnym ujęciu, a kamera pokazuje więcej miejsca przed biegnącą postacią. Ustawienia są w `FlightCameraTuning`. HUD pieszy pokazuje Ariego i portfel, ukrywa paliwo, stan auta i pasażerów. Nie dodano minimapy ani stałego panelu instrukcji do domyślnej gry.

## Przejmowanie sterowania i zapis

`OnFootInteraction` wykonuje zapytania fizyczne i przekazuje sterowanie przez `PlayerSession`. Jeden węzeł Ariego pozostaje w mapie przez kolejne wejścia i wyjścia; w aucie jest niewidoczny, bez aktywnej fizyki i kolizji. Auto nadal istnieje i zachowuje swój model, identyfikator, właściciela, paliwo, HP oraz pasażerów.

Wyjście wymaga kontaktu auta z podłożem, prędkości nie większej niż 0,25 m/s, neutralnego polecenia i wygaszonego ciągu. Autopilot i reset wykluczają interakcję. Po obu stronach bryły kolizji danego modelu sprawdzane są podłoże statyczne, nachylenie i pełna kapsuła postaci. Gdy jedna strona jest zasłonięta, używana jest druga; gdy obie są zablokowane, gracz pozostaje w aucie. Modele mogą nadpisać boczne pozycje markerami `Visual/EntryLeft` i `Visual/EntryRight`; domyślne pozycje wynikają z indywidualnej bryły kolizji. Nie ma generatora uruchamianego przy Play.

Wejście sprawdza odległość do dostępnego punktu (1,35 m), różnicę wysokości, wolnego kierowcę i przeszkody między postacią a punktem. Wskazówka i wykonana akcja używają tego samego pojazdu; jego dostępność jest ponownie sprawdzana przed przekazaniem sterowania.

`PlayerState` jest niezależny od węzła postaci i auta. Schemat zapisu 4 przechowuje dodatkowo living world; dane pieszego wprowadzone w schemacie 3 obejmują tryb, mapę, auto, pozycję, prędkość, kierunek patrzenia i miejsce ostatniego wysiadania. Schematy 1 i 2 nadal są wczytywane jako jazda autem; zapis 3 migruje stare położenie Ariego z Z = 0 na wspólny pas pieszych Z = 1,2. Walidacja danych Ariego odbywa się przed zmianą sesji. Autosave i zapis w opcjach działają również pieszo. Testy używają osobnych plików w `build/`, aby nie zastępować zapisu użytkownika.

Przygotowanie grafiki renderuje także Ariego i odwiedza pełną trasę próbek miasta również po wczytaniu zapisu pieszego. Przywraca potem jego pozycję, widoczność, kamerę i stan zaparkowanego auta.

## Granice tego etapu

Nie dodano jeszcze wnętrz, drzwi, interakcji dialogowych, obrażeń Ariego, konsekwencji śmierci ani wyskakiwania z lecącego auta. Dotychczasowy `MapRouter` pozostaje podstawą przyszłych pomieszczeń. R / opcja powrotu przenosi Ariego do wolnego miejsca ostatniego wysiadania bez zmiany kredytów i stanu auta; to pomoc prototypowa, nie ustalony system zdrowia lub ratunku. Przekroczenie dolnego marginesu świata również próbuje tego powrotu.

## Weryfikacja

Korekta skali, modeli i chodu, **macOS / Godot 4.7.2 / Apple M5: 476/476 kontroli**:

| Zestaw | Wynik |
| --- | --- |
| Wszystkie sylwetki, proporcja do cab, wspólny szkielet, dystans/FPS, płaska podeszwa, brak ślizgania, ugięcie kolan, lądowanie, ściana, zwrot, pula i dojścia | 85/85 |
| Tryb pieszy, zgodność kapsuły, zapis i wejście/wyjście z 12 modeli | 46/46 |
| Transakcje taxi i zapis kursów | 39/39 |
| Wsiadanie/wysiadanie, przeszkody, prędkość i kierunek na obu stronach auta | 55/55 |
| Populacja i kursy przez wszystkie 25 platform | 61/61 |
| Wymiary, definicje, efekty i fizyka 12 pojazdów | 178/178 |
| Tryb pieszy z rendererem, chód, hamowanie, skok, kamera i pełny start z zapisu | 12/12 |

Polecenia: `bash tools/verify.sh walking`, `on-foot`, `taxi`, `taxi-interaction`, `taxi-city`, `models`, `on-foot-render`. Osobny `human-render` renderuje porównanie z 12 autami, pięć wyglądów ludzi i 16 faz chodu/truchtu; obrazy: `build/human-vehicle-scale.png`, `build/human-gait-poses.png`. Obejrzano także Ariego w rzeczywistym mieście (`build/on-foot-depot.png`, `build/on-foot-walking.png`). Kontrola stóp mierzy piętę i palce oraz pozycję w świecie w dwóch pełnych cyklach przy 240 Hz, nie tylko ruch samej kości uda.

Czysty import bez `.godot` sprawdzono osobno. Headless na tym Macu zgłasza ostrzeżenie środowiskowe `get_system_ca_certificates`; nie występuje w sprawdzeniu czystego importu ani w teście z rendererem. Windows i fizyczny telefon wymagają osobnej oceny tej korekty. Nie wykonano eksportu ani paczki itch.io.

Poniżej wcześniejsze wyniki wdrożenia trybu pieszego, sprzed korekty chodu:

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
