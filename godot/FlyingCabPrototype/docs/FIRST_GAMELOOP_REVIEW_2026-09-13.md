# Pierwsza pętla rozgrywki — przegląd i rekomendacje

Data: 2026-09-13. **Status: analiza, nie wdrożenie.** Oceniono kod archiwalnego Godota, aktualnego Unreal oraz punkty integracji w nowym prototypie. Obejrzano istniejące arkusze animacji pasażerów. Przeliczono niezależnie przykłady opłat i zaokrągleń tankowania. Nie uruchamiano w tym przeglądzie gier ani Automation; odczyt testów nie oznacza ich ponownego zaliczenia. Wartości z nagłówków C++ są domyślnymi parametrami kodu, nie potwierdzeniem zawartości binarnego Data Asset w uruchomionej grze.

**Kontynuacja:** autor zaakceptował wdrożenie przewozów, odkładając inne pojazdy, traffic i niezależne podróże mieszkańców na następny etap. Aktualny stan kodu, zakres i weryfikacja: [raport wdrożenia](FIRST_GAMELOOP_IMPLEMENTATION.md). Poniższy przegląd zachowuje stan z chwili analizy.

## Rekomendacja

Wykorzystać doświadczenia obu projektów, ale zmienić naliczanie opłat, model pasażera i prowadzenie do celu. Z Unreal przejąć rozdział kursów, portfela, prezentacji i ruchu mieszkańców, ograniczoną liczbę ofert, identyfikatory przystanków oraz zdarzenia dla zadań. Ze starego Godota — czytelny cykl człowieka wychodzącego z budynku, czekającego na taxi i odchodzącego po wysiadaniu, a także referencje animacji.

Nowy Godot ma już sesję, stan każdego auta, limity miejsc, paliwo, obrażenia i warsztaty. Potrzebuje działających kursów i postaci korzystających z tych fundamentów. Sama rejestracja nowych usług nie stworzy rozgrywki.

| Obszar | Archiwalny Godot | Unreal | Co wykorzystać / poprawić |
| --- | --- | --- | --- |
| Ludzie | Animowane sylwetki 2D; patrol, wezwanie, dojście, wysiadanie | Pasażer kursu: cylinder i głowa przesuwane w strefie. Mieszkaniec: osobny aktor z cyklem podróży | Zachowanie i czytelność ze starego Godota; semantyczne etapy podróży z Unreal. Nowa wspólna postać, zachowująca tożsamość i wygląd |
| Kursy | Cel, licznik pasażerów i opłata w skrypcie samochodu | Osobny Dispatch, oferty z celem/ceną/czasem, jeden aktywny kurs | Oddzielić ofertę, podróż i stan osoby; kurs przypisać do konkretnego auta |
| Nawigacja | Obracana strzałka bezpośrednio do celu | Minimapa, znaczniki, najbliższa oferta, pomoc przy podjeździe | Wybrany cel pozostaje stabilny. Wyznaczać dostępną drogę przez miasto i wskazywać następny odcinek |
| Ekonomia | Globalne saldo, płatne paliwo, opłata za postęp do celu | Osobny portfel, konfiguracja cen, paliwo/naprawy/holowanie, statystyki zarobku | Cena uzgodniona przed kursem, jednokrotne rozliczenie, rzeczywiste koszty usług; ponowny balans w jednostkach nowej gry |

## Ustalenia, które trzeba uwzględnić przed implementacją

### 1. Naliczanie opłaty nagradza krążenie — poprawić w pierwszej wersji

Obie wersje dodają pełną stawkę za zmniejszenie odległości do celu, a za jej zwiększenie odejmują połowę. W Unreal dla lokalnej stawki 1,1 CR/m przelot od odległości 100 m do 90 m i z powrotem podnosi opłatę z 20 do 25,5 CR. Każdy następny taki obieg dodaje kolejne 5,5 CR. To przyrost należności za kurs, nie dowód dodatniego zysku po paliwie; ekonomia mimo to wynagradza brak postępu. Dolne ograniczenie opłaty dodatkowo łagodzi koszt oddalania.

Archiwalny kod ma ten sam wzór. Dla 0,04/piksel i 100 pikseli w każdą stronę przyrost wynosi 2; scena `car_yellow_cab.tscn` nadpisuje stawkę na 0,08, więc tam wynosi 4. Test `FlyingCab.Core.Dispatch.FareCalculation` sprawdza podejście, cofnięcie i minimum, lecz nie sprawdza całego obiegu wracającego do punktu wyjścia.

**Propozycja:** wycena przy utworzeniu oferty na podstawie dostępnej trasy i taryfy; akceptacja zamraża cenę. Po dowiezieniu wypłata dokładnie raz. Dobrowolny objazd kosztuje paliwo i czas gracza, ale nie zwiększa ceny. Na początek bez drugiego licznika czasu przejazdu i bez premii za ryzykowne manewry. Ewentualne napiwki później, z osobnym limitem i czytelną regułą.

Źródła: [Godot: opłata i zakończenie kursu](../../../archive/godot/scripts/car.gd), funkcje `_update_delivery`, `_unboard_npc`; [Unreal: obliczenia](../../../unreal/FlyingCabFlightLab/Source/FlyingCabFlightLab/FlyingCabDispatchComponent.cpp), `CalculateUpdatedFare` od linii 290; [test opłaty](../../../unreal/FlyingCabFlightLab/Source/FlyingCabFlightLab/FlyingCabCoreTests.cpp).

### 2. Liczba miejsc wymaga jednej reguły dla wszystkich pojazdów

W starym Godocie `_board_taxi()` zwiększa `passenger_count` przed sprawdzeniem, czy istnieje cel. Brak celu pozostawia zajęte miejsce mimo nieudanego wejścia. Podniesienie `max_passanger` ponad 1 nie tworzy obsługi wielu podróży: kolejne wejście nadpisuje jeden `_delivery_point` i licznik opłaty, a zakończenie usuwa tylko jednego pasażera i czyści cały kurs.

Dispatch Unreal przechowuje globalne `bPassengerOnBoard` i zakłada jedno miejsce. Ruch miejski ma inną zasadę: w `HandleVehicleStop` limit dwóch dotyczy nowo zabranych osób na danym postoju, bez odjęcia osób pozostających w środku. Przy rozbudowanej trasie może to przekroczyć zamierzony limit. `BoardVehicle` mieszkańca nie sprawdza też samodzielnie, czy pojazd obsłuży jego cel. To ryzyka rozbudowy tych kontraktów, nie stwierdzenie, że każda obecna trasa już je ujawnia.

**Propozycja:** operacja wejścia najpierw sprawdza podróż, docelowy przystanek, dostępność auta i wszystkie potrzebne miejsca, następnie rezerwuje je. Po dojściu potwierdza obsadę. Odlot, utrata auta lub anulowanie zwalnia rezerwację. Suma zajętych i zarezerwowanych miejsc nie przekracza `VehicleDefinition.max_passengers`.

Pierwszy etap: **jedno aktywne zlecenie przewozu gracza**, ale może ono dotyczyć jednej osoby lub grupy ze wspólnym celem. Wariant grupowy pozwoli sprawdzić sens pojemności aut bez dokładania wielu równoległych tras i rozliczeń. Mieszkańcy korzystają z tego samego kontraktu miejsc.

Źródła: [wejście w starym Godocie](../../../archive/godot/scripts/npc_system/npc_passanger_male.gd), od linii 387; [Dispatch Unreal](../../../unreal/FlyingCabFlightLab/Source/FlyingCabFlightLab/FlyingCabDispatchComponent.h); [wymiana mieszkańców na przystanku](../../../unreal/FlyingCabFlightLab/Source/FlyingCabFlightLab/FlyingCabLivingWorldManager.cpp), od linii 290; [obecne miejsca w aucie](../scripts/cab.gd), `board_passenger`.

### 3. Pasażer ma być człowiekiem ze stanem, a nie elementem znacznika

Stary Godot usuwa NPC przy wejściu i tworzy nowego ze sceny samochodu przy wyjściu. Nie przenosi indywidualnej tożsamości ani wariantu wyglądu. Pasażer Dispatch Unreal jest dwiema bryłami wewnątrz strefy. Jego odpowiednik w ruchu miejskim jest osobnym `FlyingCabLivingPedestrian`, związanym z `FlyingCabTrafficVehicle`. Osoby fabularne mają jeszcze oddzielne definicje tożsamości i rozmów.

W Unreal wsiadanie kursowe wymaga nakładania się strefy i małej prędkości (domyślnie do 1,8 m/s), ale nie potwierdza podparcia na właściwym tarasie. Po 0,65 s przesuwania sylwetki następuje odbiór. Przy pionowych lądowiskach przeniesienie tego warunku dopuszczałoby wejście do zawisającego auta.

**Propozycja:** jedna zapisywalna osoba z `actor_id`, wariantem wyglądu, lokalizacją i aktualną podróżą. Reprezentacja na ekranie może być ukryta w aucie lub budynku, ale osoba nadal istnieje w stanie sesji. Ta sama osoba i wygląd wracają po wysiadaniu. Dialog i zadania mogą później odwołać się do jej ID.

Odbiór: postój na odpowiednim tarasie → dojście po chodniku do wolnej strony auta → potwierdzenie wejścia. Odlot przerywa dojście; NPC wraca do bezpiecznego punktu. Wysiadanie odbywa się po właściwej stronie, na wolnym fragmencie tarasu. Animacja pokazuje postęp, lecz sama nie wydaje zgody na wejście ani pieniędzy. Odblokowanie sterowania nie może przypadkiem kończyć kursu.

Źródła: [stary NPC](../../../archive/godot/scripts/npc_system/npc_passanger_male.gd); [strefa odbioru Unreal](../../../unreal/FlyingCabFlightLab/Source/FlyingCabFlightLab/FlyingCabDeliveryZone.cpp), `Tick` od linii 116; [mieszkaniec Unreal](../../../unreal/FlyingCabFlightLab/Source/FlyingCabFlightLab/FlyingCabLivingPedestrian.cpp); [tożsamość i rozmowy Unreal](../../../unreal/FlyingCabFlightLab/Source/FlyingCabFlightLab/FlyingCabNpcDefinition.h).

### 4. Wskazanie kierunku nie wystarcza do nawigacji po tym mieście

Godotowy `TaxiMarker` obraca wskaźnik do celu. Unreal wyznacza najbliższą ofertę według odległości X/Z i pokazuje punkty na minimapie. W sprawdzonej ścieżce kursów nie ma wyszukiwania drogi omijającej budynki. Trasy Living World są autorsko wyznaczonymi trasami mieszkańców, a nie gotową nawigacją gracza.

HUD Unreal mówi „CHOOSE ON MAP”, ale znaczniki ofert są wyświetlane jako `HitTestInvisible`; nie ma tu wyboru oferty przez dotknięcie znacznika. W praktyce wybór odbywa się przez dotarcie do pasażera. Najbliższa sugestia może zmieniać się podczas lotu.

**Propozycja:** kilka czytelnych ofert, wybór jednej i stałe prowadzenie do niej. Mały, edytowalny graf przejezdnych korytarzy lotu: tarasy, bezpieczne podejścia, pionowe prześwity, autostrady i połączenia między nimi. Krawędzie respektują kierunki tras, gabaryty auta i dostęp do dzielnic. Generowanie ofert pomija nieosiągalne cele.

Na minimapie przebieg trasy, poza ekranem kierunek do następnego odcinka, przy tarasie czytelne miejsce postoju. Trasa doradza; pilot zachowuje sterowanie i swobodę skrótów. Przeliczenie po wyborze celu, zmianie dostępności lub wyraźnym opuszczeniu trasy, nie w każdej klatce. Koszt trasy może szacować czas i paliwo z uwzględnieniem wznoszenia oraz autostrady; wyświetlany szacunek nie jest gwarancją spalania.

Źródła: [TaxiMarker](../../../archive/godot/scripts/taxi_syst/TaxiMarker.gd); [prowadzenie Unreal](../../../unreal/FlyingCabFlightLab/Source/FlyingCabFlightLab/FlyingCabHudPresenterComponent.cpp), `UpdateProximityGuidance`, `UpdateObjectiveStatus`; [minimapa Unreal](../../../unreal/FlyingCabFlightLab/Source/FlyingCabFlightLab/FlyingCabTouchControls.cpp), `UpdateMinimapMarkers`.

### 5. Ekonomię balansować względem kosztu lotu i leku

Unreal ma wartościowy podział na portfel, ceny, zakupy oraz statystyki przychodów i wydatków. Warto zachować zakup ograniczony faktycznym brakiem zasobu i stanem portfela. Domyślne parametry kodu to m.in. baza kursu 20 CR, 1,1 CR/m, 1,5× stawka dystansowa między dzielnicami, paliwo 2 CR/jednostkę, naprawa 1 CR/HP, holowanie 35 CR i 3 CR za near miss. Nie są gotowym balansem nowego miasta, modeli ani kampanii.

W starym tankowaniu koszt zaokrąglany jest w dół w każdym kroku fizyki. Dla domyślnych 600 jednostek/s i 5 CR/10 jednostek przy 60 Hz wychodzi 300 CR/s, a przy 120 Hz 240 CR/s za ten sam zasób. Końcówki napełnienia mogą być darmowe. Zakup „za 10” może z kolei pobrać pełne 10 przy mniejszym faktycznym braku. Unreal jawnie zaokrągla usługi do pełnej jednostki — to konsekwentniejsza zasada, ale nadal trzeba ją zakomunikować albo zastąpić rozliczeniem rzeczywiście dodanej ilości.

Obecna naprawa w nowym Godocie pobiera koszt faktycznie przywróconych HP i dopuszcza część usługi. Zachować tę własność. Wprowadzić wspólną usługę portfela z walidacją kwot oraz identyfikatorem rozliczenia, zamiast dopisywania salda osobno przez auto, dialog i kurs. Ustalić minimalną jednostkę waluty i zachowywać resztę przy płynnym naliczaniu; nie zaokrąglać niezależnie każdego kroku.

W trybie oceny pętli gospodarczej tankowanie powinno kosztować i działać tylko w oznaczonych punktach. Darmowe paliwo na każdym tarasie oraz RESET odnawiający auto są obecnie narzędziami próby lotu i pozwalają ominąć koszty. Trzeba rozdzielić reguły próby lotu od reguł ekonomii. Zaprojektować odzyskanie sprawności przy pustym portfelu, aby nie tworzyć przypadkowej sytuacji bez wyjścia; sposób pomocy lub holowania jest jeszcze decyzją projektową. Nie kopiować darmowego pełnego odnowienia jako gospodarczego rozwiązania.

Przy ocenie kursu mierzyć: dojazd po osobę, przewóz, postój, zużyte paliwo, obrażenia i wynik po kosztach. Osobno podjazdy w górę, zjazdy, kurs lokalny, autostrada i powrót bez klienta. Przykładowe 100 m daje przy domyślnych stawkach Unreal 130 CR, ale nie mówi jeszcze, ile taki kurs zarobi w nowym Godocie. Bonus pieniężny za near miss proponuję pominąć w pierwszej pętli kampanii: w obecnej formie pieniądze pojawiają się bez płatnika i konkurują z przewozami jako źródło dochodu.

Zgodnie z [wizją](../../../docs/GAME_VISION.md) docelowy sens zarobku to **zakup leku i dostawa do siostry**. Samo zarobienie pieniędzy lub kupno dawki nie przedłuża jej czasu. Time Attack i jego cel 1000 CR nie zastępują tej pętli. Cenę dawki i przyrost czasu ustalić po pomiarach kursów; historyczne 6 i 10 minut są referencją, nie zatwierdzonym balansem.

Źródła: [tankowanie archiwalne](../../../archive/godot/scripts/gas_station.gd), od linii 117 i 144; [portfel Unreal](../../../unreal/FlyingCabFlightLab/Source/FlyingCabFlightLab/FlyingCabEconomyComponent.cpp); [parametry Unreal](../../../unreal/FlyingCabFlightLab/Source/FlyingCabFlightLab/FlyingCabEconomyAsset.h); [naprawa obecna](../scripts/services/vehicle_service.gd); [spalanie obecne](../scripts/vehicle_fuel.gd).

### 6. Kurs musi przetrwać zmianę reprezentacji i mapy

Zapis starego Godota obejmuje m.in. pieniądze, timer siostry, zadania i ostatnie auto, lecz zapis auta pomija kurs i pasażerów. `RunComponent` Unreal zapisuje wyniki Time Attack, nie trwającą podróż. Dispatch blokuje wyjście z auta przy pasażerze; ta blokada ogranicza problem w prototypie, ale koliduje z dalszymi interakcjami pieszymi.

Nowy Godot zapisuje identyfikatory pasażerów w `VehicleState`, ale nie ma jeszcze opisów osób, ofert, kursów ani rozliczeń przewozów. `register_system` nie zapisuje automatycznie stanu dodanej usługi.

**Propozycja:** kurs przechowuje `ride_id`, osoby, `vehicle_id`, początek/cel z `map_id` i `stop_id`, stan, cenę oraz wynik rozliczenia. Kontroler gracza nie jest właścicielem pasażerów. Przy wyjściu Ari zostawia ich w konkretnym aucie; dalsza polityka czekania/anulowania może być rozwijana bez przenoszenia ich do następnego pojazdu.

Pierwszy zapis musi już objąć te dane i migrację starszego schematu. Wczytanie weryfikuje powiązania i obsadę przed zmianą sesji. Zakończenie kursu i wypłata są jedną operacją z `ride_id`; ponowne zdarzenie nie daje drugiej wypłaty. Przy zmianie mapy zachowuje się stan, nie referencje do węzłów. Przejście przez granicę mapy, restart, usunięcie auta i anulowanie wymagają jawnych skutków dla podróży.

Źródła: [SaveManager](../../../archive/godot/scripts/save/SaveManager.gd), `save_min`; [zapis starego auta](../../../archive/godot/scripts/VehiclePersistence.gd), `save_car_state`; [Unreal Run](../../../unreal/FlyingCabFlightLab/Source/FlyingCabFlightLab/FlyingCabRunComponent.cpp); [obecna sesja](../scripts/session/runtime_context.gd).

## Sylwetki pasażerów

Obejrzany pasażer starego Godota korzysta z CraftPix `City_men_2`: rozpoznawalna osoba z plecakiem, animacje idle i walk, naturalny profil boczny. Repo zawiera także arkusze GandalfHardcore, m.in. biznesmena i kobietę z torebką. To różne proporcje i stylistyki; nie mieszać ich bez ujednolicenia. W scenie CraftPix animacja chodu ma 10 klatek, a idle 6. Animacji dedykowanego machania ręką ani wsiadania nie ma w tym zestawie sceny — wezwanie realizuje napis „TAXI”.

**Rekomendacja wizualna dla obecnego miasta:** proste, stylizowane postacie 3D z wyraźną głową, tułowiem, rękami i nogami, wspólnym modelem animacji oraz kilkoma wariantami ubrania/koloru. Referencje ruchu wziąć z arkuszy starego Godota. Pozwoli to zachować spójność z otoczeniem, światłem i późniejszym trybem pieszym. Wymagane na start: stanie, chodzenie, czytelne wezwanie taxi, podejście i odejście. Pełna animacja wnętrza auta nie jest potrzebna do oceny kursu.

Istniejące animowane sprite'y osadzone w świecie 3D są szybszym wariantem próbnym, jeśli celem będzie najpierw sprawdzenie tempa. Warstwa wyglądu powinna być wymienna bez zmiany reguł kursów. Żaden wariant nie ma jeszcze potwierdzonego kosztu renderowania na S25+. Nie zakładać, że przezroczysty sprite będzie automatycznie tańszy od małej bryły 3D.

## Granice odpowiedzialności w nowym projekcie

Poniższe elementy są propozycją konkretnych odpowiedzialności, a nie informacją o dodanych modułach.

| Element | Odpowiedzialność |
| --- | --- |
| Definicja i stan osoby | Tożsamość, wygląd, aktualna mapa/miejsce, etap podróży; możliwość późniejszego podpięcia dialogu |
| Kursy / dyspozytornia | Dostępne oferty, rezerwacja, związanie podróży z autem, zakończenie i anulowanie |
| Obsada pojazdu | Wspólna kontrola miejsc i rezerwacji dla gracza oraz mieszkańców; limit z definicji pojazdu |
| Przystanek w scenie | Stabilne ID, taras/podłoże, punkt oczekiwania, dojścia, wejścia do budynku i bezpiecznego wysiadania |
| Nawigacja | Graf dostępnych połączeń i obliczenie trasy; oddzielne wykonanie ruchu przez pilota lub AI |
| Portfel / taryfy | Wycena i rozliczenie kursu, zakup paliwa, napraw, później leku; zdarzenia o zmianie salda |
| Prezentacja i UI | Sylwetka, animacja, oznaczenia, karta oferty i informacja o wypłacie; odczytują stan |
| RuntimeContext | Jawne połączenie usług i zapis ich stanów między mapami; nie przejmuje logiki wszystkich systemów |

Dane modeli, taryf, profili mieszkańców i przystanków edytować w Inspectorze. Przystanek jest dzieckiem odpowiedniej lokacji i korzysta z jej transformacji, zamiast powielać ręczne współrzędne w HUD, generatorze ofert i grafie. Graf lotu oraz chodniki pieszych mogą współdzielić ID przystanków, ale potrzebują różnych reguł ruchu. Obecny `WalkingActor` jest adapterem ruchu; nadal trzeba dodać scenę osoby i zachowania.

## Proponowany pierwszy grywalny zakres

1. Kilka aktywnych przystanków na obecnej mapie — początkowo około sześciu, w tym depot i połączenie Velvet–Foundry. Miasto zachowuje istniejącą geometrię. Wybrane kursy muszą sprawdzać przelot lokalny, zmianę wysokości i wykorzystanie autostrady.
2. Dwie–trzy dostępne oferty, z widocznym człowiekiem, celem, liczbą osób, ceną i czasem oczekiwania. Jedna łatwo dostępna przy starcie. Wybór wskazuje trasę; wejście potwierdza przyjęcie kursu. Wygaśnięcie oferty jest czytelne i kończy prowadzenie do niej.
3. Postój, dojście i wejście pasażera, lot według wskazówek, wysiadanie i odejście do budynku. Jednorazowa wypłata oraz krótka informacja o zarobku. Wariant pojedynczy i grupowy sprawdzają pojemność auta.
4. Płatne paliwo i istniejące naprawy: gracz podejmuje decyzję, czy utrzymać rezerwę, naprawić uszkodzenie, czy przyjąć następny kurs. Wersja gospodarcza ma określoną regułę odzyskiwania niesprawnego auta.
5. Mały pokaz niezależnej podróży mieszkańca w aucie NPC, korzystający z tej samej obsady i tożsamości. To zalążek living world. Przejęcie tego rzeczywistego auta i reakcja jego użytkownika pozostają obowiązkową częścią pełnego pokazu zgodnie z wizją; nie uznawać samego ruchu ulicznego za ukończoną kradzież.

Najpierw ocenić tę pętlę przewozów. Następne domknięcie kampanii: kupno dawki → lot i piesze dostarczenie do siostry → faktyczny przyrost czasu. Scena spotkania i treści dialogów wymagają osobnego opracowania. Pełna wizja nadal obejmuje także zlecenie o wątpliwym moralnie charakterze, ograniczony dostęp do wyższych pięter i sprawczość Ariego. Ten przegląd nie zatwierdza mechaniki walki.

## Wydajność i sprawdzenie wyniku

Nie przenosić globalnego wyszukiwania auta przez każdego NPC w każdym kroku ze starego Godota ani aktywnej fizyki całej populacji przez całe miasto. Unreal ma zdarzenia na przystankach i osobny menedżer, ale jego piesi nadal mają indywidualny Tick i zapytania o przeszkody; obecny kod nie jest gotową polityką skalowania na telefon.

Na początek mała liczba postaci i jeden pojazd mieszkańca. Modele, materiały i animacje przygotować przed pierwszym pokazaniem, zgodnie z obecnym etapem przygotowania grafiki. W pobliżu działa ruch i kolizje; poza obszarem interakcji można utrzymywać tańszy stan logiczny. Ukrycie wyglądu nie usuwa osoby, oferty ani jej miejsc w aucie. Nie wyłączać symulacji obiektu, który zaraz może wejść w kontakt z graczem. Liczbę mieszkańców zwiększać dopiero po pomiarze; pula reprezentacji ma uzasadnienie przy częstym pojawianiu się osób, nie jako obowiązkowa komplikacja kilku stałych postaci.

Warunki odbioru implementacji:

- Ten sam pasażer pojawia się przed kursem i po nim; nie wsiada w powietrzu, nie wychodzi w ścianie ani poza tarasem. Odbiór działa po obu stronach auta.
- Brak celu, pełne auto, odlot w trakcie dojścia, wygaśnięcie, anulowanie i 0 HP nie pozostawiają zajętych rezerwacji ani podwójnych osób. Obsada ruchu NPC respektuje pozostałe miejsca przy kolejnym postoju.
- Trasa omija budynki i uwzględnia dostęp; wybrana oferta nie przeskakuje samoczynnie na inną. Gracz może zboczyć i otrzymać zaktualizowane prowadzenie.
- Krążenie nie zwiększa uzgodnionej ceny. Powtórne zakończenie ani wczytanie nie wypłaca ponownie. Zmiana kroku symulacji nie zmienia ceny tej samej ilości paliwa/naprawy.
- Zmiana sterowanego auta, wejście do pomieszczenia oraz zapis/wczytanie zachowują powiązanie podróży, osoby, auta i rozliczenia. Utrata fokusu nie wykonuje hurtowo czekających operacji po powrocie.
- Ocena ekonomii obejmuje cały czas kursu i powrotu, koszt paliwa, napraw oraz możliwość zarobienia po błędzie. Cena leku jest testowana dopiero z jego dostawą i timerem kampanii.
- Eksport sprawdzony w Chrome na S25+: pierwsze pojawienie się każdego wariantu postaci, wejście/wyjście, kilka kolejnych kursów, smog, powrót z tła i co najmniej 10 minut gry. Porównać czasy klatek z tą samą wersją bez populacji. Obecny cel projektu to 60 FPS, p95 ≤ 16,7 ms i wyjaśnienie skoków ponad 100 ms po przygotowaniu; nie jest to dotąd potwierdzony wynik tej przyszłej pętli.

Wynikiem tego zadania jest powyższa ocena i propozycja zakresu. Kod gry, balans, eksport i publikacja pozostają bez zmian w ramach tego przeglądu.
