# Flying Cab — edytor fabuły 1.1

Osobna wersja edytora v31.22. Plik z repozytorium `quest_editor` pozostaje niezmieniony; jego suma SHA-256 i pochodzenie są w `UPSTREAM.json`. Ta wersja używa własnej pamięci szkiców i nie zawiera scenariusza projektu komercyjnego.

## Uruchomienie

Otwórz `index.html` w przeglądarce. Zachowaj obok niego `app.js`, `core.js`, `flying-cab.css` i `world-catalog.js` — można przenieść cały katalog między macOS i Windows. Nie wymaga instalacji paczek, konta ani połączenia z Internetem.

Z Godota: **Projekt → Narzędzia → Flying Cab — otwórz edytor fabuły**. Wtyczka `Flying Cab Story` jest włączona w projekcie. Jeśli projekt był już otwarty podczas dodawania plików, otwórz go ponownie.

Alternatywnie, z katalogu edytora:

```bash
python3 -m http.server 8774 --bind 127.0.0.1
```

Następnie otwórz `http://127.0.0.1:8774`. Szkice przeglądarki są przypisane do adresu; między `file://` i serwerem przenoś **plik projektu**.

## Nowy quest od zera

1. Kliknij **+ Nowy quest** u góry albo **Nowy quest od zera** w prawym panelu.
2. Wybierz **W bieżącym projekcie**, aby dopisać zadanie do obecnej opowieści. Jeżeli chcesz zacząć bez przykładowego questa, wybierz **W nowym, osobnym projekcie**. Przed przełączeniem możesz pobrać dotychczasowy projekt przyciskiem w kreatorze.
3. Wpisz **Nazwę questa** i wybierz **Zleceniodawcę**. Opcja **+ Utwórz nową postać** pozwala od razu podać imię nowego NPC.
4. Kliknij **Utwórz quest**. Karta **Zacznij tutaj** prowadzi dalej: określ cel, napisz rozmowę, uzupełnij opis i nagrodę.
5. Przy **Określ cel zadania** kliknij **+ Dodaj cel**, wpisz jego opis i wybierz zdarzenie albo sprawdzany stan. Nowy cel nie ma narzuconej platformy ani rodzaju zdarzenia.
6. **Napisz rozmowę** zaznacza pierwszą żółtą wskazówkę `⟪Uzupełnij: …⟫`. Zastąp ją własnym tekstem. Cztery etapy odpowiadają ofercie, zadaniu w trakcie, oddaniu oraz rozmowie po ukończeniu. Przyjęcie i oddanie są już połączone z tym questem; ID i efektów nie musisz zakładać ręcznie.
7. Ustaw opis do dziennika i ewentualną nagrodę (domyślnie 0 CR). **Sprawdź gotowość do eksportu** pokaże pozostałe braki. Szkic możesz zapisać jako `.fcstory` na każdym etapie; puste cele i nieuzupełnione wskazówki blokują wyłącznie eksport do gry.

**Projekt** to zestaw questów i rozmów w jednym pliku. Ponowny import aktualizuje cały ten zestaw. Osobny projekt otrzymuje nowe ID importu i nie zastępuje questów poprzedniego projektu. Wybór nowego projektu zachowuje bieżący stan w historii cofania ustawień w tej sesji; dla trwałego zachowania pobierz plik przed przełączeniem.

## Przykład i import do gry

1. Edytor startuje z przykładem **Dwa powroty do Depot**. `+ Nowy quest` otwiera kreator pustego zadania; `+ Rozmowa` tworzy krótką rozmowę bez celów.
2. W **Zadanie** nadaj tytuł, wybierz rozmówcę, cel, liczbę zdarzeń i nagrodę. Wybierasz nazwy platform i postaci z list; edytor zapisuje ich prawdziwe ID. Zmiana rozmówcy aktualizuje nagłówki tekstu i domyślnego odbiorcę nagrody.
3. Napisz dialog w znanej składni. Szablon questa ma już ofertę, odmowę, rozmowę w trakcie, oddanie i podziękowanie. Przyjęcie i wypłata są podłączone.
4. W **Odpowiedź** wybierz tekst z listy i ustaw warunki oraz skutki. **Rozmowa** ustala, od którego etapu zaczyna się kolejne spotkanie. **Postacie** zawiera wspólne profile i powitania.
5. **Zapisz projekt** pobiera `.fcstory`: teksty, cele, postacie i wszystkie ustawienia. Ctrl/Cmd+S robi to samo. Trzymaj ten plik w repo razem z fabułą. Autosave jest tylko pomocniczym szkicem; nie zastępuje pliku.
6. **Eksport do Godota** sprawdza projekt i pobiera `.narrative.json`.
7. W Godocie wybierz **Projekt → Narzędzia → Flying Cab — podgląd fabuły**, wskaż JSON i sprawdź dialog. To osobna sesja z prawdziwymi warunkami, skutkami i panelem gry. Przycisk symulacji wysyła jedno zdarzenie bieżącego celu. Nie zastępuje próby fizycznego kursu ani dojścia do NPC.
8. **Flying Cab — importuj fabułę** zapisuje dane gry i podpina tematy do wybranych postaci. Po imporcie zatrzymaj i uruchom grę ponownie.

Przykład: przyjęcie u Froggy’ego, dwa **nowe kursy kończące się w Depot**, powrót do Froggy’ego, jednorazowe 75 CR. Źródło i gotowy eksport są w `examples/`. Przykład nie jest automatycznie dodawany do kampanii przy otwarciu edytora; możesz najpierw uruchomić jego podgląd. Istniejący ręczny quest `depot_driver` jest odrębną treścią.

## Warunki, cele i rozwidlenia

- **Zdarzenie** liczy nową aktywność po przyjęciu i aktywacji celu. **Stan** sprawdza bieżące zasoby, np. posiadanie 200 CR. „Zarób 200 CR” i „Miej 200 CR” to różne cele.
- Warunki na jednej liście łączą się przez **oraz**. Odwrócenie oznacza **NIE**. Alternatywy tworzysz przez osobne reguły lub odpowiedzi.
- Cele domyślnie wykonują się po kolei. Rozwiń **Rozwidlenia po wykonaniu celu**, dodaj reguły i wskaż docelowe cele. Pierwsza pasująca reguła wygrywa; ostatnia jest bezwarunkowym wariantem zapasowym. Pusty cel kończy zadanie lub otwiera odbiór nagrody.
- **Pokaż przebieg celów** zestawia wszystkie cele i ich przejścia. Kliknięcie celu otwiera jego formularz. Pętle i nieosiągalne cele są blokowane przy eksporcie. Zapis gry zachowuje tylko faktycznie wybraną ścieżkę; dziennik nie wymaga wykonania pominiętej gałęzi.
- W **Katalog** dodajesz fakty, przedmioty, uprawnienia i własne zdarzenia. Zmiana nazwy aktualizuje powiązania w bieżącym projekcie. Własne zdarzenie i zachowanie drzwi/uprawnienia wymagają podłączenia odpowiedniej mechaniki w grze.
- Skok `{ETAP_2}` wybiera kolejną wypowiedź. Przyjęcie questa to osobny skutek **Przyjmij zadanie**, a nagroda — **Oddaj zadanie i odbierz nagrodę**. Przy innym odbiorcy trzeba dodać rozmowę tej postaci z odpowiedzią oddania.
- `//warunek: ...` ze starego edytora jest opisem. Eksport blokuje taki zapis, aby nie pomylić notatki z działającą logiką. Ustaw mechanikę formularzem, a opis zmień na zwykły komentarz.

**Graf** i **Flow rozmów** pokazują tekstowe przejścia. **Czytanie** pozwala przejść dialog, ale nie wykonuje mechaniki gry. Przeglądarkowy **Podgląd** orientacyjnie pokazuje proporcje telefonu i ostrzega o długich odpowiedziach. Docelową czcionkę, przewijanie i warunki sprawdzaj w Godocie, również przy 360 × 640. Pisz krótkie odpowiedzi; pole NPC w grze ma 172 px, a odpowiedzi zachowują dotychczasowe rozmiary.

## Postać na mapie

Istniejąca Maya i Froggy dostają nowe tematy automatycznie. Nową postać dodajesz w zakładce **Postacie**. Import tworzy jej `NpcDefinition` w `resources/narrative/authored/<projekt>/npc_<id>.tres` z podłączonymi rozmowami.

W Godocie przeciągnij `scenes/narrative/npc.tscn` na mapę i przypisz wygenerowany zasób w **Profile**. Model ustawiasz na dziecku **Visual**, stopy na tarasie, Z = 1,2. Rozstawienie, model i obszary wejścia pozostają pracą w scenie. Sam edytor tekstu nie zna wolnego miejsca na platformie.

## Zapis, import i stabilność

- `.fcstory` to plik do dalszego pisania. `.narrative.json` jest eksportem dla gry, a `.txt` zawiera wyłącznie tekst. Import/eksport samego tekstu jest w **Pomoc**; wczytanie TXT usuwa ustawienia odpowiedzi i wejść bieżącego dokumentu. Nie odtwarza całego projektu.
- Automatyczne komentarze `// @fc:node …` i `// @fc:choice …` utrzymują identyfikatory przy poprawianiu tekstu. Zachowuj je. Przy kopiowaniu na nową wypowiedź usuń skopiowany komentarz ID; edytor nada nowy. Powtórzone ID blokują eksport. Dla markerów używaj liter, cyfr, `_` i `-`.
- ID projektu określa zakres ponownego importu. Ponowny import tego samego projektu zastępuje jego wygenerowane questy, rozmowy i tematy, zachowując treści innych projektów. Nie importuj kilku części kampanii pod tym samym ID — zapisuj je jako dokumenty jednego projektu albo jako różne projekty.
- Poprawki tekstów nie resetują postępu. Zmiana znaczenia celów, ich kolejności, warunków lub ścieżek wymaga podniesienia **Wersji celów**, a następnie nowej gry lub przygotowanej migracji zapisu. Sam numer nie migruje stanu. Zmiana ID/usunięcie treści używanej w zapisie także wymaga migracji lub nowej sesji.
- Import waliduje całość przed zapisem, robi kopię poprzednich plików w `build/story-import-backups/`, zapisuje natywne `.tres` i katalog na końcu. Konflikt z niezależnym questem lub dialogiem o tym samym ID zatrzymuje import. Generowanych `.tres` nie poprawiaj ręcznie — kolejny eksport je zastąpi. Trzymaj je w repo razem ze źródłowym `.fcstory`.
- W **Katalog** są cofanie i ponawianie zmian formularzy. Ctrl/Cmd+Z w polu tekstowym obsługuje tekst; te dwie historie są osobne.

## Utrzymanie

Po zmianie mapy/katalogów odśwież listy nazw z głównego katalogu repo:

```bash
python3 tools/quest_editor/update_world_catalog.py
node tools/quest_editor/tests/core.test.js
```

Z `godot/FlyingCabPrototype`:

```bash
bash tools/verify.sh story-editor
bash tools/verify.sh story-editor-render
bash tools/verify.sh narrative-validate
bash tools/verify.sh narrative
bash tools/verify.sh narrative-render
bash tools/verify.sh living-world
bash tools/verify.sh boot
```

Testy z grafiką uruchamiaj kolejno. CLI importu: uruchom Godota ze skryptem `tools/import_story.gd` i argumentami użytkownika `--input=/ścieżka/eksport.narrative.json`, opcjonalnie `--dry-run` i `--catalog=res://…`. Import nie pakuje gry.

Testy 2026-09-14 wykonano na macOS, Godot 4.7.2; Windows wymaga osobnego sprawdzenia uruchamiania przeglądarki i systemowego wyboru pliku. Browser UI sprawdzono w Chromium: dodanie rozmowy, zmiana rozmówcy, eksport, cofanie i odtworzenie zapisanego projektu.

Wersja 1.1: kreator od zera, osobny projekt, nowy zleceniodawca, neutralne cele, zaznaczane wskazówki i karta startowa. Format źródła/eksportu nadal v1; starsze pliki .fcstory pozostają zgodne.
