# Kanoniczne sterowanie: input-canonical-2026-09-04

Status: **zatwierdzony przez użytkownika punkt odniesienia**, nie gwarancja braku wszystkich możliwych błędów. Data akceptacji: 2026-09-04. Ten dokument określa aktualne zasady; raport Claude'a z 2026-09-03 opisuje diagnozę historyczną i docelową architekturę, nie polecenie automatycznego zastąpienia działającej wersji.

## Co dokładnie jest kanoniczne

- Unreal Engine 5.8, projekt `FlyingCabFlightLab`.
- `flyingcab.UseControlFrame=0`: pojazd pobiera ciągłe wartości przez `GetBoundActionValue` z Enhanced Input na swoim komponencie. To NIE jest dawna wersja oparta na bezpośrednim `IsInputKeyDown` w pojeździe.
- Q/J/R rejestrują żądania. `ProcessDeferredInputCommands()` realizuje je w kontrolerze dopiero po `Super::PlayerTick`, poza delegatami Enhanced Input. Pozostaje strażnik przejścia oraz priorytet dziennik → interakcja → reset.
- Zachowane czyszczenie wejścia przy przejściach, focus loss, zniszczeniu i recovery; stały kontekst gameplay podczas dziennika; obecna osobna obsługa pieszego, dotyku i observera.
- `flyingcab.InputTrace=1` w development: diagnostyka dostarczonych zdarzeń oraz zmian wejścia i napędu, bez samoczynnej korekty sterowania.
- Komponent ramki sterowania istnieje, ale jego wariant `UseControlFrame=1` pozostaje eksperymentem, nie następną domyślną wersją. To, że oba warianty przechodzą testy syntetyczne, nie czyni ich równoważnymi w sesji gracza.

## Dowody akceptacji i granice

- Użytkownik zgłosił naprawdę długą sesję bez żadnego problemu ze sterowaniem, po wcześniejszym doprecyzowaniu zatrzaśniętego ciągu w górę po recovery.
- Dostępny log tej sesji (`Saved/Logs/FlyingCabFlightLab.log`, sesja PIE około 11:39:22–11:49:24 czasu lokalnego) potwierdza `frame_on=0`. Log jest lokalnym plikiem rotowanym; nie jest trwałym archiwum w Git. Nie zakładamy, że gracz w tej sesji wykonał wszystkie scenariusze checklisty.
- Build `FlyingCabFlightLabEditor Win64 Development` na UE 5.8 oraz 26/26 testów z wariantem `0`: `Saved/Logs/RecoveryDefaultTraceVerifiedTests.log`. Z wariantem eksperymentalnym `1` również 26/26: `Saved/Logs/RecoveryExperimentalTraceTests.log`. To wyniki weryfikacji z poprzedniego etapu, nie nowych testów wykonanych przy oznaczaniu wersji.
- Pakiet obejmuje m.in. `DeferredInputTransitions`, `InputTransitionChain`, `EnhancedInputRelease`, `EnhancedInputFocusFlush`, `QuestJournalInput`, `DeveloperObserver`, `ActiveTowRecovery` i `RecoveryInputRelease`.
- Późniejsze rozszerzenie bez zmiany wersji kanonicznej: [INPUT_RELIABILITY_TESTS.md](INPUT_RELIABILITY_TESTS.md) — soak na dwóch seedach i test pustego baku, pełny pakiet 29/29. Historyczny manifest 32 plików pozostaje bez zmian.
- Nie udowodniono przyczyny ostatniej blokady po recovery. Udana sesja zatwierdza wariant do dalszej pracy, lecz nie dowodzi winy komponentu eksperymentalnego ani Parsec.

## Identyfikacja wersji

Kanoniczny commit: **`771e1fa9addfff7a0471f0e517e97af7ecad8c60` — „sterowanie DZIALA!”**, branch `codex/unreal-audit-fixes`. Pojawił się w repozytorium podczas oznaczania wersji; Codex w tym zadaniu nie wykonał commita. Obejmuje wcześniejsze zmiany sterowania, testów, kamery, mapy i kontrolek. W chwili zapisu manifestu śledzone pliki były zgodne z tym commitem. Wcześniejszy `2fc8be1` NIE jest wersją kanoniczną.

`INPUT_CANONICAL_BASELINE_2026-09-04.json` zawiera sumy SHA-256 wskazanych źródeł sterowania i przejść, konfiguracji oraz assetów wejścia i pojazdu. Tekst jest normalizowany do UTF-8 bez BOM i LF, assety liczone bajtowo. Testy i dokumentacja nie są objęte odciskiem — można rozszerzać je bez zmiany produkcyjnego sterowania.

Kontrola z katalogu projektu: `./scripts/Verify-InputBaseline.ps1`. Nie wymaga uruchomienia edytora i niczego nie zapisuje. Kod wyjścia 0 oznacza zgodność wskazanych plików, 1 różnice/braki. Nie kontroluje binariów buildu, ustawień lokalnych ani dowolnego nowego kodu poza manifestem. To wykrywanie zmian, nie test poprawności ani mechanizm przywracania plików. Celowa zmiana np. wyglądu w `FlyingCabPawn.cpp` też wymaga przeglądu różnicy, ale nie oznacza automatycznie regresji wejścia.

Przy oznaczaniu wersji sprawdzono zgodność 32 plików (kod 0) oraz wykrycie błędnej sumy i brakującego pliku na osobnych manifestach testowych (kod 1 z odpowiednio `CHANGED` i `MISSING`). Nie zmieniano kodu gry do testowania kontrolera. Test działa w skonfigurowanej powłoce zadania; starszy `powershell.exe` zablokował skrypt lokalną polityką wykonywania, której nie zmieniano. Nie traktuj takiej odmowy jako wykrytej różnicy plików.

Commit powyżej stanowi checkpoint kodu. Oznaczenie, manifest, instrukcje i skrypt utworzone w tym zadaniu pozostają osobnymi lokalnymi zmianami do przyszłego zatwierdzenia przez użytkownika. Nie utworzono tagu ani push. Nie cofaj całego projektu do checkpointu bez uzgodnienia — zawiera także systemy inne niż sterowanie.

## Warunki dalszych zmian

1. Przed edycją sprawdź zgodność z manifestem i zastane różnice. Zachowaj zmiany użytkownika.
2. Nie zmieniaj domyślnego wariantu, mapowań, czyszczenia ani kolejności wejścia w ramach prac nad kamerą, UI, ekonomią lub światem.
3. Kolejny etap audytu powinien najpierw dodać testy, które obserwują obecne zachowanie: losowy soak z powtarzalnym seedem i test paliwa. Diagnostyka ma być addytywna, nie stosować timeoutów zerujących trzymane klawisze.
4. Refaktor wejścia musi być osobnym uzgodnionym eksperymentem z zachowaną ścieżką powrotu. Nie usuwaj obecnych ścieżek ani przełącznika przed akceptacją następcy.
5. Przed proponowaniem nowego wariantu kanonicznego: build UE 5.8, cały pakiet Core + PIE, testy dodatnie i regresyjne oraz ręczna sesja użytkownika przez Parsec. Dopiero jawna akceptacja może nadać nowy identyfikator; zachowaj historyczny manifest.

Minimalna ręczna checklista: zwykły lot i puszczenie wszystkich wariantów klawiszy; A+D w różnych kolejnościach; J z puszczeniem w dzienniku; O → powrót → pasażer → J; Q z trzymanym kierunkiem; R z trzymanym ciągiem; zniszczenie → recovery → puszczenie → ponowne naciśnięcie; utrata focusu i powrót; brak paliwa; wejście dotykowe osobno i razem z klawiaturą. Nie uznawaj zaniku auto-repeat za dowód puszczenia: brak repeat sam w sobie nie upoważnia do automatycznego odcięcia ciągu.

Po puszczeniu klawiszy faktyczny ciąg musi wrócić do zera, choć prędkość może przez chwilę pozostać dodatnia z powodu bezwładności. Jeśli klawisz pozostaje fizycznie trzymany przez reset/focus/recovery, Unreal może wznowić wejście po repeat; obecny kontrakt nie obiecuje blokady aż do nowego wciśnięcia. Nie myl tego z utrzymaniem ciągu po rzeczywistym puszczeniu.

## Przejrzane zmiany poza logiką wejścia — 2026-09-04

Na wyraźną prośbę użytkownika zmniejszono spalanie o 25% i wyróżniono stacje minimapy. Historyczny manifest pozostaje nienaruszony, więc kontrola zgłasza teraz trzy oczekiwane `CHANGED`:

- `FlyingCabPawn.h`: tylko domyślne `VerticalFuelPerSecond` 1,8 → 1,35 i `HorizontalFuelPerSecond` 0,9 → 0,675.
- `FlyingCabVehicleVitalsComponent.h`: te same wartości domyślne w konfiguracji i polach zasobów; algorytm rozliczania i bramka napędu bez zmian.
- `FlyingCabTouchControls.cpp`: wyłącznie fragment budujący oznaczenia stacji w `BuildWidgetTree`; etykiety FUEL/REPAIR są nieinteraktywne. Żaden handler wejścia, focusu ani przejścia nie został zmieniony.

Pozostałe 29 plików nadal pasuje do historycznego manifestu. To przegląd konkretnej różnicy, nie automatyczne dopuszczenie przyszłych zmian w tych trzech plikach. Domyślna ścieżka `UseControlFrame=0`, mapowania, Q/J/R, flush i siła napędu pozostają kanoniczne. Zmiany wyglądu/kolizji ambientowego pieszego są poza manifestem i nie dotyczą postaci gracza. Wyniki weryfikacji etapu znajdują się w `AUDIT_IMPLEMENTATION_STATUS.md`.

### Rozbudowa Metro City — dalszy przegląd tych samych trzech plików

Na prośbę o mapę ×4 i mocniejszy pojazd `FlyingCabPawn.h` oraz `FlyingCabVehicleVitalsComponent.h` zmieniają także domyślny próg pełnych obrażeń 1400 → 2000. W `FlyingCabTouchControls.cpp` powiększono minimapę, dodano jej statyczne linie dróg i przesunięto panel zasobów w dół. To wartości prezentacji i fragment `BuildWidgetTree`; handlery, focus, kolejność wejścia i dotyk nadal bez zmian. Kontrola nadal wykazuje tylko te trzy oczekiwane różnice i 29 zgodnych plików. Nie zmieniono manifestu ani wariantu kanonicznego. Szczegóły miasta i testów: `METRO_CITY.md`.

### Highway Turbo — modyfikator świata, nie nowa wersja wejścia

Na prośbę użytkownika dodano automatyczny bonus w pasach autostrad. Manifest pozostaje historyczny: teraz pięć oczekiwanych `CHANGED`, pozostałe 27 plików zgodnych.

- `FlyingCabPawn.cpp`: utworzenie osobnego komponentu HighwayAssist, jego aktualizacja w Tick, mnożnik limitów prędkości, przekazanie mnożnika spalania oraz zerowanie bonusu przy resecie/recovery. Odczyt wejścia, siła ciągu, tłumienie po puszczeniu, kolejność i czyszczenie wejścia nie zostały zmienione.
- `FlyingCabPawn.h`: referencja do nowego komponentu, obok wcześniej opisanych zmian balansu.
- `FlyingCabVehicleVitalsComponent.h/.cpp`: opcjonalny argument mnożnika spalania, domyślnie 1; dotyczy tylko kosztu paliwa. Bramka pustego baku, regeneracja, obrażenia i zegary pozostają niezmienione.
- `FlyingCabTouchControls.cpp`: wyłącznie wcześniejsze zmiany minimapy; bez dodatkowych zmian przy turbo.

Nowy komponent nie czyta klawiszy, nie przechowuje poleceń i nie generuje ciągu. Zmienia parametry dostępnego lotu na podstawie położenia i stanu pojazdu. Wariant `UseControlFrame=0` pozostaje domyślny. Szczegóły i wyniki testów: `HIGHWAY_TURBO.md`.

### Osiedla i zasięg — wyłącznie wartości zasobów

Przy zagęszczeniu osiedli na prośbę użytkownika w `FlyingCabPawn.h` oraz `FlyingCabVehicleVitalsComponent.h` zmieniono `MaxFuel` 100 → 200 i `StartingFuel` 65 → 130. Reszta obsługi wejścia, fizyki, resetu/recovery i rozliczania paliwa bez zmian względem etapu Highway Turbo. Manifest nadal zgłasza te same pięć oczekiwanych plików i 27 zgodnych. Geometria, nowe perony i testy są poza chronioną logiką wejścia. Szczegóły: `RESIDENTIAL_DISTRICTS.md`.

### Wizualizacja dwóch dysz — 2026-09-09

Na prośbę użytkownika dodano `UFlyingCabThrusterVisualComponent`. Zmiany w `FlyingCabPawn.h/.cpp` obejmują wyłącznie referencję i utworzenie komponentu, zapis prędkości przed dotychczasowym tłumieniem oraz przekazanie już zastosowanego przyspieszenia napędu i tłumienia do wizualizacji, przed limitami prędkości. Reset i recovery zerują dodatkowo stan efektów. Dotychczasowe instrukcje odczytu wejścia, stosowania sił, tłumienia i ograniczania prędkości zachowały kolejność i treść; mapowania, bramki, flush, Q/J/R i wariant `UseControlFrame=0` pozostają bez zmian.

Na macOS nie ma `pwsh`, więc próba uruchomienia `scripts/Verify-InputBaseline.ps1` zakończyła się brakiem interpretera. Przed i po zmianach wykonano równoważne sprawdzenie SHA-256 w Pythonie z identyczną normalizacją tekstu UTF-8/LF: te same pięć historycznie zmienionych plików, 27 zgodnych. Przejrzano powyższy przyrost w plikach pojazdu; manifest bez zmian. Wyniki testów i granice efektu opisuje `THRUSTER_VISUALS.md`.

### Model auta o małej liczbie polygonów — MVP

Na prośbę użytkownika `FlyingCabPawn.h/.cpp` dodają miękką referencję do siatki auta, podmianę samego `VisualMesh` podczas konstrukcji/BeginPlay oraz odwracanie jego skali X zgodnie z kierunkiem ruchu. Parametr materiału `Color` pozostaje obsługiwany. Kolizja, pochylenie, fizyka, obliczenia siły, wejście, reset i recovery nie są zmieniane. Zakres opisuje `LOW_POLY_CAB.md`. Kontrola manifestu nadal wymaga równoważnego skryptu Python ze względu na brak `pwsh`; manifest pozostaje historyczny. Zgodnie z poleceniem użytkownika testy w Unreal i ocenę wizualną wykonuje użytkownik; po stronie implementacji pozostają przegląd kodu i kompilacja.

Przy dodaniu lakierów i podmianie pojazdów NPC użytkownik zlecił usunięcie oznaczenia gracza. Dalsza zmiana w `FlyingCabPawn.cpp` dotyczy wyłącznie `RefreshPlayerFocusAppearance`: obwódka jest niewidoczna, a dodatkowe światło wyłączone. Odczyt wejścia, possession, reset, recovery i kolejność działania pozostają niezmienione. Dostosowano istniejące oczekiwanie testu dotyczące widoczności oznaczenia, bez uruchamiania testów w Unreal.

### Rozmowy NPC — 2026-09-10

Po audycie użytkownik zlecił wdrożenie konfiguracji questów i rozmów. Dodano osobny modal dialogu, z pauzą, wyczyszczeniem wejść i powrotem przez istniejącą ścieżkę przywracania sterowania. `Q` nadal trafia do istniejącej kolejki interakcji; otwiera rozmowę dopiero podczas wykonania interakcji z NPC. Nie zmieniono kolejności obsługi Q/J/R, mapowań, fizyki ani domyślnego `UseControlFrame=0`.

Przed zmianami równoważne sprawdzenie SHA-256 wykazało 27 zgodnych plików i pięć wcześniej opisanych różnic; `pwsh` nie jest dostępny. Po wdrożeniu: 23 zgodne, dziewięć zmienionych. Cztery nowe różnice obejmują:

- `FlyingCabPlayerController.h/.cpp`: czas życia sesji/widgetu rozmowy, otwieranie i zamykanie modalu, wykluczenie równoczesnego dziennika/obserwatora oraz ukrycie kontrolek na czas rozmowy.
- `FlyingCabControlInputComponent.h`: dopisany powód blokady `Dialogue`, za dotychczasowymi wartościami enumu; algorytm wejścia bez zmian.
- `FlyingCabFlightLab.uproject`: rejestracja modułu edytora `FlyingCabNarrativeEditor`; wersja silnika bez zmian.

W już zmienionym `FlyingCabTouchControls.cpp` podmieniono dodatkowo tylko źródło znaczników NPC na listę profili i aktorów otwartej mapy. Zastane zmiany w Pawnie, efektach dysz, ruchu i pojazdach zostały zachowane. Historyczny manifest nie został zmieniony.

Kompilacja edytora UE 5.8 zakończyła się powodzeniem. Dodano test PIE rozmowy obejmujący m.in. puszczenie `A` w dialogu i wznowienie sterowania; uruchomienie pełnych testów zostało odrzucone w oknie uprawnień, więc brak wyniku runtime dla tego wdrożenia. Ocena w grze i ręczna akceptacja pozostają do wykonania; nie nadano nowej wersji kanonicznej. Instrukcja: `QUEST_AUTHORING.md`.
