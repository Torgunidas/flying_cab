# Jeden projekt na Macu i PC

Repo: https://github.com/Torgunidas/flying_cab — gałąź `main`.
Projekt: `unreal/FlyingCabFlightLab/FlyingCabFlightLab.uproject` — UE 5.8.

## Istniejący klon na Macu

Najpierw zamknij Unreal/Godota. W katalogu repo sprawdź:

```sh
git status
git remote -v
```

Jeśli są niezapisane w Git zmiany, zachowaj je na własnej gałęzi i zrób commit przed przełączeniem. Nie używaj `reset --hard` ani `clean` do synchronizacji.
`origin` powinien wskazywać `Torgunidas/flying_cab` (HTTPS lub SSH).

```sh
git fetch origin
git switch main
git pull --ff-only origin main
git branch --set-upstream-to=origin/main main
```

Jeśli lokalny `main` jeszcze nie istnieje, zamiast `git switch main` użyj:

```sh
git switch --track origin/main
```

Jeśli klon pobiera tylko jedną gałąź i brakuje `origin/main`, rozszerz pobieranie:

```sh
git remote set-branches origin '*'
git fetch origin
git branch -r
```

Jeśli `pull --ff-only` zgłasza rozbieżną historię, zatrzymaj synchronizację i zachowaj lokalne commity. Trzeba porównać je z `origin/main` i scalić świadomie; nie nadpisuj ich wymuszonym pushem.

W GitHub Desktop wybierz to repo, `Fetch origin`, następnie `Current branch → main` i `Pull origin`. Sprawdź ścieżkę lokalnego klonu, gdy aplikacja nadal pokazuje stare pliki.

## Środowisko

Oba komputery powinny mieć ten sam release UE 5.8, najlepiej identyczny patch/build. `EngineAssociation` oraz oba pliki Target.cs wymagają 5.8. Nie konwertuj projektu automatycznie na starszą ani nowszą wersję.

Windows: narzędzia C++ i Windows SDK obsługiwane przez tę instalację UE. Skrypt `scripts/Build-Editor.ps1` buduje `FlyingCabFlightLabEditor Win64 Development`.

Mac: pełny Xcode i macOS obsługiwane przez UE 5.8. Wersje sprawdź w [wymaganiach Epic dla macOS](https://dev.epicgames.com/documentation/unreal-engine/macos-development-requirements-for-unreal-engine). Samo Command Line Tools może nie wystarczyć. Aktywną instalację sprawdzisz przez `xcode-select -p` i `xcodebuild -version`. Skrypt `scripts/build-editor-mac.sh` buduje `FlyingCabFlightLabEditor Mac Development`.

Skrypty przyjmują folder silnika jako parametr lub zmienną środowiskową `UE_ROOT`; sprawdzają wersję w `Engine/Build/Build.version`. Ścieżki ze spacjami podawaj w cudzysłowach. Nie kopiuj DLL z PC na Maca: każdy komputer kompiluje swój moduł.

## Codzienny obieg zmian

Na komputerze, na którym zaczynasz pracę, zamknij edytor i uruchom skrypt synchronizacji. Wykonuje on `git pull --ff-only` na bieżącej gałęzi, wypisuje zmienione pliki C++ i od razu buduje moduły edytora:

macOS:

```sh
bash scripts/sync-mac.sh '/Users/Shared/Epic Games/UE_5.8'
```

Windows (PowerShell):

```powershell
.\scripts\Sync-Windows.ps1 -EngineRoot 'D:\Unreal\UE_5.8'
```

Build bez zmian kończy się w kilka sekund, więc skrypt buduje zawsze. Skrypt nie używa `reset --hard` ani `clean`; przy rozbieżnej historii zatrzymuje się tak samo jak `git pull --ff-only`, a lokalne zmiany zostawia nietknięte. Gdy chcesz tylko pobrać zmiany bez budowania (np. same assety lub dokumentacja), nadal możesz użyć `git pull --ff-only`, ale przed otwarciem edytora po zmianach w `Source/` build jest obowiązkowy.

Po pracy zapisz wszystkie zasoby w edytorze, zamknij go i sprawdź zmiany:

```sh
git status
git diff
git add unreal/FlyingCabFlightLab
git diff --cached --stat
git commit -m "Describe the completed change"
git push origin main
```

Jeśli zmieniałeś dokumentację lub skrypty, dodaj też odpowiednie pliki. Na drugim komputerze uruchom skrypt synchronizacji; build jest jego częścią, nie osobnym krokiem do zapamiętania.
Przy większych pracach można używać gałęzi funkcjonalnych; muszą być wypchnięte i ostatecznie scalone do wspólnego `main`.

To samo `.uasset`/`.umap` edytuj kolejno na komputerach: push z pierwszego, pull na drugim. Git nie potrafi sensownie połączyć dwóch niezależnych zmian binarnych.
Save gry, preferencje edytora oraz zawartość `Saved/` nie są synchronizowane przez repo.

## Dlaczego build po pullu jest obowiązkowy

Edytor uruchomiony z `.uproject` sprawdza tylko, czy pliki modułów w `Binaries/<Platforma>/` istnieją i czy ich `BuildId` zgadza się z silnikiem. Nie porównuje dat źródeł z binarką. Po pullu ze zmianami w `Source/` stary moduł ładuje się bez komunikatu: assety i konfiguracja są nowe, ale kod jest stary. Tak 2026-09-10 na Macu nie było supercara, choć repo było aktualne (szczegóły: `docs/AUDYT_PROJEKTU_2026-09-10.md`).

Zabezpieczenia dodane po audycie:

- Skrypty `scripts/sync-mac.sh` i `scripts/Sync-Windows.ps1` łączą pull z buildem.
- Moduł edytora `FlyingCabNarrativeEditor` po starcie edytora porównuje najnowszy plik w `Source/` z datą skompilowanych modułów. Gdy źródła są nowsze, loguje ostrzeżenie `Source is newer than the compiled game modules` i pokazuje powiadomienie w edytorze z poleceniem uruchomienia skryptu.
- Bootstrap świata loguje `A_R7 supercars parked: 4/4.`; brak tej linii w logu PIE oznacza moduł zbudowany przed 2026-09-10.

## Sprawdzenie zgodności

```sh
git status --short --branch
git rev-parse HEAD
git rev-parse origin/main
```

Po `git fetch origin` oba identyfikatory powinny się zgadzać na czystym `main`. Porównaj również `HEAD` na PC i Macu.
Uruchom mapę FlightLab i sprawdź lot, puszczenie klawiszy, wejście/wyjście z auta, dziennik i reset. Mac wymaga osobnej rzeczywistej próby kompilacji i gry; zgodność repo nie zastępuje tego testu.
