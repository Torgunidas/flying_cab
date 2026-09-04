# System zadań — workflow dla projektu Flying Cab

System jest lekki w runtime, ale jego format jest traktowany jako rdzeń gry. Teksty są wyłącznie prezentacją. Logika, zapis i przyszłe dialogi odnoszą się do stabilnych `FName`: `QuestId`, `ObjectiveId`, `EventId` oraz opcjonalnego `TargetId`.

## Architektura

| Element | Odpowiedzialność |
|---|---|
| `UFlyingCabQuestDefinition` | Niezmienna treść jednego zadania: tytuł, sekwencja celów, oddanie, nagrody, następne zadanie i identyfikatory przyszłych dialogów. |
| `UFlyingCabQuestCatalog` | Jedyny indeks questów używanych przez grę. Domyślna ścieżka: `/Game/Data/Quests/DA_FlyingCabQuestCatalog`. |
| `UFlyingCabQuestSubsystem` | Źródło prawdy runtime w `GameInstance`: statusy, aktywny etap, liczniki i śledzone zadanie. |
| `UFlyingCabQuestEventComponent` | Komponent do dodania do dowolnego Blueprintu. Emituje wybrane `EventId`, `TargetId` i `Amount`. |
| `AFlyingCabQuestGiver` | Konfigurowalny aktor oferujący, przypominający i przyjmujący jedno zadanie. |
| `AFlyingCabQuestInteractable` | Gotowy ogólny obiekt interakcji, którego nie trzeba programować. |

GameMode jedynie łączy ukończenie zadania z istniejącymi właścicielami nagród: ekonomią i `UFlyingCabProgressionSubsystem`. HUD tylko czyta `GetTrackerText()`.

## Tworzenie zadania w edytorze

1. W Content Browser wybierz `Add` → `Data` → `Data Asset`, a następnie klasę `FlyingCabQuestDefinition`.
2. Ustaw unikalne `QuestId`, tytuł, opis oraz kategorię `Main` albo `Side`.
3. Dodaj cele w kolejności wykonania. Każdy cel potrzebuje:
   - unikalnego `ObjectiveId` w obrębie zadania,
   - tekstu dla gracza,
   - `EventId`,
   - opcjonalnego `TargetId`, jeśli liczy się konkretny obiekt,
   - `RequiredCount` większego od zera.
4. Jeżeli gracz ma wrócić do zleceniodawcy, zaznacz `Requires Turn In`.
5. Ustaw nagrodę w kredytach i opcjonalne identyfikatory dostępu.
6. Dodaj asset do `DA_FlyingCabQuestCatalog`.
7. Przypisz asset do aktora `FlyingCabQuestGiver` na mapie albo uruchom go przez `StartQuest` z Blueprintu.

System ma wbudowany katalog zastępczy, dlatego działa także przed utworzeniem assetów binarnych. Po utworzeniu katalogu w Content Browserze jego dane automatycznie zastąpią definicje demonstracyjne C++.

## Wbudowane zdarzenia

| EventId | Emitowane, gdy |
|---|---|
| `Vehicle.Entered` | gracz przejmuje pojazd; `TargetId` to identyfikator pojazdu |
| `Vehicle.Exited` | gracz opuszcza pojazd |
| `Passenger.PickedUp` | zakończy się curbside link; `TargetId` to stabilny identyfikator dzielnicy docelowej |
| `Passenger.Delivered` | zakończy się kurs i naliczona zostanie opłata; `TargetId` to ta sama dzielnica docelowa |
| `Economy.CreditsEarned` | przyznano dodatni przychód; `Amount` to faktycznie przyznane kredyty, a wydatki nie cofają postępu |
| `Service.FuelPurchased` | faktycznie zakupiono paliwo; `Amount` to liczba jednostek |
| `Service.RepairPurchased` | faktycznie naprawiono kadłub |
| `Traffic.NearMiss` | przyznano nagrodę za near miss |
| `Interaction.Completed` | pomyślnie użyto interactable; `TargetId` pochodzi z aktora |
| `QuestGiver.Interacted` | gracz rozmawia z questgiverem |
| `Progression.AccessGranted` | terminal przyznał albo potwierdził dostęp |

Nowych typów celów nie dodajemy przez rozbudowę `switch`. Nowa mechanika emituje stabilne zdarzenie, a projektant wpisuje ten sam `EventId` w definicji zadania. Jeśli zdarzenie ma nie liczyć się globalnie, używa `TargetId`.

Literówki w `EventId` są odrzucane przez walidację katalogu. Zdarzenia emitowane wyłącznie przez Blueprinty trzeba najpierw jawnie dopisać do `AllowedCustomEventIds` w `DA_FlyingCabQuestCatalog`; dzięki temu pozostają data-driven, ale nadal podlegają kontroli pisowni.

Dzielnice mają stabilne identyfikatory w `DA_FlyingCabCityLayout`, niezależne od tekstu wyświetlanego, np. `District.YellowProjects` oraz `District.OrbitalGardens`. Użyj takiego `DistrictId` jako `TargetId`, aby cel dotyczył podjęcia pasażera jadącego do konkretnej dzielnicy albo dostarczenia go do niej.

## Questgiverzy, interactables i dialogi

- Questgiver działa już bez dialogu: pierwsza interakcja przyjmuje zadanie, kolejna śledzi je, a stan `ReadyToTurnIn` je kończy.
- Pierwsze huby są opisane wspólnie w `FlyingCabQuestHubData`: Mike działa w biurze Nightshift, a Jack na skybridge'u w Cobalt Heights. Te same dane sterują spawnem i znacznikami `M`/`J` na minimapie.
- `OfferDialogueId`, `ActiveDialogueId` i `CompletionDialogueId` są zarezerwowanymi połączeniami z przyszłym systemem dialogowym. Nie zawierają tekstu ani logiki questa.
- Przyszły dialog powinien pytać subsystem o `QuestId`/status i wywoływać jego publiczne API. Nie może bezpośrednio modyfikować `FFlyingCabQuestRuntimeState`.
- Każdy nowy rodzaj obiektu świata może implementować `IFlyingCabInteractable::GetQuestTargetId()` lub otrzymać `UFlyingCabQuestEventComponent` w Blueprintcie.

## Zasady zakresu

- Questy działają w Free Roam. Time Attack wyłącza ich zdarzenia i tracker, aby nie mieszać progresji z konkurencyjnym wynikiem.
- Stan żyje obecnie przez czas instancji aplikacji. Struktura `FFlyingCabQuestRuntimeState` ma pola `SaveGame`, ale zapis na dysk zostanie podłączony dopiero razem z zatwierdzonym zakresem pełnego save'a.
- Cele są obecnie sekwencyjne. Rozgałęzienia powinny wybierać następny asset zadania lub późniejszy dialog; nie dodajemy grafu zależności do pierwszej wersji.
- Nagrody nadaje koordynator domeny. Subsystem zadań nie posiada kredytów, licencji, pojazdów ani UI.

## Pierwszy pionowy wycinek

- `Quest.FirstShift` uruchamia się automatycznie w Free Roam: odbierz pasażera, dostarcz go, odbierz 75 kredytów.
- `Quest.NightshiftContract` oferuje Mike w biurze Nightshift: ukończ dwa kursy, wróć do zleceniodawcy, odbierz 200 kredytów.
- `Get_Money` oferuje Jack na skybridge'u w Cobalt Heights i jest pierwszym zadaniem w kategorii `Side`.
- Są to definicje zastępcze do testowania fundamentu. Docelową treść tworzymy jako Data Assets bez zmian w C++.

## Dziennik i komunikacja z graczem

- `J` otwiera modalny `SHIFT LOG`; `J`, `Esc` albo przycisk `CLOSE` zamyka ekran.
- Zakładki `MAIN QUEST` i `SIDE QUEST` dzielą zadania według kategorii. Każdy wpis pozostaje w swojej zakładce i pokazuje stan `TAKEN`, `TRACKED`, `READY` albo `DONE`.
- Wybranie wpisu pokazuje opis, bieżący cel, licznik postępu i nagrodę. `TRACK` wybiera zadanie dla małego trackera HUD, a `STOP TRACKING` ukrywa go bez porzucania zadania.
- Dotyk jest domyślny: gracz dotyka zakładek, dużych wierszy, przycisków `PREV`/`NEXT` oraz akcji śledzenia. Bez myszy działają też `A/D` lub strzałki lewo/prawo (zakładki), `W/S` lub strzałki góra/dół (wybór), `Enter`/`Spacja` (śledzenie) oraz `J`/`Esc` (zamknięcie).
- Przyjęcie zadania, ukończenie celu, gotowość do oddania, ukończenie zadania i zniszczenie aktywnej taksówki emitują duże kolejkowane plansze. Drobne informacje używają oddzielnej kolejki komunikatów HUD.
- UI konsumuje `FFlyingCabQuestJournalEntry` i `FFlyingCabQuestUpdate`. Nie czyta prywatnych map subsystemu i nie modyfikuje stanu runtime.
- Native widget jest działającym widokiem bazowym. Docelową oprawę można zastąpić klasą potomną Widget Blueprint bez przenoszenia logiki zadań do UMG.

## Test ręczny pierwszej wersji

1. Uruchom `FlightLab` i wybierz `FREE ROAM`.
2. W prawym górnym rogu powinien pojawić się tracker `FIRST SHIFT`.
3. Odbierz dowolnego pasażera. Tracker powinien przełączyć się z odbioru na bezpieczny dowóz.
4. Zakończ kurs. Powinien pojawić się komunikat ukończenia zadania i dodatkowa nagroda `75 CR` poza zwykłą opłatą za kurs.
5. Odszukaj na minimapie czarny znacznik `M`, poleć do wejścia `NIGHTSHIFT OFFICE`, wysiądź, wejdź do środka i podejdź do Mike'a oznaczonego `!`.
6. Naciśnij `Q`, aby przyjąć `NIGHTSHIFT CONTRACT`. Wróć do miasta i wykonaj dwa pełne kursy.
7. Tracker powinien pokazać kolejno `0/2`, `1/2`, a następnie prośbę o powrót do questgiversa.
8. Wróć do dispatchera. Jego marker powinien zmienić się na `?`; interakcja kończy zadanie i przyznaje `200 CR` dokładnie raz.
9. Uruchom `TIME ATTACK`. Tracker zadań nie może być widoczny, a kursy nie mogą zmieniać ich postępu ani przyznawać nagród questowych.
10. W `FREE ROAM` naciśnij `J`. Sprawdź dotykiem i klawiaturą przełączanie `MAIN QUEST`/`SIDE QUEST`, wybór zadania oraz `TRACK`/`STOP TRACKING`.
11. Odszukaj na minimapie czarny znacznik `J`, wyląduj na skybridge'u w Cobalt Heights, podejdź na piechotę do Jacka i przyjmij `Get_Money`. Zadanie powinno pojawić się jako `TAKEN` albo `TRACKED` w `SIDE QUEST`.
12. Otwórz dziennik podczas trzymania `W` albo `A`, puść klawisz i zamknij dziennik. Sterowanie nie może pozostać zablokowane.
