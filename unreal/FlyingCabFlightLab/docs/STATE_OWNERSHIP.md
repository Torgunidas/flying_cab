# Własność i czas życia stanu

Ten dokument wskazuje źródła prawdy. Widgety oraz teksty HUD są odbiorcami stanu, a nie jego właścicielami.

| Stan | Źródło prawdy | Reset | Czas życia |
|---|---|---|---|
| Kredyty i reguły transakcji | `UFlyingCabEconomyComponent` na `AFlyingCabGameMode` | `ResetCredits` podczas `StartRun` | bieżący poziom; każda zmiana salda jest wysyłana zdarzeniem do prezentera HUD |
| Konfiguracja ekonomii: stawki, ceny usług, holowanie, near-miss, recovery i cel Time Attack | `Content/Data/DA_FlyingCabEconomy` (`UFlyingCabEconomyAsset`) | edycja assetu; walidacja podczas `PostLoad` | konfiguracja projektu; runtime ma bezpieczny fallback C++ na wypadek braku lub błędnych danych |
| Oferty pasażerów, aktywny kurs, opłata i liczba ukończonych kursów | `UFlyingCabDispatchComponent` na `AFlyingCabGameMode` | `StartPassengerMarket`; aktywny kurs także po dowozie lub zniszczeniu aktywnego pojazdu | bieżący poziom; GameMode nie kopiuje tego stanu |
| Tryb biegu, cel Time Attack, czas i statystyki wyniku | `UFlyingCabRunComponent` na `AFlyingCabGameMode` | `StartRun` | bieżący poziom; ukończenie jest wysyłane zdarzeniem, a tryb przekazywany jako opcja podczas przeładowania |
| Paliwo, kadłub i stan zniszczenia | `UFlyingCabVehicleVitalsComponent` na `AFlyingCabPawn` | `ResetResources` albo `Recover` wywołane przez Pawna | życie danego pojazdu; wartości strojenia pozostają w Class Defaults Pawna |
| Rejestr pojazdów, aktywny pojazd i timery recovery | `UFlyingCabFleetComponent` na `AFlyingCabGameMode` | przeładowanie poziomu | bieżący poziom; komponent klasyfikuje zniszczenie jako aktywne albo zaparkowane i emituje wynik do GameMode |
| Fizyka, wejścia i prezentacja pojazdu | `AFlyingCabPawn` | `ResetVehicle` | życie danego pojazdu; `CameraRig` czyta wyłącznie obliczony wektor look-ahead, Pawn nie posiada własnej kamery |
| Mapowanie klawiatury | `Content/Input/IMC_FlyingCabGameplay` i sześć `IA_FlyingCab*`; kontekst instaluje `AFlyingCabPlayerController` | utworzenie lokalnego gracza; `Completed`/`Canceled` zeruje ciągłe osie, a `FlushPressedKeys` synchronicznie czyści stan sterowanego obiektu | sesja lokalnego gracza; pojazd i postać piesza bindują ten sam zestaw akcji Enhanced Input, bez legacy mappings |
| Aktualny tryb gracza: pojazd, pieszo lub nieznany | `AFlyingCabPlayerController::PlayerMode` | każde `OnPossess` | bieżący kontroler/poziom |
| Zdrowie postaci pieszej | `AFlyingCabCharacter` | utworzenie nowej postaci | pojedyncze wyjście z pojazdu |
| Dostęp do pojazdów | `UFlyingCabProgressionSubsystem` | `GrantAccess`/`RevokeAccess` dla pojedynczego prawa; jawne `ResetAccess` na początku Time Attack | instancja aplikacji; przeżywa przeładowanie mapy |
| Definicje zadań | `Content/Data/Quests/DA_FlyingCabQuestCatalog` i wskazane `UFlyingCabQuestDefinition` | edycja assetów; walidacja przy ładowaniu | konfiguracja projektu; tekst nie jest identyfikatorem logiki |
| Statusy, etapy, liczniki i śledzone zadanie | `UFlyingCabQuestSubsystem` | jawne API zadań; Time Attack wyłącza zdarzenia i tracker, ale nie przyznaje nagród | instancja aplikacji; struktura stanu jest gotowa do późniejszego `SaveGame` |
| Najlepsze czasy Time Attack | `UFlyingCabScoreSaveGame` | brak automatycznego resetu | dysk; format oznaczony `SaveVersion` |
| Topologia dzielnic, stacji, tras ruchu i granice minimapy | `Content/Data/DA_FlyingCabCityLayout` (`UFlyingCabCityLayoutAsset`), odczytywany przez `FlyingCabCityData` | edycja assetu; walidacja podczas `PostLoad` | konfiguracja projektu; jeden asset zasila dispatch, geometrię, minimapę i bootstrap świata, z fallbackiem C++ |
| Aktorzy infrastruktury tworzonej w runtime: rozszerzenie miasta, stacje, biuro, portale, terminal, pojazd serwisowy i ruch | `AFlyingCabWorldBootstrap` utworzony przez `AFlyingCabGameMode` | przeładowanie poziomu | bieżący poziom; bootstrap odświeża także stan infrastruktury dostępu |
| Lista śledzonego ruchu, delegaty near-miss, predykcja zagrożeń i czas komunikatu | `UFlyingCabTrafficAwarenessComponent` na `AFlyingCabGameMode` | zmiana aktywnego pojazdu lub przeładowanie poziomu | bieżący poziom; alert jest wysyłany bezpośrednio do prezentera HUD, a near-miss do koordynatora nagrody |
| Projekcja celu, minimapy, ekonomii, ruchu i Time Attack do HUD-u | `UFlyingCabHudPresenterComponent` na `AFlyingCabGameMode` | zmiana poziomu; odświeżanie tekstów 10 Hz | pochodna powyższych źródeł; komponent nie jest właścicielem danych rozgrywki |
| Stan wyświetlany przez HUD | `AFlyingCabPlayerController` posiada jeden `UFlyingCabTouchControls`; widget przechowuje wyłącznie ostatnią prezentację | `OnPossess` przełącza tryb, a timer 10 Hz pobiera zasoby aktywnego pojazdu | pochodna prezentera i powyższych źródeł |

## Kontrakty resetu

- `R` resetuje pozycję, prędkość, paliwo, kadłub i wejścia pojazdu. Nie resetuje kredytów ani aktywnego kursu.
- Zniszczenie aktywnego pojazdu przerywa kurs, nalicza holowanie i uruchamia recovery. Zaparkowany pojazd odzyskuje sprawność bez opłaty i bez przerwania bieżącego kursu.
- Time Attack zawsze resetuje dostęp przy starcie, aby wynik był porównywalny. Free Roam zachowuje dostęp przy przeładowaniu mapy w ramach tej samej sesji aplikacji.
- Zadania są częścią Free Roam. Time Attack nie przyjmuje zdarzeń questowych, nie pokazuje trackera i nie może przyznać nagrody z zadania.
- Śmierć pieszo przeładowuje poziom: stan poziomu znika, dostęp z `GameInstance` pozostaje, a tryb biegu jest przekazywany do nowej mapy.
- `UFlyingCabFleetComponent::ActiveVehicle` wskazuje bieżący albo ostatni aktywny pojazd także podczas chodzenia pieszo. Jawny `PlayerMode` określa sposób sterowania i nie zależy od kolejności metod w `Tick`.

## Zasada zmian

Nowa mechanika powinna modyfikować stan przez jego właściciela i przekazywać wynik do HUD zdarzeniem lub metodą synchronizującą. Pawn nie może tworzyć ani przejmować widgetu HUD; udostępnia kontrolerowi jedynie dane pojazdu. Nie należy tworzyć kolejnego niezależnego zestawu współrzędnych miasta ani kopiować aktywnego pojazdu poza `UFlyingCabFleetComponent`.
