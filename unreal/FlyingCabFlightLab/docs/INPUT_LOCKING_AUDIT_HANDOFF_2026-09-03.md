# Flying Cab — handoff do niezależnego audytu okresowego blokowania sterowania

Data raportu: 2026-09-03  
Projekt: `unreal/FlyingCabFlightLab/FlyingCabFlightLab.uproject`  
Środowisko reprodukcji: Unreal Engine 5.8, Windows, PIE w edytorze  
Stan Git w chwili przygotowania: branch `codex/unreal-audit-fixes`, HEAD `7173f37a865cd95f2d2dc1e5774f7e537d4084ca`; working tree zawiera niezatwierdzone zmiany z kilku trwających etapów projektu.

## Cel audytu

Potrzebne jest niezależne ustalenie przyczyny sporadycznego blokowania sterowania pojazdem. Problem wracał mimo kolejnych zabezpieczeń i dwóch zmian sposobu odczytu wejścia. Krótkie testy automatyczne przechodzą, natomiast awaria występuje podczas normalnej albo dłuższej sesji PIE.

Prośba do audytora:

1. Najpierw wykonać audyt tylko do odczytu, bez modyfikowania kodu.
2. Nie przyjmować automatycznie, że każdy opis „zablokowanego inputu” oznacza błąd samej warstwy wejścia. Oddzielić:
   - klawisz zapamiętany jako wciśnięty po puszczeniu,
   - klawisz fizycznie wciśnięty, ale odczytany jako puszczony,
   - poprawny sygnał wejścia odcięty przez stan gameplayu, zasoby albo fizykę,
   - wejście dotykowe pozostawione w stanie aktywnym.
3. Zakwestionować dotychczasowe założenia i zaproponować jedną docelową architekturę, a nie kolejną lokalną łatę.

## Podsumowanie wykonawcze

- Zgłoszenia dotyczyły obu osi i obu typów awarii: ciągu, który prawdopodobnie pozostał aktywny po puszczeniu, oraz ciągu, który losowo przestawał działać mimo trzymania klawisza.
- Problem występował po przejściach między trybami i UI, ale ostatnie zgłoszenie wystąpiło bez otwierania lub zamykania okien.
- Najlepiej udokumentowana ostatnia sesja nie zawiera przejścia do obserwatora, quest logu, zmiany posiadania pawna ani logowanego flushu inputu między rozpoczęciem gry a awarią.
- W chwili zniszczenia pojazdu log zawiera `keyboard thrust 1.00`, a `touch thrust 0.00`. To nie rozstrzyga sprawy, ponieważ ówczesna wartość była odczytem/cachowaniem wartości Enhanced Input, a log nie zawierał stanu surowych klawiszy, paliwa, `CanUseThrusters()` ani faktycznie przyłożonej siły.
- Po tej sesji sterowanie ciągłe pojazdu zostało tymczasowo przełączone na bezpośrednie próbkowanie `APlayerController::IsInputKeyDown()` w każdym ticku. Jest to środek diagnostyczno-stabilizacyjny, nie uzgodniona architektura docelowa.
- Dziewięć obecnych testów PIE przechodzi, lecz testy są krótkie i syntetyczne. Nie odtwarzają wielominutowej sesji, rzeczywistego focus/capture Windows, wejścia myszy emulującego dotyk ani wyczerpania paliwa podczas trzymania klawisza.

## Chronologia zgłoszeń użytkownika

W tabeli cytaty są możliwie bliskie oryginalnym zgłoszeniom. Dla starszych sesji nie ma dokładnych timestampów ani kompletnej telemetrii, dlatego nie należy dopowiadać kierunku awarii tam, gdzie użytkownik nie rozróżnił „zostało włączone” od „przestało odpowiadać”. Użytkownik deklarował, że nie zmieniał parametrów projektu pomiędzy iteracjami, jeśli nie napisał inaczej.

| ID | Zgłoszony scenariusz | Co wiadomo | Czego nie wiadomo |
|---|---|---|---|
| S1 | „W pewnym momencie jakby zablokował się thrust w prawo.” | Pierwszy sygnał problemu z poziomym wejściem. | Czy prawa siła pozostała aktywna po puszczeniu, czy przestała reagować mimo wciśnięcia; czas sesji; kontekst UI/focus; źródło keyboard/touch. |
| S2 | „Zablokowało mi thrust up i rozbiłem się o sufit. Po restarcie thrust up nadal był zablokowany. Problem może dotyczyć również innych kierunków.” | Zachowanie mocno sugeruje ciąg pozostający aktywny. Reset pojazdu nie neutralizował wtedy źródła problemu. | Stan paliwa, touch, focus i dokładna wersja kodu z sesji. |
| S3 | „Trochę polatałem i zawiesił się prawy input.” | Kolejna awaria poziomego sterowania po pewnym czasie gry. | Kierunek latch/drop, źródło inputu i zdarzenia poprzedzające. |
| S4 | „Mamy jakiś regres. Zablokował mi się lewy dopalacz.” | Awaria pojawiła się ponownie po rozbudowie mapy i systemu pasażerów. Tym razem dotyczyła lewej strony. | Kierunek latch/drop oraz związek czasowy z nowymi systemami. |
| S5 | „Co jakiś czas losowo wyłącza ciąg mimo wciśniętego przycisku.” | To jednoznaczny wariant drop: gracz trzyma przycisk, a ciąg przestaje działać. | Czy odczyt wejścia spadał do zera, czy tylko `AddForce` było blokowane; stan paliwa/hull/fizyki. |
| S6 | „Przy dłuższej sesji gry sterowanie nadal potrafi się zaciąć. Może trzeba to jakoś zrefaktorować i uporządkować, a nie robić łatę na łacie?” | Istotna korelacja z długością normalnej sesji. Problem przetrwał wcześniejsze neutralizacje stanów. | Minimalny czas reprodukcji i jeden deterministyczny ciąg kroków. |
| S7 | „Znowu zablokowane sterowanie. Tym razem w lewo. Scenariusz: byłem w trybie widza i sprawdzałem ruch pojazdów NPC, potem wróciłem do gry, podjąłem pasażera, z nim w powietrzu otworzyłem quest log, zamknąłem i już miałem zablokowane sterowanie.” | Najdokładniejsza reprodukcja przejściowa: observer → gameplay → pickup → lot → quest journal → gameplay → awaria lewej osi. | Czy lewo pozostało aktywne, czy przestało odpowiadać; czy równolegle był używany dotyk/mysz; dokładny stan focus/capture. |
| S8 | Po poprawkach dla S7: „Wydaje się działać ok.” | Konkretny łańcuch przejść przestał natychmiast reprodukować problem. | Nie był to test długoterminowy. |
| S9 | Najnowsze: „Bez żadnej zmiany okien zablokowało mi sterowanie w górę.” | Awaria nie wymaga observera ani quest logu. Zachował się log całej sesji opisany niżej. | Sam tekst nie rozstrzyga latch kontra drop; log też nie rozstrzyga warstwy wykonawczej, bo brakuje stanu paliwa i faktycznej siły. |

## Najnowsza udokumentowana sesja S9

Lokalny plik:

`unreal/FlyingCabFlightLab/Saved/Logs/FlyingCabFlightLab-backup-2026.09.02-14.03.11.log`

`Saved/Logs` nie jest śledzone przez Git. Poniżej znajduje się wystarczający wyciąg, jeżeli audyt odbywa się na innej kopii repozytorium.

Przebieg:

- `13:59:57.234` — uruchomiono Freeroam.
- `13:59:58.570` — `LevelEditor.ToggleImmersive`; viewport przełączono na widok immersyjny.
- `14:00:04.566` — podjęto pierwszego pasażera.
- `14:00:28.131` — ukończono kurs.
- `14:00:28.773` — podjęto kolejnego pasażera.
- Od uruchomienia trybu do zniszczenia nie ma wpisu o observerze, quest journalu, zmianie pawna ani czyszczeniu inputu.
- `14:02:59.710` — pojazd otrzymał 100 obrażeń od zmiany prędkości normalnej `1441.7 cm/s`; hull spadł do zera.
- Ten sam tick:

```text
LogFlyingCabFlight: Warning: Input state cleared during vehicle destroyed
(keyboard X 0.00, thrust 1.00; touch X 0.00, thrust 0.00).
```

- `14:03:02.211` — tow recovery; wszystkie raportowane stany inputu były już zerowe.

Interpretacja dowodu:

- Wartość pojazdu `KeyboardThrustInput` była równa `1.00` w momencie zniszczenia.
- Touch nie był aktywny według stanu pawna.
- Sesja działała wtedy na wersji próbkowania wartości Enhanced Input przez `GetBoundActionValue`, przed najnowszym przełączeniem pojazdu na `IsInputKeyDown`.
- Jeśli S9 oznacza „ciąg został włączony po puszczeniu”, log jest z tym zgodny, ale nie pokazuje fizycznego stanu klawisza.
- Jeśli S9 oznacza „ciąg przestał działać mimo trzymania”, log pokazuje, że warstwa wejścia nadal żądała ciągu i bardziej podejrzany staje się gate paliwa/vitals albo fizyka.
- Nie ma w tej wersji logu wartości paliwa, wyniku `CanUseThrusters()`, stanu `IsSimulatingPhysics()` ani wektora siły. Na podstawie samego logu nie wolno wybrać jednej z powyższych interpretacji.

## Ewolucja systemu wejścia

### Etap 1 — legacy mappings i ręczne stany

Projekt używał `AxisMappings`/`ActionMappings`. Pawn przechowywał wartości klawiatury i dotyku. Dodawano neutralizację stanów przy resetach, possess/unpossess i zmianach UI.

### Etap 2 — zabezpieczenia przed utratą release

Commit `130c136` (`fix(unreal): prevent stuck input and remove HUD hotkey`):

- `AFlyingCabPlayerController::FlushPressedKeys()` oprócz wywołania wersji bazowej neutralizował stany pojazdu/postaci i UI;
- włączono flush przy zmianie focusu viewportu;
- usunięto konfliktujący hotkey HUD.

### Etap 3 — migracja do Enhanced Input

Commit `69bb8af` (`feat(unreal): migrate gameplay controls to enhanced input`):

- dodano `IA_FlyingCab*` i `IMC_FlyingCabGameplay`;
- usunięto stare mapowania gameplayowe z `DefaultInput.ini`;
- ruch ciągły pojazdu i postaci był obsługiwany przez `Triggered`, `Completed` i `Canceled`;
- akcje dyskretne również przeszły przez Enhanced Input.

### Etap 4 — synchroniczna neutralizacja podczas focus/transition

Commit `df9828a` (`fix(unreal): harden input flush and widen camera framing`):

- `FlushPressedKeys()` synchronicznie wywołuje `ReleaseKeyboardInputState()` na aktualnie posiadanym pojeździe albo postaci;
- dodano testy press/release oraz focus flush.

### Etap 5 — stały mapping context i próbkowanie wartości akcji

Po reprodukcji S7 w bieżącym working tree:

- mapping context stał się stały przez observer i quest journal zamiast remove/add przy każdym przejściu;
- ruch ciągły zmieniono z eventowego zapisu `Triggered/Completed/Canceled` na próbkowanie `GetBoundActionValue()` w każdym ticku;
- observer i quest journal nadal suppressują gameplay i wykonują flush;
- dodano test dokładnego łańcucha observer → pickup → quest journal → powrót.

S9 wystąpiło już po tej zmianie.

### Etap 6 — obecny eksperyment: bezpośredni odczyt klawiszy pojazdu

Po S9 pojazd zaczął w każdym ticku odczytywać:

- poziom dodatni: `D` lub `Right`;
- poziom ujemny: `A` lub `Left`;
- ciąg pionowy: `W`, `Up` lub `SpaceBar`;
- serwis: `E`.

Odczyt wykorzystuje `APlayerController::IsInputKeyDown()`. Nie jest to odczyt stanu klawiatury bezpośrednio z Windows — nadal zależy od wewnętrznego stanu `PlayerInput` i od dostarczenia zdarzeń do UE.

Ważna niespójność obecnego working tree:

- pojazd: ciągłe klawisze są hardkodowane i czytane przez `IsInputKeyDown`;
- postać piesza: ciągłe wejście nadal jest próbkowane z Enhanced Input przez `GetBoundActionValue`;
- dotyk: osobny stan mutowany callbackami UMG;
- akcje dyskretne: nadal Enhanced Input;
- IA/IMC nadal zawierają mapowania ruchu pojazdu, ale nie są obecnie autorytatywnym źródłem dla ciągłej klawiatury pojazdu.

To omija remapping, abstrahowanie platform i przyszły gamepad. Należy ocenić ten stan jako diagnostyczny, nie automatycznie zaakceptować jako rozwiązanie końcowe.

## Obecny przepływ danych dla pojazdu

```text
klawiatura Windows
  -> UE PlayerInput / IsInputKeyDown
  -> KeyboardHorizontalInput / KeyboardThrustInput

UMG OnPressed / OnReleased / OnUnhovered
  -> TouchHorizontalInput / TouchThrustInput

keyboard + touch
  -> GetHorizontalInput / GetThrustInput
  -> suppression przez tryb gry/UI/observer
  -> Vitals->CanUseThrusters()
  -> HorizontalInput / ThrustInput albo zero
  -> CollisionBody->AddForce(...)
  -> fizyka Chaos
```

Pozioma wartość efektywna to suma klawiatury i dotyku ograniczona do `[-1, 1]`. Wartość pionowa to maksimum z klawiatury i dotyku. Przeciwne kierunki poziome naciskane jednocześnie wzajemnie się kasują.

## Obecne zabezpieczenia

- `DefaultInput.ini`: `bShouldFlushPressedKeysOnViewportFocusLost=True`.
- `AFlyingCabPlayerController::FlushPressedKeys()`:
  - wywołuje `Super::FlushPressedKeys()`;
  - neutralizuje klawiaturę aktualnego pojazdu albo postaci;
  - neutralizuje stany interfejsu dotykowego.
- Pawn czyści input przy m.in. zmianie kierowcy, zniszczeniu, tow recovery i resecie.
- Quest journal flushuje wejście przed otwarciem i podczas zamykania; mapping context pozostaje zainstalowany.
- Observer flushuje wejście przy wejściu i wyjściu; suppressuje gameplay, zamraża fizykę aktualnego pojazdu i przywraca ją po powrocie.
- Widget dotykowy wykonuje `ReleaseAllInputs()` przy destrukcji, utracie focusu, opuszczeniu kursorem widgetu, ukryciu i zmianie trybu.
- Przyciski UMG mają `OnReleased` oraz `OnUnhovered` dla kierunków, thrust i serwisu.
- W edytorze controller domyślnie ustawia `FInputModeGameAndUI`, pokazuje kursor oraz ma `bEnableMouseTouchTestingInEditor=true`. `DefaultInput.ini` ma jednocześnie `bUseMouseForTouch=False`; lokalna ścieżka testowania myszy jest realizowana przez własne UI/controller.

## Gate'y, które mogą wyglądać jak awaria inputu

W `AFlyingCabPawn::Tick()` żądane wejście jest zerowane przed `AddForce`, jeśli `Vitals->CanUseThrusters()` zwróci false. W praktyce trzeba równolegle sprawdzać co najmniej:

- paliwo;
- stan zniszczenia/hull;
- `IsGameplayInputSuppressed()`;
- `CollisionBody->IsSimulatingPhysics()`;
- stan observera i poprawność przywrócenia fizyki;
- limity prędkości/opór, które mogą subiektywnie wyglądać jak brak reakcji;
- aktywnego possesowanego pawna i jego `InputComponent`.

Po S9 dodano:

- jednorazowy log `Fuel exhausted; thrusters disabled ... while requested input was ...`;
- trwały komunikat HUD `FUEL EMPTY // THRUST OFF`;
- telemetrię F3 pokazującą `Thrusters ENABLED/DISABLED` oraz źródła keyboard/touch.

Ta diagnostyka nie była jeszcze obecna w logu z S9.

## Co pokrywają obecne testy

Ostatni pełny przebieg: 9/9 testów PIE zakończonych powodzeniem:

- `ActiveTowRecovery`
- `DeveloperObserver`
- `EnhancedInputFocusFlush`
- `EnhancedInputRelease`
- `InputTransitionChain`
- `LivingWorldCycle`
- `PassengerCourse`
- `QuestJournalInput`
- `WorldStartup`

Testy sprawdzają m.in.:

- press/release w Enhanced Input;
- synchroniczne wyzerowanie stanów po flushu focusu;
- zachowanie mapping contextu;
- przejście observer → powrót → pickup → quest journal → powrót;
- przywrócenie fizyki po observerze.

## Czego testy nie pokrywają

- Wielominutowego soak testu z setkami zmian kierunku.
- Rzeczywistych zdarzeń klawiatury Windows i realnej utraty focus/capture; testy wysyłają syntetyczne `PlayerController->InputKey(CreateSimulated(...))`.
- Przełączania viewportu immersive w czasie długiej sesji.
- Kombinacji keyboard + klikana myszą kontrolka UMG przez dłuższy czas.
- Sekwencji nakładających `W`/`Up`/`Space` oraz `A`/`D`/strzałki z różną kolejnością puszczania.
- Trzymania przycisku w chwili wejścia/wyjścia z observera, menu, pojazdu albo śmierci.
- Wyczerpania paliwa podczas utrzymywania wejścia i rozróżnienia „input aktywny, thrusters disabled”.
- Asercji, że w każdym ticku przy żądanym thrust i aktywnych thrusterach rzeczywiście powstaje niezerowa siła oraz oczekiwana zmiana prędkości.
- Wielokrotnych possess/unpossess oraz sprawdzenia, czy nie pozostaje stare `InputComponent` w stosie.

Przechodzące testy krótkie nie są obecnie dowodem stabilności manualnej sesji.

## Hipotezy do niezależnej weryfikacji

Poniższa lista nie jest rankingiem ani diagnozą. Ma zapobiec pominięciu klas problemów.

1. Cykl życia wartości/triggerów Enhanced Input gubi release albo pozostawia starą wartość po zmianie trybu wejścia.
2. Wewnętrzny stan klawiszy `PlayerInput` staje się stale pressed/released po utracie zdarzenia; wtedy także obecne `IsInputKeyDown()` może być błędne.
3. Ostatnia awaria nie leżała w wejściu: żądany thrust był aktywny, lecz paliwo lub inny warunek `CanUseThrusters()` odciął siłę.
4. Callback UMG nie zawsze dochodzi do `OnReleased`/`OnUnhovered`, zwłaszcza przy zmianie capture/focus. Nie pasuje to bezpośrednio do logu S9 (`touch thrust 0.00`), ale może tłumaczyć wcześniejsze sesje.
5. Kolejność/consumption stosu `InputComponent` powoduje, że część wartości akcji nie jest aktualizowana na aktywnym pawnie.
6. `FInputModeGameAndUI`, widoczny kursor, własna emulacja dotyku w edytorze i immersive wpływają na focus/capture nawet bez świadomego przełączania okien.
7. Fizyczny body po observerze pozostaje częściowo w niepoprawnym stanie albo nie jest budzony, a gracz interpretuje to jako utratę wejścia.
8. Przejścia possession zostawiają aktywne lub stale komponenty starego pawna; UI może pisać stan do innego obiektu niż ten aktualnie sterowany.
9. Wiele klawiszy przypisanych do jednej bool action oraz modyfikatory osi IA/IMC mają nieoczekiwaną semantykę agregacji/cancel przy nakładających się wciśnięciach.
10. Problem składa się z co najmniej dwóch niezależnych usterek: latch inputu oraz poprawne odcięcie ciągu z powodu zasobów, raportowane przez gracza tym samym słowem „zablokowane”.

## Oczekiwany rezultat audytu Claude'a

Proszę dostarczyć:

1. Ranking kandydatów root cause wraz z dowodami, kontrdowodami oraz dokładnymi plikami i symbolami/wierszami.
2. Osobną ocenę S7 i S9 — nie zakładać, że mają wspólną przyczynę.
3. Odpowiedź, czy log S9 bardziej wskazuje na latch wejścia, odcięcie przez zasoby/fizykę, czy pozostaje nierozstrzygający — wraz z uzasadnieniem.
4. Audyt assetów `IA_FlyingCab*` i `IMC_FlyingCabGameplay` w Unreal Editorze lub narzędziem potrafiącym wiarygodnie czytać `.uasset`; nie wnioskować o modifierach/triggerach z samej nazwy pliku.
5. Jedną spójną architekturę docelową dla klawiatury, dotyku i przyszłego gamepada, z jednym autorytatywnym źródłem stanu ciągłego.
6. Projekt telemetrii rejestrującej stan tylko przy zmianie albo wykrytej rozbieżności, aby nie zalać logu:
   - surowe `W/Up/Space/A/D/Left/Right`;
   - wartości IA;
   - wartości touch;
   - effective input;
   - suppression flags i aktywny input mode;
   - focus/capture viewportu;
   - aktualny controller, pawn i input component;
   - obecność mapping contextu;
   - fuel, hull i `CanUseThrusters()`;
   - `IsSimulatingPhysics()`;
   - faktycznie przyłożona siła, velocity i location.
7. Projekt dwóch testów:
   - długiego/randomizowanego soak testu wejścia z deterministycznym seedem i śladem możliwym do replay;
   - testu wyczerpania paliwa podczas trzymania thrust, który wyraźnie odróżnia stan wejścia od stanu napędu.
8. Minimalny plan migracji z obecnego mieszanego rozwiązania do architektury docelowej, z punktami weryfikacji i prostym rollbackiem.

## Najważniejsze pliki do przeglądu

- `Source/FlyingCabFlightLab/FlyingCabPawn.cpp/.h` — odczyt wejścia, agregacja keyboard/touch, gate thrusterów, `AddForce`, reset i telemetria.
- `Source/FlyingCabFlightLab/FlyingCabPlayerController.cpp/.h` — mapping context, input mode, focus flush, observer, quest journal, possession i touch UI.
- `Source/FlyingCabFlightLab/FlyingCabCharacter.cpp/.h` — alternatywna ścieżka wejścia on-foot nadal oparta o wartości Enhanced Input.
- `Source/FlyingCabFlightLab/FlyingCabTouchControls.cpp/.h` — mutable touch state i callbacki UMG.
- `Source/FlyingCabFlightLab/FlyingCabInputData.cpp/.h` — ładowanie assetów IA/IMC.
- `Source/FlyingCabFlightLab/FlyingCabVehicleVitalsComponent.cpp/.h` — `CanUseThrusters()`, paliwo i zniszczenie.
- `Source/FlyingCabFlightLab/FlyingCabFunctionalTests.cpp` — testy PIE i ich ograniczenia.
- `Source/FlyingCabFlightLab/FlyingCabCoreTests.cpp` — testy jednostkowe kontraktów inputu.
- `Config/DefaultInput.ini` — focus flush i ustawienia myszy/dotyku.
- `Content/Input/IA_FlyingCab*.uasset`
- `Content/Input/IMC_FlyingCabGameplay.uasset`
- `docs/AUDIT_IMPLEMENTATION_STATUS.md` — stan wdrożeń po wcześniejszym audycie.
- `../../AUDYT_UNREAL_FLYINGCABFLIGHTLAB_2026-08-10.md` — wcześniejszy audyt Claude'a, jako kontekst, nie jako źródło prawdy o bieżącym working tree.

## Zalecany protokół następnej reprodukcji

Przed kolejną zmianą architektury:

1. Uruchomić świeży Freeroam i włączyć telemetrię F3.
2. Zapisać, czy sesja działa w zwykłym czy immersive viewport.
3. Wykonać osobno:
   - czysty lot bez UI;
   - łańcuch S7;
   - serię nakładających się klawiszy alternatywnych (`W/Up/Space`, `A/Left`, `D/Right`);
   - lot do wyczerpania paliwa przy trzymanym thrust.
4. W chwili awarii nie restartować od razu. Zrobić screenshot F3 i HUD, puścić wszystkie klawisze, nacisnąć problematyczny klawisz ponownie raz i dopiero zakończyć PIE.
5. Zachować konkretny plik backup logu oraz zanotować, czy objaw oznaczał:
   - pojazd nadal przyspieszał po puszczeniu;
   - pojazd nie przyspieszał mimo trzymania;
   - po puszczeniu i ponownym wciśnięciu zachowanie się zmieniło.

Bez takiego rozróżnienia kolejne zgłoszenie „zablokowało się” nadal może łączyć dwie różne awarie.
