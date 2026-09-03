# Flying Cab — niezależny audyt blokowania sterowania (Claude, 2026-09-03)

Odpowiedź na handoff `INPUT_LOCKING_AUDIT_HANDOFF_2026-09-03.md`.

Projekt: `unreal/FlyingCabFlightLab` (UE 5.8.0-55116800, silnik `D:\Unreal\UE_5.8`).
Stan Git w chwili audytu: branch `codex/unreal-audit-fixes`, HEAD `8c40aa1` („backup”, 2026-09-03 10:55). Ten commit zawiera dokładnie working tree opisany w handoffie (etap 6: pojazd czyta surowe klawisze, postać piesza czyta wartości Enhanced Input). Working tree po audycie jest czysty.

Metoda:

1. Odczyt kodu projektu oraz kodu źródłowego silnika (PlayerInput, EnhancedInput, Slate, GameViewportClient, SceneViewport) — wszystkie twierdzenia o silniku mają odniesienia plik:linia do `D:\Unreal\UE_5.8`.
2. Odczyt assetów `IA_FlyingCab*`, `IMC_FlyingCabGameplay` i class defaults `BP_FlyingCabPawn` przez `UnrealEditor-Cmd -run=pythonscript` (nie ze stringów w `.uasset`).
3. Analiza logu S9 i sześciu pozostałych logów z 2026-09-02.
4. Jeden tymczasowy test-sonda PIE (`FlyingCab.Audit.InputFlushProbe`) dodany, uruchomiony dwa razy i usunięty; źródła nie zostały zmienione, binaria przebudowano po usunięciu sondy. Surowe wyniki są w rozdziale 12.

---

## 1. Podsumowanie wykonawcze

1. **Potwierdzona, deterministyczna przyczyna zatrzasku (latch) w warstwie Enhanced Input.** Gdy `FlushPressedKeys()` zostaje wywołane **z wnętrza delegata akcji Enhanced Input** (J → dziennik, R → reset, Q → possess/unpossess) w chwili, gdy klawisz ruchu jest trzymany, a gracz puści ten klawisz zanim system otrzyma auto-repeat z Windows, to wartość akcji (`FInputActionInstance::Value`) **zostaje zamrożona na wartości sprzed flushu** i `GetBoundActionValue()` / `GetActionValue()` zwraca ją aż do ponownego wciśnięcia tego samego klawisza. Sonda: scenariusze A, A′, D′, E (rozdział 12). W obecnym buildzie dotyczy to **postaci pieszej** (wyjście z pojazdu przy trzymanym D → postać sama idzie w prawo, prędkość 102 cm/s bez wciśniętego klawisza). W buildzie etapu 5 dotyczyło to pojazdu i tłumaczy S7 co do sekwencji i kierunku.
2. **S9 nie ma tej przyczyny.** Między startem trybu a zniszczeniem nie było żadnego flushu ani przejścia. Bilans paliwa (65 jednostek, 1,8/s w pionie = 36 s ciągu pionowego na zbiornik, sesja 182 s, drugi pasażer na pokładzie przez 151 s bez dowozu) czyni odcięcie ciągu przez `CanUseThrusters()` najbardziej prawdopodobnym wyjaśnieniem. Log sam w sobie nie rozstrzyga latch/drop; jedyną pozostałą ścieżką latchu jest utrata `WM_KEYUP` na poziomie systemu, której obecny build nie potrafi wykryć.
3. **Etap 6 (odczyt `IsInputKeyDown` w pojeździe) nie naprawia problemu, tylko go omija dla jednego konsumenta.** Postać piesza i każdy przyszły konsument wartości akcji (gamepad, remapping) dziedziczą usterkę. Zielone testy nie są dowodem: `InputTransitionChain` zamyka dziennik w tej samej klatce, w której go otworzył, co przypadkowo generuje „ratunkową” krawędź release i maskuje zatrzask (rozdział 7).
4. **Problem architektoniczny:** cztery równoległe źródła prawdy dla sterowania ciągłego, `FlushPressedKeys()` używane jako narzędzie przejść gameplayowych z 11 miejsc, stan wejścia cache'owany w pawnach i zerowany z sześciu punktów, klawisze zaszyte na sztywno, brak telemetrii odróżniającej „wejście żąda ciągu” od „napęd odcięty”. Docelowo: jedna ramka sterowania budowana w kontrolerze poza delegatami wejścia (rozdział 8).

---

## 2. Fakty o silniku zweryfikowane w kodzie UE 5.8

| ID | Fakt | Źródło |
|---|---|---|
| F1 | `UPlayerInput::FlushPressedKeys()` dla każdego klawisza z `bDown` wysyła symulowane `IE_Released` przez wirtualne `InputKey()`, potem dla wszystkich klawiszy zeruje `RawValue`, `bDown`, `bDownPrevious` i ustawia `bWasJustFlushed=true`. Nie kasuje akumulatorów zdarzeń. | `Engine/Private/UserInterface/PlayerInput.cpp:143-193` |
| F2 | Auto-reconcile: pierwsze `IE_Repeat` po flushu (lub po utworzeniu nowego `PlayerInput`) jest traktowane jak brakujące `IE_Pressed`; klawisz wraca do `bDown=true` natychmiast. CVar `bAutoReconcilePressedEventsOnFirstRepeat=true`. Komentarz silnika wprost wymienia „FlushPressedKeys przy zmianie input mode przy trzymanym klawiszu”. | `PlayerInput.cpp:53`, `412-436` |
| F3 | Surowy `bDown` klawisza cyfrowego jest liczony z różnicy liczby zdarzeń Pressed − Released w klatce; 0 = utrzymaj poprzedni stan. Zgubione `WM_KEYUP` daje więc trwałe `bDown=true` aż do flushu. Zdarzenia `IE_Repeat` nie zmieniają `bDown`. | `PlayerInput.cpp:1266-1282` |
| F4 | Kolejność w `ProcessInputStack`: `EvaluateKeyMapState` (EI liczy `KeyDownPrevious` PRZED przeniesieniem akumulatorów) → aktualizacja stanów klawiszy → `EvaluateInputDelegates` = `PrepareInputDelegatesForEvaluation` (ocena mapowań, post-tick akcji) → delegaty komponentów → ogon EI, w którym `bIsFlushingInputThisFrame=false` → `FinishProcessingPlayerInput`. | `PlayerInput.cpp:1302-1313`; `EnhancedPlayerInput.cpp:422-470` (linia 468) |
| F5 | EI wykrywa krawędź release TYLKO z `bDownLastTick && !bKeyIsDown`. Po flushu `bDownPrevious=false`, więc jedyną drogą do krawędzi jest `bKeyWasJustFlushed`, która wymaga `bIsFlushingInputThisFrame==true` w NASTĘPNYM `EvaluateKeyMapState` oraz niezerowego jeszcze `KeyState.Value`. Flush wykonany wewnątrz delegata jest zerowany w tej samej klatce (F4), więc krawędź nie powstaje. | `EnhancedPlayerInput.cpp:375-420`, `592-601` |
| F6 | `FInputActionInstance::Value` jest resetowane wyłącznie przy pierwszym zdarzeniu klawisza tej akcji w klatce; akcja bez zdarzeń zachowuje starą wartość, mimo że jej `TriggerState` spada do `None` (event `Completed`). | `EnhancedPlayerInput.cpp:206-227`, `671-717` |
| F7 | `FInputActionInstance::GetValue()` w 5.8 zwraca surowe `Value` niezależnie od stanu triggera (CVar `EnhancedInput.bAlwaysGetRealValueFromActionInstanceData=1`, brak override w configach silnika i projektu). Value bindingi (`GetBoundActionValue`) kopiują dokładnie tę wartość. F5+F6+F7 = zamrożona wartość akcji. | `EnhancedInput/Private/InputAction.cpp:18-21`, `51-65`; `EnhancedPlayerInput.cpp:912-926` |
| F8 | `UEnhancedPlayerInput::FlushPressedKeys()` oznacza trzymane mapowania `bShouldBeIgnored`, ale symulowane `IE_Released` z F1 przechodzi przez override `InputKey`, który natychmiast tę flagę czyści. Flaga jest w praktyce martwa; po flushu trzymany klawisz wraca (F2) razem z akcją. Sonda C i D2 potwierdzają. | `EnhancedPlayerInput.cpp:294-328`, `1021-1066` |
| F9 | Przebudowa mapowań (`RebuildControlMappings`) ignoruje do puszczenia tylko mapowania NOWE względem poprzedniego zestawu; usunięcie i ponowne dodanie kontekstu (etap 4) czyni wszystkie mapowania „nowymi”. Przebudowa jest odroczona do ticku modułu. | `EnhancedInputSubsystemInterface.cpp:886-1200` (1172-1183), `EnhancedInputModule.cpp:322-327` |
| F10 | Utrata focusu viewportu lub dezaktywacja okna → `UGameViewportClient::LostFocus` → `FlushPressedKeys()` na każdym PC (setting `bShouldFlushPressedKeysOnViewportFocusLost` lub override). Dzieje się to podczas pompowania Slate, czyli POZA delegatami — krawędź release powstaje poprawnie. | `Engine/Private/GameViewportClient.cpp:2632-2655`; `Slate/SceneViewport.cpp:1444-1463`, `1563-1586` |
| F11 | `FInputModeUIOnly` ustawia `SetIgnoreInput(true)`: podczas otwartego dziennika ani key-up, ani auto-repeat nie docierają do `PlayerInput`. Nie ma więc „ratunku” z F2 dla klawisza trzymanego przy otwieraniu dziennika. `FInputModeGameAndUI` przywraca przyjmowanie wejścia i ustawia `CaptureDuringMouseDown`. | `Engine/Private/PlayerController.cpp:6372-6387`, `6398-6414`, `6454-6468` |
| F12 | Kontroler w pauzie nadal wykonuje `TickPlayerInput` (stany klawiszy są przetwarzane), ale akcje z `bTriggerWhenPaused=false` dostają `TriggerState=None`. | `PlayerController.cpp:5504-5516`; `EnhancedPlayerInput.cpp:699-702` |
| F13 | `AppliedInputContextData` żyje na `UEnhancedPlayerInput` (per kontroler). Po przeładowaniu poziomu nowy kontroler nie ma kontekstu, więc `EnsureEnhancedInputContext()` go dodaje i mapowania są budowane. Hipoteza „brak mapowań po restarcie poziomu” jest wykluczona. | `EnhancedInputSubsystemInterface.cpp:260-290`, `698-713` |
| F14 | Pawn tickuje po kontrolerze (prerequisite dodawany w `AController::Possess`), więc `RefreshKeyboardInputState()` w pawnie czyta stan z bieżącej klatki. | `Engine/Private/Controller.cpp:503-514` |

---

## 3. Ranking kandydatów root cause

### R1 — zamrożona wartość akcji Enhanced Input po flushu z wnętrza delegata (POTWIERDZONY)

Mechanizm: F4 + F5 + F6 + F7. Warunki konieczne, wszystkie spełnione w projekcie:

1. `FlushPressedKeys()` wykonuje się wewnątrz `EvaluateInputComponentDelegates`, tj. w obsłudze akcji EI. W projekcie: `OpenQuestJournal()` (`FlyingCabPlayerController.cpp:1013`) i `CloseQuestJournal()` (`:1034`) przez `ToggleQuestJournal` zbindowane do `IA_FlyingCabQuestJournal` Started (`:343-351`); `ResetVehicle()` przez `IA_FlyingCabRestart` Started (`FlyingCabPawn.cpp:260-261`, flush w `ClearAllInputState`, `:556-562`); `TryExitVehicle`/`TryEnterVehicle` przez `IA_FlyingCabInteract` Started → `Possess()` → `UnPossessed()`/`PossessedBy()` → flush (`FlyingCabPawn.cpp:269-281`).
2. W chwili flushu trzymany jest klawisz zmapowany na akcję ciągłą (A/D/Left/Right/W/Up/Space).
3. Klawisz zostaje puszczony, zanim do `PlayerInput` dotrze auto-repeat, albo auto-repeat nie może dotrzeć (dziennik: F11). Jeśli repeat dotrze wcześniej, stan sam się naprawia (sonda D1→D2→D3).

Skutek: `GetActionValue`/`GetBoundActionValue` zwraca ±1 lub `true` przy wszystkich klawiszach puszczonych, aż do ponownego wciśnięcia i puszczenia tego samego klawisza. Zerowanie cache'u w pawnie (`ReleaseKeyboardInputState`) nic nie daje, bo następny tick ponownie czyta zamrożoną wartość — to jest zgodne z S2 („po restarcie nadal zablokowany”), choć S2 pochodzi ze starszego buildu i nie da się tego dowieść.

Dowody: sonda A (dziennik otwarty 10 klatek, A puszczone, zamknięcie przez `CloseQuestJournal`: wartość −1 po zamknięciu), A′ (zamknięcie przez `HandleNavigationKey(J)`: +1), D′ (R przy trzymanym W, natychmiastowe puszczenie: thrust=1 przy surowym W=0), E (wyjście z pojazdu przy trzymanym D, puszczenie: postać ma `LastMovementInput X=1.00` i prędkość 102 cm/s bez klawisza). Kontrdowody: B (flush spoza delegata) i F (observer przez `InputKey`, spoza delegata) są czyste.

Kto jest dziś podatny: `AFlyingCabCharacter::RefreshKeyboardInputState` (`FlyingCabCharacter.cpp:254-272`, `GetBoundActionValue`). Pojazd etapu 6 nie jest podatny tylko dlatego, że czyta surowe klawisze (`FlyingCabPawn.cpp:510-541`). Pojazd etapu 5 był podatny — patrz S7.

### R2 — odcięcie ciągu przez `CanUseThrusters()` przy pustym paliwie (bardzo prawdopodobny dla S9 i S5; „drop”)

`AFlyingCabPawn::Tick` zeruje siłę, gdy `Vitals->CanUseThrusters()` jest fałszywe (`FlyingCabPawn.cpp:204-206`; `FlyingCabVehicleVitalsComponent.cpp:164-167`). Trzymany W z paliwem 0 blokuje regenerację (`Advance`, `:57-78`: regeneracja tylko bez wejścia i przy opadaniu), więc stan „martwy ciąg” jest lepki dopóki gracz nie puści wszystkiego. Potwierdzone class defaults `BP_FlyingCabPawn`: `StartingFuel=65`, `MaxFuel=100`, `VerticalFuelPerSecond=1.8`, `HorizontalFuelPerSecond=0.9`, `DescentRegenerationPerSecond=0.12`. Zbiornik startowy = 36 s ciągu pionowego. W buildzie S9 nie było ani logu, ani komunikatu HUD o pustym paliwie; obecny build ma jedno i drugie, ale nie zapisuje zmiany w telemetrii (tylko F3 na ekranie).

### R3 — zgubione `WM_KEYUP` na poziomie Windows/Slate → trwałe `bDown=true` (możliwy, niepotwierdzony)

Z F3 wynika, że jedna zgubiona krawędź release trwale zatrzaskuje surowy stan klawisza, który obecny pojazd czyta bezpośrednio. Silnik zabezpiecza to tylko flushem przy utracie focusu (F10), z komentarzem „keyup events will sometimes not be processed (such as going into immersive/maximized mode)”. Sesje S1–S4 i S9 nie mają danych, które by to potwierdziły lub wykluczyły. W obecnym buildzie nie ma żadnego detektora tego stanu (proponowany w rozdziale 9: klawisz `bDown` bez auto-repeat przez > 1,5 s).

### R4 — flush przy utracie focusu jako „drop” (realny, ale przejściowy)

Każda utrata focusu viewportu (alt-tab, kliknięcie w okno edytora poza viewportem, `LevelEditor.ToggleImmersive` w S9 o 13:59:58) zeruje trzymane klawisze (F10). Dzięki F2 stan wraca po opóźnieniu repeat systemu Windows (250–1000 ms zależnie od ustawień). Objaw: krótkie „szarpnięcie” ciągu, nie trwałe odcięcie. Nie tłumaczy S9 (awaria 3 minuty po immersive).

### R5 — etap 4: usuwanie/dodawanie mapping contextu + ignorowanie trzymanych klawiszy (historyczny, „drop”)

W etapie 4 dziennik zdejmował i zakładał kontekst; przebudowa jest odroczona i traktuje wszystkie mapowania jako nowe (F9), więc klawisz wciśnięty między zamknięciem dziennika a tickiem przebudowy był ignorowany do puszczenia. To „drop”, nie „latch”. Etap 5 usunął tę ścieżkę i odsłonił R1.

### R6 — stan dotykowy/UMG pozostawiony aktywny (możliwy w testach myszą; wykluczony dla S9)

`SButton` przechwytuje mysz przy wciśnięciu, więc `OnReleased` dociera także poza przyciskiem; `OnUnhovered` jest dodatkowym zwolnieniem (`FlyingCabTouchControls.cpp:935-948`). Log S9: `touch thrust 0.00`. Ryzyko pozostaje przy równoczesnym `ReleaseInterfaceInputs()` z pawna i trzymanym przycisku (flaga w widgecie zerowana, przycisk nadal wciśnięty → brak ciągu do ponownego naciśnięcia) — to „drop”, nie „latch”.

### R7 — fizyka (uśpione ciało, zamrożenie po observerze) — wykluczony

`AddForce` i `SetPhysicsLinearVelocity` co tick budzą ciało; `RestoreControlledVehicleAfterObserver` przywraca symulację i prędkość (`FlyingCabPlayerController.cpp:510-523`), test `DeveloperObserver` to sprawdza. Tick pojazdu wychodzi wcześnie tylko przy `!IsSimulatingPhysics()` (`FlyingCabPawn.cpp:197-200`), co dziś nie jest logowane.

### R8 — stos `InputComponent`/possession — wykluczony jako osobna przyczyna

Wartość akcji żyje w `ActionInstanceData` współdzielonym przez wszystkie komponenty tego `PlayerInput`; komponent postaci pieszej dostaje zamrożoną wartość natychmiast po possess (sonda E). To jest element R1, nie osobna usterka.

### R9 — konfiguracja IA/IMC — wykluczona (rozdział 6)

### R10 — brak mapowań po przeładowaniu poziomu — wykluczony (F13)

---

## 4. Ocena S7 (observer → gameplay → pickup → dziennik w locie → zablokowane w lewo)

Build S7: etap 5 (stały kontekst, `GetBoundActionValue` co tick w pojeździe). Sekwencja zgodna z R1 co do joty:

1. Gracz leci w lewo (A trzymane), naciska J. Delegat `ToggleQuestJournal` → `OpenQuestJournal` → `FlushPressedKeys()` wewnątrz delegata: surowe A zostaje wyzerowane, `Horizontal.Value` zostaje na −1 (F5, F6).
2. Dziennik jest w pauzie i `UIOnly`; ani key-up A, ani auto-repeat nie docierają do `PlayerInput` (F11). Gracz puszcza A, nawiguje, zamyka.
3. Po zamknięciu `bQuestJournalOpen=false`; pawn etapu 5 czyta `GetBoundActionValue(Horizontal)` = −1 i leci w lewo bez klawisza, aż gracz sam naciśnie A. Kierunek „w lewo” i moment „po zamknięciu” pasują do zgłoszenia.

Observer i pickup w S7 nie są potrzebne do reprodukcji: sonda A′ odtwarza to bez nich. Poprawka etapu 5 („stały mapping context”) nie mogła tego naprawić, bo usunęła R5, nie R1. Etap 6 ukrył R1 dla pojazdu.

## 5. Ocena S9 (bez zmiany okien, zablokowane w górę)

Fakty z logu `FlyingCabFlightLab-backup-2026.09.02-14.03.11.log`:

- 13:59:56 PIE; 13:59:57 Freeroam; 13:59:58 `LevelEditor.ToggleImmersive`; 14:00:04 pickup; 14:00:28 dowóz i drugi pickup; 14:02:59 uderzenie 1441,7 cm/s, hull 0, `keyboard thrust 1.00`, `touch 0.00`; 14:03:02 recovery; koniec PIE.
- Brak jakiegokolwiek wpisu `Input state cleared` między 13:59:56 a 14:02:59, brak observera, dziennika, resetu, possess. Każdy flush pawna loguje się także przy zerowym wejściu (`ClearAllInputState`, `FlyingCabPawn.cpp:582-589`), więc nieobecność wpisu jest dowodem braku flushu pawna. Flushe kontrolera (start trybu, menu) miały miejsce tylko o 13:59:57.
- Wniosek 1: **R1 jest wykluczony dla S9** (brak flushu z delegata).
- Wniosek 2: bilans paliwa. Zbiornik 65 = 36 s ciągu pionowego lub 72 s poziomego. W grze z grawitacją utrzymanie wysokości wymaga ciągu przez znaczną część czasu; 182 s sesji i 151 s drugiego kursu bez dowozu mieszczą się w profilu „paliwo skończyło się w drugiej minucie, gracz nie mógł dolecieć, w końcu spadł”. Tankowanie nie jest widoczne w logu, ale log zakupu jest na poziomie Verbose (`FlyingCabFuelStation.cpp:192-198`), więc nie można go wykluczyć.
- Wniosek 3: prędkość uderzenia. `NormalImpulse/Mass` = (1+e)·Δv; 1441,7 odpowiada ~1100–1300 cm/s — zarówno maksymalnemu opadaniu (`MaxFallSpeed=1300`, scenariusz „ciąg odcięty”), jak i maksymalnemu wznoszeniu (`MaxClimbSpeed=1150`, scenariusz „ciąg zatrzaśnięty”). Nie rozstrzyga.
- Wniosek 4: `thrust 1.00` w logu oznacza tylko, że EI uważało W za wciśnięte. Jest to zgodne z (a) fizycznie trzymanym W przy pustym paliwie i (b) zatrzaśniętym surowym stanem po zgubionym key-up (R3). Wariant (b) nie ma żadnego wsparcia w danych, wariant (a) ma wsparcie w bilansie paliwa.

**Werdykt:** log S9 sam w sobie jest nierozstrzygający między latch a odcięciem; w połączeniu z parametrami paliwa i brakiem jakichkolwiek przejść wskazuje na odcięcie przez zasoby (R2) jako najbardziej prawdopodobne, z R3 jako jedyną pozostałą ścieżką zatrzasku. Diagnostyka z rozdziału 9 (zapis `CanUseThrusters`, paliwa, surowych klawiszy i siły przy każdej zmianie) rozstrzygnie następne wystąpienie jednoznacznie.

Uwaga do S5 („co jakiś czas losowo wyłącza ciąg mimo wciśniętego przycisku”): ten sam profil co R2 plus R4 (krótkie zaniki po utracie focusu).

---

## 6. Audyt assetów Enhanced Input (odczyt przez edytor)

`IMC_FlyingCabGameplay`: `RegistrationTrackingMode=Untracked`, `InputModeFilterOptions=UseProjectDefaultQuery`, 12 mapowań domyślnych, 0 profili nadpisujących:

| # | Klawisz | Akcja | Modifiers | Triggers |
|---|---|---|---|---|
| 0 | A | IA_FlyingCabHorizontal | Negate | — |
| 1 | Left | IA_FlyingCabHorizontal | Negate | — |
| 2 | D | IA_FlyingCabHorizontal | — | — |
| 3 | Right | IA_FlyingCabHorizontal | — | — |
| 4 | W | IA_FlyingCabThrust | — | — |
| 5 | Up | IA_FlyingCabThrust | — | — |
| 6 | SpaceBar | IA_FlyingCabThrust | — | — |
| 7 | E | IA_FlyingCabService | — | — |
| 8 | R | IA_FlyingCabRestart | — | — |
| 9 | F3 | IA_FlyingCabTelemetry | — | — |
| 10 | Q | IA_FlyingCabInteract | — | — |
| 11 | J | IA_FlyingCabQuestJournal | — | — |

Akcje: `Horizontal` = Axis1D, `AccumulationBehavior=Cumulative` (A+D = 0, poprawne); pozostałe Boolean, `TakeHighestAbsoluteValue`. Wszystkie: `ConsumeInput=true`, `ConsumesActionAndAxisMappings=true`, `ReserveAllMappings=false`, `TriggerWhenPaused=false`, brak triggerów i modyfikatorów na poziomie akcji. Jedyny modyfikator w projekcie to `InputModifierNegate` na A/Left.

Ocena: konfiguracja jest minimalna i poprawna; nie zawiera semantyki (Hold, Tap, Chord, dead zone), która mogłaby tworzyć zatrzaski lub przerwy. `TriggerWhenPaused=false` jest właściwe, ale oznacza, że w pauzie dziennika akcje nie mogą „dokończyć” release (F12) — istotne dla R1. Hipoteza 9 z handoffu (nieoczekiwana agregacja przy nakładaniu W/Up/Space) jest wykluczona: Boolean z TakeHighestAbsoluteValue daje `true`, gdy dowolny z klawiszy jest wciśnięty, i wartość resetuje się przy każdym zdarzeniu klawisza tej akcji.

Class defaults `BP_FlyingCabPawn` są identyczne z C++ (paliwo jak wyżej; `VerticalThrustAcceleration=2350`, `HorizontalThrustAcceleration=1400`, `MaxClimbSpeed=1150`, `MaxFallSpeed=1300`, `MaxHorizontalSpeed=1050`, `DamageImpactSpeedThreshold=700`, `DamageFullHullSpeed=1400`).

---

## 7. Dlaczego 9/9 testów PIE jest zielonych mimo R1

`FFlyingCabVerifyInputTransitionChainCommand` (`FlyingCabFunctionalTests.cpp:879-1039`): w fazie 4 test zamyka dziennik (`HandleNavigationKey(J)`) w tej samej klatce, w której faza 3 go otworzyła, i dopiero potem wysyła release A. Zamknięcie wywołuje `FlushPressedKeys()` **spoza** delegata (z latent command), co ustawia `bIsFlushingInputThisFrame=true` na następną klatkę; ponieważ `KeyState.Value` klawisza A nie zostało jeszcze wyzerowane, EI generuje krawędź release (F5) i wartość akcji spada do 0. Sonda A pokazuje, że wystarczy przytrzymać dziennik otwarty przez jedną klatkę więcej, żeby ta ratunkowa krawędź zniknęła. Testy `EnhancedInputRelease`, `EnhancedInputFocusFlush` i `QuestJournalInput` sprawdzają pole cache'owane w pawnie, które w obecnym buildzie i tak jest zerowane przez suppression, a nie stan EI.

Dodatkowo żaden test nie wysyła `IE_Repeat`, więc ścieżki F2/F8 (auto-reconcile po flushu) nigdy nie są ćwiczone, a realne zachowanie z klawiaturą różni się od testowanego (po flushu klawisz trzymany wraca sam po opóźnieniu repeat).

---

## 8. Architektura docelowa: jedna ramka sterowania

Zasady:

1. **Jedno źródło prawdy dla sterowania ciągłego:** `UFlyingCabControlInputComponent` na `AFlyingCabPlayerController`. Pawny i postać nie mają pól `Keyboard*`, `Touch*`, nie czytają `PlayerInput`, nie wołają `FlushPressedKeys()`. Dostają co klatkę niezmienną `FFlyingCabControlFrame { float Horizontal; float Thrust; bool Service; EFlyingCabInputSource Sources; EFlyingCabInputBlock Block; uint64 Frame; }` przez `ApplyControlFrame()`.
2. **Wartości klawiatury/gamepada z Enhanced Input, ale bindowane na `InputComponent` kontrolera**, nie pawna (`BindActionValue` w `SetupInputComponent`). Possession nie zmienia wtedy ani bindingów, ani odczytu. Gamepad = dodatkowe mapowania w IMC, zero zmian w kodzie.
3. **Walidacja krzyżowa zamiast zaufania jednej warstwie:** komponent traktuje wartość akcji jako ważną tylko, gdy przynajmniej jeden klawisz zmapowany na tę akcję (lista z `QueryKeysMappedToAction`, nie hardcode) jest `IsInputKeyDown` albo w tej klatce miał zdarzenie. Rozbieżność „akcja ≠ 0, żaden klawisz nie wciśnięty” przez ≥ 2 klatki = wymuszone zero + wpis telemetrii `STALE_ACTION_VALUE`. To domyka R1 niezależnie od przyszłych zmian silnika, a jednocześnie nie hardkoduje klawiszy.
4. **Dotyk jako drugi kanał w tym samym komponencie:** widget woła `SetTouchControl(EFlyingCabTouchControl, bool bPressed)`; komponent trzyma stany, a `NativeDestruct`, `NativeOnFocusLost`, `NativeOnMouseLeave`, ukrycie i zmiana trybu wołają `ReleaseAllTouch()`. Zasada łączenia: poziom = clamp(klawiatura + dotyk), pion = max — jak dziś, ale w jednym miejscu.
5. **Jawna maszyna stanów trybu wejścia** w kontrolerze: `Gameplay`, `Menu`, `QuestJournal`, `Observer`, `Transition`. Suppression jest funkcją stanu (jedno `if`), a nie trzema flagami czytanymi z trzech klas.
6. **Przejścia nigdy wewnątrz delegatów wejścia.** Delegaty (J, R, Q, O, przyciski UMG) tylko rejestrują `RequestInputMode(...)` / `RequestPossess(...)`; kontroler realizuje żądania na początku `PlayerTick` (po `Super::PlayerTick`, czyli po `ProcessInputStack`, F4) lub przez `SetTimerForNextTick`. W tym miejscu wykonuje się jedno `FlushPressedKeys()` wyłącznie wtedy, gdy UI przejmuje focus, i zerowanie ramki. Flush poza delegatem generuje poprawną krawędź release (sonda B, F), więc R1 znika także bez punktu 3.
7. **Gate'y napędu są danymi, nie ciszą:** `ApplyControlFrame` w pojeździe zwraca `EFlyingCabThrustBlock` (None, NoFuel, Destroyed, PhysicsOff, Suppressed), a HUD i telemetria pokazują to jawnie. Reset `R` czyści ramkę przez komponent, nie przez `FlushPressedKeys()`.
8. **Observer** czyta te same akcje (własny IMC o wyższym priorytecie z `ConsumeInput`, albo te same akcje filtrowane stanem), zamiast `IsInputKeyDown` w `PlayerTick` i przechwytywania `O` w `InputKey`.

Co znika: `ReleaseKeyboardInputState`, `ClearAllInputState`, `SetTouch*` na pawnie i postaci, override `FlushPressedKeys` w kontrolerze (zostaje wyłącznie wersja bazowa dla focus-lost), hardkodowane klawisze w `FlyingCabPawn.cpp:524-540` i `FlyingCabPlayerController.cpp:119-131`.

---

## 9. Telemetria (zapis tylko przy zmianie lub rozbieżności)

Kategoria `LogFlyingCabInputTrace`, jedna linia na zmianę, plus ring buffer ostatnich 5 s zrzucany przy zniszczeniu, resecie, recovery, wykrytej rozbieżności i na żądanie (F3 przytrzymane). Snapshot porównywany co klatkę:

- surowe: `IsInputKeyDown` dla W/Up/Space/A/Left/D/Right/E oraz czas od ostatniego zdarzenia (Pressed/Repeat/Released) per klawisz;
- akcje: `GetActionValue(Horizontal)`, `GetActionValue(Thrust)`, `GetActionValue(Service)` i `TriggerEvent` z `FInputActionInstance` (przez `GetActionInstanceData`);
- dotyk: cztery flagi z komponentu;
- effective: ramka sterowania + `Sources` + `Block`;
- stan: tryb wejścia, `IsGameplayInputSuppressed`, `GetCurrentInputModeDebugString()`, `bShowMouseCursor`, `HasMappingContext(IMC)`, klasa i nazwa posiadanego pawna, adres `InputComponent` pawna, liczba komponentów w stosie (`BuildInputStack` jest protected; wystarczy `GetPawn()->InputComponent` i `InputComponent` kontrolera);
- focus: `FSlateApplication::Get().IsActive()`, `HasAnyUserFocus()` viewportu (`GetGameViewport()->GetGameViewportWidget()`), `IsPaused()`;
- pojazd: paliwo, hull, `CanUseThrusters()`, `IsDestroyed()`, `IsSimulatingPhysics()`, `IsAnyRigidBodyAwake()`, wektor siły przekazany do `AddForce`, prędkość po clampie, pozycja.

Detektory rozbieżności (każdy loguje Warning z pełnym snapshotem):

1. `STALE_ACTION_VALUE`: wartość akcji ≠ 0 przy żadnym zmapowanym klawiszu `bDown` przez ≥ 2 klatki (R1).
2. `RAW_KEY_NO_REPEAT`: klawisz `bDown` przez > 1,5 s bez żadnego zdarzenia Repeat/Pressed — sygnatura zgubionego key-up (R3). Uwaga: użytkownik z wyłączonym auto-repeat da fałszywe alarmy; próg konfigurowalny.
3. `THRUST_REQUESTED_NOT_APPLIED`: żądany ciąg > 0 i siła 0 → powód (`NoFuel`/`Destroyed`/`PhysicsOff`/`Suppressed`) — rozdziela „input aktywny” od „napęd odcięty” (R2).
4. `FLUSH_IN_DELEGATE`: w debug buildzie `FlushPressedKeys()` wywołane, gdy `UEnhancedPlayerInput` jest w trakcie `EvaluateInputDelegates` (flaga ustawiana w hooku `PreProcessInput`/`PostProcessInput`) — strażnik architektury.
5. `FOCUS_LOST`/`FOCUS_GAINED` i `INPUT_MODE_CHANGED` jako zdarzenia.

Format: `[frame][t] key:W↓ Up↑ Sp↑ A↑ L↑ D↑ R↑ E↑ | act:H=-1.00 T=1 S=0 | touch:0000 | eff:H=-1.00 T=1.00 src=KB blk=None | mode=Gameplay sup=0 imc=1 pawn=BP_FlyingCabPawn_C_0 ic=0x... | focus=1 active=1 paused=0 | fuel=12.4 hull=89.3 thr=ON phys=SIM awake=1 F=(0,0,235000) v=(…) p=(…)`.

---

## 10. Dwa testy

### 10.1 Soak test wejścia z seedem i śladem do replay

`FlyingCab.Functional.PIE.InputSoak` (parametr: seed; domyślnie 1977, druga instancja z seedem z `FDateTime`). Model referencyjny w teście: fizyczny stan klawiszy + stan dotyku + tryb. Pętla 3000 kroków, każdy krok w osobnej klatce:

- losowe zdarzenia z wagami: press/release jednego z {W, Up, Space, A, Left, D, Right, E} (40%), `IE_Repeat` dla wszystkich klawiszy „fizycznie” trzymanych dłużej niż 15 klatek co 2 klatki (emulacja Windows), J z zamknięciem po 1–40 klatkach (8%), R (4%), O toggle (4%), Q wyjście i po 5–30 klatkach ponowne wejście (3%), bezpośredni `FlushPressedKeys()` jako emulacja utraty focusu (3%), kliknięcie/puszczenie przycisku dotykowego przez `SetTouch*` (8%), nic (30%);
- po każdym kroku oczekiwanie: ramka sterowania (lub `GetTest*` pawna/postaci) równa modelowi z tolerancją: 1 klatka po zdarzeniu klawisza; po flushu przy trzymanym klawiszu dozwolone 0 do czasu pierwszego emulowanego repeat; podczas Menu/Journal/Observer oczekiwane 0;
- ślad: CSV w `Saved/Automation/InputSoak_<seed>.csv` (krok, klatka, zdarzenie, oczekiwane, faktyczne, snapshot z rozdziału 9). Przy pierwszej rozbieżności test zrzuca ostatnie 50 kroków do logu i kończy się błędem; ten sam seed odtwarza przebieg.

### 10.2 Wyczerpanie paliwa przy trzymanym ciągu

`FlyingCab.Functional.PIE.FuelCutoffWhileHoldingThrust`:

1. Start Freeroam; `Vitals->InitializeVitals` z `StartingFuel=1.0` (albo `Advance` w pętli do zapasu 1 jednostki).
2. W wciśnięte; co 2 klatki `IE_Repeat`. Dopóki paliwo > 0: `GetTestKeyboardThrustInput()==1`, `CanUseThrusters()==true`, prędkość Z rośnie (siła > 0).
3. Po spadku do 0: nadal `GetTestKeyboardThrustInput()==1` (wejście żywe), `CanUseThrusters()==false`, siła 0, log `Fuel exhausted` dokładnie raz, HUD zawiera `FUEL EMPTY // THRUST OFF`, detektor `THRUST_REQUESTED_NOT_APPLIED` zgłosił `NoFuel`.
4. Release W, press W: dalej brak ciągu (wejście żywe, napęd odcięty). Dopiero `AddFuel(20)` przywraca siłę w następnej klatce.

---

## 11. Plan migracji z punktami weryfikacji i rollbackiem

Każdy krok = osobny commit na `codex/unreal-audit-fixes`; rollback = `git revert` kroku.

0. **Utrwalenie dowodu.** Dodać na stałe test regresyjny z trzema scenariuszami sondy (A′, D′, E z rozdziału 12) jako `FlyingCab.Functional.PIE.FlushInsideDelegateDoesNotLatch`; poprawić `InputTransitionChain`, żeby trzymał dziennik otwarty ≥ 5 klatek i wysyłał `IE_Repeat`. Oczekiwane: nowe testy czerwone na HEAD. Telemetria z rozdziału 9 w tym samym kroku (czysto addytywna).
1. **Containment (mały diff, natychmiastowa ulga):** przenieść `OpenQuestJournal`, `CloseQuestJournal`, `ResetVehicle`, `TryExitVehicle`, `TryEnterVehicle` na „żądanie + realizacja w następnym ticku” (`SetTimerForNextTick` albo kolejka w `PlayerTick`), usunąć `FlushPressedKeys()` z `ClearAllInputState` (zerowanie cache zostaje). Weryfikacja: testy z kroku 0 zielone, sonda B/F nadal czysta, 9 starych testów zielone. Rollback: revert. Alternatywa awaryjna bez zmian w kodzie: `[ConsoleVariables] EnhancedInput.bAlwaysGetRealValueFromActionInstanceData=0` w `DefaultEngine.ini` przywraca stare zachowanie `GetValue()` (zero, gdy nie Triggered) i maskuje R1 dla value bindingów; CVar jest oznaczony do usunięcia, więc tylko tymczasowo.
2. **Komponent sterowania + ramka** za CVarem `flyingcab.UseControlFrame` (domyślnie 1, rollback = 0): najpierw pojazd (usunięcie hardcodu klawiszy), potem postać, potem dotyk. Weryfikacja: soak 10.1 na dwóch seedach, fuel 10.2, ręczny protokół z rozdziału 13.
3. **Usunięcie starych ścieżek** (`ReleaseKeyboardInputState`, `SetTouch*` na pawnach, override `FlushPressedKeys`, `IsInputKeyDown` w observerze, przechwytywanie `O` w `InputKey`) i CVaru z kroku 2. Weryfikacja: pełny pakiet PIE + Core.
4. **Gamepad**: wyłącznie mapowania w IMC + dead zone modifier; test soak rozszerzony o `Gamepad_LeftX` przez `InputAxis`.

---

## 12. Wyniki sondy (UE 5.8, PIE, `-nullrhi`, zdarzenia symulowane przez `APlayerController::InputKey`)

Odczyt: `horizontalAction` = `UEnhancedPlayerInput::GetActionValue(IA_FlyingCabHorizontal)`, `thrustAction` = `GetActionValue(IA_FlyingCabThrust)`, `raw*` = `IsInputKeyDown`.

```text
A  A trzymane, J → dziennik (flush w delegacie):
   A1 tuż po otwarciu:                    horizontal=-1.00 rawA=0
   A2 10 klatek później, A nadal trzymane: horizontal=-1.00 rawA=0
   A3 A puszczone, 5 klatek:               horizontal=-1.00 rawA=0
   A4 CloseQuestJournal(), 3 klatki:       horizontal=-1.00 rawA=0   <- ZATRZASK
   A5 po ponownym tapnięciu A:             horizontal= 0.00
A' jak A, ale D i zamknięcie przez HandleNavigationKey(J):
   po zamknięciu:                          horizontal=+1.00 rawD=0   <- ZATRZASK (S7)
B  D trzymane, FlushPressedKeys() spoza delegata, D puszczone:
   po flushu:                              horizontal= 0.00 rawD=0
   po puszczeniu:                          horizontal= 0.00 rawD=0   <- czysto
C  W trzymane, flush spoza delegata, IE_Repeat, release:
   po flushu: thrust=0 rawW=0 | po repeat: thrust=1 rawW=1 | po release: thrust=0 rawW=0  <- auto-reconcile
D  W trzymane, R (flush w delegacie pawna), 5 klatek, IE_Repeat, release:
   po R: thrust=1 rawW=0 | po repeat: thrust=1 rawW=1 | po release: thrust=0 rawW=0  <- repeat ratuje
D' W trzymane, R, natychmiastowe puszczenie W (bez repeat), 3 klatki:
   thrust=1 rawW=0                                                     <- ZATRZASK
F  A trzymane, O (observer przez InputKey, spoza delegata), release, O:
   horizontal=0.00 w obu punktach                                       <- czysto
E  D trzymane, Q (wyjście z pojazdu; flush w delegacie), D puszczone, 5 klatek:
   horizontal=+1.00 rawD=0 pawn=character lastMovementInput=(1,0,0) velocityX=102.4  <- ZATRZASK, postać idzie sama
   po tapnięciu D: horizontal=0.00 lastMovementInput=(0)
```

Uwaga do czasu: sonda nie wysyła `IE_Repeat` bez polecenia, więc „rawA=0 przy trzymanym A” w A2 odpowiada sytuacji, w której klawisz jest trzymany krócej niż opóźnienie repeat Windows albo repeat nie może dotrzeć (dziennik, F11). Z prawdziwą klawiaturą i otwartym dziennikiem wynik A4 jest identyczny, bo `SetIgnoreInput(true)` odcina repeat.

---

## 13. Protokół następnej reprodukcji (uzupełnienie protokołu Codexa)

1. Przed sesją: telemetria z rozdziału 9 włączona, `Saved/Logs` czyste, F3 włączone.
2. Zapisać ustawienie Windows „Repeat delay” (Panel sterowania → Klawiatura); to determinuje okno, w którym trzymany klawisz po flushu jest martwy.
3. Sekwencje osobno, każda z 20 s czystego lotu po niej: (a) dziennik przy trzymanym A, puścić A w dzienniku, zamknąć J; (b) R przy trzymanym W i puszczenie W w tej samej chwili; (c) Q przy trzymanym D, puścić D po wyjściu; (d) lot do 0 paliwa przy trzymanym W; (e) alt-tab przy trzymanym W i powrót bez puszczania.
4. W chwili objawu: nie restartować; zanotować, czy pojazd nadal przyspiesza po puszczeniu (latch) czy nie reaguje na trzymanie (drop); spojrzeć na F3 (`Thrusters ENABLED/DISABLED`, `keyboard`, `touch`); tapnąć problematyczny klawisz raz; dopiero potem zakończyć PIE i zachować log.

---

## 14. Pliki i symbole, do których odnosi się audyt

Projekt: `FlyingCabPawn.cpp` (Tick 190-249, SetupPlayerInputComponent 251-267, PossessedBy/UnPossessed 269-281, ResetVehicle 312-347, RecoverVehicle 474-493, RefreshKeyboardInputState 510-541, ClearAllInputState 543-590, EnterDestroyedState 773-788); `FlyingCabPlayerController.cpp` (InputKey 139-165, FlushPressedKeys 167-179, StartRunMode 181-208, EnterMenuInputMode 294-300, RestoreGameplayInputMode 302-312, SetupInputComponent 334-358, OnPossess 360-399, SetDeveloperObserverMode 410-450, EnsureEnhancedInputContext 525-545, TryExitVehicle 631-676, TryEnterVehicle 678-718, OpenQuestJournal 981-1020, CloseQuestJournal 1022-1039, ApplyTouchControlsVisibility 1280-1297); `FlyingCabCharacter.cpp` (SetupPlayerInputComponent 125-138, RefreshKeyboardInputState 254-272); `FlyingCabTouchControls.cpp` (NativeOnFocusLost/MouseLeave 93-103, ReleaseAllInputs 461-479, bindingi przycisków 935-948, handlery 1171-1278); `FlyingCabVehicleVitalsComponent.cpp` (Advance 37-86, CanUseThrusters 164-167); `FlyingCabQuestJournalWidget.cpp` (HandleNavigationKey 150-196, ShowJournal 198-203); `FlyingCabFunctionalTests.cpp` (InputTransitionChain 879-1039); `Config/DefaultInput.ini` (63, 70, 75-76, 79-80).

Silnik: patrz tabela w rozdziale 2.
