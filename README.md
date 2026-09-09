# Flying Cab — Unreal Engine

Główny projekt gry rozwijamy w **Unreal Engine 5.8 (C++)**. Windows i macOS korzystają z **tego samego repozytorium i gałęzi `main`**.

Otwieraj: [`unreal/FlyingCabFlightLab/FlyingCabFlightLab.uproject`](unreal/FlyingCabFlightLab/FlyingCabFlightLab.uproject).
Mapa startowa: `/Game/Maps/FlightLab`. Moduł gry: `FlyingCabFlightLab`.

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

4. Otwórz `.uproject`, poczekaj na przygotowanie shaderów i uruchom mapę przyciskiem Play.

**Masz już repo na Macu, ale nie widzisz Unreala?** Zobacz [instrukcję synchronizacji i naprawy pobierania gałęzi](docs/WORKING_ON_MAC_AND_PC.md).

## Co znajduje się w repo

| Ścieżka | Przeznaczenie |
| --- | --- |
| `unreal/FlyingCabFlightLab/` | Aktywny projekt: Source, Config, Content i zasoby Build |
| `scripts/` | Budowanie edytora na obu systemach |
| `docs/` | Wspólna organizacja pracy i raport porządkowania |
| `unreal/FlyingCabFlightLab/docs/` | Dokumentacja rozgrywki, autorowania i testów |
| `archive/godot/` | Stary prototyp Godota, wyłącznie archiwum do inspiracji |

Godota otwiera się osobno przez `archive/godot/project.godot`. Zachowano jego strukturę i dawne instrukcje; nie są instrukcjami głównego projektu.
Historyczne audyty Unreal pozostają w katalogu głównym, aby zachować odnośniki.

## Zasady pracy

- Przed pracą zamknij edytor i wykonaj `git pull --ff-only` na `main`.
- Przed zmianą komputera zapisz zasoby, zrób commit i `git push origin main`. Sam commit nie wysyła zmian na GitHub.
- `Content`, `Config`, `Source`, `Build` i `.uproject` są wspólne. Pliki wynikowe oraz cache powstają osobno na każdym komputerze.
- Nie edytuj równocześnie tej samej mapy lub Blueprinta na dwóch komputerach. Zasoby `.uasset` i `.umap` są binarne.
- Zmiany UE uzgadniaj dla obu komputerów. Aktualne sterowanie ma chroniony [punkt odniesienia](unreal/FlyingCabFlightLab/docs/INPUT_CANONICAL_BASELINE.md).

Repo używa zwykłego Git; obecne zasoby Unreal nie wymagają Git LFS. Archiwum zawiera starsze paczki i eksporty, więc pierwsze pobranie może być duże.

Licencja kodu projektu: [MIT](LICENSE). Materiały zewnętrzne w archiwum mogą mieć własne warunki licencji.
