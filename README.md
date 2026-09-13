# Flying Cab

Nowy prototyp Godota ma [pierwszą pętlę taxi](godot/FlyingCabPrototype/docs/FIRST_GAMELOOP_IMPLEMENTATION.md): pasażerowie 3D, kursy, nawigacja, wypłaty, płatne paliwo i zapis. Traffic oraz niezależne podróże mieszkańców są kolejnym etapem.

**Ugh! × GTA2 w pionowej cyberpunkowej metropolii.** Ari lata, kradnie pojazdy mieszkańców i zdobywa zasoby na lek dla śmiertelnie chorej siostry, podejmując coraz bardziej wątpliwe moralnie zadania. Każda dostarczona dawka wydłuża odliczanie, a szansa na trwałe leczenie kryje się na niedostępnych piętrach bogaczy.

[Wizja gry, elevator pitch i fabuła](docs/GAME_VISION.md) — ustalenia autora z 2026-09-12; dokument zawiera również sekret bohatera i rozróżnia wizję od stanu implementacji.

## Pierwszy prototyp Godot 2,5D

Nowy, niezależny [City 02](godot/FlyingCabPrototype/README.md): cztery dzielnice w mieście 150 × 368 m, 25 tarasów na budynkach, neonowe oświetlenie, autostrady i obwodnica. Jeden pojazd, lot, miękki pułap i wymuszony powrót autopilotem spoza miasta. Paliwo zużywa się podczas ciągu; lądowiska tankują zatrzymane auto. Projekt: [godot/FlyingCabPrototype/project.godot](godot/FlyingCabPrototype/project.godot). Na przygotowanym Macu uruchom [Run Flight.command](godot/FlyingCabPrototype/Run%20Flight.command). Sterowanie: **A/D lub strzałki — kierunek, W/↑/Spacja — ciąg w górę, R — reset**; na ekranie są trzy przyciski dotykowe.

Godot ma obecnie [fundamenty architektury i optymalizacje po audycie](godot/FlyingCabPrototype/docs/ARCHITECTURE.md): przygotowanie grafiki przed lotem, profil mobilny, wspólny kontrakt postaci/pojazdu, sesję zachowywaną między mapami oraz moduły dialogów i napraw. Grywalny pokaz nadal skupia się na locie; nowe funkcje wymagają dalszej zawartości i interfejsu.

## Projekt Unreal

Główny projekt gry rozwijamy w **Unreal Engine 5.8 (C++)**. Windows i macOS korzystają z **tego samego repozytorium i gałęzi `main`**.

Otwieraj: [`unreal/FlyingCabFlightLab/FlyingCabFlightLab.uproject`](unreal/FlyingCabFlightLab/FlyingCabFlightLab.uproject).
Mapa startowa: `/Game/Maps/FlightLab`. Moduł gry: `FlyingCabFlightLab`.

Świat jest zapisany w mapie i dostępny do edycji przed Play. Pierwsze kroki: [edycja świata w UE](unreal/FlyingCabFlightLab/docs/WORLD_EDITING.md).

## Start na PC i Macu

1. Zainstaluj tę samą wersję UE 5.8 na obu komputerach oraz kompilator odpowiedni dla tej wersji silnika (Visual Studio na Windows, Xcode na macOS).
2. Pobierz repozytorium:

   ```sh
   git clone --branch main https://github.com/Torgunidas/flying_cab.git
   cd flying_cab
   ```

3. Zbuduj moduł edytora lokalnie:

   Windows, PowerShell:

   ```powershell
   .\scripts\Build-Editor.ps1 -EngineRoot 'D:\Unreal\UE_5.8'
   ```

   macOS, Terminal:

   ```sh
   bash scripts/build-editor-mac.sh '/Users/Shared/Epic Games/UE_5.8'
   ```

   Podaj rzeczywistą lokalizację swojej instalacji. Skrypty nie zmieniają wersji projektu.
   Ten sam parametr przyjmują skrypty synchronizacji `scripts/sync-mac.sh` i `scripts/Sync-Windows.ps1` (pull + build), używane przy każdej zmianie komputera.

4. Otwórz `.uproject`, poczekaj na przygotowanie shaderów i uruchom mapę przyciskiem Play.

**Masz już repo na Macu, ale nie widzisz Unreala?** Zobacz [instrukcję synchronizacji i naprawy pobierania gałęzi](docs/WORKING_ON_MAC_AND_PC.md).

## Co znajduje się w repo

| Ścieżka | Przeznaczenie |
| --- | --- |
| `unreal/FlyingCabFlightLab/` | Aktywny projekt: Source, Config, Content i zasoby Build |
| `godot/FlyingCabPrototype/` | Prototyp 2,5D do prób lotu: cztery dzielnice, autostrady, paliwo i tankowanie |
| `scripts/` | Budowanie i synchronizacja (pull + build) edytora na obu systemach |
| `docs/` | Wizja gry i fabuły, wspólna organizacja pracy oraz audyty |
| `unreal/FlyingCabFlightLab/docs/` | Dokumentacja rozgrywki, autorowania i testów |
| `archive/godot/` | Stary prototyp Godota, wyłącznie archiwum do inspiracji |

Historycznego Godota otwiera się osobno przez `archive/godot/project.godot`; nowy pokaz jest w `godot/FlyingCabPrototype/project.godot`. Zachowano strukturę archiwum i dawne instrukcje; nie są instrukcjami głównego projektu.
Historyczne audyty Unreal pozostają w katalogu głównym, aby zachować odnośniki.

## Zasady pracy

- Przed pracą zamknij edytor i uruchom skrypt synchronizacji: na Macu `bash scripts/sync-mac.sh '/Users/Shared/Epic Games/UE_5.8'`, na Windows `.\scripts\Sync-Windows.ps1 -EngineRoot 'D:\Unreal\UE_5.8'`. Skrypt wykonuje `git pull --ff-only` i od razu buduje moduły edytora.
- Sam `git pull` nie wystarcza po zmianach C++: edytor otwarty z `.uproject` ładuje starą binarkę z `Binaries/` bez ostrzeżenia i gra działa jak przed pullem (tak „zniknął” supercar na Macu, audyt 2026-09-10). Edytor pokazuje teraz ostrzeżenie na starcie, gdy `Source/` jest nowsze niż skompilowane moduły.
- Przed zmianą komputera zapisz zasoby, zrób commit i `git push origin main`. Sam commit nie wysyła zmian na GitHub.
- `Content`, `Config`, `Source`, `Build` i `.uproject` są wspólne. Pliki wynikowe oraz cache powstają osobno na każdym komputerze.
- Nie edytuj równocześnie tej samej mapy lub Blueprinta na dwóch komputerach. Zasoby `.uasset` i `.umap` są binarne.
- Zmiany UE uzgadniaj dla obu komputerów. Aktualne sterowanie ma chroniony [punkt odniesienia](unreal/FlyingCabFlightLab/docs/INPUT_CANONICAL_BASELINE.md).

Repo używa zwykłego Git; obecne zasoby Unreal nie wymagają Git LFS. Archiwum zawiera starsze paczki i eksporty, więc pierwsze pobranie może być duże.

Licencja kodu projektu: [MIT](LICENSE). Materiały zewnętrzne w archiwum mogą mieć własne warunki licencji.
