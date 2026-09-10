# Audyt konfiguracji questów i dialogów NPC

Data: 2026-09-09. Projekt: FlyingCabFlightLab, UE 5.8. Punkt odniesienia: `main`, HEAD `7a18a5d` oraz zastany working tree.

**Wniosek: obecny rdzeń questów warto zachować. Największy zysk da uproszczenie konfiguracji w edytorze, przeniesienie konfiguracji NPC z C++ do danych i dodanie osobnej warstwy rozmów. Działającego systemu dialogowego w aktywnym projekcie Unreal jeszcze nie ma.**

Audyt obejmuje źródła questów, interakcji, bootstrap świata, prezentację, konfigurację projektu, testy i dokumentację. To analiza kodu i dostępnych raportów, bez nowego uruchomienia Unreal/PIE ani oględzin paneli Details. Ocena obecnego interfejsu edytora wynika z deklaracji właściwości i braku własnych rozszerzeń edytora. Nie odczytywano kompletnych wartości wszystkich binarnych assetów w edytorze. Audyt dodaje wyłącznie ten dokument; zastane zmiany gry pozostają osobną pracą.

Ścieżki i numery linii poniżej są względne wobec `unreal/FlyingCabFlightLab`, według stanu w chwili audytu.

## 1. Co można skonfigurować dzisiaj

| Potrzeba autora | Obecna możliwość | Ograniczenie |
|---|---|---|
| Tytuł, opis, kategoria i nagrody | Data Asset `FlyingCabQuestDefinition` | Wygodna podstawa; prawa dostępu wpisuje się jako identyfikatory. |
| „Dowieź dwóch pasażerów”, „zarób 1000 kredytów” | Cel z `EventId`, `TargetId`, `RequiredCount` | Autor musi znać techniczne nazwy i semantykę zdarzeń. |
| Kilka kroków zadania | Tablica `Objectives` | Wyłącznie sekwencja; wcześniejsze zdarzenia nie są pamiętane dla przyszłych celów. |
| Nowy quest na własnym aktorze zleceniodawcy | Przypisanie assetu w instancji aktora + wpis w katalogu | Samo przypisanie assetu NPC nie rejestruje go w subsystemie. |
| Zmiana zadania, położenia i nazwy Mike'a/Jacka | Tabela `FlyingCabQuestHubData.cpp` | Konfiguracja obecnych NPC i ich markerów wymaga C++. |
| Rozmowa niezwiązana z zadaniem | Brak | Questgiver bez `QuestDefinition` odrzuca interakcję. |
| Kilka zadań i tematów u jednego NPC | Brak gotowej obsługi | Jeden questgiver ma jeden `QuestDefinition`. |
| Tekst NPC, odpowiedzi gracza, warunki odpowiedzi | Brak | Trzy pola `*DialogueId` nie mają odbiorcy ani edytora treści. |
| Własna mechanika w Blueprint | `QuestEventComponent` i publiczne API subsystemu | Komponent trzeba wywołać; nie zapewnia sam interakcji Q z dowolnym Blueprintem. |
| Zapamiętanie postępu po zamknięciu gry | Brak | Stan jest sesyjny; same specyfikatory `SaveGame` nie zapisują go na dysk. |

Obecny workflow prostego questa obejmuje utworzenie assetu, wpisanie identyfikatorów i zdarzeń, dodanie do katalogu oraz przypisanie NPC. Przy istniejących Mike'u i Jacku dochodzi powiązanie zapisane w kodzie. To system konfigurowalny danymi, ale jeszcze wymagający znajomości implementacji.

## 2. Fundamenty, które należy zachować

- **Definicja jest oddzielona od postępu.** `UFlyingCabQuestDefinition` opisuje treść, a `UFlyingCabQuestSubsystem` posiada status i liczniki. UI czyta projekcje dziennika i zdarzenia.
- **Questy reagują na zdarzenia.** Dla obecnych kontraktów nie potrzebują własnego Tick ani rozbudowanego grafu wykonawczego.
- **Logika nie identyfikuje zadań po tekstach.** Istnieją stabilne `QuestId`, `ObjectiveId`, `EventId`, `TargetId`, a tytuły/opisy korzystają z `FText`.
- **Nagrody mają swoich właścicieli.** GameMode przekazuje je ekonomii i progresji; widget nie powinien rozdawać kredytów.
- **Są zabezpieczenia i testy.** Bramka Free Roam chroni start, zdarzenia, oddanie i przyznanie nagród. Katalog pomija pojedyncze wadliwe definicje.

Wcześniejsze problemy z audytu z 18 sierpnia częściowo już naprawiono: `TurnInQuest` ma bramkę trybu, są zdarzenia zarobków i filtry dzielnic, wadliwy quest nie odrzuca całego poprawnego katalogu, a loader katalogu nie utrwala fallbacku przez procesowy statyk. Nie należy zgłaszać ich ponownie jako obecnych błędów.

## 3. Najważniejsze ustalenia

Priorytet **A** oznacza przeszkodę dla docelowego samodzielnego authoringu lub rozmów; **B** — poprawkę potrzebną przy rozszerzaniu treści; **C** — dalsze udogodnienie. Brak funkcji jest odróżniony od błędu obecnej implementacji.

### A1. Obecni NPC są definiowani poza edytorem treści

**Stan:** `FlyingCabQuestHubData.cpp:7` przechowuje Mike'a, Jacka, nazwy, położenia, oznaczenia minimapy i identyfikatory questów. Bootstrap tworzy zawsze natywną klasę `AFlyingCabQuestGiver` (`FlyingCabWorldBootstrap.cpp:202`). Minimapę zasila ta sama tabela (`FlyingCabTouchControls.cpp:540`). Sam actor ma edytowalne pola, lecz `EditInstanceOnly`, bez profilu NPC i bez odświeżania wyglądu przez `OnConstruction`/`PostEditChangeProperty`.

**Skutek:** zmiana questa Mike'a na nowy identyfikator, dodanie trzeciego NPC do tego mechanizmu lub przesunięcie go razem z markerem wymaga kodu. Dopisany ręcznie questgiver nie trafia automatycznie do listy markerów. Błędny/brakujący quest huba powoduje wcześniejszy powrót z bootstrapu (`FlyingCabWorldBootstrap.cpp:216`), zamiast pozostawić NPC zdolnego do zwykłej rozmowy.

**Rekomendacja:** profil NPC jako Data Asset, przypisywany do aktora na mapie. Położenie powinno pochodzić z aktora; marker minimapy z jego rejestracji. Mike'a i Jacka przenieść do tego samego workflow, którego później używa autor. Jeśli spawnowanie musi pozostać proceduralne, źródłem ma być edytowalna lista punktów spawnu z profilem, a nie druga tabela nazw i współrzędnych w C++.

### A2. Pola dialogowe niczego jeszcze nie uruchamiają

**Stan:** `OfferDialogueId`, `ActiveDialogueId` i `CompletionDialogueId` są tylko deklaracjami w `FlyingCabQuestDefinition.h:56`. W źródłach nie ma ich wykonawcy, definicji rozmowy, odpowiedzi, sesji dialogu ani widgetu dialogowego. `QuestGiver::Interact` przyjmuje quest bezpośrednio, oddaje gotowy albo włącza śledzenie (`FlyingCabQuestGiver.cpp:95`). Po ukończeniu prompt mówi `Q // TALK`, lecz wynik to komunikat o ukończonym zadaniu.

**Skutek:** wpisanie tekstu lub identyfikatora w obecne pola dialogowe nie stworzy rozmowy. Nie ma też „Nie teraz”, rozmowy po oddaniu zadania ani NPC bez questa.

**Rekomendacja:** oddzielny asset rozmowy oraz sesja, która interpretuje węzły i odpowiedzi. Q otwiera rozmowę, a jawna odpowiedź uruchamia `StartQuest` albo `TurnInQuest`. Profil NPC wybiera dostępne tematy zgodnie ze stanem gry. Obecne niewykorzystywane pola ukryć/oznaczyć jako nieaktywne do czasu migracji; docelowe powiązania wybierać pickerem assetów.

### A3. Ręczne identyfikatory i zbyt ogólny formularz celu

**Stan:** każdy cel wymaga `ObjectiveId`, opisu, `EventId`, opcjonalnego `TargetId` i liczby (`FlyingCabQuestTypes.h:44`). Brak selektorów zdarzeń/celów, szablonów celów i etykiet `TitleProperty` dla tablicy. Walidacja identyfikatorów zdarzeń jest dopiero w katalogu (`FlyingCabQuestCatalog.cpp:182`), a sam quest sprawdza głównie niepuste wartości (`FlyingCabQuestDefinition.cpp:39`). Kontrola `TargetId` dotyczy dzielnic w zdarzeniach pasażerów, nie dowolnych NPC, pojazdów i obiektów.

**Skutek:** autor może zapisać strukturalnie poprawny quest z literówką i zobaczyć odrzucenie dopiero przy walidacji katalogu/startowaniu gry. Duplikacja assetu kopiuje `QuestId`; duplikacja obiektów kopiuje ich domyślne identyfikatory.

**Rekomendacja:** czytelne presety: „Dowieź pasażera”, „Zarób kredyty”, „Kup paliwo”, „Użyj obiektu”, później „Osiągnij punkt rozmowy”. Po wyborze presetu panel pokazuje tylko właściwe pola. Pod spodem nadal zapisuje istniejący model zdarzenia. Zaawansowany „Własne zdarzenie” zachowuje rozszerzalność. ID nadawać przy tworzeniu/duplikacji i pozostawiać stabilne przy zmianie nazwy; nie przeliczać ich przy każdym zapisie.

**Pułapki semantyczne do pokazania w panelu:**

- `Economy.CreditsEarned` to dodatni przychód od aktywacji danego celu, również z nagród questowych; nie aktualne saldo i nie wyłącznie kursy (`FlyingCabGameMode.cpp:451,605`). Nie zmieniać zaakceptowanej semantyki „Get 1000 credits” na saldo.
- Przy `Passenger.PickedUp` filtrem jest dzielnica **docelowa pasażera**, nie miejsce odbioru (`FlyingCabGameMode.cpp:340`).
- `RequiredCount` dla paliwa/naprawy liczy kupione jednostki, nie liczbę wizyt.
- `Progression.AccessGranted` emituje terminal także przy potwierdzeniu istniejącego dostępu (`FlyingCabAccessTerminal.cpp:102`). Sama nagroda questowa wywołuje `GrantAccess`, ale nie emituje tego zdarzenia (`FlyingCabGameMode.cpp:611`). „Posiada dostęp” powinno być osobnym warunkiem stanu, nie domysłem na podstawie eventu.
- Sekwencja „dowieź → zarób” nie policzy zapłaty za kurs, który właśnie odblokował drugi cel: wypłata następuje przed `Passenger.Delivered` (`FlyingCabGameMode.cpp:366`). To obecna semantyka kolejnych kroków, którą autor musi widzieć.

### A4. Łańcuch może ominąć walidację; NPC nie podąża za łańcuchem

**Błąd potwierdzony w kodzie:** `CompleteQuest` ładuje `NextQuest`, dopisuje go do `Definitions` i uruchamia bez walidacji katalogu (`FlyingCabQuestSubsystem.cpp:392`). Walidacja definicji blokuje wyłącznie bezpośrednie wskazanie na siebie (`FlyingCabQuestDefinition.cpp:75`); katalog nie sprawdza całego łańcucha.

**Scenariusz wynikający z kodu, do testu regresji:** poprawny A wskazuje B bez celów. B może być pominięty przy konfiguracji katalogu, a po ukończeniu A zostać ponownie dodany i wystartować. `RecordEvent` pomija go z powodu braku prawidłowego indeksu celu, więc B pozostaje aktywny bez możliwości postępu.

**Osobne ograniczenie:** giver przechowuje jeden asset i cały czas pyta o jego status. Jeżeli A uruchomi B wymagający oddania, pierwotny NPC nadal obsługuje A. Nie istnieje też pole wskazujące odbiorcę oddania; drugi questgiver przypisany do tego samego zadania może je oddać. Łańcuch nie oznacza automatycznie obsługi serii zadań u tego NPC.

**Rekomendacja:** katalog pozostaje jedyną drogą rejestracji, a walidacja sprawdza docelowe assety, ich ID, członkostwo oraz cykle. Rozdzielić „po ukończeniu wystartuj następny quest” od „udostępnij następny temat u NPC”. Wprowadzić wybieralne powiązanie odbiorcy oddania i sprawdzać je w komendzie dialogu. Publiczne wywołania administracyjne/testowe mogą pozostać osobną ścieżką.

### A5. Zdarzenie interakcji nie oznacza ukończonej rozmowy

**Stan:** giver emituje `QuestGiver.Interacted` przed sprawdzeniem/zmianą statusu (`FlyingCabQuestGiver.cpp:109`). Po udanej interakcji kontroler może dodatkowo wyemitować `Interaction.Completed` (`FlyingCabPlayerController.cpp:872`). To różne zdarzenia, nie automatycznie podwójne zaliczenie tego samego eventu.

**Skutek:** pierwszy cel `QuestGiver.Interacted` nowo przyjmowanego zadania wymaga następnego Q, bo event wysłano przed startem questa. Cel oparty o `Interaction.Completed` może natomiast zaliczyć się już po tym samym przyjęciu. Po dołożeniu dialogu samo otwarcie okna mogłoby niechcący zaliczać „przekonaj Jacka”.

**Rekomendacja:** rozróżnić otwarcie interakcji, zakończenie rozmowy i osiągnięcie konkretnego punktu rozmowy. Quest typu „przekonaj Jacka” otrzymuje event dopiero z zaakceptowanej odpowiedzi/węzła. Można zachować obecny runtime: nowe `Dialogue.MilestoneReached` + stabilny ID punktu jako `TargetId`, dobierany przez edytor. Otwarcie i anulowanie dialogu nie wykonują akcji akceptacji ani oddania.

### B1. Blueprint ma API questów, lecz nie gotową ścieżkę własnego NPC

`FlyingCabInteractable.h:23` zawiera zwykłe wirtualne metody C++, bez `BlueprintNativeEvent`. Kontroler wywołuje natywny interfejs przez cast (`FlyingCabPlayerController.cpp:862`). Dlatego oznaczenie givera jako `Blueprintable` nie daje eventu „gdy rozpoczęto rozmowę” do nadpisania w Blueprint. Sam `QuestEventComponent` jedynie emituje event po wywołaniu; nie rejestruje dowolnego właściciela jako interaktywnego NPC.

Najmniejsza zmiana: natywny actor/adapter NPC implementuje istniejący interfejs i deleguje rozmowę do komponentu/sesji. Dla deva udostępnia zdarzenia prezentacji w Blueprint. To pozwala rozszerzać NPC bez przebudowy interakcji wszystkich drzwi, terminali i pojazdów.

### B2. Postęp jest indeksowy, a nie zapisany według `ObjectiveId`

Mimo komentarza przy `ObjectiveId`, runtime ma `ActiveObjectiveIndex` i `TArray<int32> ObjectiveProgress` (`FlyingCabQuestTypes.h:44,88`). `ConfigureCatalog` zachowuje stan rozpoznanych questów bez migracji zmienionej listy celów (`FlyingCabQuestSubsystem.cpp:53`). Zmiana kolejności/konstrukcji celów przy zachowanym stanie może przypisać postęp innemu krokowi.

Przed save'em i wspieraniem przeładowania treści podczas testów: postęp po stabilnym `ObjectiveId`, wersja schematu i jawna polityka migracji/resetu. Dla obecnego MVP wystarczy jawnie wymagać nowej sesji testowej po zmianie struktury. Zapis questów, decyzji fabularnych, ekonomii i dostępu trzeba zaprojektować spójnie; obecnie ekonomia resetuje się w `StartRun`, podczas gdy questy żyją w GameInstance (`FlyingCabGameMode.cpp:161`). Brak pełnego save'a jest znanym ograniczeniem zakresu, nie nową regresją.

### B3. „Jednorazowy obiekt” ma ukrytą zależność od aktywnych questów

`QuestEventComponent` ustawia `bHasEmitted` dopiero gdy jakikolwiek aktywny quest zużyje event (`FlyingCabQuestEventComponent.cpp:23`). Ta flaga należy do komponentu w świecie, nie do pojedynczego questa. Jeżeli A wykorzysta jednorazowy terminal, uruchomiony później B może nie móc użyć go ponownie w tej samej instancji aktora. `ResetAllQuests` nie resetuje tych komponentów; przeładowanie mapy odtwarza je niezależnie od zachowanego postępu questów.

Nazwać zakres blokady wprost: „w tej instancji obiektu”, „na quest”, „na sesję” — wdrażać tylko potrzebne warianty. Przy testowaniu pojedynczego questa reset powinien obejmować jego kontekst albo używać izolowanej sesji. Nie przenosić obecnego `bEmitOnce` jako mechanizmu pamięci rozmów.

### B4. Zastąpienie UI własnym Widget Blueprint wymaga dopięcia kontraktu

Dokumentacja sugeruje skórkę UMG, ale kontroler tworzy bezpośrednio natywny `UFlyingCabQuestJournalWidget` (`FlyingCabPlayerController.cpp:1140`). Metody obsługi widoku nie są wystawione do Blueprint, a pola widgetów są prywatne, bez `BindWidget` (`FlyingCabQuestJournalWidget.h:41,91`). Samo utworzenie klasy potomnej nie podmieni działającego dziennika.

Dla nowego dialogu od początku: wybieralna klasa widgetu, model widoku z mówcą/tekstem/odpowiedziami, event odświeżenia i jedna komenda wyboru odpowiedzi. Warunki i skutki pozostają w sesji dialogu. Pozwala to zmieniać layout, animacje i wygląd bez kopiowania logiki.

### C1. Diagnostyka i tracking utrudniają iterację

Brakuje panelu „dlaczego ten quest/dialog nie jest dostępny”, podglądu eventów oraz resetu pojedynczego questa. `StartQuest` zawsze przejmuje tracker (`FlyingCabQuestSubsystem.cpp:119`), autoquesty startują w kolejności iteracji mapy (`:91`), a ukończenie bez następcy wybiera ponownie najwcześniej aktywowany quest (`:403`). Może to nadpisywać świadomy wybór gracza.

Dodać czytelne powody odmowy komend zamiast samego `bool`, panel bieżącego celu i ostatnich eventów, politykę śledzenia oraz deterministyczną kolejność autostartu. To udogodnienia po domknięciu podstaw authoringu.

## 4. Proponowany workflow w Unreal

**Cel użytkowy: autor sam tworzy zadanie i rozmowę, przypisuje je Mike'owi, sprawdza warianty i uruchamia grę bez zmiany C++ ani budowania logiki każdego questa w Blueprint.**

### Formularz questa

| Pole widoczne dla autora | Przykład |
|---|---|
| Nazwa / opis / kategoria | „Nocna zmiana”, opis, Main |
| Sposób rozpoczęcia | Rozmowa z NPC |
| Warunki dostępności | First Shift — ukończony |
| Cele w kolejności | „Dowieź pasażerów”, dowolny cel podróży, liczba 2 |
| Sposób zakończenia | Oddanie u Mike'a |
| Nagroda | 200 kredytów |
| Po zakończeniu | Brak / automatycznie rozpocznij kolejny quest |

ID i surowe zdarzenia w sekcji zaawansowanej. Dodanie celu z presetów automatycznie tworzy jego stabilny identyfikator. Wiersz zwinięty pokazuje np. „Dowieź pasażerów ×2 → dowolna dzielnica”, nie sam indeks tablicy. Tooltips wyjaśniają czas naliczania i jednostkę licznika.

Na początek zachować ręczny katalog jako jawny zbiór publikowanej treści, ale dodać akcję „Dodaj do katalogu” w kreatorze/Details oraz walidację brakujących powiązań. Nie trzeba od razu automatycznie uruchamiać wszystkich assetów znalezionych w Content Browser — szkice powinny pozostać szkicami. Referencję katalogu przenieść do ustawień projektu zamiast utrzymywać wyłącznie stałą ścieżkę C++.

Technicznie wystarczą metadane właściwości i osobny moduł edytorowy z dostosowaniem Details dla questa i struktury celu. UE udostępnia do tego `IDetailCustomization` i `IPropertyTypeCustomization`; dodatkowe kontrolki mogą obejmować tylko te pola, które wymagają usprawnienia. [Epic: Details Panel Customizations](https://dev.epicgames.com/documentation/en-us/unreal-engine/details-panel-customizations-in-unreal-engine).

### Profil NPC i rozmowa

| Element | Minimalna zawartość |
|---|---|
| Profil NPC | Stabilne ID, nazwa `FText`, opcjonalny portret, marker, lista tematów. |
| Temat | Tytuł, priorytet, warunki, asset rozmowy, opcjonalny quest kontekstowy. |
| Rozmowa | Węzeł startowy i lista węzłów o stabilnych ID. |
| Węzeł | Mówca, tekst `FText`, kontynuacja lub odpowiedzi; opcjonalne audio później. |
| Odpowiedź | Tekst, warunki widoczności/dostępności, akcje, następny węzeł albo zakończenie. |
| Sesja rozmowy | Aktualny NPC i węzeł, dostępne odpowiedzi, wykonanie komend, zamknięcie. |

Początkowo wystarczą karty węzłów/odpowiedzi z wyszukiwanym wyborem przejścia, automatycznym ID oraz **podglądem rozmowy w edytorze**. Sama surowa tablica wielokrotnie zagnieżdżonych struktur przeniosłaby obecny problem questów do dialogów. Przy większej liczbie rozgałęzień warto dołożyć graf rozmowy pokazujący te same dane; questom pozostawić prostą listę celów. Pełny edytor grafowy jest dodatkowym zakresem prac.

Szablon „Zleceniodawca” powinien oferować cztery gotowe sytuacje: propozycja, przypomnienie, oddanie i rozmowa po ukończeniu. Ten szablon może używać `ContextQuest`, aby autor wybierał quest raz w temacie, zamiast powtarzać jego ID w kilku akcjach.

### Warunki i akcje wygodne dla deva

Pierwszy zestaw warunków: status questa, ukończony konkretny cel po `ObjectiveId`, posiadany dostęp i fakt fabularny. Pierwsze akcje: rozpocznij quest, oddaj quest, zapisz fakt, wyemituj punkt rozmowy. Warunki mają operatory „wszystkie/dowolny” oraz jawne zaprzeczenie; nie wymagają pisania wyrażeń tekstowych. Przykład: „Nightshift Contract jest gotowy do oddania”.

Warunki dostępności questa powinny być sprawdzane w jego API, a dialog korzystać z tego samego zapytania. Samo ukrycie odpowiedzi w UI nie blokuje startu z innej ścieżki. Komenda wyboru ponownie sprawdza warunki i zwraca wynik z powodem odmowy. Po błędzie nie przechodzi do tekstu potwierdzającego sukces. Wielokrotne kliknięcie jednej odpowiedzi nie może ponawiać nagrody. Rozliczenie nagrody za quest pozostaje w istniejącym mechanizmie jego ukończenia.

Dalsze typy warunków/akcji dev może dopisywać przez rozszerzalne klasy z obsługą Blueprint. Zwykły autor wybiera gotowy typ i uzupełnia pola. Stan rozmowy nie powinien kopiować statusów questów. Trwałe fakty fabularne wymagają własnego właściciela stanu sesji/save'a; widget przechowuje tylko prezentację.

**Identyfikatory:** w pierwszym etapie można pozostawić istniejące `FName` i dodać pickery. Gameplay Tags mają sens jako wspólny słownik zdarzeń i faktów, szczególnie przy warunkach dialogowych; UE zapewnia ich słownik, hierarchię i query. Nie zastępują referencji do assetów ani nie rozwiązują same walidacji powiązań. Migracja istniejących ID powinna zachować wartości i jawną semantykę dokładnego dopasowania eventów. [Epic: Gameplay Tags](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-gameplay-tags-in-unreal-engine).

### Przykład zachowania Mike'a

Poniższa treść to demonstracja docelowego workflow, nie zmiana istniejącego scenariusza:

| Stan | Mike | Odpowiedź gracza i efekt |
|---|---|---|
| Oferta dostępna | „Potrzebuję kierowcy na dwa kursy.” | „Biorę zlecenie” → `StartQuest`; „Nie teraz” → koniec bez przyjęcia. |
| Quest aktywny | „Wróć po wykonaniu dwóch kursów.” | Pytanie o szczegóły lub zakończenie rozmowy. |
| Gotowy do oddania | „Masz za sobą oba kursy?” | „Tak, rozliczmy je” → `TurnInQuest`; po sukcesie tekst potwierdzenia. |
| Ukończony | „Dobra robota.” | Zwykła rozmowa lub kolejny odblokowany temat. |

Nowy temat może odblokować quest Jacka, ale sam start następuje dopiero po wyborze odpowiedzi, jeżeli tak skonfigurował autor. NPC pozostaje dostępny także bez aktywnego zadania.

## 5. Walidacja i testowanie treści

Walidacja powinna wskazywać **asset → cel/węzeł → konkretne pole**, zebrać wszystkie znalezione błędy i umożliwić przejście do miejsca edycji. Obecne funkcje kończą sprawdzanie na pierwszym błędzie.

Zakres kontroli:

- unikalne ID questów, NPC, celów i węzłów; brakujące referencje i nieopublikowane zależności;
- poprawność `NextQuest` i warunków dostępności, zakazane cykle automatycznego uruchamiania;
- odbiorca oddania istnieje w wskazanym kontekście świata i obsługuje właściwy quest;
- znany typ eventu, właściwy typ celu/targetu i zgodna jednostka licznika;
- istniejące przejścia rozmowy, osiągalny start, poprawne warunki/akcje;
- brak automatycznej pętli węzłów bez wejścia użytkownika; ostrzeżenie dla sytuacji bez dostępnej odpowiedzi i bez kontynuacji/zakończenia;
- jawna informacja, czy działa katalog autora, pominięto część danych, czy uruchomiono demonstracyjny fallback.

Wystarczy rozszerzyć obecne `IsDataValid` i dodać kontrole relacji między assetami oraz aktorami. UE umożliwia walidację assetu, zależności, folderu i projektu, również z commandletu; nie ma potrzeby budowania osobnego narzędzia walidacyjnego od zera. [Epic: Data Validation](https://dev.epicgames.com/documentation/en-us/unreal-engine/data-validation-in-unreal-engine).

Podgląd dialogu powinien pozwalać ustawić pozorny stan „nieprzyjęty / aktywny / gotowy / ukończony”, wybrać odpowiedź i zobaczyć planowany skutek. Nie powinien wykonywać komend na aktualnej sesji gracza. Do diagnostyki przydaje się informacja „ta odpowiedź jest ukryta, ponieważ First Shift nie jest ukończony”.

## 6. Kolejność wdrożenia i kryteria odbioru

| Etap | Zakres | Wielkość względna | Dowód ukończenia |
|---|---|---|---|
| 1. Prosty authoring | Presety/pickery, czytelne cele, ID, dodawanie do katalogu, walidacja łańcuchów | Średnia | Autor tworzy nowy quest i uruchamia go bez wpisywania natywnych EventId; błędny następca jest wykrywany przed grą. |
| 2. NPC z danych | Profile NPC, konfiguracja Mike'a/Jacka w edytorze, jedno źródło położenia i markerów | Średnia | Autor zmienia zadanie Mike'a i dodaje NPC z markerem bez edycji C++. |
| 3. Pierwszy dialog | Asset, warunki/akcje, sesja, widget, podgląd i szablon zleceniodawcy | Duża | Działa przyjęcie, odmowa, przypomnienie, oddanie i rozmowa po ukończeniu; nagroda dokładnie raz. |
| 4. Produkcja treści | Diagnostyka, bezpieczny reset/test kontekstu, stabilny zapis postępu i faktów | Osobny zakres | Test autora oraz zapis/odczyt z ustaloną polityką wersjonowania. |

Wielkości są oceną zakresu, nie estymacją czasu. Graf dialogowy, questy równoległe/alternatywne, powtarzalność, voice-over i narzędzia masowej edycji należy dodawać wtedy, gdy wymagają ich konkretne scenariusze. Sama potrzeba rozmów z obecnymi NPC nie wymaga tych wszystkich funkcji naraz.

**Warunek integracji UI:** otwieranie/zamykanie rozmowy musi respektować kanoniczne sterowanie, odroczone Q/J/R, focus, flush i strażnik przejścia. Dziennik pokazuje istniejący wzorzec, ale jego obsługa jest zakodowana dla jednego modalu. Trzeba jawnie rozstrzygnąć pauzę podczas rozmowy, konflikt z dziennikiem i zakończenie sesji po utracie NPC/zmianie mapy. Integracja nowego modalu wymaga osobno uzgodnionego zakresu zmian wejścia zgodnie z `AGENTS.md` oraz `docs/INPUT_CANONICAL_BASELINE.md`; ten audyt go nie wdraża.

Minimalne scenariusze odbioru przyszłej implementacji:

1. Duplikacja questa daje nowe ID, a zmiana tytułu zachowuje istniejące ID.
2. Wadliwy B w łańcuchu A → B nie startuje; dwa zadania o tym samym ID wskazują błąd.
3. NPC bez questów prowadzi zwykłą rozmowę; jeden NPC ma dwa niezależne tematy questowe.
4. Otwarcie i anulowanie rozmowy nie przyjmuje ani nie oddaje zadania i nie zalicza punktu rozmowy.
5. Podwójne kliknięcie „Odbierz nagrodę”, ponowne Q i ponowne wejście do rozmowy nie dublują nagrody.
6. Cel „porozmawiaj/przekonaj” zalicza wybrany punkt rozmowy dokładnie raz; przerwanie wcześniej nie zalicza celu.
7. Warunek pokazuje czytelny powód niedostępności; warunki są ponownie sprawdzane przy wykonaniu odpowiedzi.
8. Edycja i uruchomienie kolejnej sesji PIE używa treści autora i pokazuje stan walidacji/fallbacku.
9. Dialog nie narusza blokady questów w Time Attack; zamknięcie po trzymaniu/puszczeniu kierunku nie pozostawia ruchu ani zablokowanego sterowania. Sprawdzić też dotyk, Esc, focus i konflikt J/Q.

## 7. Dowody i granice audytu

Odczytano istniejący raport `Saved/Automation/ThrusterFullNullRHI/index.json`, utworzony `2026.09.09-19.06.30`. Zawiera wynik **Success** dla:

- `FlyingCab.Core.DataAssets.Validation`;
- `FlyingCab.Core.Quests.CatalogResilienceAndCredits`;
- `FlyingCab.Core.Quests.EventDrivenLifecycle`;
- `FlyingCab.Functional.PIE.PassengerCourse`;
- `FlyingCab.Functional.PIE.QuestJournalInput`.

To wcześniejsze wyniki lokalne, nie testy uruchomione podczas tego audytu ani gwarancja zgodności binariów z całym aktualnym working tree. Test assetów ładuje rzeczywisty katalog i sprawdza dostępność trzech questów oraz część flag (`FlyingCabCoreTests.cpp:918`). Ostrzeżenia o wadliwych questach w tym przebiegu pochodzą z celowo niepoprawnych obiektów `/Engine/Transient` w teście odporności; nie są dowodem uszkodzenia produkcyjnego `Get_Money`.

Przejrzane testy nie pokrywają pominięcia walidacji przez `NextQuest`, authoringu kilku questów u jednego NPC, cyklu życia dialogu ani ponownego użycia obiektów po resecie questu. Nowe scenariusze należy zweryfikować testami implementacji i ręcznym przejściem workflow w edytorze. Repozytorium nie ma obecnie dedykowanego modułu edytora questów/dialogów; descriptor zawiera jeden moduł Runtime.

## Wdrożenie po audycie — 2026-09-10

Na polecenie użytkownika wdrożono lokalnie etap konfiguracji questów i rozmów. Aktualny workflow opisuje `QUEST_AUTHORING.md`.

- Moduł edytora: menu narzędzi, tworzenie assetów z szablonów, automatyczne identyfikatory, wybór istniejących zdarzeń i celów z list, dodawanie do katalogu oraz walidacja.
- Profile NPC i lista lokalizacji: Mike i Jack korzystają z zapisanych assetów w `Content/Data/Narrative`, po dwa tematy każdy. Przypisanie profilu do aktora na mapie nadpisuje pozycję z listy.
- Dialogi: wypowiedzi i odpowiedzi, przejścia, warunki statusu, akcje przyjęcia/śledzenia/oddania, normalny temat bez questa, podgląd symulujący wybory w edytorze.
- Runtime: sesja rozmowy oddzielona od UMG, kontrola aktualności wyboru, publiczne API questów jako jedyny wykonawca zmian, zamknięcie modalu przy utracie NPC lub zmianie trybu.
- Przepływ questów: warunki wstępne, opcjonalny odbiorca oddania, kontrola łańcuchów i odrzucenie niezweryfikowanych następników. Wybór nowego automatycznego zadania nie zastępuje innego ręcznie śledzonego zadania.

Zachowano istniejące typy celów i oryginalne definicje trzech questów. Nie wdrożono nowych mechanik konkretnego pasażera/pojazdu, zapisu na dysk ani graficznego edytora węzłowego. Rozmowy konfiguruje się w panelu właściwości, z listami przejść i podglądem.

Weryfikacja wykonana: kompilacja `FlyingCabFlightLabEditor Mac Development` w UE 5.8 zakończona powodzeniem; utworzenie i zapis sześciu przykładowych assetów przez Unreal zakończone kodem 0, bez błędów i ostrzeżeń skryptu; przegląd przyrostu wobec kopii zastanych plików; kontrola różnic i historycznego manifestu wejścia.

Dodano trzy testy Core (łańcuchy, przyjęcie/odmowa/oddanie i powtórzony wybór, podgląd/walidacja) oraz test PIE rozmowy z Mike’em (modal, wejście, rzeczywista nagroda, utrata NPC). **Testy nie zostały uruchomione**: wywołanie pełnego pakietu zostało odrzucone w oknie uprawnień. Dostęp do okna Unreal również nie został zatwierdzony, więc nie wykonano wizualnej oceny paneli. Kompilacja i zapis assetów nie zastępują tych sprawdzeń. Do praktycznej oceny służy końcowa próba z `QUEST_AUTHORING.md`.
