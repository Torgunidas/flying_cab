# Dialogi i zadania — tworzenie w Godocie

Stan: 2026-09-14, Godot 4.7.2. Definicje są zwykłymi zasobami `.tres`, edytowanymi w Inspectorze. Kolejne zadanie, odpowiedzi, warunki, nagrody i NPC dodaje się bez pisania osobnego skryptu. Walidator i scena podglądu korzystają z tego samego modelu co gra. Dostępny jest również zewnętrzny edytor tekstu i grafu z formularzami celów, warunków, rozwidleń i postaci: [Flying Cab Story Editor](STORY_EDITOR.md). To zalecana droga do pisania nowych treści; opis ręcznej edycji zasobów poniżej pozostaje referencją.

## Uruchomienie przykładów

- **F5** otwiera menu `scenes/game.tscn`; wybierz **Kontynuuj** albo **Nowa gra**. Wysiądź Q / przyciskiem dotykowym i podejdź do **Mai w Depot**. Przycisk zmieni się na **ROZMOWA / Q**. Maya stoi przy zachodnim końcu tarasu, Froggy przy wschodnim końcu **Torque / Foundry03**. Są osobnymi instancjami w `scenes/narrative/city_narrative.tscn`, dołączonej do `flight_lab.tscn`.
- Wybór „Wrócę z lekiem” przyjmuje `first_dose` i rozpoczyna odliczanie. Kup dawkę u Froggy’ego, wróć do Mai i wybierz podanie leku. Samo dotarcie, zakup lub zamknięcie rozmowy nie zalicza dostawy. Kolejne zakupy i dostawy działają także po ukończeniu pierwszego zadania.
- Po pierwszej dostawie Froggy udostępnia `froggy_favor`: dwa nowe zwykłe kursy, następnie osobiste odebranie nagrody. Wybór zapisuje fakt obietnicy, który zmienia dostępną kwestię o mieście.
- **J / ikona ≡** otwiera zadania i przedmioty. Gwiazdka oznacza śledzone zadanie; kliknięcie aktywnego zadania zmienia wybór. Nie dodaje trasy ani stałego panelu podczas lotu.
- Tekst pojawia się stopniowo; odpowiedzi kolejno pod nim. Kliknięcie wypowiedzi albo Enter/spacja odsłania tekst. Dopiero kolejne zatwierdzenie wybiera widoczną odpowiedź. W/S lub strzałki wybierają, 1–9 uruchamiają widoczne odpowiedzi, Escape/× zamyka. Dłuższe listy przewija się; nie ma limitu trzech odpowiedzi.

Roboczy wycinek ma **120 CR na początku gry, 160 CR za dawkę, 600 s początkowego czasu i +600 s za dostawę**. Drugie zadanie daje 60 CR i notatkę. To dane przykładów do testowania mechaniki, nie finalny scenariusz, wygląd postaci ani balans całej kampanii. Modele obu rozmówców są tymczasowe, z istniejącej biblioteki ludzi.

Rozmowa i dziennik pauzują świat, NPC i zegar kampanii. Utrata fokusu także zatrzymuje odliczanie; nie naliczamy czasu offline. Wygaśnięcie zapisuje stan końca gry i pokazuje możliwość rozpoczęcia nowej sesji. Zasady te odpowiadają bieżącej implementacji, a nie historycznej polityce timera starego Godota.

## Kompaktowy panel rozmowy

**Doprecyzowanie autora 2026-09-14:** samo pole wypowiedzi NPC ma wysokość **trzech standardowych rzędów odpowiedzi z odstępami: 3 × 52 + 2 × 8 = 172 px UI**. Górna krawędź panelu i początek tekstu NPC są stałe dla danego rozmiaru okna oraz ustawienia wysokości. Panel rezerwuje miejsce na trzy rzędy odpowiedzi również wtedy, gdy dostępna jest tylko jedna lub dwie, oraz na wskazówkę przewijania także wtedy, gdy jest ukryta. Długość wypowiedzi, animacja tekstu i liczba odpowiedzi nie przesuwają miejsca czytania ani kadru rozmowy. Odpowiedzi zachowują dotychczasowe pola (minimum 52 px, ta sama czcionka i odstępy). Przy zbyt długiej wypowiedzi pojawia się pasek oraz wskazówka **„↓ Przesuń tekst”**; kierunek zmienia się wraz z pozycją przewinięcia. Tekst można przewijać dotykiem albo kółkiem myszy. Każda nowa wypowiedź zaczyna się od góry pola. Dłuższa lista odpowiedzi ma własne przewijanie w obszarze wysokości trzech standardowych rzędów. Dziennik pozostaje osobnym, większym widokiem.

**Ręczna zmiana wysokości tekstu NPC:** otwórz `scenes/flight_lab.tscn`, wybierz korzeń **FlightLab → Dialogue Npc Text Height** w Inspectorze, wpisz wysokość w pikselach UI i zapisz scenę. W trakcie gry to samo pole znajdziesz przez **Scene → Remote → Game → FlightLab**. Zmiana działa od razu także podczas dialogu z pauzą; zmienia wyłącznie wysokość wypowiedzi NPC, a przyciski zachowują rozmiar. Wartość można też podejrzeć na dziecku **NarrativePanel → Npc Text Height**. Zmiany Remote są tymczasowe — wybraną wartość przepisz do Local i zapisz scenę. Przy wyjątkowo niskim oknie wysokość jest ograniczana tak, aby odpowiedzi pozostały dostępne.

Natywne gesty działają przy wyłączonym w projekcie `emulate_mouse_from_touch`: tapnięcie tekstu odsłania wypowiedź, tapnięcie widocznej odpowiedzi wybiera ją, a przeciągnięcie powyżej 8 px przewija obszar pod palcem bez wyboru. Gest jest powiązany z jednym palcem i rewizją dialogu; anulowanie lub zmiana kwestii nie zatwierdza starej odpowiedzi.

### Zwięzłość tekstów — obowiązek autora treści

- **NPC:** pisz bardzo zwięźle, zwykle jedno lub dwa krótkie zdania na węzeł. Jedna kwestia powinna przekazywać jedną istotną informację lub intencję. Dłuższą wymianę rozbij na kolejne węzły. Przewijanie jest zabezpieczeniem wyjątków, nie docelowym sposobem czytania każdej rozmowy.
- **Gracz:** odpowiedź ma mieścić się w obecnym przycisku, najlepiej w jednym wierszu. Krótka, jednoznaczna intencja: „Wrócę z lekiem.”, „Ile to kosztuje?”, „Nie teraz.”. Nie powiększaj przycisków ani nie zmniejszaj czcionki, żeby pomieścić rozwlekły tekst.
- Sprawdzaj najwęższy wspierany ekran **360×640** w podglądzie F6, z docelową czcionką. Sam limit znaków nie gwarantuje dopasowania: liczą się szerokości liter, tłumaczenie oraz podstawione wartości `{credits}`, `{medicine}`, `{seconds}`. Skróć odpowiedź, jeśli się zawija i powiększa pole.

Ta sama wskazówka jest przy polach **Text** zasobów `DialogueNode` i `DialogueChoice` w Inspectorze. UI nadal zachowuje treść dłuższych danych testowych i awaryjne przewijanie, zamiast ucinać znaczenie decyzji gracza.

## Gdzie są dane

| Zasób / scena | Rola |
| --- | --- |
| `resources/narrative/city_catalog.tres` | Jawne listy questów, dialogów, NPC, przedmiotów oraz słowniki faktów, uprawnień i własnych zdarzeń. Przycisk **Validate catalog**. |
| `QuestDefinition` | Tytuł, opis, kolejne cele, wymagane wcześniejsze questy, warunki przyjęcia, oddanie i nagrody. |
| `QuestObjective` | Jeden cel: zdarzenie z licznikiem albo sprawdzany stan. |
| `DialogueDefinition` | Domyślny początek, reguły początku i lista węzłów. |
| `DialogueNode` / `DialogueChoice` | Wypowiedź, dowolna liczba odpowiedzi, przejścia, warunki i lista efektów. |
| `NpcDefinition` / `NpcTopic` | Imię, opcjonalny portret, powitanie i tematy rozmowy. |
| `NarrativeCondition` / `NarrativeEffect` | Deklarowane wymagania i mechaniczne skutki wyboru. |
| `NarrativeItem` | Stabilne ID, nazwa i opcjonalna ikona przedmiotu. Ilość jest stanem sesji. |
| `scenes/narrative/npc.tscn` | Instancja rozmówcy do ustawienia w świecie. |
| `scenes/narrative/quest_area.tscn` | Obszar wysyłający zdarzenie wejścia kontrolowanego aktora. |
| `scenes/narrative/preview.tscn` | Podgląd F6 bez odczytu i zapisu właściwej gry. |

Katalog gry wybiera się w polu **Narrative Catalog** w `game.tscn`. Dla podglądu wybierz ten sam plik w polu **Catalog** jego głównego węzła. Katalog jest jawny: dodanie pliku do folderu samo w sobie go nie rejestruje.

## Pierwsze własne zadanie — krok po kroku

Przykład: dwa kursy kończące się w Depot, potem 75 CR od rozmówcy.

1. W panelu FileSystem wybierz `resources/narrative`, prawy przycisk → **Create New → Resource** → `QuestDefinition`. Zapisz jako `depot_driver.tres`.
2. W Inspectorze ustaw **Id** `depot_driver`, **Version** `1`, **Title** i **Description`. ID służy zapisowi, tytuł graczowi. Ustaw **Category** `side`.
3. Rozwiń **Objectives**, dodaj element i wybierz **New QuestObjective**. Kliknij nowy zasób, aby rozwinąć jego Inspector. Ustaw **Id** `return_trips`, **Description**, **Mode** `event`, **Event** `ride_completed`, **Required** `2`, **Destination Id** `depot`. Pozostałe filtry pozostaw puste.
4. Ustaw **Requires Turn In** na `On`, **Turn In Npc** na ID istniejącego NPC, np. `froggy`, **Reward Credits** na `75`. Przy `Off` nagroda trafia automatycznie po ostatnim celu.
5. Jeżeli zadanie wymaga wcześniejszego zadania, wpisz jego ID w **Prerequisites**. Ta lista tylko blokuje dostęp. **Auto Start = Off** oznacza, że potrzebna jest odpowiedź z efektem `start_quest`. `On` pozwala rozpocząć zadanie automatycznie, gdy wszystkie wymagania są spełnione.
6. Otwórz `city_catalog.tres`, dodaj zapisany plik do **Quests**. Utwórz rozmowę według następnej sekcji i dodaj ją do NPC.
7. Kliknij **Validate catalog**, potem sprawdź podgląd F6 i grę. Błędy mają nazwę questa/węzła; znajdziesz je w Output/Debugger.

Cele **domyślnie są sekwencyjne**. Opcjonalne `QuestTransition` pozwala wybrać następny cel według warunków; zasady i zapis gałęzi: [Story Editor](STORY_EDITOR.md). W obu trybach: jedno zdarzenie może postąpić wiele aktywnych questów, ale tylko jeden ich bieżący cel. Zdarzenia sprzed przyjęcia nie liczą się wstecz. Stan sprawdzamy od razu przy aktywacji oraz okresowo podczas gry. Dla „posiada 200 CR” wybierz `state` + warunek `credits >= 200`; dla „zarobi kolejne 200 CR” wybierz `event / credits_earned / required 200`.

**Required:** dla liczby kursów wpisuj liczbę całkowitą, np. dokładnie `2`. Ułamki służą m.in. ilości paliwa i naprawionym HP. Poprawka 2026-09-14 wyrównuje krok pola w Inspectorze: wcześniej wpisane `2` mogło zostać zapisane jako `2.001`, co wymagało trzeciego kursu. W zasobach utworzonych wcześniej sprawdź i ponownie wpisz zamierzoną wartość; poprawka pola nie zmienia zapisanych danych.

## Własna rozmowa — krok po kroku

1. Utwórz zasób **DialogueDefinition**, np. `depot_driver_dialogue.tres`. Ustaw **Id** `depot_driver_dialogue`, **Start Node** `offer`.
2. W **Nodes** dodaj **New DialogueNode**: **Id** `offer`, **Text** treść oferty. Puste **Speaker** korzysta z imienia NPC.
3. W **Choices** tego węzła dodaj **New DialogueChoice**: **Id** `accept`, **Text** „Zajmę się tym”, **Next Node** `accepted`, **Once** `On`. W **Effects** dodaj **New NarrativeEffect**, **Kind** `start_quest`, **Key** `depot_driver`.
4. Dodaj węzeł `accepted`, jego tekst i odpowiedź `leave`. Puste **Next Node** kończy rozmowę. Dodaj także odpowiedź odmowy w `offer`, bez efektów, z pustym przejściem.
5. Dodaj węzły `active`, `ready`, `done`. W `ready` dodaj odpowiedź `claim`, **Once** `On`, efekt **Kind** `turn_in`, **Key** `depot_driver`, **Next Node** `done`.
6. W **Entries** głównego dialogu dodaj trzy **DialogueEntry**. Każdy wskazuje odpowiedni **Node Id** i ma warunek **Kind** `quest_status`, **Key** `depot_driver`, **Status** kolejno `active`, `ready`, `completed`. Pierwsza pasująca reguła wygrywa; jeśli żadna nie pasuje, używany jest `Start Node`.
7. Dodaj plik do **Dialogues** katalogu. Otwórz profil NPC i w **Topics** dodaj **New NpcTopic** z własnym ID, tytułem i tym plikiem w **Dialogue**. Warunki tematu mogą ukryć całą ofertę, np. do ukończenia poprzedniego questa.

Każdy dialog dostaje systemowe wyjście „Inny temat”, a lista tematów „Do zobaczenia”. Nawet gdy wszystkie autorskie odpowiedzi są niedostępne, można wyjść. Niedostępna odpowiedź z **Hide Unavailable = Off** pozostaje wyłączona i pokazuje **Reason** warunku; `On` ją ukrywa. Warunki są ponownie sprawdzane przy wyborze. Rewizja widoku chroni przed starym przyciskiem i podwójnym kliknięciem.

**Once** zapisuje trwałe potwierdzenie `dialogue/node/choice`. Włącz je przy prezencie lub obietnicy, którą można otrzymać tylko raz. Zakup leku i podanie kolejnej dawki są powtarzalne i nie powinny mieć `Once`. Same `start_quest` i `turn_in` dodatkowo pilnują legalnego stanu zadania. Tekst wypowiedzi i odpowiedzi obsługuje `{credits}`, `{medicine}`, `{seconds}`; nie interpretuje kodu ani ścieżek.

## Warunki

Wszystkie warunki w jednej liście muszą być spełnione (AND). **Invert** odwraca wynik pojedynczego warunku. Alternatywy (OR) można zapisać jako osobne reguły `Entries` lub osobne odpowiedzi.

| Kind | Key / pozostałe pola |
| --- | --- |
| `quest_status` | ID questa; **Status** `inactive`, `active`, `ready`, `completed`. |
| `fact` | Nazwa z **Fact Ids** katalogu; liczbowe **Comparison** i **Amount**. Brak faktu oznacza 0. |
| `credits` | Bieżące saldo; porównanie i kwota. |
| `item` | ID z **Items** katalogu; porównanie ilości. |
| `medicine` | Liczba dawek; porównanie ilości. Lek jest osobnym stanem kampanii, nie dodatkowym generycznym przedmiotem. |
| `campaign_active` | Czy trwa odliczanie; do zaprzeczenia użyj **Invert**. |
| `fuel_percent` | Paliwo aktualnie prowadzonego auta, 0–100; porównanie. Pieszo warunek jest fałszywy. |
| `vehicle_id` | Dokładne ID aktualnie prowadzonego auta, np. `ari_cab`. Pieszo fałsz. |
| `vehicle_model` | ID modelu prowadzonego auta, np. z `vehicle_catalog.tres`. Pieszo fałsz. |
| `access` | Nazwa z **Access Ids** katalogu; sprawdza nadane uprawnienie. |

`access` jest trwałym kluczem fabularnym dostępnym w warunkach. Obecne auta i dzielnice nie są przez niego automatycznie blokowane; drzwi lub inne nowe obiekty wymagają podłączenia ich zachowania do tego warunku.

## Efekty

Cała lista efektów odpowiedzi jest planowana na kopii stanu, w kolejności z Inspektora. Dopiero sukces wszystkich operacji zatwierdza portfel, kampanię, przedmioty, fakty i questy. Błąd płatności nie może zostawić wcześniej przyznanego przedmiotu. Nie umieszczaj w danych nazw metod do wykonania.

| Kind | Key | Amount | Value |
| --- | --- | --- | --- |
| `set_fact` | Zarejestrowany fakt | Nowa wartość, zwykle 0/1 | — |
| `start_quest` | ID questa | — | — |
| `turn_in` | ID gotowego questa | — | — |
| `credit` / `spend` | — | Kwota CR | — |
| `grant_item` / `remove_item` | ID przedmiotu | Dodatnia całkowita ilość | — |
| `start_campaign` | — | Początkowe sekundy | — |
| `buy_medicine` | — | Dodatnia całkowita liczba dawek | Cena CR za dawkę |
| `deliver_medicine` | — | Dodatnia całkowita liczba dawek | Sekundy dodane za dawkę |
| `grant_access` / `revoke_access` | Zarejestrowane uprawnienie | — | — |

`start_campaign` nie pozwala restartować odliczania kolejnym wyborem. `deliver_medicine` wymaga aktywnej kampanii i zapasu; emituje `medicine_delivered` z ID rozmówcy. Czas jest zawsze w **sekundach**, nie w minutach ze starego `add_maya_time`.

Jedna akcja zakupu lub podania obsługuje maksymalnie 1000 dawek.

**Rewards** questa dopuszcza `set_fact`, `grant_item`, `grant_access`, `revoke_access`; pieniądze ustawiaj w **Reward Credits**. Nowy etap łańcucha konfiguruj przez `Prerequisites` i ewentualne `Auto Start`. Dokładne zaliczenie celu rozmowy można zapisać jako efekt `set_fact` w konkretnej odpowiedzi i cel `state` z warunkiem tego faktu.

## Zdarzenia i obszary świata

| Event | Amount / filtry dostępne z adaptera |
| --- | --- |
| `ride_completed` | 1 zakończony kurs; **Origin**, **Destination**, **Vehicle**. |
| `passenger_boarded` | 1 wejście grupy kursu; **Origin**, **Destination**, **Vehicle**. Nowe kursy są jednoosobowe. |
| `credits_earned` | Kwota przychodu; **Target** `fare`, `quest` lub `other`. Przychód z kursu przed spłatą długu; saldo sprawdzaj osobno. |
| `fuel_purchased` | Kupione jednostki paliwa; **Target** ID przystanku, **Vehicle**. |
| `vehicle_repaired` | Przywrócone HP; **Vehicle**. |
| `vehicle_entered` / `vehicle_exited` | 1; **Vehicle** i **Target** są ID auta. |
| `vehicle_taken` | 1 przejęcie; **Vehicle** ID auta, **Target** ID poprzedniego właściciela. |
| `area_entered` | 1; **Target** ID obszaru, **Vehicle** ID auta albo puste przy wejściu pieszo. |
| `medicine_delivered` | 1 za każdą podaną dawkę; **Target** ID NPC. |
| `custom` | Nazwa w **Custom Event**, zarejestrowana w **Custom Events** katalogu. Wymaga emitowania z odpowiedniego systemu rozgrywki. |

Pusty filtr akceptuje dowolną wartość. Wszystkie niepuste filtry muszą pasować. ID przystanku (np. `depot`) nie jest nazwą węzła platformy (`Pad0`). Zobacz pole **Stop Id** jej `TaxiStop`. Walidator kontroluje słowniki katalogu, ale nie wyszukuje dowolnych pojazdów i obszarów na wszystkich mapach — te identyfikatory trzeba sprawdzić w scenie.

Aby dodać cel dotarcia: zainstancjuj `quest_area.tscn` w edytowanej mapie, ustaw **Area Id**, pozycję i rozmiar **CollisionShape3D**. W celu wybierz `area_entered`, a **Target Id** ustaw na tę samą nazwę. Liczy się tylko ciało kontrolowane przez gracza, nie przechodzący NPC. **Once = Off** pozwala zaliczyć kolejne wejścia; `On` zapamiętuje pierwsze wejście w sesji, także gdy quest jeszcze nie był aktywny. Do zwykłych zadań dotarcia pozostaw `Off`.

Nowa kategoria mechaniki potrzebuje jednego adaptera w kodzie, nie skryptu dla każdego questa. Przykład po potwierdzonym zdarzeniu świata:

```gdscript
context.narrative.record_event("parcel_delivered", {
    "target": "clinic",
    "vehicle": String(vehicle.entity_id),
    "amount": 1.0,
}, "parcel/" + delivery_id)
```

Zarejestruj `parcel_delivered` jako własne zdarzenie. Ostatni argument jest trwałym, unikalnym ID operacji: ponowne zgłoszenie tego samego potwierdzenia nie dodaje postępu. Ciągłe paliwo/naprawa nie zapisują całej sesji w każdej klatce, gdy nie zmieniają aktywnego celu.

## Własny rozmówca w scenie

1. Utwórz **NpcDefinition**, nadaj **Id**, **Display Name**, **Greeting**, opcjonalny **Portrait** i tematy. Dodaj go do **Npcs** katalogu.
2. Otwórz mapę lub jej osobną scenę postaci, zainstancjuj `scenes/narrative/npc.tscn` i przypisz profil w **Profile**. Możesz zastąpić dziecko **Visual** własną sceną modelu. Tekst dziecka **Name** odświeża się z profilu.
3. Umieść korzeń na nawierzchni tarasu, na **Z = 1.2**. Korzeń oznacza stopy. Ustaw **Interaction Distance**, domyślnie 1.8 m. Sprawdź w grze, czy gracz stoi na podłodze, jest w zasięgu i nie oddziela go ściana.
4. Narracyjne postacie przykładów są poza generowanymi detalami miasta. Nie dodawaj ich przez `build_city.py`. Samo ustawienie NPC/obszaru nie wymaga kompilacji miasta; zmiana geometrii `city.tscn` nadal wymaga jawnego `compile_city.gd`.

## Huby questowe na mapie

Mapa miasta pokazuje okrągły znacznik z pierwszą literą **Display Name** każdego obecnego, widocznego NPC, u którego można teraz rozpocząć zadanie, oddać gotowe zadanie albo posunąć aktywne zadanie przez rozmowę. Obejmuje też dostarczenie kolejnej dawki leku Mai. Dotknięcie litery pokazuje imię i dostępne tematy. Zwykła pogawędka lub sam sklep nie włączają znacznika.

Nie ma osobnej listy hubów do utrzymywania: wystarczą profil w katalogu, instancja `NarrativeNpc` na mapie i poprawnie podłączone tematy. Mapa korzysta z rzeczywistej pozycji instancji, również z nadpisań w scenie poziomu. Dla obecnej zawartości są to **M / F / B** — Maya, Froggy i Bruno.

Dostępność uwzględnia warunki tematu, pierwszą pasującą regułę **Entries**, kolejne osiągalne odpowiedzi, warunki questa, koszty i zużyte wybory **Once**. Samo istnienie `start_quest` w niedostępnej gałęzi nie włącza hubu. Podczas wykonywania zadania znacznik gaśnie, jeżeli NPC nie ma innej dostępnej sprawy; wraca po spełnieniu warunków oddania. Odczyt mapy planuje efekty na kopiach danych, bez rozpoczynania rozmowy, zmiany postępu ani zapisu. Istniejący zapis schema 5 wystarcza do odtworzenia dostępności.

Sprawdzenie: `bash tools/verify.sh quest-map` oraz `bash tools/verify.sh quest-map-render`. Druga próba zapisuje `build/quest-map-active.png` i widoki 360×640 / 960×540. Test obejmuje trzy obecne postacie, przykładowego nowego NPC, warunkowe gałęzie i pętle rozmów, włączanie/wyłączanie, oddanie, ponowną dawkę, odtworzenie sesji oraz dotyk i pauzę mapy.

Weryfikacja 2026-09-15, Godot 4.7.2 / macOS: **271/271** — `quest-map` 36/36, `quest-map-render` 38/38, `narrative` 69/69, `narrative-render` 25/25, `taxi-ui` 21/21, `living-world` 82/82. Katalog: 0 błędów, 5 questów / 7 dialogów / 3 NPC. Import edytora bez błędów; widoki mapy sprawdzono wizualnie w trzech rozmiarach. Windows i fizyczny telefon nie były testowane. Bez eksportu i pakowania.

## Podgląd i sprawdzanie

Otwórz **`scenes/narrative/preview.tscn` → F6**. Na górze wybierz NPC, zadanie i stan, potem „Zastosuj stan i rozpocznij rozmowę”. Zmiana stanu resetuje wyłącznie sesję podglądu. Przy zadaniu pobocznym podgląd może oznaczyć wymagane wcześniejsze zadania jako ukończone. `ready` dla zadania bez oddania oznacza `completed`.

W Inspectorze korzenia podglądu ustawisz **Initial Credits**, **Initial Medicine**, **Campaign Seconds** i **Initial Facts**. Zero sekund oznacza nieaktywną kampanię. Pozwala to sprawdzić brak pieniędzy, brak dawki, obietnicę i stan końcowy rozmowy bez edytowania zapisu gracza. Podgląd nie sprawdza fizycznego dojścia do NPC ani pełnego przebiegu kursu — do tego służy gra i testy integracji.

Z katalogu projektu:

```bash
bash tools/verify.sh narrative-validate
bash tools/verify.sh narrative
bash tools/verify.sh narrative-render
```

Pierwsza komenda sprawdza dane, druga transakcje, zapis oraz interakcję ze światem, trzecia uruchamia renderer i zapisuje obrazy w `build/narrative-*.png`. Można też uruchomić `tools/validate_narrative.gd` z Godota i argumentem użytkownika `--catalog=res://resources/narrative/inny_catalog.tres`.

### Ostrzeżenia skryptów a błędy treści

Żółte wpisy `GDScript::reload`, np. `shadowing`, `Integer division` lub `Integer used when an enum value is expected`, dotyczą kodu `.gd`. Rozwiń wpis, aby zobaczyć plik i linię. Samo dodanie poprawnego zasobu questa nie oznacza, że to on spowodował takie ostrzeżenie. Błędy danych z **Validate catalog** wskazują kontekst, np. `Quest depot_driver/return_trips` albo dialog i węzeł.

Wynik „0 błędów” potwierdza poprawność struktury i sprawdzanych odwołań. Nie gwarantuje kompletności rozgrywki: przy **Auto Start = Off** nadal potrzebny jest dostępny temat NPC z efektem `start_quest`, a przy **Requires Turn In = On** także odpowiedź z efektem `turn_in`. Po poprawkach zatrzymaj grę i uruchom ponownie; stare wpisy debuggera mogą wymagać wyczyszczenia.

## ID, zapis i zmiany treści

- Używaj stabilnych nazw, np. `depot_driver`, `return_trips`. ID questa/dialogu/NPC/przedmiotu jest unikalne we własnej liście katalogu, ID celu w queście, węzła w dialogu, odpowiedzi w węźle. Nie używaj `/`, początkowego `__`, pustych nazw ani spacji na końcach.
- Współdzielone zasoby w Godocie pozostają współdzielone. Zmiana dialogu użytego przez dwóch NPC zmienia obie rozmowy. Przy kopii przeznaczonej na nową treść twórz nowe zasoby albo użyj **Make Unique** także dla edytowanych zasobów zagnieżdżonych. Zmień ID i popraw odwołania; samo skopiowanie pliku nie tworzy nowych identyfikatorów.
- Definicje nie przechowują postępu. Sesja zapisuje statusy, bieżący cel i liczniki pod ID, przedmioty, uprawnienia, fakty, zegar, dawki oraz potwierdzenia wyborów i zdarzeń. Po zatwierdzonym efekcie następuje zapis także podczas pauzy rozmowy.
- Schemat **5** zachowuje dane poprzednich schematów **1–4** i rozpoczyna pusty stan narracji dla starych zapisów. Dialog jako otwarte okno nie jest odtwarzany; jego zatwierdzone skutki są zachowane.
- Zmiana tytułu lub tekstu nie wymaga resetu. Usunięcie questa, zmiana ID/celów/kolejności/liczników lub ich znaczenia wymaga zaplanowanej migracji stanu albo nowej sesji. Pole **Version** ujawnia niezgodność; samo zwiększenie numeru nie wykonuje migracji.
- Zapis o niezgodnej wersji lub nieznanych wymaganych danych jest odrzucany w całości. Ekran startowy informuje o błędzie i nie nadpisuje starego pliku świeżą sesją. „Rozpocznij nową grę” jest świadomym rozpoczęciem od nowa.

## Zakres i weryfikacja lokalna

Ostrzeżenia debuggera i pole Required, 2026-09-14, macOS / Godot 4.7.2: odtworzono i usunięto **14 ostrzeżeń** w dziewięciu skryptach gry. Kontrola odizolowanej kopii z włączonymi ostrzeżeniami podniesionymi do błędów: **78/78 skryptów gry**, bez błędów skryptów (`build/warnings-before.log`, `build/warnings-after.log`; końcowa kontrola pola: `build/required-warnings-after.log`). Błąd kroku Inspectora odtworzono na kontrolce Godota: wpisane `2` dawało `2.001`; poprawiony zakres zachowuje `2` i wartości ułamkowe. Test regresji korzysta z metadanych rzeczywistego zasobu i sprawdza zakończenie celu po dokładnie dwóch kursach. Testy: **318/318** — `narrative` 69/69, `narrative-render` 25/25, `living-world` 82/82, `taxi-guidance` 48/48, `architecture` 41/41, `vehicles` 45/45, `boot` 8/8. Katalog autora: 0 błędów, 3 questy / 5 dialogów / 2 NPC; zasoby treści pozostały niezmienione. Render rozmowy sprawdzono wizualnie. Logi headless zawierają komunikat macOS o certyfikatach, a próba celowo uszkodzonego zapisu — oczekiwany błąd JSON. Bez eksportu i pakowania.

Stałe położenie rozmowy, 2026-09-14, macOS / Godot 4.7.2: **`narrative-render` 25/25**. W kolejnych renderowanych klatkach sprawdzono niezmienne granice panelu i początek tekstu przy przejściach między 1, 2, 3 i 11 odpowiedziami oraz krótkim i długim tekstem. Wskazówka przewijania nie przesuwa nagłówka; powrót z przewiniętej wypowiedzi do krótkiej zeruje przewinięcie i usuwa niepotrzebny pasek. Zachowano próby trzech rozmiarów okna, ustawienia wysokości w Inspectorze podczas pauzy, gestów dotykowych i podglądu F6. Log: `build/narrative-fixed-position-render.log`, bez błędów skryptów. Bez eksportu i pakowania.

Wcześniejsze doprecyzowanie wysokości pola NPC, 2026-09-14, macOS / Godot 4.7.2: **`narrative-render` 20/20**. Pole NPC ma 172 px, tyle co trzy standardowe odpowiedzi z odstępami. Sprawdzono zmianę na 224 px przez parametr poziomu podczas pauzy rozmowy, powrót do 172 px, dopasowanie panelu w 540×960, 360×640 i 960×540, niezależne przewijanie i obsługę dotyku oraz podgląd F6. Obraz domyślnej rozmowy sprawdzono wizualnie. Log: `build/npc-text-height-render.log`. Bez eksportu i pakowania.

Wcześniejsza aktualizacja kompaktowego panelu, przed doprecyzowaniem wysokości pola NPC, 2026-09-14, macOS / Godot 4.7.2: **83/83** — `narrative` 65/65, `narrative-render` 18/18. Sprawdzono wtedy 1/3 wysokości całego panelu w 540×960, 360×640 i 960×540, trzy pełne pola krótkich odpowiedzi, niezależne przewijanie NPC i odpowiedzi, kierunek wskazówki, natywne tapnięcie odpowiedzi i brak wyboru po przeciągnięciu. Import bez cache: **PASS, 0 błędów**, `build/clean-import-orkpycjl/output.log`. Testy dotyku wstrzykują zdarzenia ekranowe w rzeczywistym rendererze; fizyczny telefon i Windows wymagają osobnej oceny. Bez eksportu i pakowania.

Poniżej zakres i wyniki wcześniejszego wdrożenia całego systemu:

Zaimplementowano mechanikę oraz dwa przykłady; pełna kampania, finalne teksty, portrety, wnętrza, gating dzielnic i walka pozostają dalszą pracą. Pierwowzory i decyzje projektowe opisano w [porównaniu Unreal i starego Godota](DIALOGUE_QUEST_ANALYSIS_2026-09-13.md).

Weryfikacja odbywa się na lokalnym Godocie 4.7.2/macOS; testy i zasoby nie potrzebują internetu. Ograniczenie sieci nie zablokowało analizy obu projektów ani importu, walidacji i testów. Wyniki nowych testów Windows i fizycznego telefonu nie są tu deklarowane; brak tych prób nie wynika z dostępu do internetu. Nie wykonywano eksportu ani paczki itch.io.

Wyniki 2026-09-14: **404/404 sprawdzenia**. `narrative` 65/65 (w tym rzeczywisty zapis podczas pauzy, ponowny start po końcu czasu i ochrona nieprawidłowego zapisu), `narrative-render` 9/9, `architecture` 41/41, `living-world` 82/82, `on-foot` 46/46, `taxi` 39/39, `taxi-guidance` 48/48, `vehicles` 45/45, `taxi-ui` 21/21, `boot` 8/8. Walidator: 0 błędów, 2 questy, 5 dialogów, 2 NPC. Import odizolowanej kopii bez `.godot`: PASS, 0 błędów (`build/clean-import-ruxp59p5/output.log`).

Obrazy sprawdzono dla 540×960, 360×640 z większą czcionką i dziesięcioma odpowiedziami oraz 960×540 z limitem szerokości 620. Pliki: `build/narrative-dialogue.png`, `build/narrative-journal.png`, `build/narrative-long-360.png`, `build/narrative-wide.png`, `build/narrative-author-preview.png`. Zrzuty są lokalnymi wynikami testów, nie zasobami gry. W headless sandbox macOS zgłaszał odczyt certyfikatów; testy kontynuowały działanie. Test celowo uszkodzonego zapisu zgłasza błąd parsera JSON, po czym sprawdza zachowanie pliku i ekranu odzyskiwania.
