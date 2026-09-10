# Questy i rozmowy NPC — konfiguracja w Unreal 5.8

Po przebudowie projektu uruchom ponownie edytor. Otwórz **Tools → Flying Cab — Quest & Dialogue tools**. To punkt wejścia do tworzenia questów, profili NPC i rozmów oraz otwierania katalogu i listy NPC. Te same typy assetów są dostępne w Content Browser w kategorii **Flying Cab**.

## Dodanie zadania do Mike’a lub Jacka

1. Wybierz **New quest**, nadaj nazwę assetu i zapisz go w `Content/Data/Quests`.
2. Ustaw `Title`, `Description` i kategorię `Main` lub `Side`. Identyfikatory powstają automatycznie, również przy duplikowaniu assetu.
3. Rozwiń `Objectives`. Dla każdego etapu wybierz **Objective type**, wpisz opis dla gracza i ustaw `Required Count`. Przy przewozach wybierz opcjonalną dzielnicę w **Target**; `Any / no filter` oznacza dowolny cel.
4. Ustaw `Reward`. Jeśli gracz ma wrócić po nagrodę, zaznacz `Requires Turn In`. W **Turn in at** możesz wybrać konkretnego NPC albo pozostawić dowolnego NPC oferującego ten quest.
5. Kliknij **Add to catalog**, następnie **Validate**. Zapisz quest i katalog przez **Save All**.
6. Otwórz `Content/Data/Narrative/DA_NPC_Mike` lub `DA_NPC_Jack`. Dodaj element w `Topics` i wskaż nowy quest w polu `Quest`. Pusty `Title` używa tytułu zadania. Puste `Dialogue` uruchamia gotową rozmowę o przyjęciu, postępie i oddaniu zadania.
7. Kliknij **Preview conversation**, sprawdź rozmowę dla różnych stanów i zapisz profil. Uruchom nową sesję Play w **FREE ROAM**, podejdź pieszo do NPC i naciśnij `Q`.

Samo otwarcie rozmowy nie przyjmuje zadania. Gracz wybiera temat, a następnie odpowiedź przyjmującą zlecenie. Ukończone zadanie wymagające oddania otrzymuje osobną odpowiedź odbioru nagrody. Nagroda może zostać przyznana tylko raz.

## Dostępne cele

Zachowano dotychczasowe typy. Cele wykonują się kolejno; wcześniejsze zdarzenia nie są naliczane wstecz. Filtr dotyczy identyfikatora przekazywanego przez istniejącą mechanikę.

| Wybór w edytorze | Co jest liczone / filtrowane |
|---|---|
| Deliver passengers | Zakończone kursy; opcjonalny filtr dzielnicy docelowej. |
| Pick up passengers | Zakończone odbiory; filtr wskazuje dzielnicę **docelową**, nie miejsce odbioru. |
| Earn credits | Dodatni przychód od rozpoczęcia etapu, również nagrody questowe. Wydatki nie cofają postępu. |
| Buy fuel | Faktycznie kupione jednostki paliwa. |
| Repair vehicle | Faktycznie kupione jednostki naprawy. |
| Enter / Exit a vehicle | Wejścia do pojazdu lub wyjścia; zaawansowany filtr identyfikatora pojazdu. |
| Earn a near-miss bonus | Przyznane bonusy near miss. |
| Use an object / finish an NPC conversation | Użycia interactable lub normalnie zakończone rozmowy; opcjonalny filtr obiektu/NPC. |
| Finish an NPC conversation | Normalnie zakończone rozmowy po odwiedzeniu tematu; zamknięcie przez `Esc` nie liczy się. |
| Claim or confirm access at a terminal | Przyznanie lub potwierdzenie dostępu przez terminal. |

`Target ID (advanced / custom objects)` służy istniejącym identyfikatorom obiektów. Nie trzeba go wpisywać ręcznie dla dzielnic ani NPC obecnych w liście lub na otwartej mapie. Zdarzenia z własnych Blueprintów wymagają wpisu w `AllowedCustomEventIds` katalogu i emisji przez `UFlyingCabQuestEventComponent`. Nie dodano nowych mechanik konkretnego pasażera ani dostarczania konkretnego pojazdu.

## Własne dialogi

W narzędziach wybierz **New conversation — quest template** albo **New conversation — small talk**. Pierwszy szablon zawiera ofertę, przypomnienie, oddanie i stan po ukończeniu. Drugi jest zwykłą rozmową bez zadania.

- `Nodes` zawiera wypowiedzi. Pusty `Speaker` używa imienia NPC. `Choices` to odpowiedzi gracza.
- `Action` określa efekt odpowiedzi: kontynuacja, przyjęcie, oddanie, śledzenie questa lub powrót do tematów. Puste `Quest` korzysta z questa wskazanego na temacie NPC.
- **Go to** wybiera następną wypowiedź z listy zawierającej fragment tekstu. `End conversation` kończy rozmowę. Akcja `Back to topics` wraca do tematów i ignoruje przejście.
- **Default start** wskazuje start rozmowy. `Entry Rules` są sprawdzane od góry; pierwsza pasująca reguła statusu wybiera inną wypowiedź.
- `Conditions` odpowiedzi wymagają spełnienia wszystkich warunków. `Invert` odwraca warunek. Niedostępna odpowiedź jest wyłączona z wyjaśnieniem albo ukryta przez `Hide When Unavailable`.
- Teksty mogą używać `{QuestTitle}`, `{QuestDescription}`, `{Objective}`, `{Progress}`, `{Required}` i `{RewardCredits}`.

Identyfikatory wypowiedzi są automatyczne. Po dodaniu nowej wypowiedzi wybierz ją z listy przejść. **Validate** wykrywa m.in. puste teksty i nieistniejące przejścia. **Preview conversation** działa bez Play i symuluje stan questów bez przyznawania rzeczywistych nagród. Przy podglądzie samego dialogu wybierz kontekst questa; przy podglądzie profilu bierze go z tematu. Zmiana początkowego statusu i ponowne uruchomienie pozwalają sprawdzić ofertę, postęp, oddanie i stan po ukończeniu. Podgląd nie symuluje przejazdów ani ekonomii.

Przypisz gotowy asset do `Dialogue` w odpowiednim temacie profilu. Jeden NPC może mieć wiele zadań i tematów niezwiązanych z zadaniami.

## Dodanie lub przeniesienie NPC

Utwórz **New NPC profile**, ustaw imię, powitanie, literę minimapy i tematy. Następnie użyj jednej z możliwości:

- **Open NPC roster**: dodaj profil i `World Location`. Lista zasila automatyczne tworzenie NPC oraz znaczniki minimapy.
- Umieść aktora `FlyingCabQuestGiver` na mapie i przypisz `Npc Profile`. Aktor z tym samym identyfikatorem zastępuje pozycję z listy. Umieszczony nowy profil również trafia do znaczników.

Każdy osobny NPC powinien mieć osobny profil; duplikacja profilu nadaje nową tożsamość. Lokalizacja w liście jest punktem pojawienia się aktora, więc dobierz wysokość do podłoża. Nowy punkt sprawdź pieszo w Play.

Mike i Jack korzystają z `DA_NPC_Mike`, `DA_NPC_Jack` i `DA_FlyingCabNpcRoster` w `Content/Data/Narrative`. Ich oryginalne questy i lokalizacje są zachowane. Każdy ma też przykładowy temat o mieście. Domyślne assety oraz opcjonalny Widget Blueprint rozmowy wybiera się w **Project Settings → Game → Flying Cab Narrative**.

## Powiązania zadań i granice wersji

`Prerequisite Quests` wymaga ukończenia wszystkich wskazanych zadań przed przyjęciem. `Next Quest` próbuje uruchomić wskazane zadanie po ukończeniu bieżącego i respektuje jego warunki. Wszystkie powiązane questy muszą znaleźć się w katalogu; walidacja odrzuca nieprawidłowe definicje i cykle w łańcuchach następników oraz wymagań. Gdy inne wymaganie nie jest jeszcze ukończone, następny quest trzeba później przyjąć normalną drogą.

Questy i rozmowy są dostępne w Free Roam. Time Attack nie zmienia ich postępu. Stan questów żyje w `GameInstance`; nie dodano zapisu na dysk. Po zmianie struktury definicji rozpocznij nową sesję Play. Dotychczasowy dziennik `J` pozostaje widokiem statusów i śledzenia.

Dialog pauzuje grę. Odpowiedzi wybiera się kliknięciem/dotykiem lub `W/S` / strzałkami i `Enter` / `Spacja`; `Esc` zamyka rozmowę. Logika wyborów należy do `UFlyingCabDialogueSession`, a nie widgetu. Docelowy Widget Blueprint może implementować `PresentDialogue` i korzystać z `ChooseOption` / `CloseDialogue`; publiczne API subsystemu questów pozostaje właścicielem postępu i ukończenia.

## Praktyczna próba

1. Dodaj Mike’owi quest: jeden dowóz, nagroda 50 CR, wymagany powrót do Mike’a.
2. W podglądzie sprawdź cztery stany; w Free Roam otwórz temat i odmów. Zadanie nie powinno pojawić się jako przyjęte.
3. Przyjmij zadanie, wykonaj kurs, wróć i odbierz nagrodę. Kolejne otwarcie rozmowy nie może jej powtórzyć.
4. Porozmawiaj o mieście i wróć do tematów. Sprawdź też rozmowę z Jackiem.
5. Otwórz dialog z trzymanym `A`, puść klawisz w rozmowie i zamknij ją. Sprawdź dalszy ruch pieszy, wejście do auta i dziennik.
