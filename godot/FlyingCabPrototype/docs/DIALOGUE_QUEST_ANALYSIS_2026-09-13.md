# Dialogi i questy — porównanie Unreal, archiwalnego Godota i nowego prototypu

Data: 2026-09-13; uzupełnienie o archiwalnego Godota: 2026-09-14. Zakres: analiza źródeł Unreal, narzędzi autora, testów, fundamentów nowego Godota oraz kodu i treści `archive/godot`. Dokument przedstawia rekomendacje do kolejnego wdrożenia; nie oznacza implementacji ani zatwierdzenia nowego scenariusza, balansu lub polityki czasu.

**Rekomendacja po porównaniu obu źródeł: zakres możliwości fabularnych wziąć ze starego Godota, organizację sesji i zabezpieczenia wyborów z Unreala, a trwały stan oprzeć na nowej sesji Godota.** Zachować rytm prezentacji Unreala, z pionową kompozycją znaną z archiwum. Stary Godot miał szerszą integrację rozmów, przedmiotów, usług i questów kampanii; ograniczenie wdrożenia do funkcji Unreala pomijałoby część już zaprojektowanej gry. Szczegółowe ustalenia archiwum i korekta zakresu znajdują się w sekcji 10.

## 1. Co rzeczywiście jest w Unrealu

| Warstwa | Zaimplementowane zachowanie | Wartość dla Godota |
| --- | --- | --- |
| Definicja NPC | Stałe ID, imię, powitanie, opcjonalny portret, wiele tematów; temat może mieć quest i własny dialog albo zwykłą rozmowę. Pozycja należy do aktora/mapy. | NPC pozostaje postacią także bez zadania; jeden rozmówca może prowadzić kilka wątków. |
| Definicja dialogu | Węzły wypowiedzi, odpowiedzi, przejścia, domyślny start i pierwsza pasująca reguła startowa. | Rozmowę konfiguruje autor, bez osobnego skryptu dla każdego NPC. |
| Warunki | Status questa, opcjonalne odwrócenie; wszystkie warunki odpowiedzi muszą być spełnione. Niedostępne odpowiedzi można ukryć albo wyłączyć z podaniem powodu. | Ta sama rozmowa obsługuje ofertę, przypomnienie, oddanie i stan po ukończeniu. |
| Akcje odpowiedzi | Kontynuacja, przyjęcie, oddanie, śledzenie zadania, powrót do tematów. | Samo otwarcie okna nie przyjmuje zadania ani nie wypłaca nagrody. |
| Sesja dialogu | Logika niezależna od widgetu; ponowne sprawdzenie warunków przy wyborze, numer rewizji widoku, ochrona przed ponownym wejściem w wybór. | Stary przycisk lub podwójny klik nie wykonuje kolejnej akcji na zmienionym stanie. |
| Questy | `Inactive → Active → ReadyToTurnIn → Completed`; zadanie bez oddania przechodzi z ostatniego celu bezpośrednio do ukończenia. Kilka questów może być aktywnych, cele wewnątrz każdego są sekwencyjne. | Wystarczający rdzeń pierwszego wycinka kampanii. |
| Powiązania | Wszystkie wymagane poprzednie questy muszą być ukończone; opcjonalny konkretny odbiorca; `NextQuest` próbuje automatycznie przyjąć następne zadanie. | Łańcuchy i dostępność treści, z korektą semantyki odblokowania opisaną niżej. |
| Nagrody | Kredyty i uprawnienia dostępu. Quest oznacza ukończenie przed powiadomieniem odbiorców nagrody. | Oddzielenie zakończenia zadania od właścicieli portfela i dostępu; zabezpieczenie przed powtórnym oddaniem w sesji. |
| Autorowanie | Tworzenie assetów, szablony, ID, listy zdarzeń i celów, dodawanie do katalogu, walidacja, podgląd z symulowanym stanem questa. | Workflow autora jest wart odtworzenia razem z runtime. |

Źródła: [instrukcja autora](../../../unreal/FlyingCabFlightLab/docs/QUEST_AUTHORING.md), [definicja dialogu](../../../unreal/FlyingCabFlightLab/Source/FlyingCabFlightLab/FlyingCabDialogueDefinition.h), [sesja](../../../unreal/FlyingCabFlightLab/Source/FlyingCabFlightLab/FlyingCabDialogueSession.cpp), [questy](../../../unreal/FlyingCabFlightLab/Source/FlyingCabFlightLab/FlyingCabQuestSubsystem.cpp), [profil NPC](../../../unreal/FlyingCabFlightLab/Source/FlyingCabFlightLab/FlyingCabNpcDefinition.h).

Wzorzec Mike’a i Jacka łączy temat zadania z rozmową o mieście. Szablon questa ma ofertę, odmowę/powrót do tematów, przypomnienie, przyjęcie, oddanie i podziękowanie. To przydatny materiał testowy, ale nie gotowy scenariusz kampanii o siostrze. Skrypt [Create-NarrativeExamples.py](../../../unreal/FlyingCabFlightLab/scripts/Create-NarrativeExamples.py) dokumentuje utworzenie tych przykładów; nie nadpisuje już istniejących assetów. Nie zakładamy, że jego teksty są eksportem wszystkich późniejszych edycji binarnych `.uasset`.

## 2. Prezentacja: co zachować i skąd szerokość

Zachować tabliczkę imienia, ciemny dymek, widoczny świat w tle, wpisywanie wyłącznie kwestii NPC, pauzy interpunkcyjne i kolejne pojawianie się pełnych odpowiedzi. W Unrealu tempo domyślne wynosi 55 znaków/s, odstęp między odpowiedziami 0,085 s. To punkt startowy do oceny na telefonie, nie docelowo zatwierdzone wartości.

Zachować także dwie reguły obsługi: pierwsze zatwierdzenie podczas wpisywania odsłania tekst bez wyboru odpowiedzi; niewidoczna odpowiedź nie przyjmuje kliknięć ani skrótów. Przy pojawianiu się tekstu i przycisków układ nie powinien zmieniać wysokości. Animacja UI musi działać podczas pauzy świata.

Przyczyna rozciągnięcia jest jawna w [FlyingCabDialogueWidget.cpp](../../../unreal/FlyingCabFlightLab/Source/FlyingCabFlightLab/FlyingCabDialogueWidget.cpp): kotwice X 0,04–0,96, czyli **92% szerokości ekranu**, bez górnego limitu szerokości całego panelu. Podział poziomy wynosi 58% na NPC i 42% na odpowiedzi. Obszar jest zakotwiczony między 46% i 94% wysokości. Minimalne szerokości kolumn to 200 i 160 jednostek, z przerwą 18.

**Pion ograniczy szerokość, ale nie naprawi automatycznie dwóch kolumn.** Przy szerokości 540 jednostek wzór daje około 270 na kolumnę NPC i 209 na odpowiedzi; sam tekst NPC ma około 222 po odjęciu wewnętrznych marginesów. Przy 360 jednostkach minima kolumn z przerwą przekraczają dostępny pas. To wyliczenie ze źródeł, nie pomiar uruchomionego UI; skala DPI dodatkowo wpływa na rozmiar fizyczny.

Proponowany układ Godota:

1. Świat i rozmówca pozostają widoczni nad panelem; w kadrze rozmowy trzeba uwzględnić wysokość zajętą przez UI.
2. Imię i ewentualny mały portret nad wypowiedzią; **odpowiedzi pionowo pod wypowiedzią**, każda na pełną szerokość panelu.
3. Roboczo marginesy 20–24 jednostki przy bazowym 540 × 960; na szerokim oknie wyśrodkowanie i limit szerokości. Wysokość zależna od treści, z ograniczeniem do dostępnego obszaru.
4. Długie wypowiedzi dzielone przez autora na kolejne kwestie; lista odpowiedzi przewijana po przekroczeniu wysokości. Nie zmniejszać bez końca fontu, żeby upchnąć tekst. Obsłużyć też wyjątkowo długą pojedynczą kwestię.
5. Cały wiersz odpowiedzi jest polem dotykowym; podpowiedzi dostosowane do urządzenia. „Dalej” do kontynuacji i pomijanie animacji muszą być odrębnymi czynnościami od wyboru nowo pokazanego przycisku.
6. Zmiana rozmiaru i większy tekst nie mogą ucinać odpowiedzi ani resetować wykonanych wyborów. Styl zachować, ale układ i zawijanie oprzeć na możliwościach Godota; nie przepisywać mechanicznie algorytmu Slate.

Obecny Unreal nie ma przewijania rozmowy; redukuje font przy długiej kwestii lub ponad pięciu odpowiedziach. Portrety i dźwięki mają pola konfiguracyjne, ale dokumentacja wskazuje brak gotowych assetów. [Opis prezentacji i ograniczeń](../../../unreal/FlyingCabFlightLab/docs/NPC_CONVERSATION_UI.md).

## 3. Semantyka questów — zachować świadomie

Zdarzenia z rozgrywki aktualizują wyłącznie aktualny cel aktywnego questa. Jedno zdarzenie może postąpić kilka aktywnych questów, lecz nie przechodzi przez kilka kolejnych celów tego samego zadania. Wcześniejsze zdarzenia nie są odtwarzane po przyjęciu zadania.

| Obecny mechanizm | Co naprawdę oznacza | Konsekwencja dla adaptacji |
| --- | --- | --- |
| Odbiór/dowóz pasażera | Liczony jest odbiór/kurs, a filtr wskazuje dzielnicę docelową; także dla odbioru. | W Godocie rozróżnić miejsce odbioru, cel, osobę i kurs. Nazwa „pasażerowie” nie powinna sugerować liczby osób, jeżeli liczony jest kurs. |
| `CreditsEarned` | Dodatni przychód od aktywacji celu, w tym nagrody questów; wydatki nie cofają licznika. | „Zarób 100 CR”, „zarób 100 CR z kursów” i „miej 100 CR na lek” wymagają różnych reguł. Stan konta nie dowodzi posiadania leku. |
| Paliwo / naprawa | Faktycznie kupione jednostki usługi. | Emitować ilość zakupioną lub naprawione HP, nie sam klik, czas trzymania przycisku ani koszt bez jednostki. Godot ma wartości ułamkowe. |
| Wejście/wyjście z auta | Zdarzenie z ID pojazdu. | Nie utożsamiać wejścia z kradzieżą; kradzież ma własne znaczenie i właściciela. |
| Zakończenie rozmowy | Normalny koniec po odwiedzeniu dowolnego tematu; `Esc` anuluje. Emitowane są zdarzenia z ID NPC. | Rozmowa o mieście też może zaliczyć ogólny cel rozmowy. Do przekonania, przyjęcia obietnicy lub zdobycia informacji potrzeba konkretnego punktu fabularnego. |
| Terminal dostępu | Przyznanie lub potwierdzenie dostępu. | Ponowne potwierdzenie nie powinno automatycznie znaczyć nowego zdobycia uprawnienia. |
| `NextQuest` | Próba automatycznego startu bez ponownej zgody gracza; niespełnione warunki wymagają późniejszego zwykłego przyjęcia. | Rozdzielić „odblokuj ofertę” i „automatycznie rozpocznij etap”. Oferta zadania moralnego powinna zachować realny wybór autora/gracza. |

Źródła: [typy questów](../../../unreal/FlyingCabFlightLab/Source/FlyingCabFlightLab/FlyingCabQuestTypes.h), [RecordEvent i CompleteQuest](../../../unreal/FlyingCabFlightLab/Source/FlyingCabFlightLab/FlyingCabQuestSubsystem.cpp), [adapter zdarzeń i nagród](../../../unreal/FlyingCabFlightLab/Source/FlyingCabFlightLab/FlyingCabGameMode.cpp), [Finish dialogu](../../../unreal/FlyingCabFlightLab/Source/FlyingCabFlightLab/FlyingCabDialogueSession.cpp).

`Inactive` obejmuje także zadanie jeszcze niedostępne. Dostępność lepiej wyliczać z warunków niż utrzymywać kopię statusu w dialogu. Sam rdzeń Unreala nie obsługuje celów równoległych/alternatywnych w jednym queście, stanów porażki i porzucenia, terminów zadania ani powtarzalnych instancji. Nie potrzeba wszystkich tych funkcji do pierwszej rozmowy; trzeba je dodawać pod konkretny scenariusz. Odmowa „nie teraz” w szablonie nie jest trwałą decyzją moralną.

## 4. Braki istotne już dla naszej kampanii

**Warunki wykraczające poza status zadania.** Potrzebne są nazwane fakty, posiadanie dawki, dostępny budżet, stan kampanii i odpowiedni kontekst rozmówcy. Później mogą dojść dostęp do lokacji, konkretny pojazd lub relacja. Warunki powinny działać także na tematach i wyborze powitania. W Unrealu tematy profilu nie mają własnych warunków dostępności, a reguły dialogu sprawdzają tylko statusy questów.

**Skutki fabularne i usługowe.** Oprócz przyjęcia/oddania potrzeba wybranych komend, np. zapisania faktu, kupna dawki i jej przekazania. Dialog nie powinien sam modyfikować portfela, dopisywać sekund i odznaczać celu. Wywołuje usługę; usługa sprawdza aktualne warunki, wykonuje zmianę i dopiero po sukcesie publikuje wynik. Następna kwestia potwierdza rzeczywisty rezultat. Nie wykonywać dowolnych nazw metod lub ścieżek zapisanych w treści rozmowy.

**Konkretny punkt fabularny.** Nadać stabilne ID odpowiedziom i zdarzeniom fabularnym. Ogólny koniec rozmowy zachować do ogólnych celów „porozmawiaj”, a np. `Sister.MedicineRequested` czy `Contact.OfferAccepted` emitować wyłącznie z odpowiedniego skutku. To przykładowe ID projektowe. Ten sam skutek po powrocie do węzła nie może wielokrotnie zaliczać jednorazowego faktu. Zamknięcie okna nie cofa wcześniej skutecznie przyjętego zadania lub kupna.

**Zapis od pierwszego etapu.** W Unrealu stan questów żyje w `GameInstance`. Pola `SaveGame` nie oznaczają podłączenia zapisu na dysk. W Godocie zapisywać statusy, aktywny cel, postęp, fakty, śledzenie i identyfikatory rozliczonych skutków w tej samej sesji co pieniądze i lek. Postęp wiązać z ID celu, nie tylko indeksem tablicy jak w obecnym stanie runtime Unreala; zmiana kolejności treści nie może podmieniać znaczenia starego zapisu. Ustalić wersję definicji/migrację.

**Jednorazowe skutki także po wczytaniu.** Numer rewizji chroni wybór w otwartym oknie, ale nie zastępuje trwałego potwierdzenia wypłaty lub dostawy. Korzystać z identyfikatorów transakcji/wykonania, a dla powtarzalnych dostaw także z identyfikatora konkretnej dostawy. ID samego węzła nie wystarcza, jeśli ta sama kwestia służy podaniu kolejnych dawek. Zmiany stanu, potwierdzenia i powiadomienia muszą mieć ustaloną kolejność, żeby zapis z callbacka nie uchwycił części skutku.

**Zdarzenia i stan to różne rodzaje celu.** „Wykonaj dwa kursy po przyjęciu” jest licznikiem zdarzeń. „Masz dawkę” jest warunkiem aktualnego stanu i może być spełniony wcześniej; „dowiedziano się o źródle leku” jest trwałym faktem. Bez tego gracz, który samodzielnie kupił lek albo spotkał NPC wcześniej, może zostać zmuszony do powtarzania czynności. Każdy typ celu powinien mieć jawną regułę.

## 5. Co mamy już w Godocie i gdzie się podłączyć

| Element istniejący | Stan odczytany z kodu | Dalsza praca |
| --- | --- | --- |
| [DialogueSession](../scripts/services/dialogue_session.gd) | Waliduje podstawowy graf, pokazuje kwestie, przechodzi po wyborach, zawiesza/odwiesza sterowanie. Jest rejestrowany w `GameRoot`. | Dodać kontekst NPC/questa, warunki, kontrolowane skutki, rewizję, rozróżnienie anulowania i ukończenia oraz UI. Obecnie sygnał wyboru jest emitowany po przejściu/końcu; to za późno na uzależnienie przejścia od sukcesu zakupu. |
| [RuntimeContext](../scripts/session/runtime_context.gd) | Sesja niezależna od mapy, jawny rejestr usług, JSON schematu 4 z odczytem 1–4. | Nowy stan questów i potwierdzenia skutków, migracja starszych zapisów, walidacja przed zmianą sesji. |
| [CampaignState](../scripts/session/campaign_state.gd) | Kredyty, liczba dawek, słownik `flags`, timer i dostawa zużywająca dawkę oraz dodająca czas tylko raz dla danego ID. Kampania domyślnie wyłączona. | API faktów z powiadomieniami, zakup/otrzymanie leku, połączenie dostawy z NPC i questem; osobna decyzja o balansie i czasie. Słownik nie jest jeszcze pełnym systemem warunków. |
| [EconomyLedger](../scripts/taxi/economy_ledger.gd) | Wspólny portfel, `credit_once`, zachowane w zapisie potwierdzenia, zakupy usług. | Jawne wyniki operacji i źródła przychodu; nagrody questa przez ten sam portfel. |
| [RideService](../scripts/taxi/ride_service.gd) | Stan zwykłych kursów, pasażerów i rozliczeń; ogólny sygnał `changed`. | Publikować konkretne udane odbiory/dowozy z ID kursu i miejscem, bez zgadywania rodzaju zdarzenia ze zmiany całego słownika. |
| [OnFootInteraction](../scripts/actors/on_foot_interaction.gd) | Wybiera cel typu `FlightCab`, obsługuje wejście i wyjście. | Wspólny wybór dostępnej interakcji: rozmowa, auto, później drzwi/usługa. Jawny priorytet przy NPC stojącym przy aucie; jedno naciśnięcie nie uruchamia dwóch akcji. |
| [LivingWorldDirector](../scripts/living_world/living_world_director.gd) | Tożsamość właściciela, podróże, sygnał przejęcia auta, zatrzymanie planów i aut NPC przy blokadzie gracza. | Profil rozmowy powiązać z istniejącą tożsamością człowieka; nie tworzyć osobnej kopii tej samej postaci tylko do questa. Zakończyć rozmowę po utracie rozmówcy. |
| [WorldRegistry](../scripts/world/world_registry.gd) i [MapRouter](../scripts/world/map_router.gd) | Obiekty bieżącej mapy, trwałe ID, przejścia z tą samą sesją. | Zamykanie/anulowanie rozmowy przy zmianie mapy i odpinanie odbiorców zdarzeń bez utraty faktów. W obecnym routerze nie ma zakończenia aktywnej rozmowy. |
| [GameRoot](../scripts/game_root.gd) | Wczytanie sesji, zapisy okresowe i checkpointy kursów/wejścia/wyjścia. | Checkpoint po zatwierdzonym skutku fabularnym. Ustalić ponowne otwarcie rozmowy na podstawie zapisanego stanu; na start nie trzeba zapisywać animacji wpisywania. |

Docelowa odpowiedzialność jest prosta: **zasoby autora → sesja dialogu → usługi gry → zdarzenia/fakty → questy → widoki**. UI pokazuje wynik i przekazuje wybór. Questy obserwują zdarzenia, a stan ekonomii, dawek i czasu pozostaje w obecnych usługach. Definicje NPC/questów/dialogów są zasobami tylko do odczytu podczas gry; bieżący stan należy do `RuntimeContext`.

Proponowane nowe zasoby: `NpcDefinition`, `DialogueDefinition`, `DialogueNode`, `DialogueChoice`, wspólny model warunków i zatwierdzonych efektów, `QuestDefinition`, `QuestObjective` oraz jawny katalog treści. Proponowane usługi: `QuestService` i adapter skutków/zdarzeń, z rozbudową obecnego `DialogueSession`. Wystarczą pliki `.tres`, Inspector, walidator i scena podglądu; graficzny edytor węzłowy można rozważyć po sprawdzeniu pracy autora z pierwszymi rozmowami.

## 6. Lek, czas i minimalny interfejs

Wymaganie z [wizji gry](../../../docs/GAME_VISION.md) brzmi: **dostarczenie dawki przedłuża życie siostry**. Zarobek, zakup i dostawa są trzema różnymi rezultatami. Ukończenie questa nie powinno samo dodawać czasu, jeśli nie doszło do przekazania dawki. Powtarzalna potrzeba leku pozostaje regułą kampanii, niezależną od jednorazowego questa uczącego pierwszej dostawy. Wygaśnięcie to koniec gry i przerwanie dalszych skutków rozmowy.

Unreal pauzuje świat podczas dialogu. W Godocie blokada sterowania sama nie pauzuje całego drzewa: NPC jawnie ją respektują, a timer kampanii roboczo zatrzymuje się przy każdej blokadzie. Mapa i opcje pauzują także drzewo sceny. Pełny modal dialogowy musi ustalić spójne zachowanie fizyki, kursów, NPC i UI; nie wystarczy sam `suspend("dialogue")`.

Rekomendacja pierwszego wdrożenia: świadomie rozpoczęta rozmowa piesza pauzuje świat i timer kampanii, a UI pozostaje aktywne; krótkie wypowiedzi zwykłych pasażerów korzystają ze znikających dymków bez obowiązkowego otwierania modalu. **Pauza timera kampanii podczas rozmów jest rekomendacją, nadal wymaga decyzji autora.** Gdyby czas miał płynąć, system musi odświeżać warunki i przerwać rozmowę przy końcu gry.

Nie przenosić stałego trackera ani minimapy z Unreala. Dziennik może być osobnym widokiem, a przyjęcie, postęp i ukończenie krótkimi dymkami. Śledzenie questa może oznaczać wyróżnienie wpisu lub miejsca na osobnej mapie; nie oznacza nowego GPS. Zwykłe kursy zachowują automatyczny odbiór/dowóz, a istniejąca strzałka taxi nadal działa tylko w dzielnicy celu. Ewentualne wskaźniki questów wymagają własnej decyzji UI.

## 7. Proponowana kolejność wdrożenia

1. **Rdzeń danych i zapisu razem z dialogiem.** Typowane zasoby, prosty cykl questa, fakty, efekty przez usługi, rewizje wyborów, trwałe rozliczenia i migracja sesji. Zachować sekwencyjne cele zdarzeniowe, dodać warunki stanu potrzebne w pierwszym scenariuszu.
2. **Jedna grywalna rozmowa na platformie.** NPC dostępny pieszo, rozstrzyganie interakcji z autem, panel pionowy, wpisywanie/pominięcie, przyjęcie/odmowa, przypomnienie, oddanie, powrót sterowania i zapis. Wnętrze nie musi być zależnością tego etapu.
3. **Mały spójny wycinek questowy.** Rozmowa z siostrą → zdobycie środków → pozyskanie dawki → powrót i przekazanie → wydłużenie czasu. Pierwsza część może wykorzystać zwykłe kursy; konkretny kontakt, teksty i parametry pozostają autorskie. Sprawdzić też gracza już posiadającego pieniądze/dawkę.
4. **Jedna konsekwencja wyboru i drugi temat.** Oferta zadania o wątpliwym charakterze lub inny zatwierdzony wybór zapisuje fakt; ponowna rozmowa reaguje na niego. Nie wymaga to wdrażania walki ani rozstrzygania sekretu Ariego.
5. **Narzędzia autora i próba urządzenia.** Szablony, wybór istniejących ID, walidacja powiązań, podgląd różnych stanów bez zmiany prawdziwej sesji, ręczna próba długiego tekstu i odpowiedzi na telefonie. Podgląd powinien służyć już podczas tworzenia pierwszych treści.

## 8. Warunki odbioru przyszłego wdrożenia

- Otwarcie, odmowa i anulowanie bez zaakceptowanego skutku nie przyjmują questa, nie wypłacają nagrody i nie zaliczają konkretnego punktu fabularnego.
- Przyjęcie, przypomnienie, oddanie u właściwego NPC i rozmowa po ukończeniu działają; zwykły temat nie wymaga questa.
- Podwójny klik, stara rewizja, ponowna wizyta i wczytanie nie dublują nagrody, zakupu ani tej samej dostawy. Odmowa usługi nie przechodzi do tekstu potwierdzającego sukces.
- Zarobek, bieżące saldo, posiadanie leku i dostawa mają odrębne warunki. Ukończony kurs emituje zdarzenie dopiero po rzeczywistym wykonaniu; istniejąca wcześniej dawka jest uwzględniona zgodnie z typem celu.
- Następna dawka może być przekazana w kolejnej wizycie z nowym ID dostawy; sam zakup nie wydłuża czasu. Wygaśnięcia nie można odwrócić późnym kliknięciem.
- Zapis odtwarza quest, fakty, portfel, dawki i czas także po zmianie mapy; zapis ze starszej wersji dostaje poprawny pusty stan narracji. Nieznane dane są walidowane bez częściowego nadpisania sesji.
- Zakończenie dialogu zwalnia tylko własną blokadę. Trzymanie/puszczenie kierunku, dotyk, utrata fokusu, usunięcie NPC i przejście mapy nie zostawiają ruchu ani wiszącego okna. Modal nie konfliktuje z mapą/opcjami.
- Tekst i odpowiedzi mieszczą się w 540 × 960 oraz innych proporcjach z uwzględnieniem skali UI; długie treści, większy font, przewijanie i pomijanie nie powodują przypadkowego wyboru.
- Walidator sprawdza ID, przejścia, kontekst warunków i efektów, wymagane NPC/questy, cykle zależności i brak używalnej drogi wyjścia. Podgląd nie zmienia prawdziwych pieniędzy, faktów ani zasobów autora.
- Weryfikacja integracji obejmuje istniejące zestawy `architecture`, `on-foot`, `taxi` i `taxi-guidance` odpowiednio do dotkniętych ścieżek. Zmiana zapisu, sterowania NPC lub przejmowania pojazdów wymaga pełnego `living-world`. Dodane testy narracji powinny objąć powyższe reguły, a UI także próbę z rendererem. Pakowanie itch.io wyłącznie na wyraźną prośbę autora.

## 9. Granice tej analizy

Odczytano kod, dokumentację, definicje szablonów i testy; nie uruchamiano Unreal/Godota ani nie zmieniano implementacji. Wniosek o szerokości pochodzi z parametrów widgetu, a nie z nowego zrzutu ekranu. Raport [AUDIT_IMPLEMENTATION_STATUS](../../../unreal/FlyingCabFlightLab/docs/AUDIT_IMPLEMENTATION_STATUS.md) odnotowuje historyczne **49/49** testów Windows po zmianie prezentacji, w tym cykl rozmowy i jednorazową nagrodę; nie jest to wynik nowej próby ani potwierdzenie wyglądu na telefonie.

Starszy [audyt authoringu](../../../unreal/FlyingCabFlightLab/docs/QUEST_DIALOGUE_AUTHORING_AUDIT_2026-09-09.md) zawiera opis braków sprzed wdrożenia oraz późniejszy aneks. Bieżąca ocena uwzględnia kod sesji, moduł edytora i nową prezentację — nie traktuje historycznych braków całego systemu dialogów jako stanu obecnego.

## 10. Uzupełnienie: jak zaprojektowano to w starym Godocie

Odczyt z 2026-09-14 obejmuje zasoby rozmów i questów, skrypty wykonawcze, autoloady, scenę `HiCity`, postacie, UI, zapis oraz plugin autora. Archiwum pozostaje niezmienione; nie uruchamiano historycznej gry. Poniżej rozróżniono mechanizmy podłączone w źródłach od pozostawionych szkiców.

### 10.1. Model dialogu miał szerszy zakres skutków

Struktura: `DialogBook → DialogPage → DialogAnswer → requirements[] + actions[] + goto_page`. Pojedynczy `DialogData` był automatycznie opakowywany w jednostronicową książkę. **Odpowiedź miała listę działań**, a nie jedną akcję questową jak w Unrealu. Wszystkie wymagania odpowiedzi łączono przez AND; niespełnioną odpowiedź można było ukryć lub wyłączyć.

| Możliwość starego Godota | Potwierdzenie w źródłach | Co zachować w nowym projekcie |
| --- | --- | --- |
| Warunek pieniędzy | `money_req`: porównanie bieżącego salda `<`, `=`, `>` | Warunki salda, także `>=` i `<=`; niezależne od licznika przychodu. |
| Warunek przedmiotu | `has_item`, odwołanie do `ItemData` | Dostęp do kwestii tylko przy odpowiednim przedmiocie, z ilością tam, gdzie jej potrzebujemy. |
| Warunek aktywnego zadania | `active_quest` | Zachować i rozszerzyć o statusy z Unreala oraz nazwane fakty. |
| Pieniądze i leczenie | `add_money`, `deduct_money`, `add_life` | Dialogi mogą oferować usługi; wykonanie przez ich właścicieli stanu. `add_life` dotyczy zdrowia bohatera, nie czasu siostry. |
| Lek i czas | `grant_item`, `revoke_item`, `add_maya_time` | Zakup/przekazanie przedmiotu i dostawa leku są pełnoprawnymi skutkami rozmowy. |
| Pojazdy | `grant_vehicle_access`, `revoke_vehicle_access` | Klucze/uprawnienie do auta jako skutek fabularny; oddzielić uprawnienie, własność i faktyczną kradzież. |
| Questy | `start_quest`, `finish_quest` | Konkretna odpowiedź może domknąć rozmowę stanowiącą cel i rozpocząć dalszy etap. Używać walidowanego API. |
| Zachowanie rozmówcy/usługi | `start_animation`, nazwa i liczba powtórzeń | Polecenie działania NPC lub usługi; rezultat mechaniczny nie może zależeć wyłącznie od animacji. |
| Przejścia | Numer strony, `close`, akcja `npc_text` | Przejścia i koniec, lecz przez stabilne ID węzłów i jednoznaczny wynik wykonania. |

Źródła: [DialogAnswer](../../../archive/godot/scripts/dialogues/DialogAnswer.gd), [DialogRequirement](../../../archive/godot/scripts/dialogues/DialogRequirement.gd), [DialogAction](../../../archive/godot/scripts/dialogues/DialogAction.gd), [DialogManager](../../../archive/godot/scripts/ui_logic/DialogManager.gd).

To ważna korekta wcześniejszej perspektywy: zakup leku czy skutki złożone nie są wyłącznie nowymi pomysłami dopisanymi do przykładu Unreal. Mają istniejący wzorzec w autorskim projekcie Godot.

### 10.2. Przebieg kampanii zapisany w treściach i świecie

Poniżej rekonstrukcja intencji z zasobów oraz podłączeń. Nie jest potwierdzeniem bezbłędnego przejścia całego archiwum.

1. `HiCity` uruchamia timer Mai i zadanie rozmowy z nią. Domyślna konfiguracja timera w skrypcie wynosi 6 minut.
2. Rozmowa początkowa z Mayą daje 10 000, uruchamia `1_2_find_your_car` i kończy `1_1_talk_to_maya`. Osobna odpowiedź daje dostęp do `yellow_cab`.
3. Wejście do konkretnego żółtego auta zalicza znalezienie pojazdu i uruchamia podróż do myjni.
4. Podróż jest powiązana z wykryciem paliwa poniżej 20%; następny etap wymaga pełnego baku. Później gracz wraca na trasę do myjni. To przykład questa reagującego na stan pojazdu, nie tylko wejście w obszar.
5. Dotarcie do obszaru myjni uruchamia rozmowę z Froggym. Pierwszy zakup ma w treści cenę 6000, przyznaje przedmiot `The Substance` i zadanie powrotu do Mai.
6. Maya udostępnia odpowiedź o leku wyłącznie przy posiadaniu przedmiotu. Właściwe podanie usuwa go, dodaje **10 minut**, kończy zadanie dostawy i uruchamia zdobywanie środków na następną porcję.
7. Watcher salda uruchamia powrót do Froggy’ego; dalsza gałąź jego rozmowy oferuje kolejne porcje po 4000. Znajduje się tam też deklaracja gotowości zrobienia wszystkiego dla siostry, ale odpowiedź nie zapisuje trwałego faktu moralnego.

W scenie cele rozmowy i dostawy mają `complete_on_enter = false`: samo dotarcie do miejsca ma służyć lokalizacji celu, a zaliczenie następuje z odpowiedniej odpowiedzi. **To precyzyjniejszy wzorzec celu fabularnego niż ogólne „skończono rozmowę z NPC” w Unrealu.**

Źródła: [Maya](../../../archive/godot/dialogues/main_quest/maya_dialog.tres), [Froggy](../../../archive/godot/dialogues/main_quest/mr_froggy_take_2.tres), [scena HiCity](../../../archive/godot/scenes/levels/outside/hi_city.tscn), [start poziomu](../../../archive/godot/scripts/level_1.gd), [zasób leku](../../../archive/godot/items/the_substance_low.tres). Kwoty, czas, dawne imię Jo i nazwy z archiwum nie są automatycznie aktualnym balansem ani nowo zatwierdzonym scenariuszem.

### 10.3. Questy były rozbudowane przez integrację ze światem

Sam aktywny `QuestManager` jest prosty: `INACTIVE → ACTIVE → DONE`, nagroda pieniężna, następny quest i sygnały. Jeden krok fabularny jest zazwyczaj osobnym `QuestData`, a dłuższą sekwencję składają `next_quest_id`, działania dialogowe oraz obiekty umieszczone na mapie.

| Element świata | Zakres |
| --- | --- |
| `StartTrigger` | Aktywacja zadania po wejściu gracza w obszar. |
| `EndTrigger` | Zakończenie aktywnego zadania po wejściu pieszo lub autem; opcjonalnie konkretne auto prowadzone przez gracza i warunek paliwa; możliwość utworzenia sceny nagrody. Pole nazwane `min_fuel_percent` faktycznie sprawdza `<=`, co trzeba nazwać jednoznacznie przy adaptacji. |
| `EnterVehicleTrigger` | Zaliczenie wejścia do konkretnego auta, również sprawdzenie bieżącego auta przy podłączeniu. |
| `MoneyQuestWatcher` | Obserwowanie bieżącego salda, nie sumy historycznych zarobków. |
| `FuelQuestWatcher` | Obserwowanie procentu paliwa w pojeździe gracza i przepinanie po zmianie auta. |
| Rozmówca/usługa | Wskazuje zasób rozmowy i portret; konkretne odpowiedzi sterują skutkami. Stacja paliw buduje odpowiedzi z aktualnego kosztu i stanu auta. |
| Obiekt `quest_goal` | Pozycja dla mapy i wskaźnika; nie musi wykonywać zaliczenia. |

Do nowego systemu warto przenieść **rodzaje warunków i kontekst obiektu**, a ich ocenę oprzeć na jawnych usługach i zdarzeniach. Obszar przekazuje ID lokacji, aktora i pojazdu; quest sprawdza własne wymagania. Paliwo i saldo należy oceniać także przy aktywacji zadania i po wczytaniu, nie tylko po następnej zmianie wartości.

Źródła: [QuestSys](../../../archive/godot/scripts/quest_system/QuestSys.gd), [EndTrigger](../../../archive/godot/scripts/quest_system/quest_obj/EndTrigger.gd), [EnterVehicleTrigger](../../../archive/godot/scripts/quest_system/quest_obj/EnterVehicleTrigger.gd), [MoneyQuestWatcher](../../../archive/godot/scripts/quest_system/quest_obj/MoneyQuestWatcher.gd), [FuelQuestWatcher](../../../archive/godot/scripts/quest_system/quest_obj/FuelQuestWatcher.gd), [stacja paliw](../../../archive/godot/scripts/gas_station.gd).

### 10.4. Ekwipunek, zapis, edytor i pionowy UI już istniały

- `ItemData` opisuje ID, nazwę, ikonę i cenę; `GameState` przechowuje posiadane przedmioty. Jest to zbiór unikalnych typów, bez stosów/ilości: ponowne dodanie tego samego ID niczego nie zwiększa. Warto zachować przedmiotowe warunki fabularne; nie zakładać, że archiwum miało gotowy magazyn wielu dawek.
- Dziennik ma zakładki questów i ekwipunku, pokazuje aktywne/ukończone zadania i pozwala wybrać śledzone. Obecne ustalenia minimalnego UI nadal mają pierwszeństwo przed dawnym stałym wskaźnikiem.
- `SaveManager` ma pięć slotów, kopię `.bak`, dobór wolnego/najstarszego slotu i zapis: statusów questów, pieniędzy, HP, czasu Mai, przedmiotów, uprawnień do pojazdów i ostatniego auta. To rzeczywista implementacja zapisu w źródłach, szersza od samej pamięci questów Unreala. Nowa wersja powinna zachować zakres danych, korzystając z obecnego walidowanego JSON i katalogów ID.
- Plugin `Dialog Editor` umożliwia dodawanie książki, stron, odpowiedzi, akcji i wymagań oraz zapis `.tres`. To pomocnik Inspektora, bez graficznego edytora połączeń i bez symulatora rozmowy porównywalnego z Unreala.
- Projekt ma viewport **1000 × 1600**. `DialogManager.tscn` układa wypowiedź/portret i odpowiedzi w pionowym kontenerze przy dole ekranu. Rekomendowany układ pionowy ma więc bezpośrednią historyczną referencję. Animowane wpisywanie i kolejny wjazd odpowiedzi lepiej zaczerpnąć z Unreala.
- W odczytanej ścieżce starego dialogu otwarcie okna nie pauzuje drzewa ani timera Mai. Timer wygasa przez ekran śmierci i pauzę w `MobileControls`. Rekomendacja pauzy podczas rozmowy z sekcji 6 odpowiada Unrealowi, nie odtwarza starej polityki Godota; autor powinien wybrać ją świadomie.

Źródła: [GameState](../../../archive/godot/scripts/GameState.gd), [zapis](../../../archive/godot/scripts/save/SaveManager.gd), [dziennik](../../../archive/godot/scripts/ui_logic/QuestLog.gd), [plugin](../../../archive/godot/addons/dialog_editor/dialog_editor_plugin.gd), [UI dialogu](../../../archive/godot/scenes/ui_scenes/DialogManager.tscn), [timer w UI](../../../archive/godot/scripts/mobile_controls.gd).

### 10.5. Czego nie traktować jako ukończonego mechanizmu do skopiowania

1. **Brak transakcji skutków.** Manager wykonuje całą listę akcji bez zbiorczej walidacji i bez przerwania po odmowie. Pierwszy zakup u Froggy’ego nadaje lek i zmienia questy przed próbą odjęcia 6000; wynik `spend_money()` jest ignorowany, a sama odpowiedź zakupu nie ma warunku salda. Kod pozwala więc na przyznanie korzyści przy nieudanej płatności. Zachować możliwość wielu skutków, ale zatwierdzać powiązany zakup jako całość.
2. **Brak ponownej kontroli odpowiedzi.** Wymagania sprawdzane są przy pokazaniu strony; wybór nie weryfikuje ich ponownie. Nie ma rewizji widoku ani trwałych potwierdzeń efektów. To szczególnie istotne, jeżeli świat i timer nadal działają podczas rozmowy.
3. **Tylko trzy przyciski.** Zasób Froggy’ego ma na pierwszej stronie cztery odpowiedzi. Gdy trzy wcześniejsze są widoczne, czwarta nie trafia do UI. Nie można kopiować sztywnego limitu z managera.
4. **Status zapisany w zasobie definicji.** `QuestSys` zmienia `QuestData.status`; `complete()` odrzuca tylko brak questa i `DONE`, więc może zakończyć także zadanie nieaktywne. Nowy runtime powinien mieć odrębny stan i legalne przejścia.
5. **Watchery zależne od momentu uruchomienia.** Saldo jest sprawdzane przy `_ready()` i jego zmianie, bez subskrypcji aktywacji questa; paliwo podobnie. Już spełniony warunek może wymagać kolejnej zmiany zasobu, żeby zaliczyć właśnie aktywowany cel. Część triggerów usuwa się po wejściu również bez zaliczenia, a część watcherów odłącza po ukończeniu. Wymaga to jawnej obsługi aktywacji, resetu, map i odczytu.
6. **Kilka warstw historycznych.** Aktywną listą jest pole `quests` autoloadu [QuestSysRoot.tscn](../../../archive/godot/scenes/QuestSysRoot.tscn), zawierające 12 questów, w tym 9 głównych kroków. Osobny `QuestLibrary.tres` ma inną listę. `QuestStage` deklaruje DIALOG/PICKUP/AREA_ENTER/AREA_DELIVER, ale bieżące `QuestData` nie ma listy etapów, a manager ich nie wykonuje. `QuestCondition.arm/disarm` są puste. Stare `AreaEvent` i `box_obj_1` wywołują metody nieobecne w aktualnym managerze. Same te pliki nie dowodzą gotowej obsługi liczników zbierania czy złożonego grafu etapów.
7. **Niedokończone połączenia treści.** `1_6_speak_with_Froggy.next_quest_id` wskazuje wcześniejsze tankowanie, podczas gdy odpowiedź osobno uruchamia dostawę. Plik `2_2_get_money_to_but_maya_time` nie jest w aktywnym katalogu. Tytuł zadania mówi 4000, opis 6000, watcher sprawdza ściśle `> 4000`. Powtarzalne wizyty po lek są zapisane w rozmowach, lecz nie stanowią kompletnego systemu powtarzalnych instancji questów.
8. **Zapis wymaga wzmocnienia.** Zapisuje liczbę sekund, ale nie cały stan aktywności/wygaśnięcia kampanii. Odczyt nie wykonuje odpowiednika obecnej całościowej walidacji odłączonej sesji. Nie przenosić go zamiast obecnego `RuntimeContext`.

To ustalenia ze ścieżek kodu i treści, bez nowego testu runtime. Służą określeniu zakresu i warunków odbioru migracji; nie wymagają rozwijania archiwalnego projektu.

### 10.6. Skorygowany zakres wdrożenia

| Źródło | Co stanowi wzorzec |
| --- | --- |
| Stary Godot | Przedmioty, warunki salda/paliwa/konkretnego auta, złożone skutki odpowiedzi, zakup i podanie leku, powiązanie kroków z obiektami świata, zakres zapisu, pionowy układ rozmowy. |
| Unreal | Osobna sesja dialogu, modele NPC i tematów, status gotowości do oddania, warunki statusów, rewizje i ponowna walidacja wyboru, kontrola katalogu/łańcuchów, podgląd i rytm prezentacji. |
| Nowy Godot | `RuntimeContext`, kontrola gracza, rejestr świata, living world, wspólny portfel, bezpieczna dostawa leku i wersjonowany zapis. |

Pierwsza implementacja powinna mieć **trzy jawne rodzaje postępu**: zdarzenie (np. wykonany kurs), stan (np. posiadana dawka lub wymagane paliwo) i trwały fakt (np. złożona obietnica). Dialog powinien wspierać kilka zaplanowanych skutków odpowiedzi, wykonywanych przez usługi z jednoznacznym sukcesem/porażką. Zakończenie celu rozmowy przypinać do właściwego wyboru, a nie samego zamknięcia okna.

Do warunków odbioru z sekcji 8 dochodzą: lek nie jest przyznawany po odmowie płatności; stan spełniony przed aktywacją jest uwzględniany od razu; wejście do niewłaściwego auta nie zalicza zadania; czwarta i dalsze odpowiedzi pozostają dostępne; kolejne wizyty z nową dawką działają bez resetowania ukończonego pierwszego questa; zapis zachowuje przedmioty i fakty potrzebne do odtworzenia wariantu rozmowy.
