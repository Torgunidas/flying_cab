# Status wdrożenia audytu architektury

Dokument towarzyszy raportowi `AUDYT_UNREAL_FLYINGCABFLIGHTLAB_2026-08-10.md`. Raport pozostaje historycznym opisem stanu z dnia audytu; poniższa tabela opisuje aktualny projekt po kolejnych etapach refaktoryzacji.

## Ustalenia

| ID | Status | Stan obecny |
|---|---|---|
| F-01 | zakończone | Komunikaty gracza trafiają do trwałego panelu HUD przez `ShowEventMessage`; debug overlay pozostał wyłącznie dla telemetrii `F3`. |
| F-02 | zakończone | `UFlyingCabFleetComponent` rejestruje każdy pojazd i rozróżnia recovery pojazdu aktywnego od zaparkowanego. |
| F-03 | zakończone | Interactables i pojazdy są odkrywane do cache co 1 s, a kontekst interakcji odświeżany co 0,2 s zamiast skanowania świata co klatkę. |
| F-04 | zakończone | World bootstrap, dispatch, ekonomia, fleet, run/save, HUD, traffic awareness i vitals zostały wydzielone z GameMode do wyspecjalizowanych komponentów/aktora. |
| F-05 | zakończone | Rozgrywka używa assetów Enhanced Input z `Content/Input`; legacy mappings zostały usunięte. Focus flush synchronicznie czyści także stan przechowywany przez Pawna. |
| F-06 | zakończone | Granica mapy używa tagu `EastBoundary`; heurystyka pozostała wyłącznie jako głośno logowany fallback dla starych map. |
| F-07 | zakończone | Dzielnice, stacje, trasy i bounds minimapy mają jedno źródło w `UFlyingCabCityLayoutAsset`. |
| F-08 | zakończone | HUD jest próbkowany timerem 10 Hz, a widgety porównują nowe wartości przed `SetText`/zmianą widoczności. |
| F-09 | oczekuje pomiaru | Nie zmieniono stylistyki świateł bez danych. Potrzebny jest profil GPU na fizycznym urządzeniu mobilnym i test A/B lokalnych świateł zgodnie z raportem. |
| F-10 | zakończone | PlayerController tworzy i posiada jeden trwały HUD niezależny od aktualnie posiadanego pojazdu lub postaci. |
| F-11 | zakończone | Pakiet Automation obejmuje logikę domenową i przepływy PIE; zawiera 26 testów (15 Core + 11 Functional PIE), stan na 2026-09-04. |
| F-12 | zakończone | `UFlyingCabScoreSaveGame` zawiera jawne `SaveVersion`. |
| F-13 | zakończone | `FLIGHT_FEEL_TEST.md` opisuje aktualne miasto, tryby, flotę, HUD, Enhanced Input i zatwierdzony kadr kamery. |
| F-14 | zakończone | Menu otrzymuje skonfigurowany próg Time Attack; wartość pochodzi z assetu ekonomii. |
| F-15 | zakończone | Time Attack używa stałego seedu, a Freeroam rozpoczyna nową losową sekwencję ofert. |
| F-16 | zakończone | Stacje używają zdarzeń Begin/End Overlap i tickują tylko podczas obecności obsługiwanego pojazdu. |
| F-17 | zakończone | Look-ahead kamery jest zwykłym polem danych `CameraTrackingOffset`; Pawn nie posiada pozornej kamery ani SpringArma. |
| F-18 | świadomie odłożone | Pełna lokalizacja tekstów pozostaje etapem stabilizacji UX. Nowy tekst gracza powinien być tworzony jako `FText`, bez utrwalania logiki na porównaniach z przetłumaczonym tekstem. |
| F-19 | zakończone | Stan projektu i każdy etap audytu są zapisane w osobnych commitach na gałęzi `codex/unreal-audit-fixes`. |

## Otwarte prace

1. F-09 wymaga urządzenia mobilnego; wyniku nie da się wiarygodnie zastąpić testem desktopowym ani `NullRHI`.
2. F-18 należy rozpocząć dopiero po ustabilizowaniu treści i języków docelowych.
3. Etap 5 nie oznacza automatycznego portowania dalszych funkcji Godota. Kontrakty i kolejność decyzji znajdują się w `GODOT_MIGRATION_CONTRACTS.md`.

## Audyt blokowania sterowania z 2026-09-03

| Etap | Status | Stan obecny |
|---|---|---|
| Dowód regresyjny | zakończone | Testy `DeferredInputTransitions` i poprawiony `InputTransitionChain` utrzymują dziennik przez wiele klatek oraz sprawdzają Q/J/R przy aktywnym ruchu. |
| Containment | zakończone | Q/J/R zapisują żądania, a przejścia wykonywane są w `PlayerTick` po przetworzeniu Enhanced Input. |
| Ramka sterowania — pojazd | eksperymentalne, domyślnie wyłączone | Po zgłoszeniu blokady po recovery przywrócono domyślnie `flyingcab.UseControlFrame 0` (wartości Enhanced Input pobierane przez Pawn). Komponent pozostaje dostępny pod `1`; zeruje wartość akcji bez aktywnego mapowania przez dwie klatki jako `STALE_ACTION_VALUE`. Przyczyna zgłoszenia nie została odtworzona. |
| Ramka sterowania — pieszy | oczekuje | Postać piesza nadal odczytuje wartości Enhanced Input we własnym ticku. |
| Ramka sterowania — dotyk | oczekuje | Widget i pawny nadal przechowują osobne stany dotykowe. |
| Pełna telemetria i soak | częściowo | Diagnostyka development zapisuje dostarczone zdarzenia klawiszy i zmiany stanu pojazdu wraz z siłą napędu. Pełny soak oraz pokrycie pieszego i dotyku nadal oczekują. |
| Gamepad | oczekuje | Mapowania i dead zone zostaną dodane po usunięciu starych ścieżek. |

### Weryfikacja etapu ramki pojazdu — 2026-09-04

- Build `FlyingCabFlightLabEditor Win64 Development`, UE 5.8: sukces.
- Nowa ścieżka przed zgłoszeniem recovery: **25/25** testów (15 Core + 10 PIE), log `Saved/Logs/ControlFrameFinalTests.log`.
- Przełącznik awaryjny `flyingcab.UseControlFrame 0` przed zgłoszeniem recovery: **24/24** istniejących testów, log `Saved/Logs/ControlFrameFallbackVerifiedTests.log`. Po późniejszym zgłoszeniu ustawiono `0` jako domyślne — patrz poniżej.
- Nowy test `FlyingCab.Core.Input.ControlFrameValidation` sprawdza dodatnią i ujemną zamrożoną wartość przez 1000 próbek, neutralizację ramki oraz ponowne uzbrojenie diagnostyki. Licznik ochronny jest nasycany na wartości 2 i nie może się przepełnić.
- Testy używają `NullRHI` i syntetycznych zdarzeń wejścia. Nie zastępują ręcznej sesji przez Parsec ani przyszłego randomizowanego soak testu.
- Odczyt pieszego i stany dotykowe pozostają celowo bez zmian. To pierwszy fragment etapu 2 audytu, nie zakończenie całej migracji.
- Zmiany tego etapu pozostają w working tree; nie wykonano commita ani push.

Pliki etapu: `FlyingCabControlInputComponent.cpp/.h` (nowe), `FlyingCabPlayerController.cpp/.h`, `FlyingCabPawn.cpp`, `FlyingCabCoreTests.cpp` oraz ten dokument. Wcześniejsze zmiany kamery, mapy i alarmów zasobów pozostają zachowane.

Przykład poprawnego uruchomienia testu fallbacku: `-ExecCmds="flyingcab.UseControlFrame 0,Automation RunTests FlyingCab;Quit"`. Przecinek rozdziela polecenie ustawienia CVaru od kolejki Automation; poprzednia próba ze średnikiem między nimi nie uruchomiła testów.

### Ponowne zgłoszenie: blokada thrust up po recovery — 2026-09-04

- Scenariusz użytkownika: test kontrolek paliwa i hull, zniszczenie pierwszego auta, recovery, następnie blokada ciągu pionowego. Kontrolki działają. Użytkownik doprecyzował: auto nadal ciągnęło w górę po puszczeniu przycisku, nie był to brak reakcji na wciśnięcie. To doprecyzowanie wcześniejszej sesji, nie zgłoszenie wyniku po przywróceniu poprzedniej ścieżki sterowania.
- Log sesji `Saved/Logs/FlyingCabFlightLab.log`: zniszczenie o 11:09:23, recovery o 11:09:25, wyczerpanie paliwa dopiero o 11:10:14 (czas lokalny). Brak `STALE_ACTION_VALUE`. Późniejszy brak paliwa nie wyjaśnia sam w sobie zgłoszenia tuż po recovery. Dawny log nie zawierał dostarczonych zdarzeń klawiszy, więc nie dowodzi fizycznego trzymania lub puszczenia przycisku.
- Tymczasowo wyłączono nową ramkę jako źródło sterowania pojazdem (`flyingcab.UseControlFrame 0`). Zachowano wcześniejszą poprawkę odraczania Q/J/R. To izolacja ostatniej zmiany, nie potwierdzona naprawa przyczyny.
- Poprawiono `ActiveTowRecovery`: fatalne uszkodzenie przechodzi przez rzeczywisty callback kolizji Pawna, a nie samo ręczne wywołanie zdarzenia floty pomijające czyszczenie inputu.
- Dodano `FlyingCab.Functional.PIE.RecoveryInputRelease`: trzy pełne cykle zniszczenia i timera recovery, W/Spacja/strzałka w górę oraz A/D/strzałka w lewo. Test obejmuje puszczenie w czasie wraku, auto-repeat podczas i po recovery, neutralizację po puszczeniu i ponowne naciśnięcie. Zdarzenia wejścia i uderzenie są syntetyczne; test nie symuluje transportu Parsec ani rzeczywistej kolizji Chaos.
- Pierwsza wersja testu wyłączała fizykę przez `NoCollision`, przez co nie wywoływała zniszczenia. Po naprawie izolacji testowej (ignorowanie kolizji bez wyłączania fizyki) test przeszedł z ramką `1`: `Saved/Logs/RecoveryInputReproduction2.log`. Nie odtworzono blokady.
- Dodano `flyingcab.InputTrace` (domyślnie `1`, wyłącznie development). Zapisuje tylko mapowane klawisze gry na wejściu kontrolera oraz zmiany raw/EI/ramki/cache Pawna/dotyku, blokad, fizyki i siły przekazanej napędowi. Powtórzenia aktualizują czas ostatniego zdarzenia, lecz logowany jest tylko pierwszy repeat w serii. Diagnostyka niczego nie neutralizuje ani nie wstrzykuje. Wyłączenie: `flyingcab.InputTrace 0`.
- `KEY Released` oznacza zdarzenie dostarczone do kontrolera, nie syntetyczny release wewnątrz `FlushPressedKeys`. Nadal nie jest to dowód bezpośredniego stanu fizycznej klawiatury klienta Parsec. Snapshot przy przejściu może zawierać siłę z poprzedniego ticku; paliwo jest próbkowane po rozliczeniu zużycia za tick.
- Weryfikacja końcowa: build UE 5.8 Win64 Development — sukces; **26/26** testów z domyślnym `UseControlFrame=0` (`Saved/Logs/RecoveryDefaultTraceVerifiedTests.log`) i **26/26** z eksperymentalnym `1` (`Saved/Logs/RecoveryExperimentalTraceTests.log`). Oba procesy zakończyły się kodem 0. W logach potwierdzono zapis wciśnięć/puszczeń i niezerowej/zerowej siły napędu. Pierwsze uruchomienie pełnego pakietu zatrzymało się przed testami na dostępie do cache Zen w sandboxie; powyższe wyniki pochodzą z ponownego, poprawnie uruchomionego procesu.
- Następna weryfikacja: ręcznie powtórzyć ten sam scenariusz przez Parsec na domyślnej ścieżce. Jeśli ciąg ponownie pozostanie aktywny po puszczeniu, odczytać nowy log i sprawdzić, czy kontroler otrzymał `KEY Released` oraz na którym etapie pozostała niezerowa wartość. Zielone testy syntetyczne nie zamykają tego zgłoszenia.
- Pliki tego zgłoszenia: `FlyingCabControlInputComponent.cpp/.h`, `FlyingCabPlayerController.cpp/.h`, `FlyingCabPawn.cpp/.h`, `FlyingCabFunctionalTests.cpp` i ten dokument. Wcześniejsze zmiany pozostawiono bez nadpisania; brak commita i push.
