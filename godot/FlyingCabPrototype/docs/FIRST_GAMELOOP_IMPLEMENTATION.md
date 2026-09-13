# Pierwsza pętla przewozów — wdrożenie 2026-09-13

Domyślna gra (`scenes/game.tscn`, F5 / Run Flight) ma działające kursy. To wdrożenie [ustaleń przeglądu](FIRST_GAMELOOP_REVIEW_2026-09-13.md). Zgodnie z kolejną decyzją autora inne pojazdy, traffic i niezależne podróże mieszkańców są następnym etapem. Obecni ludzie to pasażerowie taxi; nie są jeszcze pełnym living world.

## Jak zagrać

1. Zaparkuj przy machającym pasażerze i puść cały ciąg. Pasażer sam podejdzie i wsiądzie. W depocie pierwszy jedzie do OCTANE. Odlot podczas wsiadania zwalnia rezerwację; człowiek wraca na taras i może ponowić wejście.
2. Cel i cenę podaje dymek. Komunikat znika. Na ekranie pozostają małe wskaźniki portfela, obsady, paliwa i stanu auta, sterowanie oraz ikonki mapy i opcji.
3. Sam zaplanuj trasę. Ikonka mapy / M otwiera osobny widok miasta z przystankami, pasażerami, serwisami, pozycją auta i celem aktywnego kursu. Po wejściu do dzielnicy celu nad autem pojawia się strzałka wskazująca wprost platformę, bez wytyczonej trasy ani dystansu. Dotknięcie przystanku na mapie pokazuje jego nazwę. Mapa zatrzymuje symulację; zamknij ją krzyżykiem, M lub Esc.
4. U celu wyląduj i puść sterowanie. Pasażer sam wysiądzie; dopiero zakończenie czynności wypłaca cenę kursu. Krótki dymek wyjaśnia przeszkodę przy tarasie, jeśli np. auto wystaje poza krawędź lub wyjście jest zajęte.
5. W depocie i TORQUE możesz przytrzymać małe przyciski paliwa i naprawy albo F / E. Przyciski pojawiają się tylko wtedy, gdy zaparkowane auto potrzebuje danego serwisu. Zwykłe przystanki nie tankują.
6. Zębatka / Esc otwiera opcje z powrotem do gry, zapisem, holowaniem i anulowaniem aktywnego kursu. Holowanie wymaga drugiego kliknięcia w tym menu; skrót R nadal wymaga dwóch naciśnięć w ciągu 4 sekund. Cena 35 CR; niedobór środków przechodzi w dług. Mapa i opcje czyszczą przytrzymane sterowanie, a po zamknięciu klawisze trzeba puścić i nacisnąć ponownie.

**Zębatka → Zapisz grę / F5** zapisuje sesję i wraca do gry, gdzie pojawia się dymek potwierdzenia. Automatyczny zapis działa co 20 sekund aktywnej gry, po rozliczeniu kursu lub holowaniu i przy utracie fokusu. Ponowne uruchomienie wczytuje ostatni poprawny zapis. Nowa próba: `-- --new-game` przy uruchomieniu natywnym lub `?new=1` w adresie Web. Kolejne zapisy nowej próby zastępują poprzednią lokalną sesję. Zapis jest lokalny dla urządzenia i domeny.

**F6 w `scenes/flight_lab.tscn`** zachowuje wcześniejszą próbę samego lotu: 250 CR, bez kursów, darmowe tankowanie na 25 padach i pełny RESET. Nie jest to domyślna pętla ekonomii.

## Zawartość i balans do oceny

| Reguła | Pierwsza wersja |
| --- | --- |
| Obsługiwane przystanki | Wszystkie 25 platform: depot oraz po 6 w Velvet, Foundry, Eden i Aurelia |
| Oferty | Do 25; najwyżej jeden oczekujący na platformie; czasy oczekiwania 112,5–187,5 s, automatyczny odbiór |
| Pasażerowie | Nowe kursy jednoosobowe; rozpoczęte grupowe kursy ze starych zapisów można dokończyć |
| Portfel na start | 120 CR; początkowe paliwo i stan z definicji auta |
| Cena kursu | `(24 CR + 0,8 CR × długość dostępnej trasy w metrach) × (1 + 0,35 × liczba dodatkowych osób)`; wycena do 0,01 CR |
| Pierwszy kurs | Depot → OCTANE: 83,36 CR przed ewentualną spłatą długu |
| Tankowanie | 1 CR/jednostkę, 15 jednostek/s, tylko rzeczywiście uzupełniona i opłacona ilość |
| Naprawa | Dotychczasowe 1 CR/HP, 20 HP/s, wspólny portfel |
| Holowanie | 35 CR; co najmniej 35 paliwa i 50% sprawności, z zachowaniem wyższych obecnych wartości |
| Brak pieniędzy na ratunek | Nieoprocentowany dług; 25% każdej następnej wypłaty spłaca go do zera |

Krążenie nie zwiększa ceny. Nie ma dodatkowego limitu czasu aktywnego przewozu ani naliczania pieniędzy w trakcie lotu. Po błędzie można kontynuować zarabianie. Obecne stawki służą ocenie pętli; czas objazdów, ryzyko kolizji i koszty napraw trzeba ponownie wyważyć po dodaniu trafficu. Nie należy jeszcze stroić ceny leku bez dostawy do siostry i działającej kampanii.

## Architektura i rozbudowa

- **`RideService`** należy do sesji. Trzyma oferty, etap podróży, grupę osób, ID konkretnego auta, wycenę, wynik i dług. Zmiana sterowanej postaci lub odtworzenie mapy nie przenosi ludzi do nowego auta.
- **`VehicleOccupancy` + `VehicleState`** rezerwują całą grupę, następnie zamieniają rezerwację na faktycznych pasażerów. Zajęte i zarezerwowane miejsca liczą się wspólnie. Ten kontrakt jest dostępny dla przyszłych podróży NPC; zmiana kierowcy nie może zostawić jego drugiego miejsca pasażera.
- **`EconomyLedger` + `CampaignState`** obsługują portfel i identyfikatory rozliczeń. Wypłata danego kursu może zajść tylko raz, także po wczytaniu. Paliwo i naprawy zachowują ułamki kwot między krokami fizyki; nie zaokrąglają kosztu każdej klatki do całych kredytów.
- **`TaxiStop`** jest dzieckiem rzeczywistego pada w edytowalnym `city.tscn`. Wsiadanie wymaga podparcia na tym padzie, spoczynku i wyłączenia ciągu. Sam przelot przez strefę nie wystarcza. Wysiadanie dodatkowo sprawdza wolne miejsce obok auta.
- **`TaxiNetwork`** buduje mały graf podejść do tarasów i centralnego korytarza, z 75 punktami dla 25 przystanków. Sprawdza bryłą prześwit z geometrią; pole `VehicleDefinition.navigation_clearance` pozwala dopasować go do większego auta. Wyniki krawędzi są pamiętane. Graf sprawdza dostępność i cenę kursu; nie przelicza prowadzenia od aktualnego położenia gracza.
- **`TaxiDirector`** łączy sesję z konkretną mapą: ruch postaci na tarasie, automatyczny odbiór, bezpieczne wejście/wyjście i dostęp do stacji. **`TaxiHud`** prezentuje dymki oraz obsługuje opcje i pauzę; osobny **`CityReferenceMap`** rysuje mapę wyłącznie po jej otwarciu.
- **`TaxiRules`** w `resources/taxi/default_rules.tres` udostępnia stawki, limity i czasy w Inspectorze. Przystanki mają stabilne ID; nowy ID trzeba także dodać do katalogu w tym zasobie, ponieważ walidator zapisu odrzuca nieznane cele.

Postaci mają cztery warianty ubioru i wyglądu, głowę, tułów oraz animowane ręce i nogi. To nowe modele 3D z jednym szkieletem i jedną siatką na osobę, wykonane jako zasoby projektu. Narzędzie `tools/build_passengers.gd` służy do jawnej przebudowy scen, nie uruchamia się podczas lotu. Stała pula 37 instancji powstaje przy otwieraniu mapy; nie ma doczytywania scen pasażerów w trakcie kursu. Ukryte osoby nie uruchamiają własnej symulacji ani animacji. Zachowują ID i wygląd przy wejściu, podróży i wyjściu. Nie dodano osobnych cieni dla każdego człowieka.

Zapis ma schemat 2, walidację przed zmianą sesji i zapis przez plik tymczasowy. Obsługuje też wcześniejszy schemat 1. Zawiera pojazdy, osoby czekające i jadące, rezerwacje, postęp wejścia/wyjścia, wybrany kurs, portfel, pokwitowania i dług. Krótkie odejście człowieka do drzwi po rozliczeniu jest prezentacją mapy i nie jest zachowywane w zapisie.

Następny traffic powinien korzystać z tej samej obsady auta, poleceń kierowcy i stanu podróży. Wtedy graf dróg trzeba rozwinąć o kierunkowe pasy, zasady pierwszeństwa i zajęte odcinki. Obecny graf sprawdza prześwit statycznej zabudowy, ale nie steruje unikaniem innych aut. Zmiana geometrii w trakcie gry wymaga unieważnienia `edge_cache`; obecna mapa jest nieruchoma. Kolejne mapy wymagają własnego katalogu przystanków i adaptera podróży, zamiast kopiowania logiki do skryptów samochodów.

## Świadome granice tej wersji

Nie ma jeszcze ruchu NPC, kradzieży, nowych modeli wizualnych aut, ogólnodostępnego chodzenia Arim, wnętrz, treści dialogów, dostaw leku ani walki. Fundamenty tych systemów nie są ich ukończoną zawartością. Kampania pozostaje wyłączona: jej globalny timer nadal ma być przedłużany **dostarczeniem dawki**, zgodnie z [wizją](../../../docs/GAME_VISION.md).

Po anulowaniu lub zniszczeniu auta pasażer jest usuwany z jego obsady i wraca do przystanku pochodzenia; nie symulujemy jeszcze ratunku osoby z powietrza. Ruch pieszy jest lokalną animacją na bezpiecznym pasie tarasu, bez ogólnego AI chodzenia po mieście. Większe grupy mają wspólne wejście/wyjście, bez otwieranych drzwi auta.

## Weryfikacja

Godot 4.7.2, macOS / Apple M5:

- **39/39** kontroli taxi, zarówno bez okna, jak i z rzeczywistym rendererem. Obejmuje 30 par tras, kolizję początkową z padem, niedostępny cel, przerwane wsiadanie i powrót człowieka, zmianę na aktora pieszego, zapis osoby na pokładzie i odtworzenie mapy, brak zapłaty za krążenie/hover, fizyczne lądowanie i wysiadanie, grupę 3 osób, rezerwacje, jednorazową wypłatę, paliwo przy 60/120 Hz, uprawnione stacje, częściowy zakup i dług za ratunek. Test transportu przenosi bryłę w pobliże celu, aby skrócić próbę; samo lądowanie wykonuje silnik fizyki.
- **256/256** dotychczasowych kontroli: architektura 41, pojazdy 45, lot 23, postój 16, granice 41, miasto 48, smog 24, efekty 18.
- **8/8** kontroli pełnego startu z grafiką i profilem balanced; przygotowanie ok. 5,0 s w tej lokalnej próbie.
- **6/6** kontroli dodatkowego ciągłego przelotu po trasie, bez przestawiania bryły: 30,63 s, 9,68 paliwa, 0 obrażeń, wypłata 83,36 CR. Testowy sterownik korzysta ze zwykłego ciągu pojazdu; nie jest to jeszcze ocena trudności ręcznego pilotażu. Polecenie: `bash tools/verify.sh taxi-route`; powtórzono też z rzeczywistym rendererem.
- Gotowy eksport PCK przechodzi kontrolę wszystkich transformacji skompilowanej geometrii. Zachowano 1226 detali w 166 grupach i 77 obiektów rzucających cień.

Łącznie **309/309** kontrole funkcjonalne, bez podwójnego liczenia powtórzenia taxi z rendererem; do tego kontrola skompilowanej geometrii. Polecenia i szczegółowe logi są w `tools/verify.sh` oraz ignorowanym `build/`.

Eksport przed poprawką świeżego importu: **`city03-597e96006744`**, paczka `build/city03-597e96006744-itch.zip`. Lokalny test WebGL2 / Chromium w przeglądarce aplikacji, okno 412 × 915: start, wybór karty, animowane wsiadanie, pasażer na pokładzie, wskazanie celu, zapis przyciskiem i ponowne otwarcie z zachowaniem osoby, ceny i celu. W konsoli brak błędów i ostrzeżeń. Po ostatniej poprawce etykiet ponownie sprawdzono finalny eksport, odczytano zgodne ID wersji w konsoli, anulowano kurs i rozwinięto trzy oferty bez błędów. Paczki nie opublikowano na itch.io.

Test Web wykrył zatrzymanie przed ekranem gry przy zwracaniu wartości bool z `JavaScriptBridge.eval` w odczycie parametru adresu. Zastąpienie wyniku ciągiem znaków i porównanie w GDScript odblokowało start; to samo zastosowano do opcji diagnostyki. Dokładnej przyczyny wewnątrz silnika/szablonu Web nie ustalono. Pozostawiono opis obejścia w kodzie.

Podczas krótkiej lokalnej próby ciągłego przelotu po przygotowaniu grafiki nie odnotowano klatek powyżej 50 ms (1836 próbek; p95 17,37 ms, maks. 17,85 ms). To ograniczony pomiar natywny przy limicie 60 FPS, nie benchmark 10 minut na telefonie. Raport `build/taxi-route-performance.json` ma ID mierzonej wersji `city03-597e96006744`. Sprawdzono zgodność plików źródłowych z hashem eksportu i manifestem plików Web; archiwum ma około 9,97 MiB. Nie wykonano pomiaru na Samsungu S25+ ani testu Windows. Wyniki lokalne nie potwierdzają jeszcze 60 FPS ani braku przycięć na telefonie; docelowa próba obejmuje minimum 10 minut, smog, kolejne kursy i powrót z tła.


## Poprawka świeżego importu — 2026-09-13

Odtworzono zgłoszony `Parse Error: Cannot get property "has" on a null object` na nowej kopii projektu bez `.godot`. Wyrażenie `preload(...).stop_ids.has(...)` odczytywało domyślne pole skryptowego zasobu podczas analizy skryptu. Przy pierwszym skanowaniu edytor widział w nim `null`; błędy kompilacji sesji i testu trasy były skutkiem tej zależności.

`RideService.from_snapshot()` wczytuje teraz typowany `TaxiRules` w czasie wykonywania, sprawdza go i pobiera `PackedStringArray` przed walidacją celów. Nie zmieniono listy przystanków ani reguł akceptacji zapisu.

Nowe polecenie **`bash tools/verify.sh import`** uruchamia `tools/check_clean_import.py`: tworzy osobną kopię źródeł w ignorowanym `build/`, bez cache, importuje ją edytorem i sprawdza log. Jest to potrzebne, ponieważ odtworzony błędny import zwrócił kod zakończenia 0. Kontrola nie usuwa pamięci ani ustawień użytkownika w roboczym projekcie. Można ją uruchomić także przez Python, podając `--engine` ze ścieżką lokalnego Godota.

Wynik po poprawce: świeży import **PASS, 0 błędów**, na tej samej kopii **39/39** testów taxi i **6/6** ciągłego przelotu. Nowa kontrola automatyczna świeżego importu również przeszła. To uzupełnienie wcześniejszych testów; poprzednie 309 kontroli nie obejmowało pierwszego skanowania edytora. Weryfikacja lokalna na macOS / Godot 4.7.2, bez nowej próby Windows ani telefonu.

Zaktualizowany eksport: **`city03-1cf17eed751b`**, `build/city03-1cf17eed751b-itch.zip`. Eksport i kontrola geometrii gotowego PCK przeszły; zgodność źródeł, manifestu i archiwum sprawdzona. Paczka zastępuje poprzednią; nie opublikowano jej na itch.io. W tej poprawce nie powtarzano ręcznej próby Web — opisane wyżej oględziny i pomiar dotyczą poprzedniej paczki.


## Historia: odbiór i wysiadanie — poprawka 2026-09-13

**Uwaga: opis kart, ręcznej akceptacji i trwałych instrukcji poniżej został zastąpiony decyzją autora o minimalnym UI i automatycznych kursach. Poprawka geometrii tarasu nadal obowiązuje.**

Zgłoszenie: **ROOM 09 → depot**, pasażer nie wsiada albo nie wysiada. W znalezionym lokalnym zapisie tej trasy taksówka stała już w depocie, ale oferta pozostawała niewybrana, osoba czekała w ROOM 09, a obsada auta była pusta. Dotychczasowa karta eksponowała miejsce dowozu jeszcze przed odbiorem i nie komunikowała dość jasno, że należy najpierw wybrać kurs. Nie zmieniano zapisu użytkownika.

Karta oferty podaje teraz osobno **ODBIÓR** i **DOWÓZ** oraz ma instrukcję dotknięcia w celu wyboru. Po akceptacji karta stanu wyróżnia miejsce odbioru oraz liczbę oczekujących osób. Dopiero potwierdzone wsiadanie przełącza ją na **NA POKŁADZIE / DOWÓZ** i cel podróży. Obraz karty i nawigacja korzystają ze wspólnego bieżącego celu. Przy postoju bez wybranego kursu najpierw pokazywana jest oferta osoby z tego tarasu; nie jest automatycznie przyjmowana.

Niezależnie od niejasności interfejsu odtworzono rzeczywistą blokadę przy krawędzi depotu. Auto o szerokości kolidera 2,2 m stało całkowicie na tarasie 10 m, z przesunięciem środka o 3,8 m. Warunek `half_width - 1.3` odrzucał taki postój mimo prawidłowego podparcia i wolnego wyjścia. `TaxiStop` korzysta teraz z rzeczywistej szerokości kolidera auta; w aktualnych scenach pad ma półszerokość 5 m. Większy samochód musi odpowiednio głębiej wjechać na taras.

Blokada ma trwały, widoczny powód na karcie: nie ten taras, trzymany ciąg, ruch auta, wystawanie poza krawędź, zajęte wyjście lub za mało miejsc. Nadal wymagane są podparcie na właściwym padzie i spoczynek. Odlot podczas wysiadania zachowuje pasażerów i nie wypłaca pieniędzy przed zakończeniem czynności.

Weryfikacja tej poprawki: **35/35** nowych testów `bash tools/verify.sh taxi-interaction`, również z rendererem, **39/39** dotychczasowych taxi, **6/6** pełnego przelotu oraz świeży import bez błędów. Nowy zestaw odtwarza niewybraną ofertę ROOM 09 → depot, wybór przez obsługę kliknięcia, zmianę bieżącego celu dopiero po wejściu, cztery kursy z 1/3 osobami przy obu krawędziach, grupę 2 osób, przerwane wysiadanie, fizyczną przeszkodę, auto częściowo poza tarasem i szerszy kolider. Sprawdzono też bezpieczny postój i wyjście 3 osób na wszystkich 6 przystankach. Fixture ustawia podejścia do tarasu, a silnik wykonuje lądowanie i obsługę ludzi; osobny test trasy pokrywa ciągły lot. Sterownik testu z grafiką ignoruje utratę fokusu pulpitu; produkcyjne blokady pozostają bez zmian i są sprawdzane w istniejącym zestawie taxi.

Paczka przed uproszczeniem UI: **`city03-7135fc30122f`**, `build/city03-7135fc30122f-itch.zip`. Eksport i kontrola geometrii gotowego PCK przeszły. Lokalnie sprawdzono WebGL2 / Chromium w oknie 412 × 915: rozróżnienie odbioru i dowozu, rozwinięcie listy, wybór zdalnego odbioru, zmianę kursu, wsiadanie w depocie i przełączenie na stan pasażera na pokładzie. Konsola i zgodność numeru wersji zostały sprawdzone. Nie wykonano nowego testu na telefonie ani pomiaru wydajności.


## Automatyczne kursy i minimalny UI — decyzja autora 2026-09-13

Zastępuje poprzednią próbę z ręcznym wyborem kart. `TaxiDirector` wybiera lokalną gotową grupę przy bezpiecznie zaparkowanym aucie i rozpoczyna rezerwację oraz dojście. Wcześniej zapisane pole `selected` nie jest warunkiem odbioru. Zachowane są limity miejsc, sprawdzanie dostępności przystanków i prześwitu dla modelu auta, przerwanie wejścia po odlocie, bezpieczne wysiadanie i jednorazowe rozliczenie.

Z ekranu lotu usunięto listy ofert, wybór 1–3, panele etapów kursu, trwałe instrukcje, minimapę, prowadzenie po trasie, dystans trasy i przycisk zapisu. Mały pasek pokazuje portfel, obsadę, paliwo oraz sprawność. Zębatka otwiera opcje z zapisem i holowaniem, a ikonka mapy otwiera osobny widok odniesienia. Kontekstowe przyciski serwisu są widoczne tylko przy potrzebie tankowania lub naprawy w danym warsztacie. Krótkie dymki nad autem przekazują cel, wypłatę i istotną blokadę; komunikat o niezmiennej blokadzie nie powtarza się bez końca. Strzałki sterowania są rysowane wektorowo, bez zależności od dostępności znaków strzałek w czcionce eksportu.

### Doprecyzowanie autora: strzałka w dzielnicy celu

`TaxiDirector.guidance_target()` udostępnia cel wyłącznie w fazie `riding`, gdy gracz prowadzi auto obsługujące kurs w tej samej mapie i dzielnicy co platforma. `CityDefinition.district_id_at()` korzysta z granic miasta oraz istniejących podziałów X=0,5, Y=160 i LowLife Y<11. Nazwa obszaru depotu w HUD nie zmienia jego przynależności do Velvet. Wyjazd z dzielnicy ponownie ukrywa strzałkę; przy kursie wewnątrz tej samej dzielnicy pojawia się po wejściu pasażera. Menu, dialog, utrata sterowanego auta, wysiadanie, anulowanie i zakończenie kursu usuwają naprowadzanie.

Wzorem jest `AFlyingCabPawn::SetProximityGuidance()` w Unreal: wskaźnik nad autem i obrót według wektora do celu. `TaxiGuidanceArrow` rysuje małą pomarańczową strzałkę z ciemną obwódką, śledząc interpolowaną pozycję auta i kamerę każdej klatki. Kierunek liczy wprost do punktu lądowania, także poza kadrem. Nie pyta grafu tras ani fizyki o przeszkody. Dymki mają dodatkowy odstęp od strzałki. Stan naprowadzania jest wyliczany z bieżącego kursu; nie wymaga nowego pola w zapisie.

Weryfikacja macOS / Godot 4.7.2: **169/169** kontroli — naprowadzanie 48, UI 21, interakcje pasażerów 52, miasto 48. Zestaw naprowadzania przeszedł również z rendererem; obejmuje wszystkie 25 przystanków, granice dzielnic, kierunki, zakończenie kursu i blokady UI. Obejrzano strzałkę przy wjeździe do Aurelii, podejściu do OCTANE i z dymkiem. Import bez cache: PASS. W istniejącym teście miasta uwzględniono rozliczany paliwowo ogon wygaszenia silników z poprzedniej zmiany fizyki. W tym zadaniu nie wykonywano eksportu ani pakowania; przeglądarka, S25+ i Windows nie były testowane.

Referencje odczytane przy zmianie: `archive/godot/scripts/ui_logic/MapOverlay.gd` (osobny widok i pauza), `archive/godot/scenes/ui_scenes/MobileControls.tscn` (ikona mapy), `archive/godot/scripts/npc_system/npc_passanger_male.gd` (automatyczne podejście i wejście do pobliskiej taksówki), a w Unreal `FlyingCabDispatchComponent.cpp` i `FlyingCabLivingPedestrian.cpp` (strefa gotowości auta i pasażer oczekujący na przystanku). Archiwów ani Unreal w tej zmianie nie rozwijano.

Mapa nie uruchamia kolejnej kamery 3D; rysuje autorskie sylwetki zabudowy, autostrady i punkty przystanków na podstawie załadowanego świata. Geometrię do widoku zbiera przy otwarciu. Wyłączono obliczanie trasy od aktualnej pozycji gracza podczas lotu. Graf pozostał do dostępności i wyceny kursów oraz dalszej rozbudowy ruchu.

Mapa i opcje zatrzymują fizykę oraz czas lokalnej symulacji. Mają własny powód blokady sterowania, czyszczą dotyk, tankowanie i naprawę. Zamknięcie zwalnia tylko ich blokadę; nie usuwa blokady dialogu ani utraty fokusu. Wyjście z mapy lub usunięcie poziomu nie pozostawia drzewa w pauzie. Nie zmienia to otwartej decyzji o zasadach przyszłego timera kampanii.

Weryfikacja na macOS / Godot 4.7.2: **209/209** kontroli (kursy 39, interakcje ROOM 09 → depot 33, interfejs 21, pełny fizyczny przelot 6, pojazdy 45, architektura 41, smog 24). Zestaw UI przeszedł również z rzeczywistym rendererem. Sprawdza m.in. pauzę auta w powietrzu, zachowanie paliwa i czasu pasażerów, obsługę mapy przez M i dotyk, opcje przez Esc, zwolnienie trzymanego ciągu, brak przenikania dotyku do auta, małe przyciski płatnego serwisu, automatyczne wejście w depocie, zniknięcie dymków, kompatybilność starej zaznaczonej oferty oraz usunięcie mapy z otwartym menu. Testy kursów zachowują rzeczywiste podparcie bryły, grupy 1/3 osób przy obu krawędziach, przerwane wysiadanie i przeszkodę obok drzwi.

Świeży import izolowanej kopii przeszedł bez błędów (`build/clean-import-sjjgskqi/output.log`). Eksport przechodzi kontrolę transformacji geometrii gotowego PCK. Lokalny WebGL2 / Chromium w widoku 412 × 915: automatyczne wejście w depocie, znikający dymek, osobna mapa z celem OCTANE, opcje i zapis z potwierdzeniem w dymku. Wyniki nie są nowym pomiarem wydajności na telefonie; Samsung S25+ i Windows wymagają osobnej próby.


Finalna paczka: **`city03-41f7930b3d6f`**, `build/city03-41f7930b3d6f-itch.zip` (9,98 MiB). Potwierdzono zgodność hash źródeł i plików ZIP z eksportem. Po ponownym otwarciu finalnej wersji Web bez `?new=1` pozostały pasażer na pokładzie i cel OCTANE zapisane przez opcje; konsola nie zgłosiła błędów ani ostrzeżeń. Potwierdzono ID finalnej wersji oraz czytelne etykiety przycisków sterowania. Paczka nie została opublikowana na itch.io.


## Jeden oczekujący na każdej platformie — 2026-09-13

Wszystkie 25 lądowisk otrzymało zapisane w edytowalnej scenie przystanki. Sześć starych identyfikatorów zachowano; 19 nowych odwołuje się do nazw lokali w czterech dzielnicach. Dopasowano punkty oczekiwania i dojścia na czterech węższych tarasach (szerokość 8 m). Generator autorstwa miasta również zna wszystkie przystanki; przy tej zmianie nie uruchamiano generatora nadpisującego scenę. Źródłowe miasto skompilowano do runtime, z zachowaniem 1226 detali, 166 grup renderowania i 77 obiektów rzucających cień.

Nowe kursy mają jednego pasażera i najwyżej jedną ofertę z danego tarasu. Przy otwarciu miasta obsadzane są wolne platformy; limit to 25 oczekujących. Osoby wychodzą z punktów wejścia, czekają, wsiadają i po dowozie odchodzą. Oczekiwanie ma losowane odchylenie ±25% od 150 sekund, aby nie kończyło się jednocześnie. Sprawdzanie uzupełnienia działa co 5 sekund; nowy oczekujący nie powstaje na przystanku zajętym odejściem osoby lub obsługą aktywnego kursu. Limit dotyczy oczekujących: osoba właśnie dowieziona może chwilowo mijać człowieka czekającego już na tym tarasie.

`RideService` odrzuca próbę utworzenia drugiej oferty na zajętej platformie. Walidator zapisu obsługuje pełny katalog 25 miejsc i odrzuca zdublowane miejsca oczekiwania. Stare oczekujące grupy są przy otwarciu mapy przekształcane w jedną osobę z jednoosobową wyceną. Rozpoczęty kurs zachowuje dotychczasowe osoby, obsadę auta i cenę aż do zakończenia. Nie zmniejszono pojemności modelu taksówki.

Pula prezentacji ma 37 instancji i cztery przygotowane wcześniej siatki wyglądu. Wspólny szkielet może użyć dowolnej z tych siatek; nierówny rozkład wariantów nie powoduje znikania ludzi wskutek wyczerpania osobnej puli danego stroju. Na mapie referencyjnej wszystkie punkty są dostępne do dotknięcia; nazwa wybranego miejsca znajduje się pod mapą, a bezpośrednia etykieta na niej wskazuje cel aktywnego kursu. Unika to nakładania się 25 nazw. Nie przywrócono GPS ani minimapy w czasie lotu.

Weryfikacja: **179/179** kontroli (taxi 39, interakcje 52, UI 21, ciągły przelot 6, nowy zestaw całego miasta 61). Kontrola sieci obejmuje wszystkie 600 kierunkowych par przystanków. Test całego miasta wykonuje rzeczywiste lądowanie, automatyczny odbiór i wysiadanie na każdej platformie, ze sprawdzeniem ceny; przestawia auto wyłącznie pomiędzy podejściami, a ciągły przelot sprawdza osobny test. Obejmuje też 7 minut symulowanej wymiany pasażerów, zapis 25 osób, migrację starych oczekujących i rozpoczętych grup oraz 25 ludzi tego samego wariantu wyglądu. Native render: dodatkowo 19/19 próbki czterech obrzeżnych tarasów, po jednym w każdej dzielnicy. Świeży import: PASS, zero błędów (`build/clean-import-gk3uk5cc/output.log`).

Paczka: `build/city03-166aab0fc1e7-itch.zip`. Zmiany stylistyki aktorów opisuje osobna [propozycja czytelności](INTERACTION_LAYER_VISUAL_PROPOSAL.md); nie weszły do tej paczki. Pełny living world, niezależne auta NPC i traffic nadal są następnym etapem.


Dodatkowy ciągły przelot z rendererem i całą populacją przeszedł 6/6 kontroli: 30,83 s, 9,73 jednostki paliwa, 0 obrażeń, wypłata 83,36 CR. Lokalny pomiar balanced na macOS, 540 × 960: 1848 klatek, p95 17,45 ms, maks. 20,39 ms, brak klatek powyżej 50 ms. Raport `build/taxi-route-performance.json` zawiera ID `city03-166aab0fc1e7`. To krótka lokalna próba, bez pomiaru na S25+ i bez nowej weryfikacji Windows. Eksport PCK przeszedł kontrolę geometrii; pliki ZIP oraz hash źródeł są zgodne. Nie publikowano paczki na itch.io.


Sprawdzono finalny eksport WebGL2 / Chromium w oknie 412 × 915: start, pojedynczy pasażer w depocie, mapa wszystkich 25 punktów, odczyt nowego przystanku EDEN / NEW SELF, opcje, zapis i ponowne wczytanie. Dodatkowy zapis po holowaniu zachował rozpoznawalną kwotę 85 CR, co potwierdza odtworzenie sesji zamiast nowego startu. Konsola bez błędów i ostrzeżeń. Użyto oddzielnego lokalnego adresu testowego; próba nie publikuje zmian na itch.io.
