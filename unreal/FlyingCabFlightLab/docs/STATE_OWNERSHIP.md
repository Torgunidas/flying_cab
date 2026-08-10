# Własność i czas życia stanu

Ten dokument wskazuje źródła prawdy. Widgety oraz teksty HUD są odbiorcami stanu, a nie jego właścicielami.

| Stan | Źródło prawdy | Reset | Czas życia |
|---|---|---|---|
| Kredyty, aktywny kurs, opłata, oferty pasażerów i statystyki biegu | `AFlyingCabGameMode` | `StartRun`; kurs także po dowozie lub zniszczeniu aktywnego pojazdu | bieżący poziom |
| Tryb biegu i cel Time Attack | `AFlyingCabGameMode` | `StartRun` | bieżący poziom; tryb jest przekazywany jako opcja podczas przeładowania |
| Paliwo, kadłub, stan zniszczenia i fizyka pojazdu | `AFlyingCabPawn` | `ResetVehicle` albo `RecoverVehicle` | życie danego pojazdu |
| Aktualny tryb gracza: pojazd, pieszo lub nieznany | `AFlyingCabPlayerController::PlayerMode` | każde `OnPossess` | bieżący kontroler/poziom |
| Ostatni aktywny pojazd podczas chodzenia pieszo | `AFlyingCabPlayerController::ActiveVehicle` | wejście lub wyjście z pojazdu | bieżący kontroler/poziom |
| Zdrowie postaci pieszej | `AFlyingCabCharacter` | utworzenie nowej postaci | pojedyncze wyjście z pojazdu |
| Dostęp do pojazdów | `UFlyingCabProgressionSubsystem` | jawne `ResetAccess` na początku Time Attack | instancja aplikacji; przeżywa przeładowanie mapy |
| Najlepsze czasy Time Attack | `UFlyingCabScoreSaveGame` | brak automatycznego resetu | dysk; format oznaczony `SaveVersion` |
| Topologia dzielnic, stacji i granice minimapy | `FlyingCabCityData` | zmiana kodu/danych | stała konfiguracja projektu |
| Aktorzy infrastruktury tworzonej w runtime: rozszerzenie miasta, stacje, biuro, portale, terminal, pojazd serwisowy i ruch | `AFlyingCabWorldBootstrap` utworzony przez `AFlyingCabGameMode` | przeładowanie poziomu | bieżący poziom; GameMode koordynuje tylko zdarzenia ekonomii, dostępu i recovery |
| Lista śledzonego ruchu, predykcja zagrożeń i czas komunikatu near-miss | `UFlyingCabTrafficAwarenessComponent` na `AFlyingCabGameMode` | zmiana aktywnego pojazdu lub przeładowanie poziomu | bieżący poziom; wynik jest wysyłany zdarzeniem do HUD-u |
| Stan wyświetlany przez HUD | `AFlyingCabPlayerController` posiada jeden `UFlyingCabTouchControls`; widget przechowuje wyłącznie ostatnią prezentację | `OnPossess` przełącza tryb, a timer 10 Hz pobiera zasoby aktywnego pojazdu | pochodna powyższych źródeł |

## Kontrakty resetu

- `R` resetuje pozycję, prędkość, paliwo, kadłub i wejścia pojazdu. Nie resetuje kredytów ani aktywnego kursu.
- Zniszczenie aktywnego pojazdu przerywa kurs, nalicza holowanie i uruchamia recovery. Zaparkowany pojazd odzyskuje sprawność bez opłaty i bez przerwania bieżącego kursu.
- Time Attack zawsze resetuje dostęp przy starcie, aby wynik był porównywalny. Free Roam zachowuje dostęp przy przeładowaniu mapy w ramach tej samej sesji aplikacji.
- Śmierć pieszo przeładowuje poziom: stan poziomu znika, dostęp z `GameInstance` pozostaje, a tryb biegu jest przekazywany do nowej mapy.
- `BoundPawn` w GameMode wskazuje bieżący albo ostatni aktywny pojazd także podczas chodzenia pieszo. Jawny `PlayerMode` określa sposób sterowania i nie zależy od kolejności metod w `Tick`.

## Zasada zmian

Nowa mechanika powinna modyfikować stan przez jego właściciela i przekazywać wynik do HUD zdarzeniem lub metodą synchronizującą. Pawn nie może tworzyć ani przejmować widgetu HUD; udostępnia kontrolerowi jedynie dane pojazdu. Nie należy tworzyć kolejnego niezależnego zestawu współrzędnych miasta ani odtwarzać trybu gracza przez porównywanie aktualnego pawna z `BoundPawn`.
