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
| A-01 | zakończone | `scripts/sync-mac.sh` i `scripts/Sync-Windows.ps1` (pull + build); ostrzeżenie edytora o źródłach nowszych niż binarka w module `FlyingCabNarrativeEditor`; linia `A_R7 supercars parked` w bootstrapie; zasady w README, `WORKING_ON_MAC_AND_PC.md` i `AGENTS.md`. Skrypt Windows, ostrzeżenie edytora i linia bootstrapu sprawdzone na Windows 2026-09-10 (sekcja „Weryfikacja na Windows” niżej). |
| A-02 | zakończone (zasada) | Reguła „build i testy na obu systemach albo jawny zapis, który system czeka” dodana do głównego `AGENTS.md`. |
| A-03 | zakończone | `scripts/verify-input-baseline.py`; wynik zgodny ze skryptem PowerShell (23 zgodne, 9 historycznie zmienionych). |
| A-04 | zakończone | Alias Findera usunięty z indeksu Git, `*-alias` w `.gitignore`. Plik lokalny pozostał. |
| A-05 | zakończone | Jawne reguły `binary` dla png/psd/gif/ase/aseprite/ai/eps/pdf/fbx/blend/otf/ttf/mp3/wav/ogg/zip/7z/pck/exe/dll oraz `*.py text eol=lf`. Bez zmian blobów; `git status` po zmianie czysty. |
| A-06 | do wykonania lokalnie | Na Macu: `git config --global --unset-all filter.lfs.clean` itd. albo `brew install git-lfs`. Nie zmieniano konfiguracji użytkownika. |
| A-07 | decyzja użytkownika | Gałęzie zdalne: `codex/unreal-audit-fixes` scalona; `codex/evaluate-vehicle-control-system-redesign`, `not_stable`, `working-dialog-system` niescalone (era Godota). |
| A-08 | zasada | Wyniki testów zapisywać w docs (data, maszyna, liczby); ścieżki `Saved/` nie są współdzielone. |
| A-09 | zakończone | Patrz A-01 (linia logu bootstrapu, ostrzeżenie edytora). |
| A-10 | zakończone (Mac i Windows) | Soak wykonuje cykl rozgrzewający `Q`/`Q` przed krokiem 1 (spawn pieszego poza pomiarem z zimnym cache); karencja przejść i sekwencja seedu bez zmian. Ścieżka wejścia gry nietknięta. Windows 2026-09-10: oba seedy zaliczone po 3019 kroków w pełnym pakiecie. |
| A-11 | wdrożone częściowo (Windows zweryfikowane) | Przyczyna: zaklinowanie kadłubów na skrzyżowaniu w martwej strefie czujnika (155 cm). Dodano diagnostykę blokera ruchu (`GetLastObstacleDescription`, `Obstacle detail` w teście Metro) i przeciśnięcie po 10 s wyłącznie przez inny pojazd NPC (`GridlockCreepAfterSeconds`, `GridlockCreepSpeed`). Windows 2026-09-10: `MetroTrafficFlow` 4/4 zaliczone (pakiet i 3 powtórzenia), 0 przeciśnięć, bez wpisów `Obstacle detail`. Arbitraż skrzyżowań pozostaje otwarty. |

Pierwsza tura poprawek (skrypty, ostrzeżenie edytora, Git) była tylko kompilowana na macOS, bez testów (polecenie użytkownika). Druga tura (A-10, A-11): build macOS bez ostrzeżeń, uruchomiono tylko testy objęte zmianami (`InputSoak` oba seedy i `MetroTrafficFlow`): 3/3 zaliczone, 0 przeciśnięć (`Saved/Logs/AuditMacA10A11_2026-09-10.log`). Pełny pakiet i weryfikacja na Windows wykonane 2026-09-10 (sekcja niżej).

### Weryfikacja na Windows — 2026-09-10

Maszyna: Windows 11 Home (10.0.26200), UE 5.8.0 z `D:\Unreal\UE_5.8`, MSVC 14.51.36252 (Visual Studio 18 Insiders; UBT ostrzega, że to wersja niepreferowana), Windows SDK 10.0.28000.0, Windows PowerShell 5.1 (bez `pwsh`), Git 2.50. Stan repo: `main` = `origin/main` = `b4fc453`, drzewo czyste przed i po, `origin` bez nowych commitów. Wykonano punkty 1–4 z sekcji 8.3 raportu audytu oraz punkt „Windows” z sekcji 8.5. Nic nie commitowano.

| Krok | Wynik |
|---|---|
| `.\scripts\Sync-Windows.ps1 -EngineRoot 'D:\Unreal\UE_5.8'` | 4 uruchomienia w PowerShell 5.1, bez błędów PowerShell; skrypt nie wymagał poprawek. Wyjście: `Already up to date.`, `No new commits; HEAD stays at b4fc453.`, UBT `Result: Succeeded` (pierwszy build 18 akcji, 24,9 s; kolejne 4 akcje, 5 s), `Editor modules are current for b4fc453.`, kod wyjścia 0. Jedyne ostrzeżenia kompilatora: 3 × C4996 z nagłówka silnika `Character.h(798)` (`GetMovementBase` przestarzałe), nie z kodu projektu. |
| Pełny pakiet `Automation RunTests FlyingCab` (NullRHI) | `Saved/Logs/AuditWinFull_2026-09-10.log`, raport `Saved/Automation/AuditWinFull_2026-09-10/index.json`: 46 wykonanych, 46 zaliczonych (36 czysto, 10 z ostrzeżeniami), 0 niezaliczonych; 94,9 s testów, 141 s procesu. Soak: oba seedy po 3019 kroków, pierwszy wiersz CSV z pełnym zrzutem stanu taksówki. `MetroTrafficFlow` 45,2 s, `Gridlock creeps during the session: 0`. W całym logu 0 linii `Error:`; `Compiled modules are current` zalogowane także w trybie `-unattended`. |
| `MetroTrafficFlow` × 3 (osobne procesy) | `AuditWinMetro1_2026-09-10.log` … `AuditWinMetro3_2026-09-10.log`: 3/3 zaliczone, za każdym razem `Gridlock creeps during the session: 0`, bez `Traffic stalled` i `Obstacle detail`; 59 s na proces. Łącznie na Windows tego dnia 4/4. |
| Ostrzeżenie o starej binarce | Po symulacji pulla (niżej) edytor GUI (`UnrealEditor.exe`, D3D12) otwarty bez builda loguje po 12 s: `LogFlyingCabNarrativeEditor: Warning: Source is newer than the compiled game modules: FlyingCabWorldBootstrap.cpp changed 2026.09.10-13.05.47, but UnrealEditor-FlyingCabNarrativeEditor.dll was built 2026.09.10-12.58.31. Close the editor and run scripts/sync-mac.sh or scripts/Sync-Windows.ps1, otherwise the game runs old code.` (czasy w logu w UTC). Powtórzone 3 razy z tym samym wynikiem (`AuditWinStaleCheck_2026-09-10.log`, `AuditWinStaleCheck2…4_2026-09-10.log`). Powiadomienie w edytorze: osobne okno Slate 352 × 187 px w prawym dolnym rogu ekranu z ikoną błędu i treścią ostrzeżenia (przechwycone przez `PrintWindow`). Po `Sync-Windows.ps1` (przekompilowany tylko dotknięty plik, 5 s) kolejne uruchomienie loguje `Compiled modules are current: UnrealEditor-FlyingCabFlightLab.dll built 2026.09.10-13.14.00, newest source 2026.09.10-13.13.10.` (`AuditWinFinal_2026-09-10.log`). |
| `A_R7 supercars parked: 4/4.` | W sesji PIE edytora GUI (test `FlyingCab.Functional.PIE.Supercar` uruchomiony przez `-ExecCmds`, zaliczony; `AuditWinStaleCheck_2026-09-10.log`) oraz w każdej sesji PIE pakietu NullRHI. |
| Manifest sterowania | `Verify-InputBaseline.ps1` przed i po: kod 1, te same 9 historycznie zmienionych plików, 23 z 32 wpisów zgodne, 0 brakujących. Sterowania nie zmieniano. |

Symulacja pulla: `origin` nie miał nowych commitów, więc zamiast `git pull` przepisano `FlyingCabWorldBootstrap.cpp` z Gita (`git checkout 338b2fa -- <plik>`, potem `git checkout HEAD -- <plik>`): treść identyczna z HEAD, `git status` czysty, nowy czas modyfikacji, czyli stan taki jak po pullu zmieniającym ten plik. Gałąź skryptu wypisująca zmienione pliki `Source/` po pullu nie została przez to wykonana; samo wywołanie `git diff --name-only 338b2fa b4fc453 -- <Source> <.uproject>` z identyczną konstrukcją ścieżek sprawdzono ręcznie w PowerShell 5.1 (6 plików, kod 0).

Uwagi:

- Ostrzeżenie wykonuje nowy moduł `FlyingCabNarrativeEditor`. Klon, którego binarka edytora pochodzi sprzed `b4fc453` (tak było na tym PC: DLL z 12:54, pull o 14:48), nie ostrzeże przy pierwszym otwarciu; po pierwszym buildzie skryptem ochrona obejmuje kolejne pulle.
- Tekst powiadomienia jest dłuższy niż jego okno: prawa i dolna krawędź go obcinają (kosmetyczne; do rozważenia krótszy komunikat albo `FNotificationInfo::WidthOverride`). W Output Log pełna linia jest czytelna.
- Ostrzeżenie `LogFlyingCabQuests: Warning: Skipping invalid quest entry … At least one objective is required.` (12 ×) pochodzi z testów `FlyingCab.Core.Quests.CatalogResilienceAndCredits` (8) i `FlyingCab.Core.Quests.ValidatedChains` (4) dla obiektów tymczasowych tworzonych przez te testy; nie dotyczy zmian z audytu.
- `Get-ExecutionPolicy` na tym PC ma `Undefined` we wszystkich zakresach; skrypty uruchamiano z polityką `Bypass` dla procesu. W zwykłym oknie PowerShell z domyślną polityką `Restricted` trzeba użyć `powershell -ExecutionPolicy Bypass -File .\scripts\Sync-Windows.ps1 -EngineRoot 'D:\Unreal\UE_5.8'` albo ustawić `RemoteSigned` dla użytkownika.
- Logi i raporty z tej sesji leżą w `Saved/` tego PC (A-08): `AuditWinFull_2026-09-10`, `AuditWinMetro1…3_2026-09-10`, `AuditWinStaleCheck_2026-09-10` i `AuditWinStaleCheck2…4_2026-09-10`, `AuditWinAfterRebuild_2026-09-10`, `AuditWinFinal_2026-09-10`.

### Prezentacja rozmowy NPC — 2026-09-10

Pełny opis warstwy prezentacji dla programisty: `NPC_CONVERSATION_UI.md`.

Na polecenie użytkownika przebudowano wygląd i rytm rozmowy z NPC. Logika rozmowy, warunki, akcje i nagrody pozostają bez zmian w `UFlyingCabDialogueSession`; zmiana dotyczy wyłącznie warstwy prezentacji.

Co się zmieniło:

- Okno zeszło z pełnoekranowego modalu do dolnego pasa ekranu. Przyciemnienie tła spadło z 0,78 do 0,30, więc miasto i rozmówca pozostają widoczni.
- Kwestia NPC wpisuje się znak po znaku (domyślnie 55 znaków na sekundę), z krótszym przytrzymaniem po przecinku i dłuższym po kropce.
- Odpowiedzi gracza stoją po prawej stronie dymka i pojawiają się pojedynczo od góry, co 0,085 s, wjeżdżając z prawej. Ich tekst jest pełny od pierwszej klatki.
- `Spacja` lub `Enter` podczas wpisywania kończy tekst zamiast wybierać odpowiedź. Klawisze `1`–`9` wybierają odpowiedź, ale tylko taką, która już się pojawiła.
- Profil NPC ma opcjonalne pole `Portrait`; puste zostawia samą tabliczkę z imieniem.
- Tempo, dystans wjazdu i opcjonalne dźwięki konfiguruje się w **Project Settings → Game → Flying Cab Narrative**, sekcja `Presentation`.

Rozwiązania warte zapamiętania:

- **Tekst nie przeskakuje podczas wpisywania.** Linia jest łamana raz, własnym algorytmem opartym o `FSlateFontMeasure`, i wyświetlana bez `AutoWrapText`. Obok stoi bliźniaczy blok tekstu z pełną treścią i widocznością `Hidden`, który rezerwuje docelową wysokość, nie rysując niczego. Odpowiedzi też są w drzewie od początku jako `Hidden`, więc kolumna nie skacze w miarę ich pojawiania się.
- **Animacja chodzi na czasie rzeczywistym.** Rozmowa pauzuje świat, więc timery `FTimerManager` by nie zadziałały. Prezentacja liczy czas z `FPlatformTime::Seconds()` w `NativeTick`, z ograniczeniem kroku do 0,25 s. Z tego samego powodu dźwięki interfejsu odtwarzane są z flagą dźwięku UI.
- **Logika prezentacji jest testowalna bez świata i bez Slate.** `FFlyingCabTypewriter`, `FFlyingCabDialogueStage` i `FFlyingCabTextWrapper` w `FlyingCabDialoguePresentation.h/.cpp` to zwykłe struktury C++. Widget tylko przekazuje im deltę i odczytuje wynik.
- **`SetText` leci wyłącznie gdy licznik odsłoniętych znaków wzrósł** i tylko w fazie wpisywania, zgodnie z wymaganiem `GODOT_MIGRATION_CONTRACTS.md` o braku stałej inwalidacji tekstów.

Weryfikacja na Windows (UE 5.8 z `D:\Unreal\UE_5.8`, testy z `-NullRHI`):

| Krok | Wynik |
|---|---|
| `.\scripts\Build-Editor.ps1 -EngineRoot 'D:\Unreal\UE_5.8'` | `Result: Succeeded`, kod 0. Jedyne ostrzeżenia to znane C4996 z nagłówka silnika `Character.h(798)`, nie z kodu projektu. |
| Pełny pakiet `Automation RunTests FlyingCab` | `Saved/Logs/DialogueVerified.log`, raport `Saved/Automation/DialogueVerified/index.json`: **49 wykonanych, 49 zaliczonych** (39 czysto, 10 z ostrzeżeniami), 0 niezaliczonych, 0 pominiętych; 94,5 s testów. W logu 0 linii `Error:`. |
| Nowe testy jednostkowe | `FlyingCab.Core.Dialogue.Typewriter`, `FlyingCab.Core.Dialogue.RevealTimeline`, `FlyingCab.Core.Dialogue.TextWrap` — zaliczone. |
| `FlyingCab.Functional.PIE.NpcConversation` | Zaliczony po rozszerzeniu o prezentację: po otwarciu tekst nie jest pełny, klawisz numeryczny przed pojawieniem się odpowiedzi nie zmienia rewizji widoku, a pominięcie pokazuje całą kwestię i wszystkie odpowiedzi. Dotychczasowe asercje o pauzie, jednorazowej nagrodzie i powrocie sterowania po puszczeniu `A` zachowane. |
| Manifest sterowania | `Verify-InputBaseline.ps1` przed i po pracy: kod 1, **te same dziewięć historycznie zmienionych plików, 23 z 32 zgodne, 0 brakujących**. Żaden plik z manifestu nie został dotknięty. |

Nie wykonano oceny wizualnej w edytorze GUI ani ręcznego przejścia rozmowy w PIE. Zielone testy nie zatwierdzają wyglądu ani tempa; próbę z sekcji „Praktyczna próba” w `QUEST_AUTHORING.md` wykonuje użytkownik. Nie commitowano.


### Świat edytowalny przed Play — 2026-09-10

Na zlecenie użytkownika aktywna mapa `Content/Maps/FlightLab.umap` została przekształcona w zapisany poziom: **505 aktorów**, 24 grupy przystanków, oddzielne obiekty geometrii/napisów, instancjonowane okna, 4 stacje paliwa, 2 warsztaty, NPC, biuro, portale, terminal, punkty pojazdów i trasy ruchu. Dodano 28 zapisanych materiałów w `Content/World/Materials`. Zmiany prezentacji dialogów zastane po pracy z Claude'em zachowano.

`AFlyingCabAuthoredWorld` przechowuje powiązania stałych obiektów. Bootstrap korzysta z mapy; tworzy tylko dynamiczne pojazdy i populację ruchu. `AFlyingCabCityExpansion` na tej mapie steruje napisem sygnalizacji i nie generuje geometrii. Brak zapisanej usługi/NPC/trasy nie powoduje jej odtworzenia z dawnych tabel. Stare mapy bez znacznika konwersji zachowują wcześniejszy fallback.

Przesunięcie `AFlyingCabDistrictAnchor` przenosi podpięte elementy i miejsca kursów. Minimapę przystanków i stacji zasilają odczyty rzeczywistych aktorów konkretnego świata. Dispatch odświeża listę po utworzeniu świata, zamiast polegać tylko na danych odczytanych w konstruktorze. Portale mają zapisywane parametry oraz referencje do ruchomych punktów docelowych. Trasy NPC korzystają z istniejącego mechanizmu tras umieszczonych w poziomie.

Instrukcja pierwszych kroków i ograniczenia: **`WORLD_EDITING.md`**. Edycja tras pozostaje ręczna: przesunięcie budynku nie przebudowuje ich automatycznie. Obszary autostrad/bonus Highway Turbo i linie dróg minimapy nadal wynikają z granic układu danych. Pełną grupę przystanku należy przesuwać w X/Z; obrót/skala całej grupy nie zmienia rozstawu stref pasażera. Nowe identyfikatory lokacji questowych wymagają spójności z katalogiem danych walidacji.

Weryfikacja: Windows 11, UE 5.8.0, MSVC 14.51.36252, 2026-09-10:

| Kontrola | Wynik |
|---|---|
| Build edytora | **Succeeded**, kod 0. Pozostało znane ostrzeżenie C4996 z nagłówka silnika `Character.h` i informacja o niepreferowanej wersji MSVC; nowe pliki bez ostrzeżeń kompilatora w końcowym buildzie. |
| Jednorazowa konwersja i zapis | `Bake-EditableWorld.py`: 505 aktorów, kod 0, bez błędów i ostrzeżeń commandletu. |
| Pełny pakiet NullRHI | **50/50 zaliczone**: 40 bez ostrzeżeń, 10 z dotychczasowymi ostrzeżeniami, 0 błędów, 0 pominiętych. `Saved/Automation/EditableWorldFull/index.json`, lokalny log `Saved/Logs/EditableWorldFull.log`. |
| Nowy `FlyingCab.Functional.PIE.AuthoredWorld` | Zapisane obiekty po wczytaniu, brak generowania geometrii/duplikowania stacji, referencja docelowa portalu, odczyt przesuniętych aktorów bez modyfikowania współdzielonego assetu: zaliczone. |
| `Verify-EditableWorld.py` | Na kopii mapy: geometria istnieje przed Play, odmowa ponownej konwersji, przesunięcie całego przystanku i osobno budynku, dziedziczenie pozycji stacji, zapis/ponowne wczytanie, zachowanie koloru i materiału oraz referencji portalu: zaliczone. Kopia testowa usunięta. Log `Saved/Logs/EditableWorldPersistence2.log`, kod 0. |
| Renderowanie i kolizje osiedla | `ResidentialAccess` z renderowaniem poza ekranem: **1/1**, kod 0. Obejrzano wygenerowany widok platformy `Saved/Automation/ResidentialPlatform.png`: budynek, okna, platforma, napisy i stacja obecne. Zrzut SceneCapture ma szum tła; nie jest pełną ręczną sesją edytora. |
| Manifest wejścia | Te same 9 historycznych różnic, 23/32 zgodne, 0 brakujących. Jedyny przyrost w chronionym pliku dotyczy odczytu danych minimapy, opisany w `INPUT_CANONICAL_BASELINE.md`. |
| macOS | **Build i pełny pakiet Automation dla tego etapu czekają na weryfikację.** Brak dostępu do Maca w tej sesji. Wspólna mapa i źródła, UE 5.8, bez rozdzielania wersji systemowych. |

Nie wykonano commita ani push. Zmiany świata i zastane zmiany dialogów pozostają w drzewie roboczym. Instrukcje edycji NPC oraz dokument własności stanu zaktualizowano do mapy jako źródła położenia. Lokalne logi, kopia bezpieczeństwa mapy i binaria pozostają w ignorowanych katalogach.
