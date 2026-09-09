# Przegląd i uporządkowanie repo — 2026-09-09

## Ustalony punkt wyjścia

- GitHub: `Torgunidas/flying_cab`, domyślna gałąź `main`.
- Dotychczasowy `main`: `fd9f13f` (Godot).
- Najnowsza pobrana wersja Unreal: `9981910` na `origin/codex/unreal-audit-fixes`, 25 commitów przed `main`, bez rozbieżności historii.
- Ta wersja Unreal była już wysłana na GitHub. Nie sprawdzano lokalnego klonu Maca; możliwe przyczyny niewidoczności to wybrany `main`, brak fetch lub pobieranie tylko jednej gałęzi.

## Przejrzana struktura Unreal

- Descriptor wskazuje UE 5.8 i moduł Runtime `FlyingCabFlightLab`. Target gry i edytora również wymagają 5.8.
- W repo są Source, Config, Content, zasoby Build/Mac oraz mapa FlightLab wskazana jako startowa i domyślna.
- Content zawiera Blueprint pojazdu, dane miasta, ekonomii i świata, katalog i definicje questów oraz akcje i kontekst Enhanced Input.
- Zależności modułu: Core, CoreUObject, Engine, InputCore, EnhancedInput, UMG, Slate, SlateCore.
- Konfiguracja ma sekcje Windows i Mac. Przeszukanie Source/Build/scripts nie wykazało bezpośrednich nagłówków Windows ani bezwzględnych ścieżek do katalogów użytkowników. To kontrola statyczna, nie dowód kompilacji na Macu.
- Chroniony wariant sterowania pozostaje `flyingcab.UseControlFrame=0`.

## Zmiany organizacyjne

- Wspólnym punktem pracy staje się `main`, zawierający dotychczasową historię Unreal oraz porządkowanie repo przez fast-forward, bez przepisywania historii.
- Projekt Unreal zachowuje ścieżkę `unreal/FlyingCabFlightLab/`.
- Godot przeniesiono do `archive/godot/`, zachowując wewnętrzne ścieżki i zawartość 3784 plików. 316 plików metadanych macOS wyłączono z nowego drzewa Git; pozostały lokalnie i w historii.
- README i instrukcje dla agentów w katalogu głównym wskazują Unreal. Instrukcja Mac/PC opisuje naprawę widoczności gałęzi, budowanie i synchronizację.
- Dodano skrypty budowania z kontrolą wersji silnika; ścieżka instalacji jest parametrem lokalnym.
- Dodano ignorowanie metadanych OS/IDE oraz jawne reguły LF dla źródeł i skryptów; zasoby Unreal są oznaczone jako binarne.
- Nie dodano Git LFS ani nie przepisano starych dużych plików w historii. Pierwszy klon nadal pobiera historyczne materiały Godota.
- Nie usuwano historycznych gałęzi. Nie zmieniano kodu, assetów, konfiguracji rozgrywki ani wersji UE.

## Weryfikacja i granice

- Porównanie identyfikatorów blobów Git: wszystkie 3784 zachowane pliki archiwum identyczne z oryginałami; pominięte pliki to wyłącznie metadane OS.
- Brak zmian w śledzonych plikach `unreal/` względem `9981910`.
- Brak kolizji ścieżek różniących się tylko wielkością liter. Brak śledzonych Binaries, Intermediate, Saved, DerivedDataCache projektu Unreal.
- `git diff --cached --check`: bez błędów; składnia skryptu Bash sprawdzona przez `bash -n`.
- Nowy skrypt Windows uruchomił UnrealBuildTool dla `FlyingCabFlightLabEditor Win64 Development`: **Succeeded**, target aktualny (0 akcji kompilacji). Lokalny silnik: 5.8.0, changelist 55116800. To build przyrostowy, nie czysta rekompilacja.
- Lokalny MSVC 14.51 jest nowszy niż preferowane przez tę instalację UE 14.50; UBT zgłosił ostrzeżenie, a sprawdzenie zakończyło się powodzeniem.
- Historyczny kontroler baseline zgłasza pięć różnic udokumentowanych wcześniej w `INPUT_CANONICAL_BASELINE.md`: Pawn.cpp/.h, VehicleVitalsComponent.cpp/.h i TouchControls.cpp. Nie zmieniano tych plików ani manifestu.
- Nie wykonano kompilacji, testów PIE ani sesji gry na fizycznym Macu. Po pobraniu `main` należy wykonać opisany build i ręczną próbę mapy na Macu.
