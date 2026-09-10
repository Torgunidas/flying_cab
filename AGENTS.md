# Flying Cab — repozytorium główne

- Aktywny projekt to `unreal/FlyingCabFlightLab/FlyingCabFlightLab.uproject`, Unreal Engine 5.8, C++.
- Wspólna gałąź Windows/macOS: `main`. Nie twórz osobnych wersji gry dla każdego systemu.
- `archive/godot/` to historyczna referencja do inspiracji. Nie rozwijaj jej bez wyraźnej prośby użytkownika.
- Przed zmianami w Unreal przeczytaj `unreal/FlyingCabFlightLab/AGENTS.md`. Zachowaj chronione sterowanie i wersję silnika.
- Śledź Source, Content, Config, zasoby Build i descriptor projektu. Nie dodawaj cache, lokalnych ustawień IDE ani binariów kompilacji Unreal.
- Używaj ścieżek względnych wobec repo/projektu. Ścieżki instalacji silnika podawaj lokalnie, bez zapisywania ich w konfiguracji gry.
- Po `git pull` ze zmianami w `Source/`, `*.Build.cs`, `*.Target.cs` lub `.uproject` przebuduj moduły edytora przed otwarciem edytora: `scripts/sync-mac.sh` (macOS) lub `scripts/Sync-Windows.ps1` (Windows) robią pull i build razem. Edytor ładuje starą binarkę bez ostrzeżenia; sam `git pull` nie aktualizuje kodu gry (audyt 2026-09-10).
- Zadanie zmieniające C++ jest zakończone dopiero po buildzie i pakiecie testów Automation na obu systemach albo po jawnym zapisie w dokumentacji, który system czeka na weryfikację.
- Kontrola chronionego manifestu sterowania: `unreal/FlyingCabFlightLab/scripts/Verify-InputBaseline.ps1` (Windows) lub `unreal/FlyingCabFlightLab/scripts/verify-input-baseline.py` (macOS, ten sam algorytm).
- Aktualny stan projektu i otwarte punkty: `docs/AUDYT_PROJEKTU_2026-09-10.md` oraz `unreal/FlyingCabFlightLab/docs/AUDIT_IMPLEMENTATION_STATUS.md`.
