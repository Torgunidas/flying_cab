# Highway Turbo — 2026-09-04

Automatyczna premia dla aktualnie prowadzonego auta na dwóch autostradach przez środek miasta i czterech bokach obwodnicy. Nie wymaga przycisku. Dotyczy każdego przejmowanego pojazdu gracza (`AFlyingCabPawn`), nie tylko startowej taksówki. Ambientowe NPC nadal korzystają ze swoich limitów tras: szybko na autostradach, wolniej w osiedlach i przy przystankach; ten etap nie zmienia ich nawigacji.

## Parametry

- Prędkości maksymalne ×1,5: pozioma 1050 → 1575 cm/s, wznoszenie 1150 → 1725, opadanie 1300 → 1950.
- Spalanie ×0,5. Regeneracja, ceny paliwa, obrażenia i siła ciągu bez zmian.
- Łagodne wejście 0,5 s, wyjście 0,8 s. Bonusy prędkości i paliwa interpolują razem; na skrzyżowaniach nie sumują się.
- Limonkowe smugi za kierunkiem ruchu i napis `HWY TURBO` pod autem. Osobne komponenty bez kolizji, bez zmiany czerwono-niebieskich ostrzeżeń zasobów ani strzałki celu.
- Bez kierowcy, przy pustym baku, zniszczeniu lub wyłączonej fizyce premia znika. Reset i recovery zerują stan.

Szybka iteracja w edytorze: Blueprint pojazdu → Components → **HighwayAssist** → Details → **Flying Cab / Highway Turbo**. `Speed Multiplier`, `Fuel Consumption Multiplier`, `Entry Seconds`, `Exit Seconds`. Nie zmieniono zapisanych ustawień Blueprintu ani mapy.

## Granice i architektura

`FlyingCabCityData::GetHighwayStrips()` jest wspólną definicją sześciu prostokątów X/Z dla widocznych pasów i premii. Sprawdzany jest środek pojazdu, przy |Y| ≤ 200 cm. Na peronach, stacjach, w osiedlach i osobnym wnętrzu biura nie ma bonusu. Szerokości i pozycje pasów są identyczne jak przed tą zmianą.

`UFlyingCabHighwayAssistComponent` obsługuje wyłącznie wykrycie strefy, interpolację mnożników i efekt. Nie dotyka wejścia. Pawn wykorzystuje mnożnik limitów prędkości i przekazuje niezależny mnożnik spalania do Vitals; nie powstaje sztuczny ciąg. Zwykłe hamowanie po puszczeniu działa również w turbo.

## Weryfikacja

Nowe testy `FlyingCab.Core.City.HighwayTurbo` i `FlyingCab.Functional.PIE.HighwayTurbo` obejmują granice stref, brak bonusu na peronach, koszty paliwa, niezmienioną regenerację, żywy Blueprint pojazdu, limity w obu kierunkach X/Z, puszczenie sterowania, wejście/wyjście, kolizje efektu, reset, recovery, pusty bak, wrak i opuszczone auto. Opcjonalny parametr `-FlyingCabCaptureTurbo` zapisuje podgląd do `Saved/Automation/HighwayTurbo.png` przy uruchomieniu z renderowaniem.

Wynik na UE 5.8 / Win64 Development:

- Build `FlyingCabFlightLabEditor`: sukces. Pozostały wcześniejsze ostrzeżenia o nowszym MSVC i przestarzałym API w nagłówku silnika; bez nowych błędów kompilacji.
- Pełny pakiet **37/37** z `flyingcab.UseControlFrame=0`, exit code 0: `Saved/Logs/HighwayFullTests.log`. W tym soak wejścia, release/focus, J/Q/R, recovery, pusty bak oraz 180-sekundowy test ruchu miejskiego.
- Dodatkowy test z renderowaniem: **1/1**, exit code 0: `Saved/Logs/HighwayVisualTest.log`. Podgląd `Saved/Automation/HighwayTurbo.png` obejrzany: smugi i napis czytelne na tle pasa.
- Kontrola manifestu: pięć oczekiwanych różnic opisanych w `INPUT_CANONICAL_BASELINE.md`; historyczny manifest bez zmian. `git diff --check` bez błędów whitespace.

To weryfikacja automatyczna i statycznego podglądu, nie nowa ręczna sesja gry przez Parsec ani ponowne zatwierdzenie wariantu sterowania.

## Pliki tego etapu

- `Source/FlyingCabFlightLab/FlyingCabCityData.h/.cpp` — wspólna definicja pasów.
- `Source/FlyingCabFlightLab/FlyingCabCityExpansion.cpp` — rysowanie tych samych pasów ze wspólnej definicji.
- `Source/FlyingCabFlightLab/FlyingCabHighwayAssistComponent.h/.cpp` — nowy komponent premii i efektu.
- `Source/FlyingCabFlightLab/FlyingCabPawn.h/.cpp` — małe podpięcie komponentu.
- `Source/FlyingCabFlightLab/FlyingCabVehicleVitalsComponent.h/.cpp` — mnożnik kosztu paliwa.
- `Source/FlyingCabFlightLab/FlyingCabHighwayTests.cpp` — testy.
- `docs/HIGHWAY_TURBO.md`, `docs/INPUT_CANONICAL_BASELINE.md` — dokumentacja i przegląd różnic manifestu.

Bez zmian assetów, Godota, wersji UE ani kanonicznej ścieżki wejścia. Bez commita/push.
