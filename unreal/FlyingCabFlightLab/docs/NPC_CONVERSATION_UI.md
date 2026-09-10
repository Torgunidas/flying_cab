# Prezentacja rozmowy NPC

Okno rozmowy zajmuje dolny pas ekranu. Kwestia NPC wpisuje się znak po znaku, a odpowiedzi gracza pojawiają się po jej prawej stronie pojedynczo, od góry. Ten dokument opisuje warstwę prezentacji. Treść rozmów i ich konfigurację opisuje `QUEST_AUTHORING.md`.

**Warstwa prezentacji nie posiada żadnego stanu gry.** Wybory, warunki, akcje i nagrody należą do `UFlyingCabDialogueSession` i publicznego API `UFlyingCabQuestSubsystem`, zgodnie z `STATE_OWNERSHIP.md`. Widget czyta `FFlyingCabDialogueView` i woła `ChooseOption`.

## Gdzie co siedzi

| Plik | Zawartość |
|---|---|
| `Source/FlyingCabFlightLab/FlyingCabDialoguePresentation.h/.cpp` | Cała logika czasu, bez UObject i bez Slate: `FFlyingCabTypewriter`, `FFlyingCabDialogueStage`, `FFlyingCabTextWrapper`, `FFlyingCabDialoguePacing`, `FlyingCabEase::OutCubic`. |
| `FlyingCabDialogueWidget.h/.cpp` | Budowa drzewa widgetów, `NativeTick`, obsługa klawiszy, dźwięki. Stałe layoutu i paleta leżą w anonimowej przestrzeni nazw na górze `.cpp`. |
| `FlyingCabNarrativeSettings.h/.cpp` | Parametry dla projektanta i `GetPacing()`, które pakuje je w `FFlyingCabDialoguePacing`. |
| `FlyingCabNpcDefinition.h` | Opcjonalne pole `Portrait` w profilu NPC. |
| `FlyingCabDialogueSession.h/.cpp` | Pole `Portrait` w `FFlyingCabDialogueView`, ustawiane w `Refresh()`. |
| `FlyingCabDialoguePresentationTests.cpp` | Testy jednostkowe warstwy czasu. |

Kto tworzy widget: `AFlyingCabPlayerController::OpenDialogue` (`FlyingCabPlayerController.cpp`). Ta ścieżka **nie została zmieniona** i jest objęta chronionym manifestem sterowania.

## Przepływ jednej kwestii

`UFlyingCabDialogueSession::OnChanged` wywołuje `RefreshView()`. Ta metoda przepisuje mówcę, portret, komunikat zwrotny, przebudowuje wiersze odpowiedzi i woła `Stage.BeginLine()`. Dalej pracuje już tylko zegar.

| Faza `EFlyingCabDialoguePhase` | Kto ją kończy | Co się dzieje |
|---|---|---|
| `PanelIntro` | upływ `PanelIntroSeconds` | Dymek i tabliczka: krycie 0 do 1, skala od `PanelIntroStartScale` do 1. Tylko pierwsza kwestia w oknie. |
| `Measuring` | `TryLayoutLine()` gdy zna szerokość | Pomiar i jednorazowe złamanie linii. Niewidoczne dla gracza. |
| `Typing` | `FFlyingCabTypewriter::IsFinished()` | Odsłanianie znak po znaku z przytrzymaniem na interpunkcji. |
| `Staggering` | ostatnia odpowiedź osiąga postęp 1 | Odpowiedzi wjeżdżają kolejno od góry. |
| `Ready` | — | Zaznaczenie i wybór aktywne. |

`AdvancePresentation(float)` to jedyne wejście zegara. `NativeTick` tylko przekazuje do niej deltę, a testy wołają ją bezpośrednio.

## Dwie decyzje, które łatwo cofnąć przez pomyłkę

**Tekst nie przeskakuje, bo linia jest łamana raz.** `TryLayoutLine()` mierzy słowa przez `FSlateFontMeasure` i wstawia twarde znaki nowej linii, a `VisibleLine` ma `AutoWrapText` wyłączone. Gdyby włączyć zawijanie i odsłaniać przez `Left(N)`, ostatnie słowo w linii przeskakiwałoby w dół w trakcie pisania. Wysokość dymka trzyma `GhostLine`: bliźniaczy blok z pełną treścią i widocznością `Hidden`, który zajmuje miejsce, ale nie jest rysowany. Z tego samego powodu wiersze odpowiedzi są w drzewie od początku jako `Hidden`, a nie `Collapsed`; `Collapsed` sprawiłoby, że kolumna rośnie w miarę ich pojawiania się.

**Zegar jest ścienny, nie światowy.** Rozmowa woła `SetPause(true)`, więc `FTimerManager` stoi. `NativeTick` liczy deltę z `FPlatformTime::Seconds()`, ograniczoną do 0,25 s na krok. Dźwięki muszą lecieć z flagą dźwięku UI w `PlaySound2D`, inaczej pauza je wycisza.

Trzecia rzecz, mniejsza: `SetText` na `VisibleLine` wykonuje się wyłącznie gdy licznik odsłoniętych znaków wzrósł i tylko w fazie `Typing`. To wymóg z `GODOT_MIGRATION_CONTRACTS.md` o braku stałej inwalidacji tekstów.

## Sterowanie wewnątrz okna

`HandleDialogueKey(const FKey&)` jest publiczna i deterministyczna, więc testy nie muszą przechodzić przez Slate. `NativeOnKeyDown` tylko odfiltrowuje powtórzenia dla klawiszy zatwierdzających i **zawsze zwraca `FReply::Handled()`**. Ten zwrot trzyma `Q`, `J` i `R` poza modalem; jego usunięcie wpuściłoby je do odroczonej kolejki poleceń kontrolera.

Pierwsze `Enter` lub `Spacja` w trakcie wpisywania woła `SkipReveal()` i nie wybiera odpowiedzi. Klawisze `1`–`9` oraz kliknięcie działają tylko na odpowiedziach, które już się pojawiły; bramką jest `Stage.IsOptionRevealed(Index)` sprawdzane także wewnątrz `ChooseOption`, więc Blueprint nie obejdzie tej zasady.

## Jak coś zmienić

- **Tempo, dystans, dźwięki:** Project Settings, Game, Flying Cab Narrative, sekcja `Presentation`. Bez rekompilacji.
- **Wygląd:** stałe na górze `FlyingCabDialogueWidget.cpp` (proporcje sceny, kolory, stopnie fontu) oraz `BuildLayout()`.
- **Nowy element animowany:** dodaj odczyt w `ApplyPresentation()`, a jego przebieg w czasie w `FFlyingCabDialogueStage`, nie w widgecie. Wtedy da się go pokryć testem jednostkowym.
- **Własny wygląd bez dotykania C++:** `UFlyingCabNarrativeSettings::DialogueWidgetClass` przyjmuje Widget Blueprint dziedziczący po `UFlyingCabDialogueWidget`. Implementuje `PresentDialogue` i woła `ChooseOption` oraz `CloseDialogue`.

## Granice

Nie zmieniaj `OpenDialogue`/`CloseDialogue`, pauzy, `FlushPressedKeys`, strażnika przejścia, focusu, kolejności `Q`/`J`/`R` ani mapowań wejścia. Te elementy leżą w plikach objętych manifestem `INPUT_CANONICAL_BASELINE_2026-09-04.json`. Po pracy nad prezentacją kontrola `Verify-InputBaseline.ps1` musi zwracać tę samą listę różnic co przed nią; nowy wpis oznacza wyjście poza zakres.

## Testy

| Nazwa | Co pokrywa |
|---|---|
| `FlyingCab.Core.Dialogue.Typewriter` | Tempo, przytrzymanie na interpunkcji, pominięcie, brak przekroczenia długości, liczenie dźwięków. |
| `FlyingCab.Core.Dialogue.RevealTimeline` | Fazy, kolejność i moment pojawienia się odpowiedzi, wariant po pominięciu, kwestia bez odpowiedzi. |
| `FlyingCab.Core.Dialogue.TextWrap` | Zachowanie wszystkich słów, szerokość linii, brak pustych linii, słowo dłuższe od linii, autorskie znaki nowej linii. |
| `FlyingCab.Functional.PIE.NpcConversation` | Rozmowa od otwarcia do oddania zadania, plus prezentacja: tekst nie jest pełny od razu, klawisz numeryczny przed pojawieniem się odpowiedzi nic nie robi, pominięcie pokazuje całość. |

Testy sterują czasem przez `AdvancePresentation(float)`, nigdy przez `NativeTick`. Do odczytu stanu służą `GetRevealedCharacterCount()`, `GetTotalCharacterCount()`, `GetRevealedOptionCount()`, `GetSelectedOption()` i `IsRevealComplete()`.

## Znane ograniczenia

- Brak assetów dźwiękowych w repozytorium; pola dźwięków są puste i cisza jest stanem domyślnym.
- Portretów też nie ma; pole `Portrait` w profilu NPC jest puste, więc widoczna jest sama tabliczka z imieniem.
- Przy więcej niż sześciu liniach kwestii font schodzi o stopień, przy więcej niż pięciu odpowiedziach wiersze mają mniejszy font. Nie ma przewijania; bardzo długa kwestia z wieloma odpowiedziami nie była sprawdzona wizualnie.
- Zmiana rozmiaru okna w trakcie wpisywania nie przełamuje linii na bieżąco. Przełamanie następuje dopiero po zakończeniu kwestii.
- Ocena wizualna i tempo wymagają ręcznej sesji; wynik testów jej nie zastępuje.
