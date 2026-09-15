# Ekran startowy i nowa gra — 2026-09-14

**F5 / zwykłe uruchomienie** pokazuje ekran startowy. Miasto, NPC i sesja kampanii powstają dopiero po wyborze:

- **Kontynuuj** wczytuje autosave `user://taxi-session.json`. Przycisk jest nieaktywny, jeśli pliku jeszcze nie ma.
- **Nowa gra** zaczyna pełną nową sesję. Jeśli istnieje zapis, ekran prosi o potwierdzenie i pozwala anulować.
- Podczas gry: **zębatka / Escape → Nowa gra → Potwierdź nową grę**. Zamknięcie opcji anuluje potwierdzenie.
- Przycisk **Nowa gra** po końcu czasu siostry korzysta z tego samego resetu.

Nowa sesja ma 120 CR, pełny bak, sprawne auto w depocie oraz początkowy stan miasta. Usuwa poprzedni postęp questów, fakty, przedmioty, dawki, odliczanie, nagrody, kursy, dług, przejęte auta i stan map. Kampania zaczyna odliczanie dopiero po odpowiednim wyborze w rozmowie z Mayą.

Nowy zapis zastępuje poprzedni **od razu**, przed przygotowaniem miasta; nie czekamy na okresowy autosave. Zapis używa istniejącej operacji przez plik tymczasowy i zmianę nazwy. Odmowa zapisu zatrzymuje uruchomienie nowej sesji i pokazuje komunikat. Nieprawidłowy lub niezgodny autosave po „Kontynuuj” pozostaje na dysku, a menu pozwala świadomie rozpocząć od nowa.

## Grafika i edycja

Menu wykorzystuje dokładnie plakat z `archive/godot/Assets/sky.png`, wskazany przez historyczną scenę `archive/godot/scenes/ui_scenes/MainMenu.tscn`. Kopia w bieżącym projekcie to **`assets/ui/start_poster.png`**; pliki mają identyczny SHA-256. Archiwum pozostaje źródłem referencyjnym.

Scena do edycji i podglądu F6: **`scenes/ui/start_screen.tscn`**. Jest instancją w `scenes/game.tscn`. W Inspectorze można zmieniać teksty, motyw przycisków oraz teksturę `Root/Artwork`. Układ zachowuje cały plakat i jego proporcje: w pionie przyciski znajdują się pod grafiką, w poziomie obok. Sama scena UI uruchomiona F6 jest podglądem; pełną obsługę zapisu zapewnia F5 przez `game.tscn`.

`GameStartScreen` obsługuje widok, potwierdzenie i pasek przygotowania. `game_root.gd` tworzy odłączoną sesję, wczytuje albo zapisuje jej stan, usuwa poprzednią mapę i sesję, następnie wykonuje zwykłe przygotowanie grafiki. Reset działa w obrębie tej samej instancji punktu wejścia, zachowując wybrany `save_path`, katalog narracji i profil jakości. Blokada uruchamiania odrzuca podwójne kliknięcia i opóźnione autosave starej sesji.

Skrót developerski `-- --new-game` oraz `?new=1` w URL samego eksportu Web nadal uruchamiają nową grę bez menu. F6 w `flight_lab.tscn` pozostaje osobnym podglądem poziomu.

## Weryfikacja

```bash
bash tools/verify.sh start-menu
bash tools/verify.sh start-menu-render
bash tools/verify.sh boot
bash tools/verify.sh living-world
```

Testy używają własnych zapisów w `build/`. `start-menu` obejmuje start bez pliku, kontynuację, anulowanie, reset podczas gry i po końcu kampanii, natychmiastowy powrót po restarcie, uszkodzony zapis i odmowę zapisu. Próba graficzna obejmuje 540×960, 360×640, 960×540 oraz przycisk w opcjach.

Potwierdzone lokalnie na macOS / Godot 4.7.2: `start-menu` **44/44**, `start-menu-render` **10/10**, `boot` **8/8** (profil balanced, przygotowanie 5950 ms), `on-foot-render` **12/12**, `narrative` **65/65**, `living-world` **82/82**, `architecture` **41/41**, `taxi-ui` **21/21**. Import kopii bez cache: **PASS, 0 błędów**, log `build/clean-import-lsppxlxz/output.log`. Obrazy: `build/start-menu-540x960.png`, `build/start-menu-360x640.png`, `build/start-menu-960x540.png`, `build/start-menu-continue.png`, `build/start-menu-confirmation.png`, `build/start-menu-options.png`.

Nowy ekran nie był jeszcze sprawdzony na Windows ani fizycznym telefonie. Eksport/pakowanie pozostają osobnym zadaniem na wyraźną prośbę autora.
