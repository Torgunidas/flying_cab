# Flying Cab Story Editor 1.1

Stan: 2026-09-14. Narzędzie autora jest w `tools/quest_editor/` głównego repo. [Instrukcja krok po kroku](../../../tools/quest_editor/README.md).

To niezależna kopia dostarczonego edytora v31.22 z formularzami mechaniki i kompilatorem. Oryginalny plik w repo `quest_editor` pozostaje bez zmian. Źródło autora `.fcstory` zawiera teksty i ustawienia; eksport `flying-cab-narrative` v1 jest deklaratywnym JSON-em. Importer tworzy zwykłe zasoby Godota. Gra nie uruchamia JavaScriptu ani tekstowych komend z eksportu.

## Integracja

Wtyczka `addons/flying_cab_story` dodaje do **Projekt → Narzędzia** otwieranie edytora, import i podgląd eksportu. Jest włączona w `project.godot`. Podgląd scala eksport w pamięci z aktualnym katalogiem i korzysta z `NarrativeService`, `DialogueSession` oraz `NarrativePanel`. Nie czyta i nie zapisuje sesji gracza. Ręczna symulacja zdarzenia używa filtrów bieżącego celu, więc sprawdza logikę, a nie fizyczne wykonanie kursu.

Importer przypisuje wygenerowanym questom/dialogom/przedmiotom i tematom NPC metadane `fc_story_project`. Przy ponownym imporcie zastępuje zawartość tego projektu; pozostałe zasoby pozostają w katalogu. Istniejące profile NPC zachowują tematy innych projektów. Nowe profile trafiają do `resources/narrative/authored/<projekt>/`. Ich instancje i modele nadal umieszcza się w scenie.

Importer najpierw buduje i waliduje kandydacki katalog, porównuje wersje celów i przygotowuje kopię wcześniejszych plików. Natywne zasoby mają trwałe ścieżki; katalog zapisuje się jako ostatni. Błąd zapisu przywraca zapisane wcześniej bajty z kopii. Błąd walidacji niczego nie zapisuje. Nie ma automatycznego importu przy Play ani eksportu paczki gry.

## Rozgałęzione cele

`QuestObjective.transitions: Array[QuestTransition]` jest opcjonalne. Pusta lista zachowuje poprzednie wykonanie sekwencyjne. Każde przejście ma `target` (ID celu lub pusty koniec) i listę `NarrativeCondition`. Pierwsza pasująca reguła wygrywa; ostatnia musi być bez warunków. Walidacja odrzuca nieznane cele, pętle, zasłonięte reguły i nieosiągalne cele.

Zadanie z przejściami zapisuje dodatkowo `path`, czyli odwiedzone cele. Jedno zdarzenie postępuje najwyżej jeden cel danego questa. Dziennik pomija niewybraną gałąź. Walidator zapisu sprawdza legalne krawędzie, początek i koniec ścieżki, liczniki odwiedzonych celów i zerowy postęp pominiętych. Stan nadal używa schematu 5: istniejące questy sekwencyjne zachowują dawny zapis, a nowe gałęzie są opt-in. Dodanie gałęzi do opublikowanego questa wymaga wyższej wersji i migracji lub nowej gry.

## Weryfikacja

- `node tools/quest_editor/tests/core.test.js` z repo: parser źródła, eksport, identyfikatory, renaming, błędne odwołania, reguły i oryginalna suma SHA.
- `bash tools/verify.sh story-editor`: rzeczywisty eksport, natywny zapis `.tres` i ponowny odczyt, reimport, NPC, wersjonowanie, wybór obu gałęzi, zapis i wypłata.
- `bash tools/verify.sh story-editor-render`: osobny podgląd, faktyczny tekst i przyciski, 172 px, symulacja celu, odbiór nagrody, skrót do poprawnego stanu gałęzi.
- Regresja: `narrative-validate`, `narrative`, `narrative-render`, pełny `living-world`, `boot`. Renderery kolejno. Obrazy/logi są w ignorowanym `build/`.

Główna scena i kampania nie dostają przykładowego questa automatycznie. W `tools/quest_editor/examples/` jest gotowy projekt i eksport do świadomego importu lub podglądu.

Wyniki lokalne (macOS, Godot 4.7.2): kompilator 26/26, import/runtime 42/42, podgląd importu 12/12, narracja 69/69, render narracji 25/25, living world 82/82, boot 8/8. Walidacja katalogu: 0 błędów; próba importu bez cache `.godot`: PASS. Windows nie był uruchamiany w tej sesji.

Wersja edytora 1.1 dodaje **+ Nowy quest** z kreatorem pustego zadania, wyborem osobnego projektu i nowego NPC oraz kartą **Zacznij tutaj**. Zmiana dotyczy narzędzia autora; schema źródeł i importu pozostaje v1, a runtime i zapis gry nie ulegają zmianie.

Weryfikacja 1.1: kompilator 35/35, import/runtime 42/42, narracja 69/69. Eksport uzupełnionego questa od zera (cel typu stan) przyjęty przez natywny importer w dry-run. W przeglądarce sprawdzono tworzenie w bieżącym i nowym projekcie, nowego NPC, zaznaczanie wskazówek oraz uzupełnienie szkicu do poprawnego eksportu. Test nowego NPC uwzględnia katalog, do którego autor już zaimportował przykład.
