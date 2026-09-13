# Pojazdy, obrażenia i warsztaty — 2026-09-13

**Nowsze wdrożenie — biblioteka pojazdów:** [12 modeli, geometria, odporność, Inspector i zapis](VEHICLE_LIBRARY.md). Dziesięć nowych wariantów ma własne zapisane modele 3D; cab i legacy shuttle zachowano. Dostępna jest `scenes/vehicle_showroom.tscn`. Odporność redukuje obrażenia niezależnie od maksymalnego HP. Ten dokument poniżej opisuje pierwotny etap dwóch modeli oraz jego historyczne pomiary; aktualne parametry i weryfikacja Windows są w dokumencie biblioteki.

**Aktualizacja — pętla przewozów:** domyślna gra ma teraz 120 CR, płatne paliwo na dwóch stacjach i holowanie zamiast darmowego RESET. Wartości testowe 250 CR i pełny RESET poniżej dotyczą wcześniejszej próby pojazdów (nadal dostępnej przez F6). [Aktualna instrukcja](FIRST_GAMELOOP_IMPLEMENTATION.md).

Wdrożone w aktualnym prototypie Godot 4.7.2: definicje różnych modeli aut, rzeczywisty wpływ masy na fizykę, wytrzymałość, obrażenia od kolizji, ograniczenie miejsc pasażerów, zapis modelu i dwa działające warsztaty. Scena demonstracyjna nadal zaczyna się z jedną taksówką. Przykład shuttle ma inne parametry, ale korzysta ze wspólnej grafiki i geometrii auta; nie jest nowym modelem graficznym ani populacją NPC.

## Edycja w Inspectorze

Otwórz `scenes/cab.tscn` albo wybierz `Cab` w `scenes/flight_lab.tscn`. Rozwiń pole **Definition** przypisane przez skrypt `scripts/cab.gd`. Jest to zasób `VehicleDefinition` z polami eksportowanymi do Inspectora. Możesz też otworzyć bezpośrednio `resources/vehicles/basic_cab.tres`. Edytuj lokalne zasoby przed Play; stan Remote służy diagnostyce i nie zapisuje zmian do modelu.

| Pole | Jednostka / znaczenie | Cab | Shuttle |
| --- | --- | ---: | ---: |
| `thrust_force` | Pionowy ciąg, N | 2350 | 3500 |
| `horizontal_thrust_force` | Poziomy ciąg, N | 1400 | 1900 |
| `thrust_rise_seconds` | Czas narastania 0–100% mocy na każdej osi, s | 0,18 | 0,18 |
| `thrust_release_seconds` | Czas wygaszenia 100–0% mocy; także przed zmianą kierunku, s | 0,12 | 0,12 |
| `mass_kg` | Masa używana przez lot i solver zderzeń, kg | 100 | 180 |
| `max_hull` | Maksymalna wytrzymałość, HP | 100 | 220 |
| `fuel_capacity` | Pojemność zbiornika, jednostki prototypu | 100 | 180 |
| `starting_fuel` | Paliwo przy tworzeniu nowego egzemplarza; ograniczone pojemnością | 100 | 150 |
| `fuel_vertical_rate` | Zużycie przy pełnym ciągu pionowym, jednostki/s | 1,4 | 2,0 |
| `fuel_horizontal_rate` | Zużycie przy pełnym ciągu poziomym, jednostki/s | 0,7 | 1,0 |
| `max_passengers` | Miejsca pasażerów, bez kierowcy | 3 | 6 |

Pozostałe grupy zawierają limity prędkości, tłumienie, zasady paliwa/autostrad i parametry obrażeń. Domyślne wartości to balans próbny. Masa 100 kg zachowuje wcześniejszą konfigurację prototypu; nie jest deklaracją masy finalnej taksówki. Paliwo i obecność pasażerów nie zmieniają jeszcze masy — obecnie `mass_kg` jest całą masą symulowanego pojazdu.

Każdy egzemplarz ma osobny `ThrustResponse`, który w stałym kroku fizyki zamienia polecenie sterowania na narastającą/wygasającą moc. Zwykłe puszczenie klawisza lub dotyku pozostawia krótki, rozliczany paliwowo impuls. Zerowy bak i 0 HP natychmiast odcinają napęd; reset, wczytanie, zmiana kierowcy i blokada sesji zerują pamięć ciągu. Moc nie trafia do zapisu sesji, więc wczytanie nie odtwarza przytrzymanego gazu. Ustawienie czasu na 0 przywraca natychmiastową reakcję w danym modelu.

Przyspieszenie napędu = siła / masa. Bazowe 2350/100 i 1400/100 zachowują 23,5 i 14 m/s². Grawitacja jest niezależna od masy, zgodnie z regułą swobodnego spadania; arcade'owe limity prędkości i tłumienie po puszczeniu pozostają jawnie strojonymi parametrami. Autopilot i kierunek dysz korzystają z tych samych obliczonych przyspieszeń. Przy bardzo dużej masie i małym ciągu auto może nie wystartować — pionowy ciąg musi przekraczać ciężar `mass_kg * gravity`.

## Dodanie modelu i egzemplarza

1. Skopiuj `resources/vehicles/basic_cab.tres`, nadaj nowe `model_id`, nazwę i parametry. Nie kopiuj skryptu pojazdu. Edycja wspólnego zasobu zmienia wszystkie sceny, które z niego korzystają; dla pojedynczego wariantu użyj **Make Unique**, zapisz osobny `.tres` i nadaj osobne ID modelu.
2. Dodaj zasób do tablicy **Models** w `resources/vehicle_catalog.tres`. Katalog wiąże stabilne ID z zasobami przy wczytywaniu. Nie zmieniaj istniejącego ID bez migracji zapisów. `validation_errors()` wykrywa nieprawidłowe liczby, brak ID i duplikaty w katalogu.
3. Utwórz scenę dziedziczącą po `scenes/cab.tscn` i przypisz nową **Definition**. Przykład: `scenes/vehicles/heavy_shuttle.tscn`. Możesz w tej scenie dalej opracować wygląd i kolizję. Zachowaj kontrakty węzłów `Visual`, `FlightFX` i `Headlights`, z których korzystają obecne adaptery prezentacji.
4. Każdemu egzemplarzowi nadaj unikalne w całej sesji `entity_id` oraz `initial_owner_id`. Nie powielaj `shuttle_01` przy wstawianiu kilku kopii. Mapa rejestruje nowe auta także po starcie.
5. Do zmiany kontrolera używaj `PlayerSession.take_control()`, a do wysyłania poleceń kontraktu kierowca + rewizja. Do obsady używaj `board_passenger(id)` i `leave_passenger(id)` — nie dopisuj pasażerów bezpośrednio do tablicy stanu. Boarding odrzuca pełne auto, duplikat, pusty ID, kierowcę i unieruchomiony pojazd.

`VehicleDefinition` jest wzorcem modelu. `FlightCab` tworzy prywatną kopię ustawień dla działania konkretnego egzemplarza. `VehicleState` przechowuje jego ID/model, właściciela, kierowcę, pasażerów, paliwo, stan techniczny i pozycję. Dwa auta tego samego modelu nie dzielą paliwa, uszkodzeń ani stanu animacji.

Zapis nadal ma schemat 1: nowe opcjonalne pole `model` ma domyślne `basic_cab` dla wcześniejszych zapisów. `condition` pozostaje ułamkiem 0–1; HP = `condition * max_hull`. Wczytanie rozwiązuje zapisany model przed odtworzeniem aktora i odrzuca nieznany model, paliwo przekraczające pojemność, duplikaty pasażerów i nadmiar obsady, bez częściowej zmiany sesji. Katalog definiuje bieżący balans; zmiana maksymalnego HP zachowuje procent uszkodzenia. Obniżenie pojemności/obsady istniejącego modelu może wymagać jawnej migracji zapisów. Menu zapisu i trwałość zapisu Web pozostają osobnym etapem.

## Obrażenia

`VehicleVitals` odpowiada za obrażenia i ich sygnały. Publiczne wejście `FlightCab.apply_damage(amount_hp, source)` przyjmuje rzeczywiste HP, odrzuca ujemne i nieskończone wartości i nie zależy od UI ani od przyszłych reguł walki. Sygnały `damaged` i `disabled` pozwalają dołączyć dźwięk, efekty oraz reakcje NPC.

Przy zderzeniu porównywana jest prędkość wychodząca z poprzedniego kroku z wynikiem solvera, wzdłuż normalnej kontaktu. Pomiar zachowuje dwa ostatnie kroki prędkości: wykrywanie ciągłe CCD potrafi wyhamować auto krok przed udostępnieniem normalnej kontaktu. Test 21 wysokości podejścia pokrywa różne momenty zetknięcia między krokami fizyki. Przy kilku punktach manifold używana jest największa zmiana, raz na krok. Samo ślizganie się równolegle do ściany, zwykłe hamowanie i spoczynek nie naliczają obrażeń. W Godot Physics 4.7.2 zaobserwowano zero z `get_contact_impulse()` w części pierwszych kontaktów mimo wyraźnej zmiany prędkości; test prawdziwego lądowania wykrył ten przypadek. Dlatego źródłem pomiaru jest wynik prędkości, a nie raport impulsu. API kontaktów: [PhysicsDirectBodyState3D](https://docs.godotengine.org/en/stable/classes/class_physicsdirectbodystate3d.html).

Domyślnie próg wynosi 7 m/s, odstęp pomiędzy naliczeniami 0,15 s, a obrażenia to `0.7 * max(0, delta_v_normal - 7)^2 * mass_kg / 100`. Większe `max_hull` nie zwiększa otrzymywanych obrażeń. Masa ma wpływ na odpowiedź solvera oraz skalę uszkodzenia własnego nadwozia. To strojenie arcade, nie model deformacji ani symulacja energii pochłanianej przez strefy zgniotu.

HUD pokazuje stan nadwozia i krótko podświetla ramkę przy uderzeniu. Przy 0 HP silniki nie dostają ciągu i nie zużywają paliwa; wrak zachowuje fizykę oraz kolizje. Nie ma jeszcze deformacji, wybuchów, stopniowej utraty mocy ani mechaniki walki. Smog nie zadaje obrażeń. RESET/R pozostaje narzędziem demonstracyjnym: przywraca auto i pełne paliwo na punkcie startu, ale nie odnawia pieniędzy.

## Warsztaty

- Depot Ariego: `City/LandingPads/Pad0/Workshop`, X=−22, taras Y=54, wysokość HUD około 102 m.
- Foundry / TORQUE — REPAIR / BODY SHOP: `City/LandingPads/Foundry03/Workshop`, X=22, taras Y=134, wysokość HUD około 182 m.

Stacje mają napis WARSZTAT i turkusowy plus na minimapie. Wystarczy zaparkować, puścić ciąg i **przytrzymać E lub panel naprawy**. W scenie `repair_station.tscn` można ustawić ID, nazwę, cenę i tempo. Bieżące wartości: **1 CR/HP**, **20 HP/s**. Sesja otrzymuje 250 CR na start; to fundusz do próby, a nie gotowa gospodarka kampanii. Zapis odtwarza portfel bez ponownego bonusu. Zwykłe tankowanie nadal pozostaje bezpłatne.

`RepairStation` jest strefą potomną rzeczywistego lądowiska. Samo przelecenie przez strefę nie wystarcza: wymagany jest kontakt z tym właśnie lądowiskiem, spoczynek, brak ciągu i brak autopilota/resetu. `WorkshopInteraction` dodatkowo sprawdza aktywnego kierowcę i blokady sesji. Nie naprawia auta podczas rozmowy, po utracie fokusu ani po zmianie sterowanego pojazdu.

`VehicleService.repair(state, model, campaign, amount_hp, cost_per_hp)` atomowo ogranicza naprawę stanem nadwozia i portfelem. Zwraca faktycznie przywrócone HP oraz emituje ID pojazdu, HP i cenę. Ostatnia część kredytu kupuje odpowiednią część HP. Puszczenie przycisku i start od razu przerywają usługę. Nawet 0 HP można naprawić, jeżeli auto już znajduje się w warsztacie; poza nim pozostaje prototypowy RESET. Nowy warsztat umieszczaj jako dziecko kolejnego podłoża, nadając unikalny `station_id`, a następnie jawnie kompiluj miasto. Generator autorowania również zachowuje te dwie stacje, lecz jego uruchomienie nadal nadpisuje `city.tscn`.

## Wykorzystane starsze rozwiązania

Archiwalny Godot: `archive/godot/scripts/vehicle_base.gd`, `car.gd`, `car_neo.gd`, `car_testowy.gd` oraz miejsca ich użycia. Wykorzystano ideę odrębnego modelu/egzemplarza, pól w Inspectorze, limitu miejsc i pomiaru kolizji po normalnej. Nie przeniesiono starej integracji w pikselach, powielonego progu uszkodzeń, bezpośredniego sterowania HUD z auta ani wyłączania kolizji wraku. Nie znaleziono w przejrzanych skryptach samodzielnego warsztatu porównywalnego z UE.

Unreal: `FlyingCabVehicleVitalsComponent.h/.cpp` oraz `FlyingCabRepairStation.h/.cpp`. Zaadaptowano oddzielny moduł zasobów auta, próg i nieliniowe obrażenia z krótką blokadą powtórzeń, wyłączenie ciągu przy 0 HP oraz płatną naprawę po zatrzymaniu i przytrzymaniu E. Implementacja Godota dodatkowo wymaga rzeczywistego podparcia, ma dotyk i rozlicza ułamkowe HP. Nie przeniesiono regeneracji paliwa przy opadaniu ani waluty/recovery kampanii. Archiwum i Unreal były wyłącznie referencją w tym zadaniu.

## Weryfikacja

Godot 4.7.2, macOS / Apple M5. **45/45 nowych kontroli** w `tests/vehicle_tests.gd`, zarówno headless, jak i z normalnym rendererem. Obejmują prawdziwe zderzenia, 21 podejść CCD i lądowania na depocie, wpływ masy na lot i impulsy, dwa modele i ich paliwo/autopilota, miejsca, uszkodzenia, unieruchomienie, naprawy klawiaturą/dotykiem, płatności, blokady, obie stacje, katalog oraz zapis. Rzeczywiste mocne lądowanie testowe pozostawiło 67,59/100 HP; zderzenie czołowe aut o masach 100/200 kg pozostawiło 49,82/99,25 HP. Łagodne lądowanie i późniejszy postój nie uszkadzają nadwozia.

Przeszły też istniejące zestawy: lot 23/23, postój 16/16, przestrzeń/paliwo 41/41, miasto 48/48, smog 24/24, efekty 18/18, architektura 41/41 oraz start z rendererem 8/8. Razem **264 sprawdzenia**, bez podwójnego liczenia powtórzonego zestawu pojazdów. Kompilacja i kontrola geometrii faktycznego PCK: PASS, wszystkie 1226 detali w 166 grupach pozostają poprawne. Obejrzano HUD przy pełnym i uszkodzonym nadwoziu oraz panel warsztatu. Pozostałe historyczne testy renderowania pasów i interpolacji nie były w tej zmianie powtarzane; powyższe testy funkcjonalne nie są benchmarkiem FPS.

Nowy eksport: **`build/city03-59cf0ff42a4a-itch.zip`**, również pod nazwą `build/FlyingCabFlight01-itch.zip`. Zawiera poprawkę poprzedniego błędu geometrii. Sprawdzono końcową paczkę lokalnie w Chromium/WebGL2: start po przygotowaniu, lot z depotu, mocne lądowanie (HUD około 69% HP), naprawa przyciskiem ekranowym do 100% i spadek portfela z 250 do 218 CR po zaokrągleniu HUD. Konsola nie zgłosiła błędów ani ostrzeżeń. Paczka nie została wysłana na itch.io. S25+/Android i Windows wymagają sprawdzenia tej wersji.

Uruchom z katalogu prototypu: `bash tools/verify.sh vehicles`, a pełny eksport: `bash tools/export-web.sh`. Dodatkowe nowe modele graficzne i efekty muszą przejść przygotowanie grafiki przed widocznym pojawieniem się; dzisiejszy przykład shuttle współdzieli już przygotowywane materiały taksówki. Populacja, strumieniowanie pojazdów i rozbudowa grafiki nie są objęte samym dodaniem parametrów modelu.
