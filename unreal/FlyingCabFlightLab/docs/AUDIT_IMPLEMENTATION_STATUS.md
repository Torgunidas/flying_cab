# Status wdrożenia audytu architektury

Dokument towarzyszy raportowi `AUDYT_UNREAL_FLYINGCABFLIGHTLAB_2026-08-10.md`. Raport pozostaje historycznym opisem stanu z dnia audytu; poniższa tabela opisuje aktualny projekt po kolejnych etapach refaktoryzacji.

## Ustalenia

Rozbudowa miasta ×4, taryfy między osiedlami, wytrzymałość i problemy ruchu wykryte w testach są opisane w [METRO_CITY.md](METRO_CITY.md). Nie jest to zmiana kanonicznego sterowania ani zamknięcie wszystkich pozostałych punktów audytu.

| ID | Status | Stan obecny |
|---|---|---|
| F-01 | zakończone | Komunikaty gracza trafiają do trwałego panelu HUD przez `ShowEventMessage`; debug overlay pozostał wyłącznie dla telemetrii `F3`. |
| F-02 | zakończone | `UFlyingCabFleetComponent` rejestruje każdy pojazd i rozróżnia recovery pojazdu aktywnego od zaparkowanego. |
| F-03 | zakończone | Interactables i pojazdy są odkrywane do cache co 1 s, a kontekst interakcji odświeżany co 0,2 s zamiast skanowania świata co klatkę. |
| F-04 | zakończone | World bootstrap, dispatch, ekonomia, fleet, run/save, HUD, traffic awareness i vitals zostały wydzielone z GameMode do wyspecjalizowanych komponentów/aktora. |
| F-05 | zakończone | Rozgrywka używa assetów Enhanced Input z `Content/Input`; legacy mappings zostały usunięte. Focus flush synchronicznie czyści także stan przechowywany przez Pawna. |
| F-06 | zakończone | W Metro City wszystkie granice wynikają z bounds układu; usunięto starą wschodnią ścianę i heurystykę jej otwierania. |
| F-07 | zakończone | Osiedla, przystanki, usługi i bounds są w `UFlyingCabCityLayoutAsset`; aktywne trasy i populacja w `UFlyingCabLivingWorldProfile`, generowane na podstawie tego układu i zapisane do dalszej edycji. |
| F-08 | zakończone | HUD jest próbkowany timerem 10 Hz, a widgety porównują nowe wartości przed `SetText`/zmianą widoczności. |
| F-09 | oczekuje pomiaru | Nie zmieniono stylistyki świateł bez danych. Potrzebny jest profil GPU na fizycznym urządzeniu mobilnym i test A/B lokalnych świateł zgodnie z raportem. |
| F-10 | zakończone | PlayerController tworzy i posiada jeden trwały HUD niezależny od aktualnie posiadanego pojazdu lub postaci. |
| F-11 | zakończone | Pakiet Automation obejmuje 35 wariantów (18 Core + 17 Functional PIE): wejście, dwa seedy soak, diagnostykę, GameplayComfort oraz kontrakt i dłuższy test ruchu Metro City. Ostatni pełny przebieg 35/35 — `MetroQueueVerifiedTests.log`, 2026-09-04. |
| F-12 | zakończone | `UFlyingCabScoreSaveGame` zawiera jawne `SaveVersion`. |
| F-13 | zakończone | `FLIGHT_FEEL_TEST.md` opisuje aktualne miasto, tryby, flotę, HUD, Enhanced Input i zatwierdzony kadr kamery. |
| F-14 | zakończone | Menu otrzymuje skonfigurowany próg Time Attack; wartość pochodzi z assetu ekonomii. |
| F-15 | zakończone | Time Attack używa stałego seedu, a Freeroam rozpoczyna nową losową sekwencję ofert. |
| F-16 | zakończone | Stacje używają zdarzeń Begin/End Overlap i tickują tylko podczas obecności obsługiwanego pojazdu. |
| F-17 | zakończone | Look-ahead kamery jest zwykłym polem danych `CameraTrackingOffset`; Pawn nie posiada pozornej kamery ani SpringArma. |
| F-18 | świadomie odłożone | Pełna lokalizacja tekstów pozostaje etapem stabilizacji UX. Nowy tekst gracza powinien być tworzony jako `FText`, bez utrwalania logiki na porównaniach z przetłumaczonym tekstem. |
| F-19 | checkpoint kodu zapisany | Commit `771e1fa` („sterowanie DZIALA!”) obejmuje zaakceptowaną wersję na `codex/unreal-audit-fixes`. Nowe oznaczenie kanoniczne i dokumentacja pozostają lokalne do zatwierdzenia przez użytkownika; Codex nie wykonał commita/tagu/push w tym etapie. |

## Otwarte prace

1. F-09 wymaga urządzenia mobilnego; wyniku nie da się wiarygodnie zastąpić testem desktopowym ani `NullRHI`.
2. F-18 należy rozpocząć dopiero po ustabilizowaniu treści i języków docelowych.
3. Etap 5 nie oznacza automatycznego portowania dalszych funkcji Godota. Kontrakty i kolejność decyzji znajdują się w `GODOT_MIGRATION_CONTRACTS.md`.

## Audyt blokowania sterowania z 2026-09-03

**Aktualny punkt odniesienia:** `input-canonical-2026-09-04`, commit `771e1fa` („sterowanie DZIALA!”), `flyingcab.UseControlFrame=0`. Użytkownik potwierdził długą sesję bez problemów; log sesji potwierdza wariant `0`. Kontrakt i kontrola zmian: [INPUT_CANONICAL_BASELINE.md](INPUT_CANONICAL_BASELINE.md). Opisy wcześniejszych zgłoszeń poniżej pozostają historią, nie otwartym poleceniem przebudowy sterowania.

| Etap | Status | Stan obecny |
|---|---|---|
| Dowód regresyjny | zakończone | Testy `DeferredInputTransitions` i poprawiony `InputTransitionChain` utrzymują dziennik przez wiele klatek oraz sprawdzają Q/J/R przy aktywnym ruchu. |
| Containment | zakończone | Q/J/R zapisują żądania, a przejścia wykonywane są w `PlayerTick` po przetworzeniu Enhanced Input. |
| Ramka sterowania — pojazd | eksperymentalne, domyślnie wyłączone | Po zgłoszeniu blokady po recovery przywrócono domyślnie `flyingcab.UseControlFrame 0` (wartości Enhanced Input pobierane przez Pawn). Komponent pozostaje dostępny pod `1`; zeruje wartość akcji bez aktywnego mapowania przez dwie klatki jako `STALE_ACTION_VALUE`. Przyczyna zgłoszenia nie została odtworzona. |
| Ramka sterowania — pieszy | oczekuje | Postać piesza nadal odczytuje wartości Enhanced Input we własnym ticku. |
| Ramka sterowania — dotyk | oczekuje | Widget i pawny nadal przechowują osobne stany dotykowe. |
| Pełna telemetria i soak | częściowo | Soak (2 × 3000 kroków z wejściem pojazdu/pieszego/dotyku i recovery), test paliwa oraz osobny pasywny obserwator z buforem 5 s, focus/trybami, powodami odcięcia, detektorami i zrzutami są wdrożone. Pełny prywatny stan pieszego/widgetu, transport Parsec i rzeczywisty wielodotyk pozostają poza tym etapem. |
| Gamepad | oczekuje | Mapowania i dead zone zostaną dodane po usunięciu starych ścieżek. |

### Co jeszcze pozostało z audytu Claude'a

1. **Testy bez zmiany sterowania — wykonane:** `InputSoak` z dwoma powtarzalnymi seedami, modelem oczekiwanego wejścia, repeat, Q/J/R/O, symulowanym focus, dotykiem i recovery; CSV do odtworzenia błędu. Osobno `FuelCutoffWhileHoldingThrust` sprawdza żądanie wejścia, faktyczną siłę, komunikat pustego baku i wznowienie po tankowaniu. Szczegóły: [INPUT_RELIABILITY_TESTS.md](INPUT_RELIABILITY_TESTS.md).
2. **Pasywna diagnostyka — wdrożona w uzgodnionym zakresie:** [INPUT_PASSIVE_DIAGNOSTICS.md](INPUT_PASSIVE_DIAGNOSTICS.md). Osobny subsystem, bufor 5 s, focus/input mode, triggery/kontekst/service, jawne powody odcięcia i detektory zapisujące plik, bez korekty sterowania. Pełny prywatny stan pieszego/widgetu nie został dodany, aby nie zmieniać chronionej ścieżki. `STALE_ACTION_VALUE` nadal koryguje wyłącznie eksperymentalną ramkę; nowy `ACTION_WITHOUT_MAPPED_KEY` tylko obserwuje. `RAW_KEY_NO_REPEAT` nie jest dowodem puszczenia i nie może automatycznie odcinać ciągu.
3. **Architektura — odłożona za zgodą na osobny eksperyment:** wspólna ramka pojazd/pieszy/dotyk, pojedynczy stan trybu zamiast równoległych flag, observer przez akcje, jawny powód blokady napędu. Nie przełączamy kanonicznego wariantu tylko dlatego, że taki był docelowy plan audytu.
4. **Dopiero po zatwierdzeniu następcy:** usunięcie starych ścieżek, dodatkowych flushów i CVaru migracyjnego; gamepad/remapping i test na rzeczywistym urządzeniu mobilnym. Obecny etap containment odroczył Q/J/R, ale NIE zrealizował zalecenia usunięcia flushu z `ClearAllInputState` — zachowujemy sprawdzony wariant.

Udana sesja ręczna, soak i pasywna diagnostyka nie ustalają przyczyny historycznej blokady po recovery. Zmiana działającej ścieżki nadal wymaga osobnego uzgodnienia. Z wcześniejszego ogólnego audytu nadal odłożone są pomiary GPU na urządzeniu mobilnym (F-09) i lokalizacja (F-18).

### Komfort rozgrywki po oblotach — 2026-09-04

- Ambientowy pieszy otrzymał sylwetkę identyczną z wizualizacją pasażera taksówki gracza (cylinder + głowa, te same meshe i skale). Jego kapsuła jest `QueryOnly`, ignoruje kanał gracza i nie blokuje ruchomych pojazdów. Nadal działa jako obiekt wykrywany przez czujniki ruchu NPC; po wysiadaniu i wyjściu z budynku nie wraca już fizyczna kolizja.
- Spalanie obu osi obniżone o **25%**: pion 1,8 → 1,35, poziom 0,9 → 0,675 jednostki/s. Startowy bak 65%, ceny, regeneracja, siła, prędkość i sterowanie bez zmian. Sam ciąg pionowy na startowym baku wystarcza teoretycznie na około 48 zamiast 36 s; nie jest to obietnica długości rzeczywistego kursu.
- Minimapa: kontrastowe, nieinteraktywne etykiety `FUEL` (zielone, 36×20) i `REPAIR` (fioletowe, 50×20) na nieprzezroczystym tle. Są odsunięte nad właściwy pin i połączone krótką linią; żółte oferty pasażerów pozostają osobnymi znacznikami.
- Build UE 5.8 Win64 Development poprawny; **33/33**, kod 0: `Saved/Logs/GameplayComfortFullTests.log`. Obejmuje istniejący `LivingWorldCycle`, dwa seedy soak oraz nowy `GameplayComfort`: rzeczywiste wartości Blueprinta/instancji, spalanie, board/exit, brak blokady w sweepie gracza, widoczność w zapytaniu czujnika, zgodność sylwetek i etykiety HUD.
- Osobny test z renderowaniem poza ekranem przeszedł: `Saved/Logs/GameplayComfortVisualTests.log`. Obejrzano `Saved/Automation/ComfortMinimap.png` z ofertami przy stacjach. To render widgetu Unreal, nie makieta; nie zastępuje oceny całej sceny podczas gry przez Parsec.
- Historyczny manifest pozostaje bez zmian. Trzy oczekiwane różnice przejrzane i opisane w `INPUT_CANONICAL_BASELINE.md`: tylko wartości zasobów w `FlyingCabPawn.h` / `FlyingCabVehicleVitalsComponent.h` oraz fragment budowy minimapy w `FlyingCabTouchControls.cpp`; pozostałe 29 pozycji zgodne. Żaden handler ani kolejność wejścia nie zostały zmienione.
- Pliki etapu: `FlyingCabLivingPedestrian.h/.cpp`, `FlyingCabPawn.h`, `FlyingCabVehicleVitalsComponent.h`, `FlyingCabTouchControls.cpp`, nowy `FlyingCabGameplayComfortTests.cpp`, `FLIGHT_FEEL_TEST.md`, `docs/INPUT_CANONICAL_BASELINE.md` i ten status. Brak zmian map/assetów, Godota, commita/tagu/push. Wcześniejsze lokalne zmiany zachowano.

### Weryfikacja pasywnej diagnostyki — 2026-09-04

- Build UE 5.8, `FlyingCabFlightLabEditor Win64 Development`: sukces. Pełny pakiet **32/32**, kod procesu 0: `Saved/Logs/PassiveDiagnosticsVerifiedTests.log`.
- Test PIE potwierdził zapis KEY, siły, pauzy/dziennika, pliku przed incydentem i recovery. Celowe wstrzyknięcie akcji bez klawisza wygenerowało raport, ale NIE zostało wyzerowane przez diagnostykę. Przełączanie diagnostyki 0 → 1 przy trzymanym W nie przerwało ciągu. W tym przebiegu raport akcji bez klawisza wystąpił tylko w celowym fixture; nie w soak.
- Sprawdzono zawartość plików w `Saved/Logs/InputDiagnostics`, w tym `NoFuel`, cache klawiatury/dotyku, focus, recovery i jawny brak aktualnego dowodu siły w przejściu. Katalog jest ignorowany przez Git. Wszystkie 32 pliki manifestu kanonicznego nadal zgodne, bez zmiany sum.
- Przy przygotowaniu poprawiono nową diagnostykę: sygnały klawiszy mogą docierać, gdy `GWorld` wskazuje edytor; przypisanie jest teraz dozwolone tylko dla jednego lokalnego świata gry. Poprawiono też asercje testów: dziennik może mieć strażnik `Transition`, a znacznik zdarzenia `second` kolidował z nagłówkiem `history_seconds`. Wcześniejsze nieudane przebiegi `PassiveDiagnosticsTests1.log` i `PassiveDiagnosticsFinalTests.log` nie oznaczały regresji sterowania.
- Nowe pliki: `FlyingCabInputDiagnosticsBuffer.h`, `FlyingCabInputDiagnosticsSubsystem.h/.cpp`, `FlyingCabInputDiagnosticsTests.cpp`, `docs/INPUT_PASSIVE_DIAGNOSTICS.md`. Rozszerzone: `FlyingCabInputReliabilityTests.cpp`, `docs/INPUT_RELIABILITY_TESTS.md`, ten status. Bez zmian kanonicznych źródeł wejścia, konfiguracji, assetów, map, Godota, commita/tagu/push.
- To testy syntetyczne z `NullRHI`, nie nowa ręczna akceptacja przez Parsec. Nie przełączono wariantu kanonicznego ani nie uznano całego audytu za zakończony.

### Weryfikacja nowych testów niezawodności — 2026-09-04

- UE 5.8, `FlyingCabFlightLabEditor Win64 Development`: build poprawny. Kanoniczna ścieżka `UseControlFrame=0` pozostaje niezmieniona.
- Pełny pakiet **29/29**, kod procesu 0: `Saved/Logs/CanonicalReliabilityFinalTests.log`.
- Soak: seedy 1977 i 9042026, po 3000 kroków plus neutralizacja końcowa; łącznie m.in. 110 otwarć dziennika, 58 zmian possession, 41 resetów i 6 zniszczeń/recovery. Zdarzenia/model/wyniki/siła były identyczne w dwóch wykonaniach obu seedów po pominięciu globalnego numeru klatki i czasu logów.
- Kontrola negatywna: osobny proces z `-FlyingCabSoakDropRelease` celowo pominął po jednym release; oba seedy wykryły błąd (kroki 57 i 27), oczekiwany niezerowy kod: `Saved/Logs/CanonicalReliabilityNegativeControl.log`. To kontrolowana awaria fixture, nie regresja gry.
- Podczas przygotowania poprawiono wyłącznie test: synchroniczny odbiór telemetrii na wątku gry, aktualizowanie poprzedniej oczekiwanej klatki ruchu również w przejściach oraz oczekiwanie na kolejkę komunikatów HUD. Pierwsze nieudane przebiegi `CanonicalReliabilityTests1/2.log` nie dowodzą regresji sterowania.
- Kontrola wersji kanonicznej: wszystkie 32 pliki zgodne. Zmiany tego etapu: nowy `FlyingCabInputReliabilityTests.cpp`, `docs/INPUT_RELIABILITY_TESTS.md`, aktualizacja tego statusu i link w dokumencie wersji kanonicznej. Bez zmian assetów, map, źródeł produkcyjnego wejścia, commita, tagu i push. Nowe testy pozostają lokalne do zatwierdzenia przez użytkownika.

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

## Audyt projektu — 2026-09-10 (Mac/PC, repo, supercar)

Raport: `docs/AUDYT_PROJEKTU_2026-09-10.md` w katalogu głównym repo (sekcja 8 opisuje wdrożone poprawki i przekazanie dla Codexa). Przyczyna braku supercara na Macu: po pullu edytor ładował moduł skompilowany przed zmianami, bez ostrzeżenia. Kod supercara kompiluje się na macOS i przechodzi test `FlyingCab.Functional.PIE.Supercar`.

| ID | Status | Stan obecny |
|---|---|---|
| A-01 | zakończone | `scripts/sync-mac.sh` i `scripts/Sync-Windows.ps1` (pull + build); ostrzeżenie edytora o źródłach nowszych niż binarka w module `FlyingCabNarrativeEditor`; linia `A_R7 supercars parked` w bootstrapie; zasady w README, `WORKING_ON_MAC_AND_PC.md` i `AGENTS.md`. Skrypt Windows i ostrzeżenie edytora czekają na sprawdzenie na Windows. |
| A-02 | zakończone (zasada) | Reguła „build i testy na obu systemach albo jawny zapis, który system czeka” dodana do głównego `AGENTS.md`. |
| A-03 | zakończone | `scripts/verify-input-baseline.py`; wynik zgodny ze skryptem PowerShell (23 zgodne, 9 historycznie zmienionych). |
| A-04 | zakończone | Alias Findera usunięty z indeksu Git, `*-alias` w `.gitignore`. Plik lokalny pozostał. |
| A-05 | zakończone | Jawne reguły `binary` dla png/psd/gif/ase/aseprite/ai/eps/pdf/fbx/blend/otf/ttf/mp3/wav/ogg/zip/7z/pck/exe/dll oraz `*.py text eol=lf`. Bez zmian blobów; `git status` po zmianie czysty. |
| A-06 | do wykonania lokalnie | Na Macu: `git config --global --unset-all filter.lfs.clean` itd. albo `brew install git-lfs`. Nie zmieniano konfiguracji użytkownika. |
| A-07 | decyzja użytkownika | Gałęzie zdalne: `codex/unreal-audit-fixes` scalona; `codex/evaluate-vehicle-control-system-redesign`, `not_stable`, `working-dialog-system` niescalone (era Godota). |
| A-08 | zasada | Wyniki testów zapisywać w docs (data, maszyna, liczby); ścieżki `Saved/` nie są współdzielone. |
| A-09 | zakończone | Patrz A-01 (linia logu bootstrapu, ostrzeżenie edytora). |
| A-10 | wdrożone, do weryfikacji na Windows | Soak wykonuje cykl rozgrzewający `Q`/`Q` przed krokiem 1 (spawn pieszego poza pomiarem z zimnym cache); karencja przejść i sekwencja seedu bez zmian. Ścieżka wejścia gry nietknięta. |
| A-11 | wdrożone częściowo, do weryfikacji na Windows | Przyczyna: zaklinowanie kadłubów na skrzyżowaniu w martwej strefie czujnika (155 cm). Dodano diagnostykę blokera ruchu (`GetLastObstacleDescription`, `Obstacle detail` w teście Metro) i przeciśnięcie po 10 s wyłącznie przez inny pojazd NPC (`GridlockCreepAfterSeconds`, `GridlockCreepSpeed`). Arbitraż skrzyżowań pozostaje otwarty. |

Pierwsza tura poprawek (skrypty, ostrzeżenie edytora, Git) była tylko kompilowana na macOS, bez testów (polecenie użytkownika). Druga tura (A-10, A-11): build macOS bez ostrzeżeń, uruchomiono tylko testy objęte zmianami (`InputSoak` oba seedy i `MetroTrafficFlow`): 3/3 zaliczone, 0 przeciśnięć (`Saved/Logs/AuditMacA10A11_2026-09-10.log`). Pełny pakiet i Windows pozostają do wykonania.
