# Metro City — czterokrotnie większy obszar

**Aktualizacja 2026-09-10:** aktywna mapa zawiera już zapisaną geometrię i obiekty dostępne przed Play. Bieżący sposób edycji opisuje [WORLD_EDITING.md](WORLD_EDITING.md). Poniższe wzmianki o generowaniu przy starcie opisują poprzedni etap.

Stan implementacji: 2026-09-04. Unreal Engine 5.8. Sterowanie gracza niezmienione.

Późniejsze rozszerzenia: [Highway Turbo](HIGHWAY_TURBO.md) oraz [24 platformy mieszkalne i większy bak](RESIDENTIAL_DISTRICTS.md). Poniższy opis 12 przystanków dokumentuje bazowy etap Metro; obecnie każde osiedle ma sześć platform, usługi znajdują się na dachach, a bak ma 200 jednostek.

## Układ

Obszar X/Z wynosi 40000 × 13000 cm (wcześniej 20000 × 6500): dwukrotnie szerszy i wyższy, czterokrotnie większa powierzchnia. Granice: X −15000…25000, Z 0…13000. Środek krzyża: X 5000, Z 6500.

| Osiedle | Przystanki gracza | Paliwo | Warsztat |
|---|---|---|---|
| Lewo–góra: Ashline | Ashline Market, Zenith Spire, Nightshift Square | Ashline Charge | Nightshift Repair |
| Prawo–góra: Orbital | Glassward Transit, Cobalt Heights, Orbital Gardens | Glassward Fuel | — |
| Lewo–dół: Yellow | Yellow Projects, Midtown Exchange, Skyline Terraces | Midtown Fuel | — |
| Prawo–dół: Foundry | Neon Docks, Rainline Bazaar, Foundry Yards | Rainline Energy | Foundry Bodyworks |

Każde osiedle ma osobny peron ambientowych NPC. Miejsca podejmowania i oddawania graczy-pasażerów leżą na przeciwnych końcach szerokich platform. Mike i Jack dostali osobne place, bez triggerów pasażera. Mike: lewo–góra; Jack: prawo–dół. Wejście do Nightshift znajduje się przy Mike'u; wnętrze przeniesiono poza powiększony obszar gry. Pozycja PlayerStart i światła poziomu zostały zachowane.

Stare 23 aktory geometrii areny, platform, bramek i oznaczeń usunięto z `FlightLab.umap`, zastępując je generowaną geometrią miasta. Poprzedni układ można odzyskać z Git. Tło obejmuje cały nowy obszar. Projekt Godot nie został zmieniony.

## Ruch

- 40 samochodów: 16 na dwóch kierunkach obwodnicy, 12 na pętlach autostrad krzyża i 12 na czterech trasach osiedlowo-autostradowych.
- 8 pieszych: wychodzą z budynków, wsiadają, wysiadają i wracają do budynków. Nie blokują fizycznie gracza.
- Te same auta obsługują perony, rozpędzają się na wspólnych pasach, wracają przez obwodnicę i zjeżdżają na osiedle. Brak teleportowania podczas przejazdu. To autorowane, ciągłe pętle, a nie swobodne wyszukiwanie trasy po grafie całego miasta.
- Autostrady: do 1400 cm/s. Lokalne odcinki: 450–800 cm/s; węzły posiadają edytowalny `SpeedLimit` dla odcinka wejściowego.
- Skrzyżowanie centralne: 16 s przejazdu poziomego, 4 s opróżniania, 16 s pionowego, 4 s opróżniania. Czas jest wspólny dla wszystkich tras. Widoczna informacja nad skrzyżowaniem.
- Czujnik przeszkód ogranicza sprawdzanie do punktu zatrzymania przy peronie/czerwonym świetle; pieszy czekający dalej nie uniemożliwia dojazdu do drzwi.
- Pieszy oczekujący na dokładnie ten przystanek jest dozwolonym celem podjazdu, nie blokadą drogi. Pozostali, w tym idący piesi, nadal zatrzymują auto.
- Dojścia i odejścia pieszych mają rozdzielone pasy Y −75/+75 cm na szerokim peronie. Eliminuje to wzajemną blokadę dwóch przeciwnie idących osób; auta nadal latają w płaszczyźnie Y=0.
- Oczekiwanie na podjazd rozpoczyna się w promieniu 180 cm wzdłuż ścieżki od punktu wejścia. Dwie osoby mogą dołączyć do kolejki bez prób zajmowania dokładnie tej samej pozycji. Pojazd pozostaje przypisany do peronu do faktycznego odjazdu na 300 cm; stojące auto może przyjąć spóźnionego pasażera także po upływie nominalnego postoju.
- Stary ruch liniowy nadal wyłączony.

## Ekonomia i wytrzymałość

- Kurs lokalny: dotychczasowa stawka 1,10 kredytu/m postępu do celu.
- Między osiedlami: stawka ×1,5, czyli 1,65 kredytu/m; opłata początkowa pozostaje 20. Premia obejmuje wycenę oferty i rzeczywisty licznik. Stawka jest ustalana przy podjęciu pasażera.
- Maksymalnie 6 oczekujących ofert, 4 początkowe, ważne 60–100 s. W taksówce nadal tylko jeden pasażer.
- Próg pełnych obrażeń: 1400 → 2000 cm/s zmiany prędkości normalnej do uderzenia. Próg bezpiecznego kontaktu 700 i kwadratowy przebieg pozostają. Uderzenie 1400 powoduje teraz około 29 zamiast 100 punktów obrażeń. Liczba punktów hull pozostaje 100.
- Spalanie pozostaje na obniżonych wcześniej wartościach 1,35 / 0,675 jednostki/s; nie zwiększono prędkości ani siły ciągu gracza.
- Stare rekordy Time Attack nie zostały skasowane; po zmianie mapy i ekonomii nie są miarodajnym porównaniem balansu.

## Iteracja w Unreal

- `Content/Data/DA_FlyingCabCityLayout`: osiedla, przystanki, nazwy i usługi. Wartości domyślne nowego układu są w konstruktorze C++.
- `Content/Data/DA_FlyingCabLivingWorldProfile`: zapisane trasy, liczebność, prędkości, punkty, postoje i sygnalizacja. To aktywny profil miasta. Aktory `FlyingCabLivingRoute` ręcznie ustawione w poziomie nadal mają pierwszeństwo nad profilem.
- `Content/Data/DA_FlyingCabEconomy`: `InterNeighborhoodFareMultiplier`.
- Blueprint pojazdu → `Flying Cab / Damage / DamageFullHullSpeed`.
- `scripts/Update-MetroCity.py` to jawne narzędzie autorowania, nie migracja uruchamiana przy starcie. Nadpisuje układ i profil domyślnym wariantem; nie uruchamiać po własnych zmianach bez świadomej decyzji. Po przesunięciu osiedli trzeba odpowiednio poprawić zapisane trasy albo świadomie je wygenerować ponownie.
- Geometria miasta powstaje przy uruchomieniu poziomu, jak poprzednia rozbudowa. Do oglądania całego układu użyj trybu obserwatora.

## Weryfikacja

`FlyingCab.Core.City.MetroContract`: powierzchnia ×4, usługi na osiedlach, premie taryfowe, fazy skrzyżowania, populacja, wytrzymałość.

`FlyingCab.Functional.PIE.MetroTrafficFlow`: pełna bryła auta na próbkowanej trasie względem rzeczywistej geometrii; 180 sekund czasu gry, wszystkie pojazdy jadą, postoje nie przekraczają 45 s, piesi wsiadają i wysiadają.

`FlyingCab.Functional.PIE.GameplayComfort`: faktyczne wartości Blueprint/live pawn, piesi, oznaczenia usług; opcjonalny render minimapy i miasta z `-FlyingCabCaptureMinimap` (bez `-NullRHI`).

Wyniki: build `FlyingCabFlightLabEditor Win64 Development`, UE 5.8 — sukces. Pełny pakiet **35/35** (18 Core + 17 Functional PIE), proces zakończony kodem 0: `Saved/Logs/MetroQueueVerifiedTests.log`. Test ruchu przeszedł 180 sekund czasu gry przy przyspieszeniu ×4: wszystkie 40 aut przejechało ponad 100 m, żadne nie utknęło na ponad 45 s, a wszyscy ośmiu piesi wsiedli i ukończyli przynajmniej jeden kurs. Próbkowanie bryły auta nie wykazało kolizji tras z geometrią.

Wcześniejsze przebiegi wykryły rzeczywiste problemy skalowania: za niski prześwit pod podstawami platform, patrzenie czujnika za przystanek, blokujące się przeciwne potoki pieszych oraz kolejkę próbującą zająć jeden dokładny punkt. Poprawiono geometrię, semantykę przystanku i ścieżki; nie zwiększano limitu 45 s, żeby ukryć zatory. Ostatni test sprawdza każdego pieszego, nie tylko sumaryczny licznik kursów.

Kontrola manifestu wejścia: 29 zgodnych plików oraz trzy przejrzane różnice ograniczone do spalania, wytrzymałości i wyglądu HUD. Wariant `UseControlFrame=0` bez zmian. Ostrzeżenia buildu dotyczą dotychczasowego MSVC Preview nowszego od zalecanego przez UE oraz przestarzałego API w nagłówku silnika; nie zmieniano toolchainu ani wersji UE.

Końcowy test wizualny `GameplayComfort` z renderowaniem poza ekranem również przeszedł, kod 0: `Saved/Logs/MetroFinalVisual.log`. Obejrzano render minimapy i całego miasta. Pliki lokalne: `Saved/Automation/ComfortMinimap.png` oraz `MetroOverview.png`. Te testy nie zastępują oceny tempa i wrażeń z lotu przez użytkownika.

## Pliki tego etapu

- Poziom: `Content/Maps/FlightLab.umap`; nowy zapisany profil `Content/Data/DA_FlyingCabLivingWorldProfile.uasset`.
- Układ i geometria: `FlyingCabCityData.h/.cpp`, `FlyingCabCityLayoutAsset.h/.cpp`, `FlyingCabCityExpansion.h/.cpp`, `FlyingCabQuestHubData.cpp`, `FlyingCabWorldBootstrap.h`.
- Ruch: `FlyingCabLivingWorldTypes.h`, `FlyingCabLivingWorldProfile.h/.cpp`, `FlyingCabLivingWorldManager.cpp`, `FlyingCabLivingPedestrian.cpp`, `FlyingCabTrafficVehicle.h/.cpp`; nowe `FlyingCabMetroRoutes.cpp`, `FlyingCabTrafficSignals.h`.
- Taryfy i wytrzymałość: `FlyingCabDispatchComponent.h/.cpp`, `FlyingCabEconomyAsset.h/.cpp`, `FlyingCabPawn.h`, `FlyingCabVehicleVitalsComponent.h`.
- Prezentacja: `FlyingCabTouchControls.cpp`.
- Weryfikacja: `FlyingCabCoreTests.cpp`, `FlyingCabFunctionalTests.cpp`, `FlyingCabGameplayComfortTests.cpp`; nowy `FlyingCabMetroTests.cpp`.
- Narzędzie: `scripts/Update-MetroCity.py`.
- Dokumentacja: ten plik, `INPUT_CANONICAL_BASELINE.md`, `AUDIT_IMPLEMENTATION_STATUS.md`, `FLIGHT_FEEL_TEST.md`.

Pliki źródłowe powyżej znajdują się w `Source/FlyingCabFlightLab`. Pozostałe zastane zmiany diagnostyki, raportów, wyglądu pieszych i wcześniejszego obniżenia spalania zachowano. Żadnych commitów ani push; produkty buildu i logi pozostają lokalne i ignorowane przez Git.

## Zator na skrzyżowaniu i przeciśnięcie — 2026-09-10

Podczas audytu projektu `FlyingCab.Functional.PIE.MetroTrafficFlow` raz zgłosił kolumnę 12 aut na pionowym pasie X ≈ −13 614 (Z 5 998–7 436). Czoło kolumny stało w `WaitingForObstacle` bez przeszkody z czujnika: jego kadłub był zaklinowany z autem jadącym w poprzek, którego czujnik nie obejmuje (zaczyna 155 cm przed autem), a auto poprzeczne widziało czoło czujnikiem i też czekało. Zmiany w `FlyingCabTrafficVehicle`:

- `GetLastObstacleDescription()` opisuje przeszkodę z czujnika, aktora i komponent blokujący sweep ruchu, czas oczekiwania i liczbę przeciśnięć; test Metro dopisuje to jako `Obstacle detail` i podsumowuje `Gridlock creeps during the session`.
- Gdy sweep ruchu blokuje inny pojazd NPC, czujnik nic nie widzi i oczekiwanie trwa ≥ `GridlockCreepAfterSeconds` (10 s), auto przesuwa się bez sweepu z prędkością ≤ `GridlockCreepSpeed` (150 cm/s), aż droga będzie czysta; zdarzenie jest logowane jako `Warning`. Reguła nie dotyczy gracza, pieszych ani geometrii; kolejka za autem na przystanku widzi je czujnikiem, więc nie przeciska się.
- Przyczyna źródłowa (brak arbitrażu na skrzyżowaniach bez sygnalizacji) pozostaje otwarta; szczegóły i wynik weryfikacji: `docs/AUDYT_PROJEKTU_2026-09-10.md` w katalogu głównym repo, sekcja 8.4.
