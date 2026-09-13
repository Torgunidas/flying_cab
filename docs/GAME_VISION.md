# Flying Cab — wizja gry i fabuły

Data ustalenia: 2026-09-12. Źródło: opis autora projektu oraz doprecyzowania timera, żyjącego miasta i sprawczości Ariego w rozmowie projektowej.

Ten dokument jest punktem odniesienia dla projektowania rozgrywki, fabuły i świata, niezależnie od silnika. Opisuje docelową wizję; nie jest raportem ukończonych funkcji. W razie rozbieżności ze starszymi propozycjami fabularnymi lub decyzjami o zakresie kampanii pierwszeństwo mają poniższe ustalenia autora. Zasady techniczne, chronione sterowanie i status aktywnego projektu pozostają określone w `AGENTS.md`.

## Elevator pitch — bez ujawniania sekretu bohatera

**Ugh! spotyka GTA2 w pionowej cyberpunkowej metropolii.** Ari pilotuje latające pojazdy i podejmuje zlecenia, żeby zdobywać kolejne dawki drogiego leku dla śmiertelnie chorej siostry. Mieszkańcy przemierzają miasto pieszo i pojazdami, które Ari może kraść. Każda dostawa leku kupuje siostrze trochę czasu, a rosnąca presja pcha go do coraz bardziej wątpliwych moralnie zadań. W mieście gigantycznych wieżowców biedni żyją w trującej mgle na dole, a bogaci wysoko ponad nimi. Szansa na trwałe wyleczenie siostry kryje się na piętrach, do których ludzie tacy jak Ari nie mają dostępu.

## Założenia autorskie

- Punkt odniesienia: skrzyżowanie **Ugh! i GTA2** w cyberpunkowym świecie.
- Główny bohater nosi robocze imię **Ari**.
- Jego motywacją jest ratowanie śmiertelnie chorej siostry. Praca kierowcy i przewozy służą zdobywaniu zasobów potrzebnych do tego celu.
- Siostra wymaga kolejnych dawek drogiego leku. Dostarczenie dawki wydłuża globalne odliczanie do jej śmierci.
- Presja zdobywania zasobów prowadzi Ariego do coraz bardziej wątpliwych moralnie zadań.
- Ari dostrzega szansę na trwałe wyleczenie siostry w wyższych, niedostępnych dla biednych rejonach miasta. Dokładna metoda leczenia i przebieg finału pozostają do napisania.
- Pionowa struktura świata uzasadnia pionową orientację gry na telefonie.
- Prototyp musi pokazywać **living world**: NPC chodzą, wsiadają do aut, latają nimi i wysiadają. Gracz może kraść ich pojazdy.
- **Ari ma być sprawczy i kompetentny.** Gracz musi móc wpływać na świat i przebieg sytuacji. Autor nie chce bohatera odczuwanego jako słaby i bezradny.
- System walki nie został jeszcze określony. Wymaganie sprawczości nie przesądza o broni, strzelaniu, walce wręcz ani szczególnych zdolnościach androida.

## Ari — sekret fabularny, do użytku projektowego

**Ta sekcja zawiera ujawnienie centralnego sekretu bohatera.**

Ari jest androidem zaprogramowanym przez ojca dziewczyny do opieki nad nią. Ma zaimplementowane sztuczne wspomnienia i nie wie, że jest androidem. Uważa dziewczynę za swoją siostrę i działa zgodnie z tą relacją.

Jeżeli nie dostarczy leku na czas i siostra umrze, Ari również się wyłącza. To koniec gry. Związek pomiędzy życiem siostry a funkcjonowaniem Ariego jest faktem fabularnym, a nie wyłącznie umownym warunkiem porażki.

Moment i sposób ujawnienia prawdy graczowi oraz Ariemu nie zostały jeszcze określone. Nie ustalono też, co siostra wie o jego pochodzeniu. Publiczny pitch powyżej zachowuje ten sekret.

W dawnym prototypie siostra nazywa się **Maya**, a bohater w dialogach bywa nazywany **Jo**. Maya pozostaje nazwą odniesienia do istniejących materiałów; aktualne robocze imię protagonisty to Ari. Nie przemianowano historycznych zasobów.

## Timer kampanii i lek

**Ustalenie autora: dawki wydłużają odliczanie.** „Stały timer” oznacza globalną presję czasu związaną z życiem siostry, a nie nieprzekraczalny limit długości całej kampanii. Nie ustalono dodatkowego, niezależnego terminu na znalezienie trwałego lekarstwa.

- Gracz musi zdobyć zasoby, pozyskać dawkę i dostarczyć ją siostrze przed wyczerpaniem czasu.
- Dostarczona dawka przedłuża pozostały czas. Samo zarobienie pieniędzy lub kupienie leku nie jest ukończeniem ratunku.
- Lek zapewnia czasowe podtrzymanie życia. Trwałe wyleczenie stanowi dalszy cel fabularny.
- Wyczerpanie czasu bez kolejnej dawki oznacza śmierć siostry, wyłączenie Ariego i koniec gry.
- Jest to zegar kampanii powiązany z fabułą. Obecny tryb Time Attack nie zastępuje tego założenia.

Do ustalenia przy projektowaniu balansu: czas początkowy, czas dodawany przez dawkę, ceny, dostępność i możliwość magazynowania leku. Zachowanie odliczania podczas dialogów, menu, pauzy i po zamknięciu aplikacji wymaga osobnej decyzji. Globalne odliczanie nie jest samo w sobie ustaleniem, że czas płynie poza uruchomioną grą.

## Pionowy świat

Świat jest dystopią arcologii i gigantycznych wieżowców. Wysokość zamieszkania wyraża pozycję społeczną. Pionowość łączy kompozycję ekranu telefonu, podróż przez miasto i fabularne dążenie do miejsca, w którym można uratować siostrę.

| Warstwa miasta | Ustalone założenie świata |
| --- | --- |
| Dół | Spowity trującą mgłą z zanieczyszczeń. Zamieszkany przez biednych; obecni są zbiry, przestępcy i środowiska wykorzystujące desperację mieszkańców. |
| Środkowe piętra | Świat ludzi walczących o awans i mieszkanie wyżej. Wyścig szczurów i presja statusu społecznego. |
| Najwyższe piętra | Świat bogaczy, zasadniczo odmienny od życia na dole i niedostępny dla biednych. Tu Ari dostrzega szansę na trwałe wyleczenie siostry. |

Nie ustalono, że zanieczyszczenia są przyczyną choroby siostry. Frakcje, instytucje i docelowe sposoby przekraczania granic między warstwami pozostają do zaprojektowania.

### Cztery dzielnice dużego prototypu — ustalenie autora

Autor zlecił powiększenie poziomu Godota dwukrotnie w poziomie i czterokrotnie w pionie. Wszystkie lądowiska mają być częściami budynków wyrastających z ziemi. Inspiracja wizualna: cyberpunkowe filmy noir, pastelowe neony, kicz dolnego miasta i kontrast z elegancją górnych pięter.

| Położenie | Funkcja określona przez autora | Interpretacja obecnego prototypu |
| --- | --- | --- |
| Lewy dół | Ponura dzielnica rozrywkowa: bary i burdele | **Velvet** — fuksja, lila, nocne kluby, hotele na godziny, ciasno zestawione reklamy |
| Prawy dół | Ponura dzielnica biznesowa: biura, warsztaty, stacje paliw | **Foundry** — turkus, bursztyn, odsłonięte instalacje, serwisy i nocne biura |
| Lewa góra | Jaśniejsze, eleganckie kliniki, wellbeing, augmentacja | **Eden** — mięta, perłowe obudowy, zaokrąglone fasady i zieleń |
| Prawa góra | Jaśniejszy, elegancki świat finansów, bankowości i dużych pieniędzy | **Aurelia** — chłodne szkło, złote podziały i prywatna bankowość |

Górne dzielnice są obecnie otwarte, aby można było testować lot. Nie usuwa to docelowego fabularnego wymagania ograniczonego dostępu dla biednych. Nazwy dzielnic i lokali są roboczą interpretacją, nie osobno zatwierdzonym kanonem.

Pomiędzy dzielnicami mają przebiegać trasy szybkiego ruchu wzorowane na Unreal. Obecna adaptacja obejmuje centralny krzyż i obwodnicę, płynne zwiększenie limitów prędkości do ×1,5 i obniżenie spalania do ×0,5 bez dodatkowego przycisku lub automatycznego ciągu. Bieżący stan sceny, testów i narzędzi opisuje [City 02](../godot/FlyingCabPrototype/docs/CITY_02.md).

### Miejsce Ariego nad smogiem — dalsze ustalenie autora

Ari Cab Depot ma znajdować się na jednym z wieżowców, tuż nad najniższym poziomem miasta: zanieczyszczonym, przestępczym i zdegradowanym. Tę warstwę wypełnia mgła; po zjechaniu do niej pojazd automatycznie włącza reflektory. Inspiracją autora jest klimat *Blade Runnera* i trzy przekazane kadry: chłodna mgła, ginące w niej sylwetki wysokich budynków oraz punkty i snopy światła w ciemności.

Dalsze ustalenie autora: pogłębić low city, aby dolna autostrada stanowiła wyraźną granicę z bardziej cywilizowanym miastem i przebiegała poza mgłą. Oznaczenia pasów na wszystkich autostradach mają być nieruchome; życie ma budować przyszły ruch pojazdów. Autor doprecyzował, że zwiększoną prędkość ma pokazywać efekt związany z pojazdem. Dopalacze mają dynamicznie obracać się względem karoserii przy locie w lewo i prawo, z mniejszym wychyleniem niż w próbie Unreal. Bieżąca interpretacja to krótkie turkusowe smugi przy rzeczywistym przekroczeniu zwykłych limitów prędkości, jaśniejsze i dłuższe płomienie oraz skręt dysz do 16°; wartości pozostają do oceny w grze.

Bieżąca interpretacja: dno obniżone o 48 m (Y = −48 m), depot zachowany na zachodniej arcologii na Y = 54 m, czyli 102 m nad nową ziemią. Dolna autostrada zajmuje Y = 11…21 m. Smog zanika do Y ≈ 0 m, a najwyższe animowane strzępy sięgają najwyżej Y = 2 m, pozostawiając 9 m prześwitu pod trasą. Robocza nazwa najniższej warstwy to **LowLife**. Wysokości, kolory, tempo włączania świateł i szczegóły zabudowy są parametrami próby, a nie ostatecznym balansem. Nie zatwierdzono obrażeń od smogu ani osobnego zużycia paliwa przez reflektory. Przestępczy charakter miejsca jest na tym etapie pokazany scenografią; zachowania NPC pozostają dalszym zadaniem.

## Granice miasta jako kontrola przestrzeni powietrznej

Ustalenie autora z 2026-09-12: opuszczanie dostępnej przestrzeni ma być uzasadnione w świecie gry. Przy maksymalnej wysokości miękki pułap wyhamowuje pojazd, pojawia się komunikat **„Max alt reached”**, a próba dalszego wznoszenia może zwiększać zużycie paliwa. Punktem odniesienia jest soft ceiling pierwotnego Godota.

Na boki można przekroczyć granice miasta. Komunikat **„Non authorized out of grid movement. Forced return”** zapowiada przejęcie sterowania; autopilot fizycznie sprowadza auto do obszaru miasta. Gracz otrzymuje kontrolę po zakończeniu powrotu.

Autor zatwierdził dodanie paliwa i tankowania na lądowiskach do bieżącego prototypu. Postój bez ciągu ma mieć zerowe spalanie. Parametry wysokości i granic, tempo spalania, tankowania oraz szczegóły rezerwy autopilota są wartościami do testów; ich bieżący stan opisuje [README prototypu](../godot/FlyingCabPrototype/README.md). Darmowe tankowanie i pełny bak po resecie służą obecnym próbom, nie określają ekonomii kampanii.

Propozycja projektowa do dalszego rozwinięcia: powiązać dostępny pułap i obszar lotu z aktualną strefą miasta, tak aby zasady dostępu współgrały z fabularnym dążeniem Ariego do wyższych pięter.

## Żyjące miasto — obowiązkowy element prototypu

**Ustalenie autora:** pokaz ma zawierać mieszkańców poruszających się pieszo, wsiadających i wysiadających z aut oraz latających nimi. Pojazdy NPC mogą być kradzione przez gracza. Ten zakres należy do prototypu pokazującego przyszły produkt.

Proponowane kryteria pokazu, wynikające z tego wymagania:

- Gracz może zaobserwować spójny cykl mieszkańca: dojście do auta, wejście, lot, lądowanie i wyjście. Czytelne powiązanie postaci z pojazdem buduje wrażenie, że mieszkańcy korzystają ze świata.
- Cykl odbywa się również bez uruchamiania questa przez gracza. W pobliżu toczy się zwykłe życie miasta.
- Gracz może przejąć pojazd należący do uczestnika tego ruchu i sam nim odlecieć. Zdolność prowadzenia tylko osobnego auta przygotowanego dla gracza nie wykazuje jeszcze tej funkcji.
- Przejęcie przerywa dotychczasowe używanie pojazdu przez NPC. Dalsze zachowanie postaci powinno uwzględniać utratę auta; czytelna reakcja właściciela lub świadka jest proponowanym sposobem pokazania wpływu gracza.
- Mała populacja i krótka trasa mogą wystarczyć do pierwszego sprawdzianu. Docelowa liczba mieszkańców i aut wynika z czytelności oraz pomiarów na telefonie, a nie z obowiązku symulowania całej metropolii naraz.

Sposób kradzieży pojazdu zaparkowanego lub zajętego, zachowanie kierowcy i pasażerów oraz reakcje właścicieli i świadków pozostają do zaprojektowania. Policja, pościgi, poziomy poszukiwania i reputacja nie zostały jeszcze zatwierdzone jako wymagania prototypu.

## Sprawczość Ariego i otwarta kwestia walki

Niska pozycja społeczna Ariego ma współistnieć z jego kompetencją i zdolnością wpływania na otoczenie. Bieda, koszt leku i ograniczony dostęp do wyższych pięter tworzą presję, ale gracz powinien mieć działania, dzięki którym zmienia sytuację. Kradzież i wykorzystanie pojazdu NPC są jednym z ustalonych przejawów tej sprawczości.

Do sprawdzenia w projekcie: możliwość samodzielnego inicjowania działania, skutecznego reagowania na przeszkody i korzystania z okazji tworzonych przez żyjący świat. Skutek powinien być widoczny w zachowaniu postaci, dostępności pojazdu lub dalszym przebiegu zdarzenia.

**System walki pozostaje otwarty.** Propozycją do osobnej próby jest prosta interakcja konfrontacyjna, np. odepchnięcie albo obezwładnienie przy przejęciu pojazdu. To wariant projektowy, nie ustalona mechanika. Rodzaj przemocy, odporność Ariego, ryzyko porażki i sterowanie dotykowe wymagają decyzji. Nie przypisujemy mu automatycznie nadludzkiej siły ani widocznych zdolności ujawniających sekret androida.

## Konsekwencje dla projektu rozgrywki

### Rozszerzony zakres systemów — ustalenie autora 2026-09-12

Autor dodał do zakresu **tryb pieszy Ariego, różne pomieszczenia i mapy, dialogi oraz naprawę pojazdu**. System walki jest planowany na przyszłość; jego konkretna forma, balans i sterowanie pozostają do ustalenia. Architektura ma umożliwiać dokładanie kolejnych systemów bez przebudowy modelu gracza i kampanii.

Tryb pieszy i prowadzenie pojazdu powinny korzystać ze wspólnej sesji gracza. Przejście do wnętrza lub innej mapy nie może gubić stanu zostawionego auta, postępu zleceń, zasobów ani czasu siostry. Dialogi i naprawa są odrębnymi systemami, korzystającymi z tego samego stanu sesji. Stan techniczny auta nie powinien zależeć od konkretnej, jeszcze nieustalonej implementacji walki.

Wdrożenie fundamentów w Godocie opisuje [architektura prototypu](../godot/FlyingCabPrototype/docs/ARCHITECTURE.md). Reużywalny adapter chodzenia, obsługa przejść map, kontroler dialogów i usługa naprawy nie oznaczają ukończonych lokacji, animacji, rozmów ani pełnej rozgrywki tych systemów.

### Kierunek projektowania rozgrywki

Poniższe punkty są interpretacją projektową ustaleń autora i kierunkiem do sprawdzenia w prototypie:

1. **Lot ma znaczenie dla ratunku.** Czas przejazdu, wybór trasy, lądowanie, paliwo i stan pojazdu wpływają na możliwość dostarczenia dawki.
2. **Zasoby mają osobistą stawkę.** Zarobek pozwala kupić siostrze czas; wydatki na utrzymanie pojazdu konkurują z kosztem leku.
3. **Eskalacja moralna wynika z presji.** Zadania i ich konsekwencje powinny pokazywać, dlaczego Ari przekracza kolejne granice. Konkretne czyny i stopień swobody wyboru wymagają scenariusza.
4. **Awans w mieście jest postępem fabularnym.** Docieranie wyżej przybliża do szansy na leczenie i odsłania kolejne warstwy społeczeństwa.
5. **Relacja z siostrą nadaje sens odliczaniu.** Rozmowy i powroty z lekiem powinny pozwalać poznać ją jako postać oraz odczuć skutek udanej dostawy.
6. **Żyjące miasto stwarza okazje do działania.** Ruch mieszkańców, ich postoje i pojazdy mogą wpływać na decyzje gracza; kradzież zmienia obserwowaną sytuację.
7. **Presja współistnieje ze sprawczością.** Ari potrafi działać skutecznie, a koszt jego decyzji buduje napięcie moralne i ekonomiczne.

## Prototyp 2,5D — kierunek i wymagany pokaz

W rozmowie poprzedzającej zapis wizji rozważono Godota jako lżejsze narzędzie do iterowania grywalnego wycinka: modele i otoczenie 3D, lot w jednej płaszczyźnie, kamera z boku pod lekkim kątem oraz pionowy ekran telefonu. Celem jest pokazanie charakteru gotowej gry i testowanie jej przez przeglądarkę na urządzeniu mobilnym.

Proponowany pierwszy wycinek powinien połączyć spotkanie z siostrą, zdobycie pieniędzy, lot po lek i dostawę wydłużającą odliczanie. Powinien też pokazać co najmniej jedną ofertę zadania o wątpliwym moralnie charakterze oraz perspektywę niedostępnych wyższych pięter.

Obowiązkowe uzupełnienie ustalone przez autora: działający cykl życia NPC i możliwość kradzieży jego pojazdu. Pokaz ma pozwalać graczowi uczestniczyć w życiu miasta i zmieniać przebieg zdarzeń. Szczegóły scenariusza, skala pokazu, forma ewentualnej walki i ujawnianie sekretu Ariego wymagają osobnej decyzji; living world, kradzież i sprawczość są już ustalonym zakresem.

Aktualizacja 2026-09-12: autor zlecił rozpoczęcie nowego projektu Godot od podstaw, w pierwszym kroku ograniczonego do jednego pojazdu i lotu. Projekt powstał w [godot/FlyingCabPrototype](../godot/FlyingCabPrototype/README.md). Sterowanie dotykowe czerpie ze starego Godota, a model lotu z Unreal. Living world, kradzież, sprawczość i kampania pozostają wymaganiami kolejnych etapów pełnego pokazu. Unreal Engine 5.8 pozostaje docelowym projektem gry, a `archive/godot/` historycznym materiałem źródłowym.

### Istniejące podstawy w Unreal — odczyt z 2026-09-12

[Living World authoring](../unreal/FlyingCabFlightLab/docs/LIVING_WORLD_AUTHORING.md) opisuje ruch po trasach, przystanki i przewóz pieszych. Kod `FlyingCabLivingPedestrian.cpp` zawiera wejście pasażera do pojazdu oraz zakończenie podróży z powrotem do chodzenia. Jest to punkt odniesienia dla przyszłego pokazu.

Sprawdzona ścieżka wejścia gracza w `FlyingCabPlayerController.cpp` wyszukuje pojazdy typu `AFlyingCabPawn`, podczas gdy pojazdy ruchu `AFlyingCabTrafficVehicle` są osobnym typem aktora. Samo działanie ruchu, przewozu pasażerów i prowadzenia auta przez gracza nie potwierdza więc kradzieży pojazdów NPC. Przejęcie pojazdu i zachowanie jego dotychczasowych użytkowników wymagają osobnego projektu oraz weryfikacji. Ten zapis wynika z odczytu dokumentacji i kodu; nie wykonano w tej aktualizacji testu gry ani zmian implementacji.

## Powiązanie z pierwotnym Godotem

Odczyt materiałów źródłowych potwierdził następujące elementy:

| Materiał | Zawartość istotna dla wizji |
| --- | --- |
| [Rozmowa z Mayą](../archive/godot/dialogues/main_quest/maya_dialog.tres) | Chora siostra, kończące się pieniądze pozostawione przez ojca, obietnica opieki, podanie leku i akcja `add_maya_time`. W starym dialogu podanie dawki dodaje 10 minut. |
| [Rozmowa z Froggym](../archive/godot/dialogues/main_quest/mr_froggy_take_2.tres) | Pozyskiwanie kosztownej substancji i deklaracja gotowości zrobienia wszystkiego dla siostry. |
| [Główny wątek questów](../archive/godot/scripts/quests/main_story/) | Rozmowa z Mayą, odnalezienie auta, dotarcie do Froggy'ego, powrót z lekiem i dalsze zarabianie na substancję. |
| [Globalny stan gry](../archive/godot/scripts/GameState.gd) | Odliczanie czasu Mai, funkcja `add_maya_time` i sygnał `maya_time_expired`. Domyślna wartość w skrypcie wynosi 6 minut; nie jest to zatwierdzony balans przyszłej gry. |
| [Przedmiot leku](../archive/godot/items/the_substance_low.tres) | Substancja występuje jako przedmiot o nazwie „The Substance”. |

Wymienione zasoby są dowodem wcześniejszego zarysu i mechanizmu wydłużania czasu; nie potwierdzają kompletnej implementacji nowej wizji, ujawnienia androida ani wszystkich zakończeń. Sekret Ariego i pionowy podział społeczny zostały zapisane tutaj na podstawie aktualnego opisu autora.

Wcześniejszy zapis w [kontraktach migracji z Godota](../unreal/FlyingCabFlightLab/docs/GODOT_MIGRATION_CONTRACTS.md), który wyłączał timer Mai na rzecz Time Attack, został zaktualizowany: timer kampanii jest teraz wymaganiem wizji, a jego implementacja pozostaje osobnym zadaniem.


## Minimalny interfejs i samodzielna nawigacja — ustalenie autora 2026-09-13

Core projektu to minimalny UI. Gracz ma obserwować miasto i prowadzić auto na głównej mapie, samodzielnie planować trasę oraz rozwijać znajomość poziomu. Stała minimapa z wytyczonym przelotem odciągała uwagę od prowadzenia w poprzednich iteracjach.

Wsiadanie i wysiadanie pasażerów jest automatyczne po bezpiecznym zatrzymaniu auta, jak w historycznym Godocie i Unreal. Nie wymaga przyjęcia karty oferty. Informacje przekazują krótkie dymki, które pojawiają się i znikają. Zapis należy do okna opcji pod małą zębatką. Mapa jest osobnym widokiem otwieranym ikonką mapy lub miasta, jako odniesienie do planowania; bez stale widocznej minimapy i prowadzenia gracza po wyliczonej trasie.

Bieżący prototyp zatrzymuje symulację w widoku mapy i opcji, wzorem `MapOverlay` historycznego Godota. Docelowe zasady czasu kampanii w menu nadal wymagają osobnego ustalenia; kampania nie jest jeszcze uruchomiona. Algorytm tras może służyć wewnętrznej wycenie i przyszłemu ruchowi NPC, bez zamieniania interfejsu w GPS.

Późniejsze doprecyzowanie autora (2026-09-13): podczas kursu pojawia się strzałka wskazująca ogólny kierunek docelowej platformy, wzorem Unreal. Naprowadzanie jest dostępne **dopiero w dzielnicy celu**. Nie wyznacza drogi między przeszkodami; dotarcie do właściwej dzielnicy i wybór trasy pozostają po stronie gracza.


### Pasażerowie i czytelność świata — ustalenie autora 2026-09-13

Autor ograniczył oczekiwanie do **jednego pasażera na platformie** i rozszerzył odbiór oraz dowóz na wszystkie platformy miasta. Pojazdy i piesi mają być wyraźnie widoczni względem architektury. Autor wskazał trzy plany: dalekie tło, miasto z platformami oraz warstwę interakcji pieszych i pojazdów. Konkretna technika wyróżnienia wymaga oceny propozycji wizualnej; kontury, dodatkowe światło lub zmiana proporcji postaci nie są jeszcze zatwierdzone.
