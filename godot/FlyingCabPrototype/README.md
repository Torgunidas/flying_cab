# Flying Cab — City 02

**Kamera 2026-09-14:** zbliżenie zaczyna się przy zwalnianiu i podejściu do tarasu: 19 m w locie → 6 m przy lądowaniu → 4,2 m pieszo. Tryb pieszy ma bliski, poziomy kadr platformówkowy i spokojne ujęcie krótkich skoków. [Parametry w Inspectorze i weryfikacja](docs/PLATFORM_CAMERA.md).

**Ekran startowy i reset 2026-09-14:** F5 pokazuje plakat ze starego Godota oraz **Kontynuuj / Nowa gra**. Nowa gra resetuje cały postęp i od razu zastępuje autosave. Dostępna również pod **zębatką → Nowa gra → Potwierdź nową grę**. [Obsługa, grafika i testy](docs/START_SCREEN.md). Scena menu do edycji: `scenes/ui/start_screen.tscn`.

**Dialogi i questy 2026-09-14:** zasoby `.tres` do edycji w Inspectorze, walidator katalogu, podgląd F6 i pionowe okno rozmowy. Maya czeka w Depot, Froggy na Torque / Foundry03. Wysiądź, podejdź i wybierz Q / ROZMOWA. Przykłady obejmują zakup i dostawę leku, odliczanie kampanii oraz poboczne zadanie z oddaniem nagrody. Dziennik: J / ≡. [Instrukcja tworzenia własnych zadań i dialogów](docs/NARRATIVE_AUTHORING.md). Stan narracji należy do schematu 5; wcześniejsze zapisy 1–4 są obsługiwane. Jest to pierwszy wycinek, nie pełna kampania.

**Platforma do prób pojazdów:** na górze Foundry, tuż pod Crosstown, znajduje się 53-metrowy taras **FOUNDRY / TEST FLEET**. Na zachodnim końcu jest miejsce LAND HERE; dalej stoi komplet 12 modeli, z trzema kolorami normal car i starszym shuttle. Wyląduj, wysiądź i podejdź do wybranego auta — Q / WSIĄDŹ. Działa także po wczytaniu istniejącej gry. [Położenie i edycja](docs/CITY_02.md#platforma-testowa-foundry--2026-09-13).

**Living world 2026-09-13:** 38 aut na pętlach, czterech mieszkańców z pełnymi podróżami, osiem zaparkowanych aut do przejęcia i spacerowicze na wszystkich 25 platformach. Ari chodzi na tym samym planie co NPC i przechodzi przed autami bez kolizji; Q pozwala przejąć pusty pojazd. Stan populacji i kradzieży zapisuje się w sesji. [Zakres, Inspector, ograniczenia i weryfikacja](docs/LIVING_WORLD.md).

**Dymki i granice — korekta 2026-09-13:** dymki śledzą renderowaną pozycję auta w każdej klatce. Boczne granice odsunięto o 25 m; od skraju autostrady do wymuszonego powrotu jest 28 m. Wiszące tablice **AIRSPACE CONTROL / CITY PERIMETER / TURN BACK** ostrzegają 20 m przed granicą. [Układ, edycja i weryfikacja](docs/CITY_02.md#granice-i-tablice-ostrzegawcze--2026-09-13).

**Skala i chód postaci — korekta 2026-09-13:** ludzie mają 1,16 jednostki wzrostu, około 20% więcej niż dach bazowego cab. Nowe modele mają osobne kolana, kostki i łokcie; stopa stoi płasko i pozostaje w miejscu podczas podparcia. Pasażerowie idą zdecydowanie, Ari lekko truchta, a rytm wynika z pokonanego dystansu. Zaktualizowano również kolizję postaci. [Parametry, porównanie z 12 autami i lokalna weryfikacja 476/476](docs/ON_FOOT.md#synchronizacja-chodu--korekta-2026-09-13).

**Tryb pieszy Ariego 2026-09-13:** zaparkuj i naciśnij **Q**, chodź A/D, skacz W/spacją, wróć do auta przez Q. Te same działania mają przyciski dotykowe. Kamera przybliża postać; zapis zachowuje tryb pieszy, pozycję Ariego i stan zostawionego auta. W razie utknięcia pieszo: R albo opcja powrotu do miejsca wysiadania. [Zakres, parametry i weryfikacja Windows 603/603](docs/ON_FOOT.md). Wnętrza, drzwi i obrażenia postaci pozostają dalszym etapem.

**Biblioteka pojazdów 2026-09-13:** ukończono dziesięć nowych wariantów 3D, wspólne definicje parametrów, odporność na obrażenia, odtwarzanie wyglądu z zapisu, okresowe lampy policji i model lawety z pustym punktem ładunku. Razem 12 pozycji z cab i starszym shuttle. Otwórz `scenes/vehicle_showroom.tscn` → F6 lub na Windows uruchom `Open Vehicle Library.ps1`. [Modele, parametry i wyniki 651/651 kontroli Windows](docs/VEHICLE_LIBRARY.md). Bibliotekę wykorzystuje już living world; laweta nie ma jeszcze mechaniki przewozu.

**Pierwsza pętla rozgrywki jest wdrożona:** ludzkie postaci 3D, 25 przystanków, automatyczne kursy, jeden oczekujący na platformie, mapa na żądanie, jednorazowe wypłaty, płatne paliwo i zapis sesji. [Instrukcja, architektura i wyniki](docs/FIRST_GAMELOOP_IMPLEMENTATION.md); [wcześniejszy przegląd Godota i Unreal](docs/FIRST_GAMELOOP_REVIEW_2026-09-13.md). Traffic i niezależne podróże mieszkańców są opisane w aktualizacji living world powyżej.

**Architektura i wydajność:** [wdrożenie po audycie](docs/ARCHITECTURE.md) — przygotowanie grafiki przed lotem, ograniczenie kosztu cieni i detali, profil Web oraz moduły sesji, sterowania pieszo/autem, map, dialogów i napraw. [Naprawa eksportu z 2026-09-13](docs/WEB_GEOMETRY_FIX_2026-09-13.md) usuwa ukośne płaszczyzny wynikające z niezapisanych transformacji detali. Poprawiony ZIP zbudowano i sprawdzono w przeglądarce; wymaga podmiany na itch.io i testu na S25+. Dokument naprawy zawiera zaktualizowane pomiary wydajności kompletnej sceny.

Pierwszy nowy prototyp **Godot 4.7.2 / GDScript / 2,5D**, utworzony od podstaw 2026-09-12. Obecny etap **City 02**: miasto 200 × 368 m, cztery dzielnice, 25 tarasów lądowiskowych na budynkach sięgających ziemi, krzyż i obwodnica tras szybkiego ruchu, różne pojazdy i ruch NPC, miękki pułap, kontrola granic, paliwo i tankowanie, obrażenia od kolizji oraz dwa płatne warsztaty. Pokaz obejmuje też pasażerów i pętlę zarabiania. To pierwszy krok do [wizji gry](../../docs/GAME_VISION.md). Działają też niezależne podróże mieszkańców i przejmowanie zaparkowanych aut. Questy i timer mają pierwszy grywalny wycinek opisany powyżej; pełna kampania i walka pozostają dalszymi etapami.

Szczegóły nowego poziomu, oświetlenia, premii autostradowej i edycji: **[City 02](docs/CITY_02.md)**. Góra jest dostępna od początku próby; mapa pod osobną ikonką pokazuje dzielnice, autostrady, lądowiska i pas smogu. Ari startuje na tarasie wieżowca na Y = 54 m, czyli 102 m nad pogłębionym dnem miasta. Poniżej dolnej autostrady znajduje się LowLife: zdegradowane podmiasto pogłębione o 48 m. Mgła i automatyczne reflektory zaczynają się niżej niż trasa. Oznaczenia wszystkich autostrad pozostają nieruchome. Przy rzeczywiście zwiększonej prędkości auto zostawia krótkie turkusowe smugi, a jego płomienie wydłużają się i jaśnieją. Cztery dopalacze płynnie skręcają do 16° względem karoserii, przeciwnie do kierunku ciągu, i wracają do pionu po puszczeniu sterowania.

**Czytelność pieszych i aut:** [propozycja wizualna trzech planów](docs/INTERACTION_LAYER_VISUAL_PROPOSAL.md). To kierunek do oceny, bez zmiany stylu w bieżącej paczce.

**Lokalne naprowadzanie podczas kursu:** po wejściu do dzielnicy celu nad autem pojawia się pomarańczowa strzałka, wzorowana na wskaźniku Unreal. Wskazuje bezpośrednio docelową platformę, także poza kadrem. Znika po opuszczeniu tej dzielnicy, rozpoczęciu wysiadania, anulowaniu lub zakończeniu kursu; jest też ukryta w mapie i opcjach. Depot jest częścią Velvet. To ogólny kierunek, bez pathfindingu, dystansu i prowadzenia między budynkami. Test: `bash tools/verify.sh taxi-guidance`.

## Pojazdy i naprawa

Wybierz `Cab` w scenie i rozwiń **Definition** w Inspectorze. Zmienisz tam ciąg pionowy i poziomy (N), masę (kg), wytrzymałość (HP), zbiornik, paliwo początkowe, zużycie paliwa i liczbę pasażerów. Przyspieszenie wynika z ciągu podzielonego przez masę; ustawienia bazowego auta zachowują dawny lot. Nowy model powstaje przez skopiowanie zasobu `.tres`, nadanie `model_id` i dodanie go do `resources/vehicle_catalog.tres`. Przykład cięższego modelu: `scenes/vehicles/heavy_shuttle.tscn`; korzysta jeszcze z tej samej grafiki auta. [Pełna instrukcja i architektura pojazdów](docs/VEHICLES_AND_REPAIR.md).

Warsztaty są w **depocie Ariego** i **Foundry / TORQUE — REPAIR / BODY SHOP** (X=22, Y=134; około 182 m nad ziemią). Mapa oznacza serwisy plusem. Zaparkuj, puść ciąg i przytrzymaj E lub mały przycisk naprawy. Koszt próbny: 1 CR/HP, szybkość 20 HP/s. Domyślna gra zaczyna się od 120 CR. Paliwo kupisz w tych samych dwóch miejscach za 1 CR/jednostkę (F lub dotyk). Silne uderzenia uszkadzają auto; 0 HP wyłącza napęd. HOLUJ / R kosztuje 35 CR i przywraca awaryjne zasoby; przy pustym portfelu powstaje dług. Szczegóły: [pętla przewozów](docs/FIRST_GAMELOOP_IMPLEMENTATION.md).

## Uruchomienie na tym Macu

**Windows:** `Open Editor.ps1` otwiera edytor, `Run Flight.ps1` grę, `Open Vehicle Library.ps1` galerię. Lokalną ścieżkę do Godota 4.7.2 można zapisać w ignorowanym `build/godot-path.txt`; zmienna `FC_GODOT_BIN` ma pierwszeństwo. Bez tych ustawień narzędzia sprawdzają `build/tools/godot-4.7.2/` i PATH. [Szczegóły](docs/VEHICLE_LIBRARY.md#oglądanie-i-uruchamianie). Projekt i sceny pozostają wspólne z macOS.

Otwórz **[Run Flight.command](Run%20Flight.command)** — uruchamia grę bez edytora. **[Open Editor.command](Open%20Editor.command)** otwiera projekt do edycji. Stabilny Godot jest przygotowany lokalnie w ignorowanym `build/tools/Godot.app`.

Okno gry na komputerze automatycznie zajmuje około 98% dostępnej wysokości ekranu razem z paskiem tytułu, zachowuje proporcje obrazu 9:16 i jest wyśrodkowane. Uwzględnia rozdzielczość Retina i rozmiar obramowania; interfejs skaluje się razem z obrazem. Podglądem osadzonym w edytorze nadal zarządza edytor — do dużego osobnego okna użyj `Run Flight.command`. Testy skryptowe zachowują wskazany rozmiar okna. Przy ręcznym uruchomieniu z `--resolution` dodaj `-- --keep-window-size`, aby pominąć automatyczne dopasowanie. Automatyczne dopasowanie okna nie dotyczy telefonu ani przeglądarki.

Na innym komputerze zainstaluj Godot 4.7.2, zaimportuj [project.godot](project.godot) i naciśnij **F5**. Scena główna `scenes/game.tscn` przygotowuje grafikę za ekranem startowym, a następnie udostępnia poziom. **F6** w `scenes/flight_lab.tscn` uruchamia dawną próbę samego lotu: bez kursów, z 250 CR, darmowym paliwem i pełnym resetem; pomija też przygotowanie grafiki. Projekt jest wspólny dla macOS i Windows; skrypty `.command` służą tylko wygodnemu uruchamianiu na Macu. Binariów silnika i katalogu `build/` nie przesyłamy przez Git.

## Sterowanie

| Działanie | Telefon / ekran dotykowy | Klawiatura |
| --- | --- | --- |
| Ciąg w lewo | Przytrzymaj lewą strzałkę | A lub ← |
| Ciąg w prawo | Przytrzymaj prawą strzałkę | D lub → |
| Ciąg w górę | Przytrzymaj prawy przycisk ↑ | W, ↑ lub Spacja |
| Opadanie | Puść ciąg w górę | Puść W / ↑ / Spację |
| Odbiór / dowóz | Automatycznie po zaparkowaniu i puszczeniu ciągu | Tak samo |
| Mapa miasta (pauza) | Ikonka mapy | M |
| Opcje (pauza) | Zębatka | Esc |
| Anulowanie kursu | Zębatka → Anuluj kurs | Esc, następnie wybierz akcję |
| Tankowanie po zaparkowaniu na stacji | Przytrzymaj panel paliwa | Przytrzymaj F |
| Płatny powrót do depotu | Zębatka → Holuj → Potwierdź | R dwukrotnie |
| Zapis sesji | Zębatka → Zapisz grę | F5 |
| Naprawa po zaparkowaniu w warsztacie | Przytrzymaj panel naprawy | Przytrzymaj E |
| Wejście / wyjście Ariego przy zaparkowanym aucie | Przycisk WSIĄDŹ / WYSIĄDŹ | Q |
| Chodzenie / skok pieszo | Strzałki / przycisk SKOK | A/D lub ←/→; W, ↑ lub Spacja |
| Pieszo: powrót do miejsca wysiadania | Zębatka → Wróć do miejsca wysiadania | R |

Zachowano pomysł ze starego Godota: dwa kierunki pod lewym kciukiem i osobny ciąg pod prawym, bez joysticka. Można trzymać kierunek i ciąg jednocześnie oraz przesuwać palec między strzałkami. Puszczenie jednego palca nie zwalnia pozostałych przycisków. Dotyk i klawiatura mają osobne źródła stanu; zmiana okna czyści sterowanie. Po resecie trzymane klawisze trzeba puścić i nacisnąć ponownie.

Próba nowej warstwy: opuść prawą krawędź depotu i puść ciąg pionowy. Przelecisz przez czystą dolną autostradę (około 59–69 m nad ziemią), a potem zjedziesz w smog. Przy Y ≤ −6 m włączą się reflektory; po wzniesieniu do Y ≥ 0 m zgasną. Na wysokościomierzu to w przybliżeniu 42 i 48 m nad nowym dnem. Nie wymagają nowego przycisku. Reset przywraca auto na podwyższony depot.

Próba lotu: wzleć nad start, przytrzymaj kierunek, puść go i poczuj wytracanie prędkości. Puść ciąg w górę i wróć na platformę. Potem spróbuj dotrzeć do lądowiska po lewej i wyżej. Holowanie jest zawsze dostępne przy sterowaniu autem, także bez pieniędzy. Boczne granice uruchamiają autopilota; wysokość ogranicza miękki pułap. Po lądowaniu na stacji puść ciąg i przytrzymaj F / panel paliwa, aby zatankować.

**Reakcja silników — 2026-09-13:** ciąg w górę, w lewo i w prawo narasta od zera do pełnej mocy w **0,18 s**, a po puszczeniu wygasa w **0,12 s**. Krótkie tapnięcie daje mniejszy impuls przy podejściu do lądowania. Zmiana lewo–prawo najpierw wygasza poprzedni kierunek, potem buduje przeciwny. Tłumienie dryfu płynnie rośnie wraz ze spadkiem mocy, a dysze i spalanie odzwierciedlają faktyczny ciąg. Pełne przyspieszenia i limity prędkości pozostają jak w tabeli niżej. Parametry **Thrust Rise Seconds / Thrust Release Seconds** są w **Cab → Definition → Engine response**; ustawienie 0 wyłącza daną rampę. Klawiatura i dotyk korzystają z tej samej fizyki 60 Hz. Test: `bash tools/verify.sh thrust`.

Weryfikacja tej zmiany na macOS / Godot 4.7.2: **293/293** kontroli (`thrust` 28, lot 23, przestrzeń/paliwo 43, pojazdy 45, postój 16, kurs 6, architektura 41, interakcje pasażerów 52, UI 21, efekty 18). Zestaw reakcji silników przeszedł również z rendererem; 100 ms tap zmniejsza opadanie, a rzeczywiste lądowanie kończy się postojem, 100 HP i zerowym spalaniem. Import bez cache: PASS. Eksport **`build/city03-c0f404213277-itch.zip`** przeszedł kontrolę geometrii PCK i uruchomienie w lokalnym Chromium/WebGL2 bez błędów. Nie opublikowano go na itch.io; odczucie lotu na S25+ i Windows wymaga sprawdzenia na tych urządzeniach.

## Co pochodzi z Unreal

Wartości podstawowej taksówki z `unreal/FlyingCabFlightLab/Source/FlyingCabFlightLab/FlyingCabPawn.h` i reguły z `FlyingCabPawn.cpp` przeliczono z centymetrów na metry. Grawitacja 9,8 m/s² odpowiada domyślnej grawitacji UE; odczytana konfiguracja projektu jej nie nadpisuje. To adaptacja modelu do Godot Physics, nie deklaracja identyczności obu solverów.

| Parametr | Wartość prototypu |
| --- | --- |
| Przyspieszenie poziome / pionowe | 14 / 23,5 m/s² |
| Maksymalny lot poziomy | 10,5 m/s |
| Maksymalne wznoszenie / opadanie | 11,5 / 13 m/s |
| Tłumienie po puszczeniu kierunku / wznoszenia | 1,7 / 1,5 |
| Bazowe tłumienie liniowe | 0,05 |
| Masa | 100 kg |
| Przechył wizualny | Do 12°, zależnie od przyspieszenia |
| Wyprzedzenie kamery | 3,8 m poziomo / 1,6 m pionowo |

Pojazd jest bryłą fizyczną `RigidBody3D`; ruch w głąb i obroty kolidera są zablokowane. Przechyla się tylko model. Kamera patrzy z boku pod kątem 5°, z perspektywą 55°. Odległość 19 m przyjęto z wcześniejszego zatwierdzonego kadru opisanego w `FLIGHT_FEEL_TEST.md`; aktualny domyślny kod kamery Unreal ma już 32 m. W pierwszym pokazie na telefonie wybrano bliższy kadr dla czytelności pojazdu.

W Godocie integracja działa ze stałym krokiem 60 Hz, z limitami prędkości po integracji. UE rozdziela `Tick` i fizykę Chaos, więc kolejność pracy solvera i reakcje na kontakt mogą dawać różnice. Tarcie 0,7 i odbicie 0,08 są ustawieniami pierwszego prototypu, do oceny podczas lądowania.

Dodano paliwo, płatne tankowanie i uszkodzenia. Pełny darmowy reset zachowano wyłącznie w próbie lotu uruchamianej przez F6; domyślna gra korzysta z holowania. Dźwięk pozostaje dalszym etapem. Nie przenoszono ze starego Godota chwilowego zerowania prędkości „hover” ani boostu z podłoża, których nie ma w podstawowym modelu lotu Unreal.

## Granice miasta i paliwo

Boczne ściany i twardy sufit zostały usunięte. Pozostały znaczniki granicy; poza nią auto może rzeczywiście lecieć. Po przekroczeniu granicy wyświetla się **„Non authorized out of grid movement. Forced return”**, a autopilot przejmuje ciąg i wraca zwykłą fizyką pojazdu do punktu około 6 m wewnątrz miasta. Najpierw wyhamowuje ruch na zewnątrz. Przy niskim locie podnosi auto do bezpiecznej wysokości, a przy bardzo wysokim sprowadza je poniżej strefy pułapu. Działa w wolnych bocznych korytarzach aktualnej sceny; po zmianie zabudowy trzeba zweryfikować trasę powrotu.

Po uspokojeniu prędkości autopilot oddaje sterowanie i pokazuje **„Pilot control restored”**. Klawisz lub palec trzymany podczas powrotu trzeba puścić i nacisnąć ponownie. Reset działa także podczas powrotu. Zwykłe przekroczenie granicy nie teleportuje auta; automatyczny reset zabezpiecza jedynie awarię fizyki prowadzącą do spadnięcia pod poziom.

Soft ceiling stopniowo tłumi wznoszenie w górnych 10 m dostępnej przestrzeni. **„Max alt reached”** ostrzega w tej strefie. Adaptacja `_apply_soft_ceiling` z `archive/godot/scripts/car.gd` zachowuje stopniowe wyhamowanie, a dodatkowy malejący limit prędkości zapobiega pełzaniu coraz wyżej przy ciągle trzymanym ciągu. Puszczenie ciągu pozwala opaść. Archiwum nie zmieniano.

Poza autostradami obowiązują poniższe bazowe koszty. Pasy szybkiego ruchu płynnie podnoszą limity prędkości do ×1,5 i obniżają spalanie do ×0,5 ([szczegóły](docs/CITY_02.md)).

Bak ma 100 jednostek testowych. Ciąg pionowy spala 1,4 jednostki/s, poziomy 0,7 jednostki/s; przy locie po skosie koszty się sumują. Próba wznoszenia w strefie pułapu stopniowo zwiększa koszt pionowego ciągu do 3×. Samo przebywanie wysoko z puszczonym ciągiem nie spala paliwa. Pusty bak odcina ciąg pilota, zachowując bezwładność i grawitację.

**Historyczny tryb testu lotu (F6):** każde z 25 lądowisk jest oznaczone w scenie grupą `refuel_pad`. Po ustabilizowaniu auta i puszczeniu sterowania tankuje z szybkością 25 jednostek/s, do pełnego baku. HUD pokazuje **„REFUELING”**. Zwykłe podłoże poza lądowiskami nie tankuje. Darmowe tankowanie, pełny bak po resecie i wartości spalania są ustawieniami tego testu, do oceny przez autora.

Wymuszony powrót korzysta z dostępnego paliwa, a po jego wyczerpaniu ma awaryjną rezerwę działającą tylko do oddania sterowania. Dzięki temu pusty bak nie pozostawia auta poza miastem. Nie dodaje ona paliwa do głównego zbiornika. To reguła zabezpieczająca bieżący prototyp; balans i uzasadnienie rezerwy w kampanii pozostają do ustalenia.

Parametry paliwa są w [resources/vehicles/basic_cab.tres](resources/vehicles/basic_cab.tres), a przestrzeni miasta w [resources/city_02.tres](resources/city_02.tres):

| Parametr | Wartość testowa |
| --- | --- |
| Lewa / prawa granica X | −99,5 / 100,5 m |
| Miękkie hamowanie / maksymalna wysokość Y | 310–320 / 320 m |
| Punkt powrotu | 6 m wewnątrz granicy |
| Docelowa prędkość powrotu | Do 5 m/s |
| Ziemia Y | −48 m |
| Wysokość punktu powrotu Y | −38…307 m, co najmniej 10 m nad ziemią |
| Bak / tankowanie | 100 jednostek / 15 jednostek/s w pętli; 25 jednostek/s w próbie F6 |
| Spalanie pionowe / poziome | 1,4 / 0,7 jednostki/s |
| Maksymalny mnożnik kosztu ciągu pionowego przy pułapie | 3× |

Pułap i progi smogu odnoszą się do współrzędnej świata Y; `return_min_height` oznacza prześwit nad ziemią. Wysokościomierz pokazuje wysokość spodu auta nad nowym dnem (Y − ground_height − 0,35 m): na depocie około 102 m. Zabezpieczenie przed wypadnięciem z poziomu działa dopiero 20 m poniżej dna. Progi należy przesuwać wraz z rozwojem dostępnych stref miasta.

Weryfikacja macOS / Godot 4.7.2: **24/24** kontroli smogu i automatycznych reflektorów (`bash tools/verify.sh smog`), **18/18** kontroli dysz i efektu express (`bash tools/verify.sh fx`), **7/7** porównań renderowanych klatek pasów (`bash tools/verify.sh lanes`), **48/48** kontroli miasta i autostrad (`bash tools/verify.sh city`), **41/41** kontroli granic i paliwa (`bash tools/verify.sh airspace`), **23/23** lotu, **16/16** postoju i **4/4** prezentacji przy kroku 120 FPS / fizyce 60 Hz. Testowano obie strony, pusty bak nisko i wysoko, brak teleportacji, przejęcie i oddanie sterowania z trzymanymi klawiszami oraz palcem, reset autopilota, stopniowe hamowanie, aktywną fizykę przy pułapie, wyczerpanie paliwa i tankowanie. Ostrzeżenia oraz pasek paliwa sprawdzono w renderowanym oknie. To historyczne wyniki etapu lotu; aktualny eksport i testy opisuje [raport pętli przewozów](docs/FIRST_GAMELOOP_IMPLEMENTATION.md). Test na telefonie nadal wymaga wykonania.

## Edycja

- [resources/vehicles/basic_cab.tres](resources/vehicles/basic_cab.tres) — parametry lotu, autostrad i paliwa; edytowalne w Inspectorze.
- [resources/city_02.tres](resources/city_02.tres) — granice, pułap, powrót i dzielnice; wspólne dane dla pojazdu i HUD.
- [scripts/flight_camera_tuning.gd](scripts/flight_camera_tuning.gd) — zasób parametrów kamery, przypisany w Inspectorze poziomu jako `camera_tuning`.
- [scenes/cab.tscn](scenes/cab.tscn) — model pojazdu, kolider i cztery niezależne mocowania dysz.
- [scenes/cab_flight_fx.tscn](scenes/cab_flight_fx.tscn), [scripts/cab_flight_fx.gd](scripts/cab_flight_fx.gd) — smugi przyspieszenia, jasność płomieni, kąt i tempo skrętu dopalaczy; nie zmieniają fizyki ani paliwa.
- [scenes/city.tscn](scenes/city.tscn) — statyczne budynki, tarasy i trasy; otwórz tę scenę do edycji dzielnic.
- [scenes/city_runtime.tscn](scenes/city_runtime.tscn) — zoptymalizowany wynik `tools/compile_city.gd`; po edycji źródła przebuduj go [zgodnie z instrukcją](docs/ARCHITECTURE.md#praca-z-projektem-i-kolejny-pomiar).
- [scenes/flight_lab.tscn](scenes/flight_lab.tscn) — instancja miasta, oświetlenie, podłoże, pojazd i kamera.
- [scenes/game.tscn](scenes/game.tscn) — start projektu: trwała sesja, usługi, mapy, profil grafiki, przygotowanie renderowania i diagnostyka.
- [scripts/flight_controls.gd](scripts/flight_controls.gd) — rozkład przycisków, multitouch i HUD.

Geometria i ikona są nowe, bez zależności od assetów archiwum. Skrypty lotu, ustawienia i stan sterowania są oddzielone od sceny otoczenia.

## Eksport do przeglądarki

Preset `Web` używa Compatibility i jednego wątku. Oficjalne szablony 4.7.2 są przygotowane na tym Macu w ignorowanym `build/templates/`. Eksport: `bash tools/export-web.sh` — skrypt kompiluje scenę miasta, zapisuje ID źródeł, eksportuje i pakuje. Wynik ma trafić do `build/web/index.html`; powstaje `build/FlyingCabFlight01-itch.zip`, ZIP z ID wersji i manifest hashów. Przed **Project → Export → Web → Export Project** wykonaj ręcznie kompilację miasta i `python3 tools/stamp_build.py`. Szczegóły oraz status weryfikacji: [architektura](docs/ARCHITECTURE.md).

Po poprawnym eksporcie można uruchomić `python3 -m http.server 8777 --directory build/web` i otworzyć `http://localhost:8777`. Do testu na telefonie w tej samej sieci użyj lokalnego adresu IP Maca zamiast `localhost`. Na itch.io wybierz HTML Game i oznacz Mobile Friendly po sprawdzeniu sterowania na urządzeniu.

Na świeżym komputerze rozpakuj `templates/web_release.zip` i `templates/web_debug.zip` z [oficjalnych szablonów Godota](https://godotengine.org/download/archive/4.7.2-stable/) do `build/templates/`, zgodnie z presetem. Nie dodawaj ich do repozytorium.

**Aktualna paczka pętli taxi 2026-09-13 (jednoosobowe kursy na wszystkich 25 platformach):** `build/city03-166aab0fc1e7-itch.zip`; nieopublikowana na itch.io. [Zakres sprawdzenia](docs/FIRST_GAMELOOP_IMPLEMENTATION.md).

**Poprzednia paczka z pojazdami i warsztatami 2026-09-13:** `build/city03-59cf0ff42a4a-itch.zip` zastępuje wcześniejszą paczkę. Zawiera również poprawkę geometrii. Nie została opublikowana na itch.io.

**Historia naprawy eksportu 2026-09-13:** na [itch.io](https://torgerd.itch.io/newcab) odtworzono błąd ukośnych płaszczyzn w nowszej paczce autora. Przyczynę naprawiono; lokalnie zbudowano i sprawdzono Web **`city03-8a443ef67981`**. Gotowy ZIP jest w `build/city03-8a443ef67981-itch.zip`; nie przesłano go na itch.io. Pierwsze przejścia smog/Eden po przygotowaniu miały lokalnie 9/45 ms, a depot 217 wywołań rysowania zamiast bazowych 911. Wcześniejsze wyniki po optymalizacji nie obejmowały poprawnej geometrii i zostały zastąpione. [Szczegóły naprawy i testów](docs/WEB_GEOMETRY_FIX_2026-09-13.md); telefon i Safari/iOS nadal wymagają sprawdzenia.

## Weryfikacja

Po zgłoszeniu ROOM 09 → depot: **35/35** nowych kontroli interakcji (`bash tools/verify.sh taxi-interaction`), **39/39** taxi, **6/6** przelotu i świeży import. Rozdzielono ofertę, odbiór i dowóz; poprawiono postój przy krawędzi oraz dodano widoczne przyczyny blokady. [Szczegóły](docs/FIRST_GAMELOOP_IMPLEMENTATION.md#odbiór-i-wysiadanie--poprawka-2026-09-13).

Po błędzie pierwszego otwarcia projektu dodano `bash tools/verify.sh import`: import osobnej kopii bez cache oraz kontrola błędów w logu. Po poprawce: import bez błędów, taxi 39/39 i przelot 6/6 na świeżej kopii. [Opis poprawki](docs/FIRST_GAMELOOP_IMPLEMENTATION.md#poprawka-świeżego-importu--2026-09-13).

Po wdrożeniu przewozów: **309/309** kontrole funkcjonalne, dodatkowo kontrola gotowego PCK; [zakres i ograniczenia](docs/FIRST_GAMELOOP_IMPLEMENTATION.md#Weryfikacja). Kursy: `bash tools/verify.sh taxi`. Poniższe wyniki opisują wcześniejsze etapy.

Aktualne testy pojazdów i warsztatów: **45/45**, także z normalnym rendererem, przez `bash tools/verify.sh vehicles`. Wyniki pozostałych kontroli i zakres ręcznej próby Web: [raport](docs/VEHICLES_AND_REPAIR.md).

Po wdrożeniu architektury ponownie zaliczono dotychczasowe **181/181** kontroli oraz **41/41** nowych kontroli architektury i **8/8** pełnego startu z rendererem: razem **230/230**. Nowe polecenia: `bash tools/verify.sh architecture` i `bash tools/verify.sh boot`. [Dokument wdrożenia](docs/ARCHITECTURE.md) opisuje zakres testów i osobny benchmark czasu rzeczywistego. Poniższe sekcje zachowują historię wcześniejszych poprawek.

`bash tools/verify.sh` uruchamia testy w silniku. macOS / Apple M5 / Godot 4.7.2: **23/23 zaliczone** — limity i bezwładność, porównanie kroków czasowych, dwa palce, przesuwanie palca, utrata fokusu, łączenie dotyku z klawiaturą, fizyczny start, lądowanie, reset i przelot przez obie nowe granice miasta. Odczyt sceny oraz renderowanie OpenGL/Metal sprawdzono lokalnie. Testy nie zastępują oceny feelingu przez autora.

### Stabilny postój i kontrola ciągu — 2026-09-12

Podczas odtworzenia skaczącego licznika na platformie sprawdzono klawiaturę, stan wskaźników dotyku/myszy, polecenie kontrolera oraz polecenie odczytane przez integrator pojazdu. Wszystkie były zerowe. Przed poprawką bryła miała prędkość 0,139–0,315 m/s i podskakiwała o około 1–2 mm: własny integrator ponownie dodawał grawitację po rozwiązaniu kontaktu, a pojazd nie mógł przejść w spoczynek. Chwilowa utrata kontaktu powodowała wskazania 0/1 km/h mimo wcześniejszego filtra HUD.

Pojazd przechodzi teraz w fizyczny spoczynek po 0,1 s potwierdzonego podparcia na nieruchomej, poziomej powierzchni, przy prędkości poniżej 0,1 m/s i zerowym poleceniu ciągu. Ciąg, reset, zewnętrzny impuls lub utrata podparcia wybudzają bryłę. Usunięto wcześniejsze zerowanie składowej prędkości wyłącznie dla HUD — licznik korzysta bezpośrednio z prędkości fizycznej.

`bash tools/verify.sh idle`: **16/16 zaliczone** (w tym trzy kontrole zerowego ciągu i spalania podczas postoju). Pomiar obejmuje trzy dziesięciosekundowe postoje: na starcie, po rzeczywistym lądowaniu z puszczonym ciągiem oraz po resecie. Po poprawce każda z 600 próbek postoju ma zerowy input, zerową prędkość i brak przesunięcia. Dodatkowe kontrole sprawdzają start po postoju, ruch po zewnętrznym impulsie bez polecenia ciągu i spadanie po usunięciu platformy. Wyniki potwierdzono zarówno headless, jak i w oknie z rendererem Compatibility przy kroku renderowania 120 FPS i fizyce 60 Hz. Testy lotu ponownie dały **23/23**.

**Zasada paliwa:** `cab.command` to żądanie pilota, `cab.applied_command` to rzeczywiście realizowany ciąg po uwzględnieniu autopilota i dostępnego paliwa, a `cab.fuel_burn_rate` to spalanie z głównego baku. Grawitacja, ruch po impulsie i postój bez ciągu nie spalają paliwa. Wyjątek rezerwy wymuszonego powrotu opisano powyżej.

### Poprawka szarpania podczas lotu — 2026-09-12

Przy jednostajnym locie auto poruszało się ze stałą prędkością 10,5 m/s, ale kamera śledziła surową pozycję fizyczną aktualizowaną co 1/60 s. Renderowanie pomiędzy tymi krokami powodowało drgania auta względem kamery. Włączono interpolację fizyki; kamera w każdej klatce śledzi `get_global_transform_interpolated()`, z wyłączoną własną automatyczną interpolacją. Przechył modelu i dysze aktualizują się w krokach fizyki. Reset czyści historię interpolacji po teleportacji i jednocześnie ustawia kamerę. Parametry przyspieszeń, prędkości i tłumienia pozostają takie same.

Rozwiązanie odpowiada [zaleceniom Godota dla kamery z interpolacją](https://docs.godotengine.org/en/stable/tutorials/physics/interpolation/advanced_physics_interpolation.html) oraz [resetowania interpolacji po teleportacji](https://docs.godotengine.org/en/stable/tutorials/physics/interpolation/using_physics_interpolation.html).

Test `tests/presentation_tests.gd` używa prawdziwej sceny i renderera. Na potrzeby izolacji płynnego przelotu wyłącza w swoim egzemplarzu grawitację i kolizje oraz podaje ciąg bezpośrednio, niezależnie od fokusu okna. Po trzech sekundach rozpędzania mierzy siedem sekund lotu; sprawdza stałą prędkość, stabilność kadru oraz pozycję pojazdu i kamery tuż po resecie. Pozostały zestaw testuje zwykłe sterowanie i kolizje.

| Ustawienie testu | Odchylenie zmian ekranowej pozycji auta między klatkami |
| --- | --- |
| Przed poprawką, krok renderowania 120 FPS / fizyka 60 Hz | 2,33775 px |
| Po poprawce, 120 FPS / 60 Hz | 0,00016 px |
| Po poprawce, 144 FPS / 60 Hz | 0,00014 px |
| Próba diagnostyczna po poprawce, 120 FPS / fizyka tylko 10 Hz | 0,00026 px |

Wszystkie cztery kontrole prezentacji przeszły w trzech ustawieniach po poprawce, a testy lotu ponownie dały 23/23. To pomiar przy wymuszonym kroku klatek, nie benchmark wydajności ani potwierdzenie działania na telefonie. Zwykła gra nadal używa fizyki 60 Hz i domyślnej synchronizacji obrazu.

Powtórzenie na Macu: `bash tools/verify.sh presentation 120`, następnie opcjonalnie `bash tools/verify.sh presentation 144` lub diagnostycznie `bash tools/verify.sh presentation 120 10`. Test otwiera własne okno i kończy je automatycznie. Po zmianie ustawień projektu uruchom grę ponownie; już działająca instancja nie wczyta poprawki.

Godot 4.7.2 podczas zakończenia headless importu edytora zgłosił błąd zamykania wątku dokumentacji i sygnał 11. Import zasobów zakończył się; osobne uruchomienia gry, renderowania i testów działały poprawnie. Zapisujemy ten problem narzędzia, aby kod zakończenia importera nie był uznawany za samodzielny dowód poprawności projektu. Windows i urządzenia mobilne nie były testowane.

## Zewnętrzny edytor fabuły

[Story Editor 1.1](docs/STORY_EDITOR.md) — kopia autorskiego edytora z formularzami questów, warunków, rozwidleń, postaci oraz importem do Godota. Uruchomienie: Projekt → Narzędzia → Flying Cab — otwórz edytor fabuły.
