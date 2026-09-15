# City 02 — cztery dzielnice

**Aktualizacja po audycie:** gra korzysta z kompilowanej `scenes/city_runtime.tscn`, a źródłem edycji pozostaje `scenes/city.tscn`. Detale są grupowane w małych sektorach i mają ograniczone cienie. Ekran startowy przygotowuje grafikę przed lotem. [Architektura i pomiary](ARCHITECTURE.md) opisują nowe profile, instrukcję kompilacji oraz status eksportu; poniżej zachowano opis układu miasta.

Rozbudowa z 2026-09-12 na polecenie autora. Po pierwszym powiększeniu z 75 × 80 do 150 × 320 m pogłębiono podmiasto o kolejne 48 m. W korekcie z 2026-09-13 dodano po 25 m bocznego zapasu na manewry. Przestrzeń lotu ma obecnie **200 × 368 m**: granice X to −99,5…100,5 m, ziemia Y = −48 m, pułap Y = 320 m. Środek pozostał przy X = 0,5 m. Podział góra/dół przebiega na Y = 160 m; otwarty dostęp do górnych dzielnic jest ustawieniem próby.

## Granice i tablice ostrzegawcze — 2026-09-13

Zewnętrzne krawędzie tras pozostają przy X = −71,5 i 72,5 m. Zapas do granicy wynosi teraz **28 m**, wcześniej 3 m. Wymuszony powrót nadal odbywa się fizycznym autopilotem, z przekazaniem sterowania 6 m wewnątrz granicy. Mapa na żądanie i przygotowanie grafiki pobierają nowe rozmiary z `CityDefinition`. Poszerzono też warstwy smogu ze 174 do 224 m, aby obejmowały całą nową przestrzeń lotu.

Po obu stronach wisi po 15 tablic, od Y = −32 do 304 m co 24 m. Środki tablic znajdują się przy X = −79,5 i 80,5 m, czyli **20 m przed granicą**. Treść: **AIRSPACE CONTROL / CITY PERIMETER / TURN BACK**. Ciemna obudowa, bursztynowa rama, pas ostrzegawczy i stalowe zawieszenie są jedną wspólną siatką. Trzy napisy są statyczne. Tablice znajdują się za płaszczyzną lotu (Z = −2,5), nie mają kolizji, migania ani własnych świateł/cieni. Oznaczniki samej granicy przesunięto na nowe współrzędne.

Edycja: `scenes/city.tscn` → `PerimeterWarnings`, wspólny model `scenes/perimeter_sign.tscn`. `tools/build_perimeter_sign.gd` jawnie przebudowuje wyłącznie model tablicy. Zaktualizowano recepturę `tools/build_city.py`, ale nie uruchamiano pełnego generatora miasta. Po zmianie źródła wykonano `tools/compile_city.gd`; wynik zachowuje 1226 grupowanych detali, 166 partii oraz 77 obiektów rzucających cień. Walidacja ponownie wczytanej sceny nie znalazła zmienionych lub brakujących transformacji.

Jednocześnie poprawiono dymki nad autem: osobny `TaxiNoticeBubble` odświeża się w każdej klatce po kamerze i strzałce. Korzysta z interpolowanej pozycji pojazdu. Łamanie tekstu jest przeliczane tylko przy zmianie treści lub szerokości, a główny HUD zachowuje odświeżanie 10 Hz. Dymki nadal znikają po czasie i są ukrywane w mapie/opcjach.

Weryfikacja tej korekty na macOS / Apple M5 / Godot 4.7.2: miasto **51/51**, granice i paliwo **43/43**, UI **21/21**, smog **24/24**, prezentacja dymków i tablic **8/8**, kontrola kompilacji **PASS**. Test prezentacji: 120 FPS przy fizyce 60 Hz, 724 klatki i 724 rysowania dymka; maksymalny błąd zakotwiczenia względem renderowanego auta **0,00000 px**. Obejmuje rozpędzanie, zmianę kierunku, lot ukośny, hamowanie, reset, wygaszenie oraz mapę i strzałkę. To kontrola synchronizacji, nie benchmark telefonu. Polecenie: `bash tools/verify.sh perimeter-render`.

Pełny start gry w profilu `balanced`: **8/8**, przygotowanie grafiki 6129 ms na tym Macu; po przygotowaniu wracają prawidłowe położenie auta, paliwo, światła i sterowanie. Łącznie zestawy tej korekty: **155/155** oraz kontrola kompilacji.

Obejrzano `build/perimeter-west.png`, `build/perimeter-smog.png` i `build/bubble-guidance.png`. Wariant wschodni zapisuje się jako `build/perimeter-east.png`. Windows i telefon wymagają osobnego sprawdzenia tej korekty. Nie wykonano eksportu ani paczki itch.io.

## Charakter dzielnic

| Dzielnica | Miejsce | Wygląd i funkcje |
| --- | --- | --- |
| Velvet | Lewy dół | Różowo-fioletowy nocny świat barów, prywatnych pokoi, klubów i hoteli na godziny. Reklamy, neony, odsłonięte instalacje. |
| Foundry | Prawy dół | Bursztynowo-turkusowe biura, stacje, warsztaty i logistyka. Żaluzje techniczne, pompy, wentylacja. |
| Eden | Lewa góra | Miętowe kliniki, wellbeing, augmentacja i medycyna długowieczności. Perłowe kolumny, obłe elementy fasad i zieleń. |
| Aurelia | Prawa góra | Prywatna bankowość i zarządzanie majątkiem. Chłodne szkło, złote pionowe podziały i reprezentacyjne wejścia. |

Nazwy lokali są robocze. Każda dzielnica ma sześć tarasów, oprócz nich istnieje startowy depot: **25 lądowisk z tankowaniem**. Depot przeniesiono na pierwszą zachodnią arcologię: taras X = −22, Y = 54 m; punkt startu auta (−22, 54,4, 0). Tankowanie jest nadal darmową regułą testową na wszystkich lądowiskach, niezależnie od tematu lokalu.

## Konstrukcja i przestrzeń lotu

Sześć głównych wieżowców ma ciągłą konstrukcję od ziemi. Tarasy, w tym przeniesiony depot Ariego, dochodzą tylną krawędzią do fasad i mają widoczne wsporniki. Górne lokale znajdują się na tych samych arcologiach, których niższe części obsługują dolne miasto. Tło obejmuje kolejną warstwę wież wyrastających z ziemi.

Lot nadal odbywa się w XY, przy Z = 0. Konstrukcje wież stoją za płaszczyzną lotu; wysunięte tarasy mają fizyczne kolizje. Detale fasad pozostają za torem pojazdu. Geometrię autorujemy jako rzeczywistą scenę, dostępną w edytorze przed uruchomieniem gry. Nie jest generowana ponownie przy każdym Play.

## Sześć tras szybkiego ruchu

| Trasa | Środek X/Y | Szerokość / wysokość |
| --- | --- | --- |
| Spine | 0,5 / 160 | 12 / 288 m |
| Crosstown | 0,5 / 160 | 144 / 12 m |
| West Ring | −66,5 / 160 | 10 / 288 m |
| East Ring | 67,5 / 160 | 10 / 288 m |
| Low Ring | 0,5 / 16 | 144 / 10 m |
| Sky Ring | 0,5 / 304 | 144 / 10 m |

Pasy są holograficznym oznakowaniem wolnej przestrzeni, bez ścian lub fizycznej nawierzchni. Na **wszystkich sześciu autostradach kreski są nieruchome**. Shader oznaczeń nie korzysta z czasu; usunięto również parametr przewijania z materiałów i narzędzia autorowania. Życie tras ma budować przyszły ruch pojazdów. Prostokąt wizualny i obszar premii korzystają z tej samej pozycji i rozmiaru węzła. Głębokość strefy to Z ±2 m. Oznaczenie `EXPRESS` w HUD informuje o działającym wspomaganiu; minimapa pokazuje trasy, tarasy i pozycję auta.

Adaptacja [Highway Turbo z Unreal](../../../unreal/FlyingCabFlightLab/docs/HIGHWAY_TURBO.md): maksymalne prędkości ×1,5 (poziomo 15,75 m/s, wznoszenie 17,25 m/s, opadanie 19,5 m/s), spalanie ×0,5, wejście 0,5 s, wyjście 0,8 s. Skrzyżowania nie mnożą premii. Bazowy ciąg, grawitacja, tłumienie i sterowanie pozostają takie same. Strefa nie dodaje automatycznego ciągu. Pusty bak, reset, spoczynek i wymuszony powrót wyłączają premię. Samo położenie w pasie bez ciągu nie spala paliwa.

Pułap zaczyna hamować w ostatnich 10 m, czyli Y = 310…320. Po przekroczeniu bocznych granic autopilot nadal zawraca do punktu 6 m wewnątrz, po wolnym korytarzu zewnętrznym. Cel mieści się w Y = −38…307 m; minimalne 10 m prześwitu jest liczone od nowego dna. Przy zmianie geometrii trzeba sprawdzić kolizje tras i dojścia do lądowisk.

## Prezentacja przyspieszenia i skręt dopalaczy

Efekt jest związany z autem: trzy krótkie turkusowe smugi ustawiają się za rzeczywistym kierunkiem lotu, również podczas wznoszenia i opadania. Ich długość i widoczność rosną wraz z przekroczeniem zwykłego limitu prędkości na danej osi; maksymalna długość środkowej smugi wynosi 5,5 m. Sam wjazd w autostradę, stanie w jej obrębie lub zwykły lot po skosie nie uruchamiają efektu. Wejście jest wygładzane w 0,18 s, wyjście w 0,25 s. Reset, postój, pusty bak i autopilot od razu ukrywają smugi. Prędkość może je podtrzymywać podczas szybkiego opadania bez ciągu, ale nie zapala wtedy płomieni.

Każdy z czterech dopalaczy ma własny punkt obrotu: obudowa, obręcz i płomień skręcają razem, a mocowanie pozostaje na karoserii. Strumień wychyla się przeciwnie do przyłożonego ciągu poziomego. Limit wynosi **16° względem bryły auta**; przy pełnym ciągu po skosie wychylenie to około 14°. Skręt jest płynny, po puszczeniu sterowania wraca do neutralnego położenia. Odwrócenie modelu w lewo zachowuje właściwy kierunek strumienia. Płomień występuje wyłącznie przy faktycznie przyłożonym ciągu; w locie express staje się jaśniejszy i do 40% dłuższy.

Punktem odniesienia jest odczyt `FlyingCabThrusterVisualComponent.cpp` z Unreal: tam strumień odwzorowuje pełny wektor przyspieszenia, także poziomy. Tutaj zmniejszono zakres animacji zgodnie z prośbą autora. `scripts/cab_flight_fx.gd` czyta stan lotu, nie zmienia komend, sił, prędkości, spalania ani kamery. Parametry efektu są w `scenes/cab_flight_fx.tscn` i jego skrypcie.

## Oświetlenie

### LowLife — warstwa pod depotem

Na dalszą prośbę autora najniższe części miasta otrzymały zdegradowaną zabudowę: ciemne podstawy wież ze słabo oświetlonymi oknami, osiem niskich bloków usługowych od ziemi, zardzewiałe kontenery, wentylację, zaślepione okna, stare latarnie i oznaczenia nieformalnego handlu. Centralny dawny budynek depotu został usunięty. Dno wraz z niską zabudową obniżono do Y = −48 m, a fundamenty, fasady i instalacje wież oraz tło przedłużono do nowej ziemi. Podmiasto zajmuje 59 m od ziemi do dolnej krawędzi Low Ring (Y = 11 m). Depot, cztery dzielnice i wszystkie trasy zachowały swoje położenie. Prześwity dróg i 25 miejsc tankowania pozostały dostępne.

Smog jest najgęstszy poniżej Y = −18 m, z miękką granicą do Y = 0 m; najwyższe strzępy animowanych warstw sięgają najwyżej Y = 2 m. Dolna autostrada zajmuje Y = 11…21 m i ma co najmniej 9 m prześwitu nad smogiem. Stanowi granicę podmiasta i bardziej cywilizowanej części miasta. Korzysta z mgły wysokościowej i głębokościowej oraz trzech animowanych, półprzezroczystych warstw zakotwiczonych w świecie. Przejście kolorów i światła otoczenia przy opadaniu daje chłodny, ciemny dół. Odległe sylwetki znikają szybciej niż auto i pobliskie powierzchnie. Parametry uwzględniają kamerę odsuniętą od płaszczyzny lotu o 19 m.

Reflektory włączają się automatycznie przy Y ≤ −6 m i gasną przy Y ≥ 0 m, z przejściem trwającym 0,6 s. Różne progi zapobiegają miganiu przy krawędzi smogu. Pojazd ma dwa rzeczywiste światła kierunkowe typu SpotLight3D i miękki wizualny snop. Profile `desktop`/`quality` włączają cienie reflektorów, a `balanced`/`performance` je wyłączają. Światła i snop podążają za kierunkiem auta. HUD pokazuje `LIGHTS / AUTO`. Reset na depot gasi je od razu. Pusty bak nie wyłącza świateł bezpieczeństwa; nie dodano zużycia paliwa przez oświetlenie.

To efekt przeznaczony do obecnego Compatibility: widoczny snop jest lekką geometrią z shaderem, a nie symulacją rozpraszania volumetric fog. Same reflektory oświetlają geometrię; cienie zależą od profilu. Nie zmieniano modelu lotu, sterowania ani spalania.

Wspólne wysokości i przejście: `resources/smog.tres`. Logika lamp: `scripts/cab_lights.gd`; ich geometria, zasięg i kolor: `scenes/cab_lights.tscn`. Wszystkie parametry są dostępne do dalszego strojenia. Wysokościomierz pokazuje wysokość spodu auta nad nową ziemią (Y − ground_height − 0,35 m), więc na depocie wskazuje 102 m. Minimapę rozszerzono do Y = −48 m; podział dzielnic pozostaje na Y = 160 m, a LowLife jest osobnym pasem pod autostradą. Próg awaryjnego resetu również liczony jest względem nowej ziemi (Y < −68 m).

### Pozostałe dzielnice

Projekt pozostaje w rendererze Compatibility, z myślą o dalszych próbach przeglądarkowych. Zastosowano materiały emisyjne, glow, kolorowe lokalne światła, cienie kierunkowe, SSAO i mgłę. Światła miejscowe mają ograniczony zasięg i wygaszanie z odległością. Światło otoczenia, kolor nieba i gęstość mgły łagodnie zmieniają się pomiędzy dolną a górną częścią miasta. Efekty są częścią renderowanej sceny, a nie obrazem w tle.

Wersja Compatibility ma własną implementację glow; użyto obsługiwanych przez nią parametrów opisanych w [dokumentacji Godota](https://docs.godotengine.org/en/stable/tutorials/3d/environment_and_post_processing.html#glow). Nie włączano Forward+, volumetric fog ani SDFGI. Wygląd oraz wydajność telefonu i eksportu webowego wymagają osobnej weryfikacji.

## Edycja

- `scenes/city.tscn`: budynki, tarasy i trasy, gotowe do edycji w Godocie.
- `scenes/city_runtime.tscn`: osobny wynik `tools/compile_city.gd`; nie edytuj go zamiast źródła. Po zmianach źródła wykonaj kompilator, a następnie Play. Eksporter webowy wykonuje tę kompilację automatycznie.
- `scenes/flight_lab.tscn`: oświetlenie, podłoże, pojazd, kamera i interfejs.
- `scripts/highway.gd`: edytowalny rozmiar, premia prędkości i kosztu każdej trasy. Przesunięcie węzła przesuwa też obszar działania.
- `scripts/city_atmosphere.gd`: przejście światła i mgły między warstwami miasta.
- `shaders/facade.gdshader`, `shaders/air_lane.gdshader`: okna fasad i oznakowanie tras.
- `tools/build_city.py`: jawne narzędzie autorowania obecnej sceny. **Ponowne wykonanie nadpisuje `city.tscn`; nie uruchamiaj go po własnych zmianach w edytorze bez ich zachowania.** Gra nie wywołuje tego narzędzia.

Wersja sprzed pogłębienia jest lokalnie zachowana w `build/pre-deeper-low-city/city.tscn`. Poprzednia scena została lokalnie zachowana w ignorowanym `build/pre-city-expansion/flight_lab.tscn`. Żadnych plików Unreal ani archiwum Godota nie zmieniano w tej rozbudowie.

## Weryfikacja

Zestaw `bash tools/verify.sh city` sprawdza wymiary, fizyczne połączenie każdego tarasu z fundamentem budynku, prześwit pełnej bryły pojazdu na sześciu trasach, lądowanie i tankowanie na wszystkich 25 platformach oraz działanie premii prędkości i paliwa. Pozostałe zestawy obejmują lot, sterowanie, granice, postój i interpolację kamery.

Podglądy z rzeczywistego renderera: `build/screenshots/city-low-ring.png`, `city-start.png`, `city-lowlife.png`, `city-smog-canyon.png`, `city-smog-edge.png` oraz kadry czterech dzielnic i `city-overview.png`. Tworzy je `tests/city_capture.gd`; pozycje obserwacyjne i zamrożenie auta występują tylko w tym narzędziu. Ujęcie całego miasta jest widokiem przeglądowym, a nie kadrem zwykłej gry.

**Wynik macOS / Apple M5 / Godot 4.7.2: 181/181 kontroli zaliczonych** — smog i reflektory 24/24, nieruchome oznaczenia 7/7, dysze i efekt express 18/18, miasto i autostrady 48/48, lot 23/23, granice i paliwo 41/41, postój 16/16, prezentacja 4/4. Test prezentacji z rendererem Compatibility przy kroku 120 FPS i fizyce 60 Hz: prędkość 10,50000 m/s, odchylenie zmian pozycji ekranowej 0,00016 px. To test interpolacji, nie benchmark wydajności telefonu. Obejrzano rzeczywiste podglądy dolnej autostrady poza smogiem, pogłębionego podmiasta z reflektorami i całego miasta. `tests/lane_render_tests.gd` porównuje faktycznie renderowane piksele wszystkich sześciu tras w dwóch chwilach: obrazy są identyczne. Próba kontrolna z ukryciem trasy potwierdza obecność oznaczeń w klatkach. `tests/flight_fx_tests.gd` sprawdza efekt podczas rzeczywistego lotu w obu kierunkach, wznoszenia i opadania, limit i płynność obrotu, mocowania, reset, brak paliwa, postój, autopilota i opuszczenie wspomagania. Obejrzano podglądy `build/screenshots/flight-express-right.png`, `flight-express-left.png`, `flight-express-up.png` wygenerowane przez `tests/flight_fx_capture.gd`; ta próba wyłącza grawitację do utrzymania poziomego lotu i korzysta ze zwykłej kamery gry. Test smogu obejmuje też lot z włączonym głównym kontrolerem aż do nowego dna, poprawny odczyt wysokości i dodatkową głębokość minimapy. Test smogu obejmuje rzeczywiste opadanie i wznoszenie, progi i płynność przełączenia, orientację reflektorów, reset, pusty bak oraz brak dodatkowego ciągu i spalania. Windows, eksport webowy i urządzenia mobilne nie były testowane. Ruch NPC i kampania pozostają kolejnymi etapami.


## Platforma testowa Foundry — 2026-09-13

Dodatkowy, **53-metrowy taras** znajduje się u góry przemysłowej dzielnicy, tuż pod poprzeczną autostradą Crosstown. Nawierzchnia: **Y = 150 m**, zasięg **X = 8…61 m**. Jest połączony z istniejącymi arcologiami i ma widoczne wsporniki. Zachodni koniec, oznaczony **LAND HERE**, zostaje wolny do przylotu własnym autem; wygodny punkt lądowania to X = 11,5. Nad nim widnieje tablica **FOUNDRY / TEST FLEET**.

W rzędzie stoi 12 zwykłych, grywalnych samochodów: cab, heavy shuttle, lorry, supercar, limousine, luxury car, police car, poor car, tow car i trzy kolory normal car. Numer oraz nazwa stanowiska są na froncie tarasu. Odstęp między bryłami wynosi 1,2 m, a wspólny pas pieszy biegnie przed autami na Z = 1,2. Wejście i wyjście używają dotychczasowego Q / przycisku dotykowego. Auta mają puste miejsce kierowcy i nieprzypisanego właściciela, więc nie wymagają dodatkowej reguły kradzieży ani menu wyboru modelu.

Parametry pochodzą z normalnych definicji pojazdów: masa, ciąg, paliwo i uszkodzenia działają jak w mieście. Model lawety nadal nie przewozi ładunku. Każde auto ma własne stałe ID `test_fleet/<model>`; zapis zachowuje jego położenie i stan. Zużyte lub odprowadzone auto nie jest automatycznie zastępowane nowym. Nowa gra odtwarza komplet; zwykły zapis odtwarza miejsce pozostawienia auta.

Taras istnieje zarówno w nowej, jak i wczytanej grze. Nie jest dodatkowym przystankiem taxi ani stacją paliw: pozostaje 25 miejsc kursów, a to osobny plac testowy. Razem z living world i początkowym cab w mieście jest teraz **63 pojazdy**.

Edycja geometrii: `scenes/foundry_test_platform.tscn`, instancja w `scenes/city.tscn`. Edycja aut: `scenes/foundry_test_fleet.tscn`, instancja w `scenes/flight_lab.tscn`; każdy samochód pozwala rozwinąć `definition` w Inspectorze. Flota fizyczna nie jest częścią kompilacji detali miasta. `tools/build_test_yard.py` jawnie przebudowuje tylko te dwie sceny; nie uruchamia się przy Play. Receptura `tools/build_city.py` zachowuje instancję nowej platformy, ale nie uruchamiano pełnego generatora miasta. Po zmianach geometrii wykonano kompilację miasta i walidację ponownie wczytanej sceny: PASS, 166 partii detali i 78 obiektów rzucających cień.

Sprawdzenie: `bash tools/verify.sh test-yard` — **68/68**, komplet modeli, rzeczywiste lądowanie w miejscu przylotu, wejście/start/lądowanie/wyjście z każdego auta, przejście całego rzędu pieszo, stan i wybrany pojazd po wczytaniu. `test-yard-render`: **6/6**, zapisuje trzy widoki z grafiki. Import edytora: PASS. Powtórzony pełny `living-world`: **82/82**, w tym prześwit wszystkich tras oraz ponad 11 minut ruchu przed i po wczytaniu. Geometria nie przecina pasów NPC ani autostrad. Wyniki dotyczą macOS / Godot 4.7.2; nie wykonano eksportu itch.io.
