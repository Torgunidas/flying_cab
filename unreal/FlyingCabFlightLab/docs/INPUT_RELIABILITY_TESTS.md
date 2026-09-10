# Automatyczna weryfikacja kanonicznego wejścia

Testy znajdują się w `Source/FlyingCabFlightLab/FlyingCabInputReliabilityTests.cpp`, wyłącznie pod `WITH_DEV_AUTOMATION_TESTS`. Nie zmieniają produkcyjnych plików z manifestu wersji kanonicznej ani assetów/map na dysku. Bazowy wariant to `flyingcab.UseControlFrame=0`.

## InputSoak

- Dwa warianty Automation: `FlyingCab.Functional.PIE.InputSoak.Seed1977` i `Seed9042026`. Drugi seed można zastąpić parametrem procesu `-FlyingCabSoakSeed=12345`; pierwszy pozostaje stałym punktem odniesienia.
- Każdy przebieg wykonuje 3000 kroków w oddzielnych klatkach oraz końcowe sprawdzenie neutralności po puszczeniu wszystkich wejść. Stały krok symulacji 1/60 s jest ustawiany tylko na czas testu i przywracany w destruktorze komendy, również po błędzie. To około 50 s symulacji na seed, wykonywane szybciej bez renderowania — nie wielogodzinna sesja ani test wydajności.
- Niezależny model pamięta fizycznie trzymane klawisze, wejście dostarczone po ostatnim flushu, dotyk i oczekiwany tryb. Oczekiwań nie pobiera z aktualnych wartości Unreal. Po flushu trzymany klawisz może wrócić dopiero po kolejnym press/repeat.
- Zdarzenia: A/Left/D/Right/W/Up/Space/E, repeat po 15 krokach co drugi krok, J z puszczeniami w otwartym dzienniku, O, Q, R, symulowany focus flush i delegaty przycisków UMG. Dodatkowo trzy zniszczenia poprzez callback kolizji i rzeczywiste timery recovery.
- Sprawdzane są surowe klawisze, wartości akcji, cache pojazdu/postaci, tryb/possession, zapotrzebowanie na serwis, poziomy ruch postaci oraz siła napędu z uwzględnieniem dotyku. Przejścia mają dwie klatki tolerancji; ruch postaci dopuszcza próbkę bieżącej lub poprzedniej klatki. Po zakończeniu wszystkie wejścia muszą pozostać neutralne.
- Odczyt siły jest pasywny: test odbiera istniejące wpisy `LogFlyingCabInputTrace`. Nie dodaje accessorów ani logiki do kanonicznego Pawna. `flyingcab.InputTrace=1` jest wymaganym warunkiem testu.
- CSV w `Saved/Automation/InputSoak_<seed>.csv`: krok, klatka, zdarzenia, stany modelu, dotyk, oczekiwane i faktyczne wartości, siła, wynik oraz ostatni snapshot auta. Snapshot auta jest historyczny, gdy sterujemy pieszym; odczyt klawiatury i ruchu postaci nadal jest bieżący. Pierwsza rozbieżność przerywa test i wypisuje ostatnie 50 kroków do logu.
- Ponowne użycie seeda odtwarza generator zdarzeń i krok symulacji. Przy porównywaniu CSV pomijaj globalny numer klatki i czasy rzeczywiste w snapshotach. Plik dla tego seeda jest nadpisywany przez kolejne uruchomienie — zachowaj kopię ważnego śladu przed replayem.

## FuelCutoffWhileHoldingThrust

`FlyingCab.Functional.PIE.FuelCutoffWhileHoldingThrust` ustawia wyłącznie w instancji PIE 1 jednostkę paliwa i wyłącza regenerację, aby odizolować zachowanie pustego baku. Sprawdza kolejno:

1. W i repeat dają aktywne wejście, dodatnią siłę oraz rzeczywisty ruch w górę.
2. Po wyczerpaniu paliwa wejście pozostaje aktywne, ale napęd przechodzi na zero. Ostatnia opłacona klatka może jeszcze mieć siłę — paliwo jest rozliczane po `AddForce`.
3. Ostrzeżenie `Fuel exhausted` pojawia się dokładnie raz; snapshot pokazuje `NoFuel`; HUD faktycznie pokazuje `ENERGY EMPTY` i `THRUSTERS OFF`, po obsłużeniu wcześniejszych komunikatów w kolejce.
4. Puszczenie i ponowne naciśnięcie W nie omija braku paliwa ani nie powiela ostrzeżenia.
5. Dodanie 20 jednostek paliwa przy trzymanym W przywraca siłę i ruch bez dodatkowego wciśnięcia; późniejsze puszczenie wyłącza ciąg.

Test paliwa niezależnie sprawdza snapshot `NoFuel` i rzeczywistą siłę. Później dodana pasywna diagnostyka ma osobny test, opisany w [INPUT_PASSIVE_DIAGNOSTICS.md](INPUT_PASSIVE_DIAGNOSTICS.md); jej detektor rozbieżności nie zastępuje tego sprawdzenia.

## Uruchamianie

Najpierw `scripts/Verify-InputBaseline.ps1`, build `FlyingCabFlightLabEditor Win64 Development` w UE 5.8, następnie osobny `UnrealEditor-Cmd` z projektem i argumentami:

```text
-Unattended -NoSplash -NullRHI -ExecCmds="Automation RunTests FlyingCab;Quit"
```

Sam soak: `-ExecCmds="Automation RunTests FlyingCab.Functional.PIE.InputSoak;Quit"`. Sam test paliwa: analogiczny filtr `FlyingCab.Functional.PIE.FuelCutoffWhileHoldingThrust`. Używaj własnego `-AbsLog=...`, aby nie zastępować logu ręcznej sesji. Nie przełączaj wariantu kanonicznego.

Kontrola negatywna (wyłącznie świadome sprawdzenie skuteczności testu): dodaj `-FlyingCabSoakDropRelease` do procesu uruchamiającego sam soak. Test celowo nie dostarczy jednego puszczenia klawisza, ale model nadal będzie oczekiwał neutralizacji. **Oczekiwany wynik to błąd obu seedów i niezerowy kod procesu**, z `NEGATIVE_DroppedRelease` w osobnych `InputSoak_<seed>_negative.csv`. Bez tego jawnego parametru nic nie jest gubione ani wstrzykiwane poza normalnymi zdarzeniami scenariusza.

## Ograniczenia

Test izoluje auto i postać poza miastem, usuwa wpływ grawitacji/kolizji otoczenia i uzupełnia paliwo w soak. Nie sprawdza transportu Parsec/Windows/Slate, rzeczywistego wielodotyku, gamepada, fizycznej kolizji Chaos ani losowego przeładowania poziomu. Focus loss jest symulowany wywołaniem kontrolera, dotyk delegatami UMG, a postać ma testowy tryb lotu bez ryzyka śmierci. Test ruchu postaci obejmuje wejście i poziom, nie mechanikę skoku/lądowania. Ręczna akceptacja nowej implementacji nadal jest wymagana; testy nie zamieniają eksperymentalnej ramki w wersję kanoniczną.

## Wynik wdrożenia — 2026-09-04

Build UE 5.8 Win64 Development i cały pakiet **29/29** poprawne (`Saved/Logs/CanonicalReliabilityFinalTests.log`). Dwukrotne wykonanie każdego z seedów dało identyczne zdarzenia, model i wyniki po wyłączeniu pól czasu/globalnej klatki z porównania. Kontrola negatywna prawidłowo wykryła celowo zgubione puszczenia: SpaceBar w kroku 57 dla 1977 i Up w kroku 27 dla 9042026 (`Saved/Logs/CanonicalReliabilityNegativeControl.log`, oczekiwany błąd procesu). Manifest kanoniczny: 32/32 pliki zgodne, bez aktualizacji sum.

## Rozgrzewka soaku — 2026-09-10

`InputSoak.Seed1977` raz nie przeszedł na Macu w pierwszej sesji PIE procesu z zimnym cache: pierwsze `Q` w kroku 4 nie zdążyło utworzyć postaci pieszej w dwóch klatkach karencji. Soak wykonuje teraz przed krokiem 1 jeden cykl `Q` (wyjście) i `Q` (powrót), czekając na rzeczywisty spawn i powrót do taksówki (limit 120 klatek na przejście, przekroczenie kończy test błędem `Warm-up: ...`). Model wejścia, karencje, seedy i zliczanie pokrycia są bez zmian; wiersz `Start` w CSV pojawia się po rozgrzewce. Ścieżka wejścia gry nie została zmieniona. Szczegóły: `docs/AUDYT_PROJEKTU_2026-09-10.md` w katalogu głównym repo.
