# Edycja świata w Unreal Engine 5.8

Od 2026-09-10 mapa `Content/Maps/FlightLab.umap` zawiera zapisany świat. Budynki, platformy, dekoracje, stacje, NPC, biuro i trasy są dostępne przed naciśnięciem Play. Uruchomienie gry nie odbudowuje ani nie nadpisuje tej geometrii.

## Pierwsza edycja

1. Po przebudowaniu modułów otwórz projekt i mapę **Content → Maps → FlightLab**.
2. W **World Outliner** rozwiń folder **Flying Cab**. Wyszukaj `ResidentialTowerAM` i zaznacz budynek. **F** ustawia na nim widok.
3. **W** włącza przesuwanie, **E** obracanie, **R** skalowanie. To skróty edytora, używane poza Play. Przeciągnij wybraną oś; dokładną wartość wpiszesz w **Details → Transform**.
4. **Ctrl+Z** cofa zmianę, **Ctrl+S** zapisuje mapę. Zmiany wykonuj poza Play. Zatrzymanie gry odrzuca zwykłe zmiany wykonane w jej testowej kopii.
5. Naciśnij Play i sprawdź zmieniony fragment. Po zatrzymaniu gry możesz dalej edytować zapisane obiekty.

Gra odbywa się w płaszczyźnie **X/Z**; **Z** to wysokość, **Y** to głębokość. Elementy potrzebne do lądowania pozostaw w płaszczyźnie lotu. Dekoracje mogą leżeć z tyłu.

## Foldery i powiązania

| Folder | Zawartość i sposób pracy |
|---|---|
| `Stops` | 24 obiekty `STOP - …` z podpiętymi budynkami, platformami, napisami, oknami, usługami i punktami A_R7. Przesuń obiekt `STOP`, aby przesunąć cały przystanek razem z lokalizacjami kursów i jego znacznikiem minimapy. Rozwiń go, aby edytować pojedynczy budynek lub dekorację. |
| `City` | Granice miasta, dekoracje pasów ruchu, place NPC, perony transportu ambientowego i napisy. |
| `NPC` | Mike i Jack. Możesz przesuwać ich bez zmiany listy NPC w assetach; mapa określa pozycję rozmówcy i jego znacznika. Profil oraz tematy pozostają edytowalne w Details. |
| `Office` | Biuro, wejście, wyjście, terminal i punkty docelowe przejść. Przesunięcie biura przesuwa jego podpięte elementy. Portal wskazuje obiekt docelowy, więc po zapisaniu korzysta z jego aktualnej pozycji. |
| `Vehicles` | Punkt pojawienia się taksówki serwisowej. Punkty `A_R7 spawn - …` są podpięte pod odpowiednie przystanki w `Stops`. Samochody powstają dopiero przy Play w miejscach tych punktów. |
| `Routes` | Zapisane trasy samochodów i pieszych. W Details rozwiń `Route Nodes`; położenia węzłów mają uchwyty edytora. Zmieniaj dane węzłów, a nie samą pochodną krzywą `RouteSpline`, odbudowywaną z tych danych. `Spawn Count` ustala liczbę agentów. |
| `System` | Ustawienia powiązań świata i kontroler napisu sygnalizacji. Zachowaj te dwa obiekty podczas zwykłej edycji poziomu. |

## Co aktualizuje się razem

- Przesunięcie `STOP`: podpięta geometria, miejsca odbioru/dowozu, znacznik przystanku, podpięte stacje i punkty pojazdów.
- Przesunięcie samej stacji: rzeczywista strefa tankowania/naprawy i jej znacznik minimapy. Podłoże trzeba przesunąć lub przygotować osobno.
- Przesunięcie samego budynku: wyłącznie budynek. Dzięki temu można zmienić zabudowę bez przesuwania miejsca kursu.
- Przesunięcie NPC: rozmówca i jego znacznik; osobny plac nie jest automatycznie przebudowywany.
- Zmiana trasy: przebieg ruchu jej agentów. Inne trasy i ich wspólne miejsca przesiadek wymagają spójnej edycji.

Całe przystanki przesuwaj w X/Z. Obracanie lub skalowanie całej grupy nie zmienia stałego rozstawu dwóch stref pasażera (±820 cm w X); obrót i skalę stosuj do poszczególnych obiektów geometrii. `Definition.StopLocation` jest zachowaną daną źródłową; w zapisanej mapie pozycję kursów określa **Transform obiektu STOP**.

## Granice automatyzacji

Od 2026-09-11 nowe autostrady można układać jako **Content → Tile_Set → Highway → BP_HighwayTile**, z premią przesuwaną i duplikowaną razem z pasem. Instrukcja: [TILE_SET.md](TILE_SET.md). Poniższe ograniczenie nieruchomych stref dotyczy dawnych dekoracji RingNorth itp.

Trasy NPC nie omijają automatycznie nowych przeszkód. Po przesunięciu budynku sprawdź przejazd w Play; przy większej zmianie układu popraw także trasy. Dekoracja autostrady jest osobnym obiektem, ale obszar bonusu Highway Turbo i linie dróg minimapy nadal wynikają z konfiguracji granic miasta. Przesuwanie dekoracji nie przenosi obszaru bonusu.

Usunięte stacje, NPC, przystanki i trasy nie są odtwarzane z domyślnych danych. Nie duplikuj `STOP` z tym samym `DistrictId` ani trasy z tym samym `RouteId`: identyfikatory wiążą je z zadaniami i ruchem. Dodawanie nowej lokalizacji questowej wymaga także uwzględnienia jej w katalogu danych używanym do walidacji questów. Geometrię i dekoracje można duplikować niezależnie.

`DA_FlyingCabCityLayout` pozostaje źródłem domyślnego układu, granic, ustawień autostrad i walidacji. Nie przestawia już istniejących budynków zapisanych w mapie. Stare instrukcje zmieniania całego miasta wyłącznie przez ten asset opisują etap sprzed konwersji.

## Implementacja i bezpieczeństwo zapisu

- `AFlyingCabAuthoredWorld` wskazuje zapisane obiekty i punkty spawnu. Bootstrap rejestruje usługi z mapy i uruchamia dynamiczną populację.
- `AFlyingCabDistrictAnchor` jest źródłem światowych pozycji przystanków. `GetWorldDistricts`, `GetWorldFuelStations`, `GetWorldRepairStations` odczytują konkretny świat, bez zmieniania współdzielonego assetu ani globalnego stanu między sesjami PIE.
- Geometria to osobne `AFlyingCabWorldGeometry` i aktory napisów. Okna są instancjonowane w obrębie przystanku. Kolory mają zapisane materiały w `Content/World/Materials`.
- `FlyingCabWorldAuthoring::BakeCurrentWorld` i `scripts/Bake-EditableWorld.py` służą jednorazowej konwersji. Odmawiają ponownego wykonania na gotowej mapie. Nie uruchamiaj starego `Update-MetroCity.py` jako sposobu zwykłej edycji.
- Generator runtime pozostaje dla starszych/testowych map bez zapisanego świata; aktywna `FlightLab` go nie używa.
- `scripts/Verify-EditableWorld.py` sprawdza przesuwanie rodzica i budynku, dziedziczenie pozycji stacji, zapis/odczyt mapy, referencje portali oraz blokadę powtórnej konwersji na tymczasowej kopii mapy. `FlyingCab.Functional.PIE.AuthoredWorld` sprawdza użycie zapisanej geometrii bez duplikacji i odczyty danych z aktorów.
