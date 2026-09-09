# Osiedla mieszkalne i większy bak — 2026-09-04

## Zmiana gameplayowa

- Bak **200 jednostek**, start **130** (nadal 65%). Spalanie, Highway Turbo i cena **2 CR/jednostkę** bez zmian. Pełny bak daje dwukrotny zasięg względem wcześniejszej pojemności przy tym samym sposobie lotu. Uzupełnienie większej ilości paliwa kosztuje proporcjonalnie więcej; tankowanie nadal respektuje dostępne saldo.
- Reset odtwarza 130 jednostek. Recovery z pustego baku daje 50 jednostek, zgodnie z dotychczasową regułą 25%. Kontrolka 20% odnosi się teraz do 40 jednostek, nie do 20.
- **24 platformy mieszkalne, po 6 na osiedle**: zachowano oryginalne 12 przystanków i ich identyfikatory/kolejność, dopisano 12 nowych. Wszystkie są dostępne dla autonomicznych ofert pasażerów i celów kursów. Nadal maksymalnie sześć oczekujących ofert i jeden pasażer w taksówce.
- Solidne wieżowce stoją w płaszczyźnie lotu, na środku platformy. Trzeba je oblatywać nad dachem, pod platformą lub z boku. Okna i dekoracje są niekolizyjne; wszystkie okna korzystają z jednego komponentu instancjonowanych meshy.
- **Lewa krawędź: PICKUP, prawa: DROPOFF**. Strefy oddalone o 1640 cm (poprzednio 860 cm), bez zmiany mechaniki curbside link. Pełna szerokość stref mieści się na platformach.
- Cztery stacje paliw i dwa warsztaty mają **wydzielone lądowiska na dachach** odpowiednich budynków. Fizyczny dach jest podłożem; marker i strefa serwisowa przesunięte razem. Minimapę aktualizuje to samo źródło danych. Quest giverzy pozostają na swoich oddzielnych placach.
- Autostrady, obwodnica, trasy NPC i kanoniczne sterowanie pozostają bez zmian. Nie zmieniono zapisanej mapy ani assetu tras.

## Nowe adresy

| Osiedle | Nowe przystanki |
|---|---|
| Yellow / lewo–dół | Yellow Steps, Lower Stacks, East Tenements |
| Ashline / lewo–góra | Ashline Court, Furnace Homes, Lantern Heights |
| Orbital / prawo–góra | Silica Court, Aurora Stacks, Relay Homes |
| Foundry / prawo–dół | Copper Steps, Boiler Court, Foundry East |

## Przygotowanie pod wnętrza

Każda lokalizacja ma stabilny `DistrictId` i dwa nazwane punkty sceny `HomeEntry_<kod>_Left/Right` z tagami `FutureResidentialInterior` oraz `DistrictId`. Leżą poza solidną ścianą, na wysokości postaci stojącej na platformie. To tylko punkty zaczepienia pod późniejsze portale i zadania piesze — **wnętrza oraz nowe questy nie są jeszcze zaimplementowane**. Obecne wejście przy Mike'u nie zostało przeniesione.

Szybka iteracja:

- `BP_FlyingCabPawn` → Class Defaults → Flying Cab / Resources → `Max Fuel`, `Starting Fuel`.
- `DA_FlyingCabCityLayout` → Districts → `Stop Location`, `Residential Tower Height`, identyfikatory, nazwy. Wysokość 400–1200 cm, domyślnie 800; modyfikując ją, trzeba zachować wolną przestrzeń nad dachem i trasy NPC.
- Geometria jest budowana przy rozpoczęciu gry, tak jak wcześniejsze Metro City. Ocena całego osiedla: Play → tryb obserwatora. Bez automatycznego zapisywania/przebudowy mapy w edytorze.

## Weryfikacja

Test `ResidentialContract` sprawdza pojemność, koszt i ilość tankowania, reset/recovery, skrajne perony i brak zabudowy w pasach autostrad. `ResidentialAccess` bada rzeczywiste kolizje wieżowców, podejścia z boku i z góry do wszystkich peronów, obrys lądującej taksówki, podłoże peronów/dachów oraz przestrzeń przyszłych wejść pieszych i placów quest giverów. `MetroTrafficFlow` nadal bada pełny obrys pojazdów na trasach oraz długi przejazd wszystkich NPC.

Wyniki końcowe:

- Build `FlyingCabFlightLabEditor Win64 Development`, UE 5.8: sukces. Tylko wcześniejsze ostrzeżenia o nowszym MSVC i przestarzałym API nagłówka silnika.
- **39/39** testów Core + PIE z `flyingcab.UseControlFrame=0`, exit code 0: `Saved/Logs/ResidentialFinalTests.log`. W tym kanoniczne testy release/focus/J/Q/R/recovery, soak wejścia, 180 sekund ruchu NPC i pełna kontrola geometrii osiedli.
- **2/2** testy z renderowaniem: `Saved/Logs/ResidentialVisualTests.log`. Obejrzano `Saved/Automation/ResidentialNeighborhood.png`, `ResidentialPlatform.png` i `ComfortMinimap.png`. Po renderze skorygowano jeszcze tylko niewidoczne punkty przyszłych wejść do rzeczywistej wysokości kapsuły postaci, co objął końcowy pełny test.
- `git diff --check` bez błędów. Kanoniczny manifest nadal wykazuje pięć znanych różnic i 27 zgodnych plików; w tym etapie zmieniono w nich tylko wartości pojemności/zapasu paliwa.

W trakcie weryfikacji poprawiono dwa ciasne podejścia: Ashline Court obniżono o 200 cm, Lantern Heights przesunięto o 250 cm w prawo. Zaktualizowano także stare oczekiwanie 12 przystanków w teście assetu do 24. Wcześniejsze logi `ResidentialFullTests.log` i `ResidentialVerifiedTests.log` są wynikami pośrednimi z błędami, nie wynikiem końcowym. Nie zmieniano tras ani nawigacji NPC w celu obejścia testów.

Testy i rendery nie zastępują oceny odczucia lotu przez użytkownika; nie wykonywano ręcznej sesji przez Parsec.

## Pliki zmienione w tym etapie

- `FlyingCabPawn.h`, `FlyingCabVehicleVitalsComponent.h`: pojemność i zapas początkowy.
- `FlyingCabCityData.h/.cpp`: dodatkowe adresy, skrajne perony, dachowe usługi, wysokości i punkty wejść.
- `FlyingCabCityExpansion.h/.cpp`: kolizyjna zabudowa, dekoracje, instancjonowane okna i punkty przyszłych portali.
- `FlyingCabCityLayoutAsset.cpp`: walidacja wysokości wieżowców.
- `FlyingCabCoreTests.cpp`, `FlyingCabFunctionalTests.cpp`, `FlyingCabMetroTests.cpp`, `FlyingCabGameplayComfortTests.cpp`, `FlyingCabHighwayTests.cpp`: nowe oczekiwane liczby przystanków i zasoby, bez osłabiania testów wejścia.
- `FlyingCabResidentialTests.cpp`: nowe testy geometrii i baku.
- Ten dokument, `METRO_CITY.md`, `INPUT_CANONICAL_BASELINE.md`, `FLIGHT_FEEL_TEST.md`: opis aktualnego etapu.

Wszystkie źródła C++ powyżej znajdują się w `Source/FlyingCabFlightLab`. Bez zmian Godota, UE, manifestu wejścia, commita ani push.
