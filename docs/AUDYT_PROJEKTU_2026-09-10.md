# Audyt projektu — 2026-09-10

Data: 2026-09-10. Punkt odniesienia: `main`, HEAD `338b2fa` („supercar”), identyczny z `origin/main`, working tree czyste. Maszyna audytu: MacBook Pro (Apple M5, macOS 26.6.2), UE 5.8.0 (CL 55116800) w `/Users/Shared/Epic Games/UE_5.8`, Xcode 26.1.1. Komputer Windows nie był dostępny; jego stan wynika z commitów i dokumentacji.

**Wniosek główny: repozytorium na Macu było aktualne, ale edytor uruchamiał moduł gry skompilowany o 08:24, czyli sprzed pobrania supercara (13:33). Unreal nie zaproponował przebudowy, bo `BuildId` starej binarki zgadzał się z silnikiem. Po przebudowie modułu skryptem z repo kod supercara kompiluje się na macOS bez błędów i ostrzeżeń, a pakiet testów Automation na Macu przeszedł dwukrotnie po 45 z 46, z jedną inną, niepowtarzalną porażką w każdym przebiegu (soak przy zimnym starcie, zator ruchu NPC); `FlyingCab.Functional.PIE.Supercar` zaliczony w obu przebiegach.**

Ścieżki są względne wobec katalogu repo, chyba że zaznaczono inaczej.

## 1. Zakres i metoda

- Sprawdzono: stan Git lokalny i zdalny, integralność assetów binarnych, logi edytora (`~/Library/Logs/Unreal Engine/FlyingCabFlightLabEditor`), logi narzędzi budowania, zawartość `Binaries/` i `Intermediate/`, konfigurację projektu, źródła (analiza statyczna), manifest chronionego sterowania, dokumentację i skrypty.
- Wykonano: build `FlyingCabFlightLabEditor Mac Development` skryptem `scripts/build-editor-mac.sh` oraz pełny pakiet `Automation RunTests FlyingCab` z `-NullRHI`.
- Nie wykonano: ręcznej sesji gry na Macu, testów z renderowaniem Metal, odczytu pełnej zawartości assetów `.uasset`/`.umap` (tylko nagłówki, referencje i sumy kontrolne), żadnej kontroli na komputerze Windows.
- Audyt dodaje wyłącznie ten dokument. Lokalna przebudowa (`Binaries/`, `Intermediate/`) i logi testów (`Saved/`) są ignorowane przez Git. Nie wykonano commita, nie zmieniono kodu, assetów ani konfiguracji.

## 2. Dlaczego na Macu nie było supercara

### 2.1 Oś czasu (reflog Git, znaczniki plików, logi edytora na Macu)

| Czas (Mac) | Zdarzenie | Dowód |
|---|---|---|
| 08:24 | build modułu `FlyingCabFlightLab` na Macu | `Binaries/Mac/libUnrealEditor-FlyingCabFlightLab.dylib`, obiekty w `Intermediate/Build/Mac` |
| 12:28 | build modułu edytora `FlyingCabNarrativeEditor` | `Binaries/Mac/libUnrealEditor-FlyingCabNarrativeEditor.dylib` |
| 12:44 | commit `f25ce58` „questy, fury, karabiny” wykonany na Macu | `git reflog` |
| 13:33 | commit `338b2fa` „supercar” wykonany na PC i wysłany na GitHub; dokumentacja opisuje build i 46/46 testów na Windows | `git log`, `unreal/FlyingCabFlightLab/docs/A_R7_SUPERCAR.md` |
| 13:33:34 | `git pull --ff` na Macu (GitHub Desktop), fast-forward do `338b2fa`; 49 plików, w tym 14 w `Source/` | `git reflog`, `git show --stat 338b2fa` |
| 13:36 i 13:44 | uruchomienie edytora z `.uproject` bez kompilacji; PIE o 13:37 i 13:45 | `FlyingCabFlightLab.log` i backup `-11.37.56` |

### 2.2 Dowody z logu edytora (sesja 13:44–13:46)

- `LogModuleManager: InternalLoadLibrary: 'FlyingCabFlightLab' ('.../Binaries/Mac/libUnrealEditor-FlyingCabFlightLab.dylib')` — załadowano binarkę z 08:24.
- Jedyne uruchomienie narzędzi budowania w tej sesji to Turnkey `VerifySdk` z `-nocompile` (`~/Library/Logs/Unreal Engine/LocalBuildLogs/Log.txt`). UnrealBuildTool nie kompilował projektu.
- Przed audytem w `Intermediate/Build/Mac` nie było żadnego pliku `FlyingCabSupercar*`. Kod supercara nigdy nie został na Macu skompilowany.
- PIE zalogowało `World bootstrap completed: 4 fuel stations, 2 repair shops, 40/40 traffic vehicles and 8 living pedestrians.` bez zatok, czterech A_R7 i A_R7 na trasach Express — to zachowanie starego kodu.
- Repozytorium było w porządku: 21 assetów `Content/Vehicles/A_R7/*.uasset` ma poprawny nagłówek pakietu UE (`C1 83 2A 9E`) i sumy zgodne z indeksem Git; wszystkie zapisane przez UE 5.8; `.gitattributes` oznacza `*.uasset` i `*.umap` jako binarne. Git niczego nie zgubił ani nie uszkodził.

### 2.3 Dlaczego Unreal nie ostrzegł

Edytor uruchomiony z `.uproject` sprawdza tylko, czy pliki modułów istnieją i czy `BuildId` w `Binaries/Mac/UnrealEditor.modules` (tu `55116800`) zgadza się z silnikiem. Nie porównuje dat źródeł z binarką. Po pullu zawierającym zmiany C++ stary `.dylib` ładuje się bez komunikatu, assety i konfiguracja są nowe, ale nie ma kodu, który by ich użył. Ten sam mechanizm zadziała w drugą stronę, gdy PC pobierze zmiany C++ zrobione na Macu.

`docs/WORKING_ON_MAC_AND_PC.md` wspomina o przebudowie po zmianach C++ jednym zdaniem; w codziennym obiegu ten krok został pominięty.

### 2.4 Naprawa wykonana podczas audytu

```
bash scripts/build-editor-mac.sh '/Users/Shared/Epic Games/UE_5.8'
```

Wynik: `Result: Succeeded`, 34 akcje (32 kompilacje, plik modułu, link), 40 s, bez błędów i ostrzeżeń kompilatora; Mac SDK 26.1, minimalny macOS 14.0. Nowa binarka `libUnrealEditor-FlyingCabFlightLab.dylib` z 13:58; w `Intermediate` pojawił się `FlyingCabSupercarTests.cpp.o`.

Weryfikacja pakietem testów (ten sam sposób co `ThrusterFullNullRHI.log` z 2026-09-09):

```
"/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor-Cmd" unreal/FlyingCabFlightLab/FlyingCabFlightLab.uproject -unattended -nop4 -nosplash -nosound -NullRHI -ExecCmds="Automation RunTests FlyingCab" -TestExit="Automation Test Queue Empty" -abslog=<log>
```

Przebieg 1 (`Saved/Logs/AuditMacFullTests_2026-09-10.log`, 14:01–14:03): **46 testów wykonanych, 45 zaliczonych, 1 niezaliczony**. `FlyingCab.Functional.PIE.Supercar` zaliczony: cztery publiczne A_R7, mesh `SM_A_R7_Supercar` załadowany (pierwsze zbudowanie w DDC na tym Macu, 0,04 s), NPC A_R7 na trasach Express, possession i limit 3150 cm/s. Niezaliczony: `FlyingCab.Functional.PIE.InputSoak.Seed1977`, krok 5, `Expected mode/possession transition did not happen` — pierwszy `Q` w soaku (wyjście z auta) nie utworzył postaci pieszej w ciągu dwóch klatek karencji. Był to pierwszy świat PIE w tym procesie, tuż po pierwszym zbudowaniu siatki A_R7 i z `FlushAsyncLoading` w tych samych klatkach. `Seed9042026` zaliczony (3019 kroków).

Przebieg 2, tylko soak (`Saved/Logs/AuditMacInputSoakRerun_2026-09-10.log`): `Seed1977` i `Seed9042026` zaliczone, po 3019 kroków, bez budowania siatek.

Przebieg 3, pełny pakiet z rozgrzanym cache (`Saved/Logs/AuditMacFullTests2_2026-09-10.log`, 14:08–14:10): **46 wykonanych, 45 zaliczonych, 1 niezaliczony**. `Seed1977` i `Supercar` zaliczone, bez budowania siatek. Niezaliczony `FlyingCab.Functional.PIE.MetroTrafficFlow`: `Traffic stalled >45 game seconds` dla 12 pojazdów NPC ustawionych w kolumnie na pionowym pasie X ≈ −13 614, wysokości 5 998–7 436 (trasy Metro.Ring.Clockwise, Metro.Cross.South/West, Metro.Commuter.Neighborhood.NW/NE), wszystkie w stanie `WaitingForObstacle`. Czoło kolumny (`FlyingCabTrafficVehicle_0`) zgłasza `Last obstacle: None`, czyli blokadę przez trafienie nierejestrowane jako „żywa” przeszkoda (geometria statyczna lub inny typ kolizji); pozostałe pojazdy czekają na siebie nawzajem. Zatoki A_R7 leżą przy X = −9 830, −7 170 i 9 170, daleko od zatoru, więc zaparkowane supercary nie są blokerem.

Przebieg 4, tylko ten test (`Saved/Logs/AuditMacMetroRerun_2026-09-10.log`): zaliczony.

Ocena: obie porażki są niepowtarzalne (soak: 1 z 3 uruchomień, wyłącznie przy zimnym starcie; ruch NPC: 1 z 4 uruchomień na Macu, licząc przebieg z 2026-09-09) i żadna nie dotyczy kodu supercara ani kompilacji na Macu. Na Windows ten sam kod dał 46/46. Zator ruchu jest jednak defektem widocznym dla gracza (stojąca kolumna NPC), a soak jest wrażliwy na czas pierwszego spawnu pieszego. Patrz A-10 i A-11.

Do wykonania przez użytkownika: otworzyć `.uproject` na Macu, uruchomić PIE i sprawdzić zatoki A_R7 (Yellow Projects, Ashline Market, Neon Docks, Glassward Transit) oraz wejście `Q`. To ręczna akceptacja, której test NullRHI nie zastępuje.

### 2.5 Jak nie dopuścić do powtórki

1. Zasada: po każdym `git pull`, który zmienił `unreal/FlyingCabFlightLab/Source/`, pliki `*.Build.cs`, `*.Target.cs` lub `.uproject`, przed otwarciem edytora uruchom skrypt budowania. Build bez zmian trwa kilka sekund, więc można go wykonywać po każdym pullu bez sprawdzania.
2. Skrypt synchronizacji na każdy system (`scripts/sync-mac.sh`, `scripts/Sync-Windows.ps1`): `git pull --ff-only`, sprawdzenie `git diff --name-only ORIG_HEAD HEAD -- unreal/FlyingCabFlightLab/Source unreal/FlyingCabFlightLab/FlyingCabFlightLab.uproject`, build, komunikat. Skrypt zamiast pamiętania o kroku.
3. W `StartupModule` modułu gry logować datę kompilacji (`__DATE__ __TIME__`), a w linii `World bootstrap ...` liczbę A_R7. Log będzie wtedy jednoznacznie pokazywał, że binarka jest starsza od źródeł.
4. Uruchamianie edytora z Xcode lub Ridera buduje moduł przed startem. Otwieranie `.uproject` z Findera albo Launchera tego nie robi.
5. Definicja „gotowe” dla zadań z C++: build i pakiet NullRHI na obu systemach albo jawny zapis w dokumentacji, że drugi system czeka na weryfikację. Historia (`24a3a16`, poprawka kompilacji Mac dla testów Metro/Residential) pokazuje, że kod pisany na Windows bywał niekompilowalny na Macu.

## 3. Repozytorium i praca Mac ↔ PC

| Obszar | Stan | Uwagi |
|---|---|---|
| Synchronizacja | OK | `main` = `origin/main` = `338b2fa`, brak lokalnych zmian, brak rozbieżności historii. |
| Integralność assetów | OK | Nagłówki i sumy kontrolne zgodne; 0 plików z CRLF w indeksie; wszystkie binaria wykryte jako `-text`. |
| Śledzony alias Findera | do naprawy | `unreal/FlyingCabFlightLab/FlyingCabFlightLab.uproject-alias` (macOS alias, 1108 B) trafił do repo w commicie `7a18a5d` „thrusters”. Na Windows bezużyteczny, w Finderze wygląda jak drugi projekt. Usunąć z indeksu (`git rm --cached`) i dodać `*-alias` do `.gitignore`. |
| `.gitattributes` | do uzupełnienia | `* text=auto` i jawne `binary` tylko dla `.uasset`/`.umap`. PNG, PSD, FBX, BLEND, ZIP, PCK, EXE, AI, EPS zależą od heurystyki Gita. Dziś poprawnie, ale warto dopisać jawne reguły. |
| Rozmiar | świadomy koszt | `.git` 73 MB; 4 047 śledzonych plików, z czego 3 784 w `archive/godot` (w tym `FlyingCab.console.exe`, `FlyingCab.pck`, trzy archiwa ZIP, pliki PSD). Decyzja z 2026-09-09: bez LFS i bez przepisywania historii. Jeśli dojdą kolejne modele i tekstury, rozważyć LFS dla `unreal/**/assets/**`. |
| Konfiguracja Gita na Macu | ryzyko utajone | Globalne `filter.lfs.*` z `required=true`, a `git-lfs` nie jest zainstalowany. Dziś bez skutków. Jeśli ktoś doda `filter=lfs` do `.gitattributes`, checkout i commit na tym Macu zaczną się kończyć błędem. Usunąć filtry z `~/.gitconfig` albo zainstalować git-lfs. |
| Gałęzie zdalne | porządki | `codex/unreal-audit-fixes` scalona do `main` (można usunąć). `codex/evaluate-vehicle-control-system-redesign`, `not_stable`, `working-dialog-system` niescalone, z okresu Godota. Zdecydować: usunąć lub opisać jako archiwalne. |
| CI | brak | `.github/` nie istnieje. Dla UE bez własnego runnera to typowe; tym ważniejszy jest skrypt synchronizacji z 2.5. |
| Dowody testów | tylko lokalnie | Dokumentacja odwołuje się do 31 plików w `Saved/Logs` i `Saved/Automation`, ignorowanych przez Git. Dowód istnieje wyłącznie na maszynie, która go wytworzyła. Dalej zapisywać w docs datę, maszynę i wynik liczbowy, a ścieżek `Saved/...` nie traktować jako współdzielonych. |
| README i instrukcje | spójne | UE 5.8, mapa `/Game/Maps/FlightLab`, moduły i skrypty zgadzają się z konfiguracją. W README brakuje wyraźnego kroku „po pullu ze zmianami C++ zbuduj moduł zanim otworzysz edytor”. |

## 4. Projekt Unreal — stan techniczny

### 4.1 Kompilacja i przenośność

- Źródła: 125 plików, około 25,5 tys. linii; moduł runtime `FlyingCabFlightLab` i moduł edytora `FlyingCabNarrativeEditor`. Brak `#if PLATFORM_*`, brak ścieżek bezwzględnych w kodzie, konfiguracji, skryptach i dokumentacji. Brak `TODO`/`FIXME`.
- Build Mac (clang, Xcode 26.1.1) czysty; UBT nie zgłosił ostrzeżeń.
- Ustawienia: `TargetedHardwareClass=Mobile`, `Scalable`, okno PIE 540×960 (pion), orientacje Android/iOS pionowe, Windows DX12 SM6, Mac Metal SM6. Spójne z założeniem gry mobilnej rozwijanej na desktopie.
- `DefaultGame.ini`: `/Game/Vehicles/A_R7` w `DirectoriesToAlwaysCook`. Potrzebne, bo mesh jest ładowany z C++ przez `FSoftObjectPath`, nie przez referencję z mapy.

### 4.2 Testy

- 45 testów Automation (24 `FlyingCab.Core.*`, 21 `FlyingCab.Functional.PIE.*`) w 12 plikach, wszystkie pod `WITH_DEV_AUTOMATION_TESTS`; soak rozwija się na dwa seedy, stąd 46 wykonań w pełnym przebiegu.
- Ostatni pełny przebieg na Macu przed audytem: 2026-09-09, 41 testów (`ThrusterFullNullRHI.log`), sprzed commitów z dialogami i supercarem. Dla `f25ce58` dokumentacja odnotowuje, że pełnych testów nie uruchomiono; dla `338b2fa` testy wykonano wyłącznie na Windows.
- Przebiegi audytu na Macu (binarka 13:58, NullRHI): opis i ocena w 2.4. Karencja soaku po `Q` wynosi dwie klatki (`ResetModelAfterFlush`, `Grace = 2`), a pierwszy spawn postaci pieszej w świeżym procesie może potrzebować więcej przy zimnym cache. Poprawka należy do testu (dłuższa karencja dla pierwszego przejścia albo rozgrzewka przed krokiem 1), nie do chronionej ścieżki wejścia.
- `MetroTrafficFlow` niezaliczony w jednym z dwóch pełnych przebiegów (zator kolumny NPC), zaliczony w powtórzeniu; szczegóły w 2.4 i A-11.
- 17 linii `LogAutomationTest: Error: Condition failed` na starcie każdej sesji edytora pojawia się tuż po `LogTemp: Error test: UE::UnifiedErrorTest...`, także w logach z 2026-09-09. To wewnętrzny test silnika, nie kod projektu.
- Test `FlyingCab.Functional.PIE.Supercar` zapisuje zrzut `A_R7_InGame.png` tylko z parametrem `-FlyingCabCaptureSupercar`, więc działa poprawnie z NullRHI.

### 4.3 Chroniony punkt odniesienia sterowania

- Manifest `INPUT_CANONICAL_BASELINE_2026-09-04.json` sprawdzony równoważnym skryptem Python (ta sama normalizacja UTF-8/LF i SHA-256): 23 pliki zgodne, 9 zmienionych, 0 brakujących. To dokładnie dziewięć plików opisanych w `INPUT_CANONICAL_BASELINE.md` po wdrożeniu rozmów NPC. Commit supercara nie dodał nowych różnic; jego zmiany w `FlyingCabPawn.*` i `FlyingCabPlayerController.cpp` dotyczą plików już na liście i są opisane w sekcji „A_R7 Supersport — 2026-09-10”.
- `scripts/Verify-InputBaseline.ps1` nie działa na Macu (brak `pwsh`). Dokumentacja trzykrotnie odnotowuje „równoważne sprawdzenie w Pythonie”, którego nie ma w repo. Dodać `scripts/verify-input-baseline.py` i wskazać go w `AGENTS.md` obok skryptu PowerShell.

### 4.4 Przegląd commitu „supercar” (kod)

- `Supercars` w `AFlyingCabWorldBootstrap` to `UPROPERTY(Transient) TArray<TObjectPtr<AFlyingCabPawn>>`; bezpieczne dla GC, rejestrowane we flocie przez `GetSupercars()`.
- `ConfigureAsSupercar` jest zabezpieczone `check(!HasActorBegunPlay())` i wywoływane wyłącznie po `SpawnActorDeferred`. Poprawne.
- Gdy trace zatoki zawiedzie, `SpawnSupercars` zwraca `false`; bootstrap loguje `Error: No safe A_R7 bay at ...` i `World bootstrap completed with missing actors`, gra startuje dalej. Linia podsumowania nie wymienia liczby A_R7, więc nie odróżnia starej binarki od nowej.
- Współrzędne dysz (`RimMount`) i skala 220 cm są zaszyte w `FlyingCabThrusterVisualComponent.cpp` i zależą od konkretnego eksportu meshu. Reeksport w Blenderze bez zachowania osi i centrowania przesunie strumienie. `Import-A_R7.py` sprawdza tylko wymiary bryły; warto dodać kontrolę położenia felg lub przenieść punkty mocowania do danych.
- NPC A_R7: `ConfigureAsSupercar` pojazdu ruchu wywołuje `ApplyVisualBody()` i `LoadSynchronous`; po pierwszym załadowaniu mesh jest w pamięci, sześć wywołań to pomijalny koszt.
- Dwa przemianowania zmiennych (`Padding` → `OptionBorder`, `Character` → `PlayerCharacter`) nie zmieniają logiki; wyglądają na usunięcie ostrzeżeń o przesłanianiu nazw.
- `A_R7_SUPERCAR.md` deklaruje: „macOS compilation and a user's manual driving session have not been performed”. Kompilacja i testy NullRHI na Macu są teraz wykonane (ten audyt). Sesja ręczna nadal nie.

### 4.5 Drobne

- `Content/Developers/torgerd/` i `Config/Layouts/` istnieją lokalnie jako puste katalogi i nie są śledzone.
- `FLIGHT_FEEL_TEST.md` leży w katalogu projektu, poza `docs/`.
- Pliki `.DS_Store` w `unreal/` istnieją lokalnie i są ignorowane.

## 5. Ustalenia i priorytety

| ID | Priorytet | Ustalenie | Zalecenie |
|---|---|---|---|
| A-01 | P1 | Po pullu zmian C++ edytor ładuje starą binarkę bez ostrzeżenia; codzienny obieg nie zawiera obowiązkowego builda | skrypt synchronizacji na oba systemy, log daty kompilacji, wyraźny krok w README |
| A-02 | P1 | Weryfikacja na drugim systemie nie jest częścią definicji „gotowe” (supercar: tylko Windows; dialogi: bez pełnych testów) | reguła w `AGENTS.md`: build i pakiet NullRHI na obu systemach albo jawny zapis, że drugi system czeka |
| A-03 | P2 | Kontrola manifestu sterowania tylko w PowerShell; na Macu brak `pwsh` | dodać `scripts/verify-input-baseline.py` |
| A-04 | P2 | Śledzony alias Findera `FlyingCabFlightLab.uproject-alias` | `git rm --cached`, wpis w `.gitignore` |
| A-05 | P3 | `.gitattributes` bez jawnych reguł `binary` dla png/psd/fbx/blend/zip/exe/pck | dopisać reguły |
| A-06 | P3 | Globalna konfiguracja LFS na Macu bez zainstalowanego git-lfs | usunąć filtry albo zainstalować |
| A-07 | P3 | Stare gałęzie zdalne (jedna scalona, trzy niescalone z ery Godota) | usunąć lub opisać |
| A-08 | P3 | Dowody testów wyłącznie w ignorowanym `Saved/` | zapis wyników w docs, bez ścieżek `Saved/` jako dowodu |
| A-09 | P3 | Linia podsumowania bootstrapu nie wymienia A_R7; brak daty kompilacji w logu | dopisać w kodzie |
| A-10 | P2 | `InputSoak.Seed1977` niezaliczony raz w zimnym starcie (dwie klatki karencji na pierwsze `Q`), potem zaliczony | uodpornić test: rozgrzewka przed krokiem 1 lub większa karencja dla pierwszego przejścia; bez zmian w ścieżce wejścia |
| A-11 | P2 | `MetroTrafficFlow`: sporadyczny zator kolumny NPC na pionowym pasie X ≈ −13 600, czoło czeka na przeszkodę nierejestrowaną w diagnostyce; 1 z 4 przebiegów na Macu | zapisywać w diagnostyce także przeszkody statyczne i innych typów; dodać wyjście z impasu po czasie oczekiwania; powtórzyć test kilkukrotnie na Windows |

## 6. Zalecany obieg zmiany komputera

Mac:

```
git switch main && git pull --ff-only
git diff --name-only ORIG_HEAD HEAD -- unreal/FlyingCabFlightLab/Source unreal/FlyingCabFlightLab/FlyingCabFlightLab.uproject
bash scripts/build-editor-mac.sh '/Users/Shared/Epic Games/UE_5.8'
open unreal/FlyingCabFlightLab/FlyingCabFlightLab.uproject
```

Windows (PowerShell): te same kroki z `.\scripts\Build-Editor.ps1 -EngineRoot 'D:\Unreal\UE_5.8'`.

Jeśli drugie polecenie cokolwiek wypisuje, build jest obowiązkowy. Build bez zmian kończy się w kilka sekund, więc bezpieczniej wykonywać go zawsze. Edytor powinien być zamknięty podczas pulla i builda.

## 7. Co zweryfikowano, a czego nie

- Zweryfikowano na Macu: zgodność repo, integralność assetów, build, pakiet testów NullRHI (dwa pełne przebiegi i dwa powtórzenia pojedynczych testów), manifest sterowania.
- Nie zweryfikowano: komputer Windows (opis z dokumentacji), ręczna rozgrywka na Macu, render Metal, pełna zawartość assetów binarnych.
- Wynik testów NullRHI nie zastępuje ręcznej akceptacji użytkownika opisanej w `INPUT_CANONICAL_BASELINE.md`.

## 8. Wdrożone poprawki po audycie (2026-09-10) i przekazanie dla Codexa

Na polecenie użytkownika wdrożono poprawki bez uruchamiania testów Automation. Wszystkie zmiany są w working tree, bez commita. Kompilację sprawdzono tylko na macOS; Windows czeka na weryfikację (lista niżej).

### 8.1 Co się zmieniło

| Obszar | Plik | Zmiana |
|---|---|---|
| Synchronizacja Mac | `scripts/sync-mac.sh` (nowy) | Odmawia pracy przy uruchomionym edytorze, `git pull --ff-only` na bieżącej gałęzi (ostrzega, gdy to nie `main` lub są lokalne zmiany), wypisuje zmienione pliki `Source/` i `.uproject`, zawsze uruchamia `build-editor-mac.sh`. Bez `reset --hard` i `clean`. |
| Synchronizacja Windows | `scripts/Sync-Windows.ps1` (nowy) | Ten sam przebieg w PowerShell, build przez `Build-Editor.ps1`. **Nie był uruchomiony na Windows** (na Macu brak `pwsh`). |
| Ostrzeżenie edytora | `Source/FlyingCabNarrativeEditor/FlyingCabNarrativeEditor.cpp` | Po `OnFEngineLoopInitComplete` porównuje najnowszy plik `Source/**` (`cpp`, `h`, `cs`, `inl`) z datą modułów `FlyingCabFlightLab` i `FlyingCabNarrativeEditor`. Gdy źródła są nowsze: `Warning` w logu i powiadomienie w edytorze z poleceniem uruchomienia skryptu synchronizacji. Pomijane w commandletach i trybie `-unattended`. Moduł runtime bez zmian architektury. |
| Log bootstrapu | `Source/FlyingCabFlightLab/FlyingCabWorldBootstrap.cpp` | Dodatkowa linia `A_R7 supercars parked: %d/%d.` po dotychczasowym podsumowaniu; istniejąca linia bez zmian. |
| Kontrola manifestu | `unreal/FlyingCabFlightLab/scripts/verify-input-baseline.py` (nowy) | Odpowiednik `Verify-InputBaseline.ps1` dla macOS; te same komunikaty i kody wyjścia. Uruchomiony: 23 zgodne, 9 historycznie zmienionych, kod 1. |
| Git | `.gitignore`, `.gitattributes`, indeks | `*-alias` ignorowane, alias Findera usunięty z indeksu (`git rm --cached`, plik lokalny zachowany). Jawne `binary` dla mediów, modeli i archiwów, `*.py text eol=lf`. `git status` nie pokazuje zmian w plikach binarnych. |
| Dokumentacja | `README.md`, `docs/WORKING_ON_MAC_AND_PC.md`, `AGENTS.md`, `unreal/FlyingCabFlightLab/AGENTS.md`, `docs/INPUT_CANONICAL_BASELINE.md`, `docs/A_R7_SUPERCAR.md`, `docs/AUDIT_IMPLEMENTATION_STATUS.md` | Skrypty synchronizacji w codziennym obiegu, mechanizm starej binarki, reguła „gotowe = build i testy na obu systemach”, wskazanie skryptu Python, statusy A-01…A-11. |

Nie zmieniono żadnego pliku z manifestu chronionego sterowania (kontrola przed i po: te same 9 różnic). Nie zmieniono assetów, konfiguracji gry ani wersji silnika. Nie zmieniono globalnej konfiguracji Gita użytkownika (A-06) ani gałęzi zdalnych (A-07).

### 8.2 Weryfikacja wykonana na Macu

`bash scripts/sync-mac.sh '/Users/Shared/Epic Games/UE_5.8'` uruchomiony na tym Macu (14:26): ostrzeżenie o lokalnych zmianach, `git pull --ff-only` bez nowych commitów, UnrealBuildTool przekompilował `FlyingCabNarrativeEditor.cpp` i `FlyingCabWorldBootstrap.cpp`, zlinkował oba moduły, `Result: Succeeded` w 9,4 s, bez ostrzeżeń i błędów kompilatora (Clang 17, Xcode 26.1.1). Skrypt Python kontroli manifestu uruchomiony: kod 1, 9 znanych różnic. `bash -n` skryptu Mac bez błędów. Skrypt Windows nie był uruchamiany (brak `pwsh` na Macu).

Testów Automation nie uruchamiano zgodnie z poleceniem użytkownika. Kontrola manifestu: bez nowych różnic.

### 8.3 Do zrobienia na Windows (Codex)

1. Zamknąć edytor i uruchomić `.\scripts\Sync-Windows.ps1 -EngineRoot '<UE_5.8>'`. Sprawdzić: komunikat o zmienionych plikach `Source/`, build `Succeeded`, brak błędów PowerShell (skrypt pisany bez możliwości testu na Windows).
2. Uruchomić pełny pakiet `Automation RunTests FlyingCab` (NullRHI, jak dotąd) i zapisać wynik w `AUDIT_IMPLEMENTATION_STATUS.md`. Oczekiwane: 46 wykonanych, bez regresji; nowa linia logu bootstrapu nie zmienia zachowania gry.
3. Sprawdzić ostrzeżenie edytora: po `git pull` z dowolną zmianą w `Source/` otworzyć edytor **bez** builda; w Output Log powinno być `Source is newer than the compiled game modules`, a w rogu edytora powiadomienie. Po buildzie linia `Compiled modules are current`.
4. W logu PIE potwierdzić `A_R7 supercars parked: 4/4.`.

### 8.4 Druga tura poprawek (A-10, A-11) — wdrożone na Macu tego samego dnia

Na polecenie użytkownika („można nie czekać”) wdrożono również oba punkty testowe, bez pełnego pakietu testów.

- **A-10, soak** (`FlyingCabInputReliabilityTests.cpp`): przed krokiem 1 `FInputSoakCommand` wykonuje jeden cykl rozgrzewający `Q` (wyjście z auta) i `Q` (powrót), czekając na faktyczny spawn postaci pieszej i powrót do taksówki (limit 120 klatek na każde przejście; przekroczenie kończy test błędem `Warm-up: ...`). Po rozgrzewce model, `Grace = 3` i zdarzenie `Start` są takie same jak dotąd, a sekwencja losowa z seedu jest nietknięta. Karencja dwóch klatek dla przejść pozostaje bez zmian, więc test nie stał się łagodniejszy; jedynie pierwszy spawn pieszego nie jest już mierzony z zimnym cache.
- **A-11, diagnostyka** (`FlyingCabTrafficVehicle.h/.cpp`, `FlyingCabMetroTests.cpp`): pojazd zapamiętuje aktora i komponent, które zablokowały jego przesunięcie (sweep w `SetActorLocation`), oraz czas oczekiwania `ObstacleWaitSeconds`; `GetLastObstacleDescription()` zwraca `sensor=… move=…/… wait=…s creeps=…`. Test Metro dopisuje tę linię jako `Obstacle detail` przy każdym zgłoszonym zatorze i podsumowuje liczbę przeciśnięć w sesji (informacyjnie).
- **A-11, wyjście z impasu:** ustalono mechanizm zatoru: czujnik zaczyna 155 cm przed autem, więc auto zaklinowane zderzakiem z autem jadącym w poprzek nic nie widzi (`sensor=None`), a mimo to nie może się ruszyć; auto poprzeczne widzi je czujnikiem i czeka. Nowa reguła: gdy sweep ruchu blokuje **inny pojazd NPC**, czujnik nic nie widzi, a oczekiwanie trwa ≥ `GridlockCreepAfterSeconds` (10 s), auto przesuwa się bez sweepu z prędkością ≤ `GridlockCreepSpeed` (150 cm/s), aż sweep będzie czysty. Loguje `Warning` (`LogFlyingCabTrafficVehicle`) i zlicza `GridlockCreepCount`. Nigdy nie przeciska się przez gracza, pieszych ani geometrię miasta; kolejka za autem stojącym na przystanku nie spełnia warunku, bo tam czujnik widzi auto z przodu. Oba parametry są `EditDefaultsOnly`; `0` wyłącza regułę.

Weryfikacja na Macu (14:40, `Saved/Logs/AuditMacA10A11_2026-09-10.log`): build `Succeeded` bez ostrzeżeń (12 plików, 23 s). Uruchomiono wyłącznie testy objęte zmianami: `InputSoak.Seed1977` i `Seed9042026` zaliczone po 3019 kroków (38 i 20 zmian possession), `MetroTrafficFlow` zaliczony bez zatorów, `Gridlock creeps during the session: 0`, czyli reguła przeciśnięcia nie uruchomiła się w normalnym ruchu. Pierwszy wiersz CSV soaku ma teraz pełny zrzut stanu taksówki (przed poprawką w chwili błędu był pusty). Rozgrzewkę sprawdzono w zwykłym starcie procesu; scenariusza z zimnym cache siatki nie odtwarzano, bo wymagałby wyczyszczenia współdzielonego DDC. Pełnego pakietu nie uruchamiano.

### 8.5 Otwarte punkty przekazane Codexowi

- **A-11, przyczyna źródłowa:** przeciśnięcie usuwa objaw. Trwałe rozwiązanie to arbitraż skrzyżowań bez sygnalizacji (np. przyznawanie pierwszeństwa jednej osi lub odsunięcie węzłów tras tak, aby auta w poprzek nie zatrzymywały się w zasięgu kadłuba). Nowa linia `Obstacle detail` w logu testu Metro powinna to potwierdzić przy następnym zatorze.
- **Windows:** po `Sync-Windows.ps1` uruchomić pełny pakiet i powtórzyć `MetroTrafficFlow` kilka razy; sprawdzić, że `Gridlock creeps during the session` jest zwykle 0, a soak przechodzi z rozgrzewką dla obu seedów.
- **A-06:** na Macu globalna konfiguracja Gita ma filtry LFS z `required=true` bez zainstalowanego `git-lfs`; do usunięcia albo doinstalowania przez użytkownika.
- **A-07:** decyzja o usunięciu lub opisaniu starych gałęzi zdalnych.
