# Dysze wektorujące — 2026-09-09

Dwa widoczne zespoły napędu na bokach przedniej i tylnej części taksówki. Metalowa obudowa z ceramicznym kanałem i pierścieniem wylotu; żar wnętrza, półprzezroczysta turbulentna struga oraz osobna warstwa refrakcji gorącego powietrza. Animacja działa w płaszczyźnie lotu X/Z i zachowuje czytelność z bocznej kamery.

## Zachowanie i granice fizycznego modelu

`UFlyingCabThrusterVisualComponent` dostaje próbkę zastosowanego przyspieszenia napędu oraz rzeczywistej zmiany prędkości po istniejącym tłumieniu. Próbka powstaje przed ograniczeniem prędkości. Efekty aktualizują się po fizyce; nie odczytują klawiszy, nie dodają sił i nie zmieniają zużycia paliwa. Grawitacja, uderzenia i obcięcie prędkości do limitu nie uruchamiają fikcyjnego ciągu.

- Unoszenie: wylot w dół. Lot ukośny: kierunek przeciwny do sumy sił.
- Hamowanie podczas lotu w prawo: wylot w prawo. Hamowanie opadania przez ciąg: silniejszy wylot w dół.
- Kanoniczne tłumienie po puszczeniu przedstawiamy jako kontrciąg. To świadoma interpretacja wizualna istniejącej mechaniki, bez doliczania paliwa za automatyczne hamowanie.
- Narastanie i zanikanie emisji są wygładzane. Podczas obrotu dyszy emisja maleje, jeśli jej kierunek nie pokrywa się jeszcze z kierunkiem żądanej siły. Gorący metal stygnie wolniej niż struga.
- Brak paliwa, wrak, wyłączona fizyka i opuszczony pojazd wyłączają efekt. Reset i recovery czyszczą także podmuch przy ziemi. Brak nowej próbki nie utrzymuje poprzedniego odpalenia.
- Zmienność dwóch strug pochodzi z niezależnej fazy turbulencji, a nie z przypadkowej nierównowagi sił.

To stylizacja przepływu do gry 2.5D, nie symulacja CFD. Wylot turbiny jest głównie półprzezroczysty; najcieplejszy obszar przy dyszy ma delikatną bursztynową poświatę. Nie ma rakietowego łańcucha diamentów ani stałego długiego płomienia.

## Podłoże i wydajność

Dwa promienie wzdłuż wylotu, do 360 cm, maksymalnie 20 razy na sekundę. Natężenie podmuchu zależy od mocy, odległości i kąta padania. Pula 24 cząstek tworzona przy pierwszym kontakcie strugi z nawierzchnią; dalsza praca bez tworzenia obiektów w każdej klatce. Pył pozostaje w świecie po odlocie auta. Tag komponentu `ThrustWet` daje chłodniejszą, krócej żyjącą mgiełkę wodną; `ThrustNoDust` wyłącza unoszenie cząstek z danej powierzchni. Mapa nie ma nowego systemu deszczu ani automatycznego wykrywania wilgotności.

Komponent udostępnia `PlumeLengthScale`, `bEnableHeatDistortion` i `bEnableSurfaceOutwash`. Refrakcja korzysta z Pixel Normal Offset; jej widoczność zależy od ustawień jakości i kontrastu tła. Materiały mają miękkie wygaszanie przy przecięciu z geometrią. Brak nowych kolizji, cieni dynamicznych i zależności od Niagara. Zasoby są wspólne dla macOS/Windows.

## Zasoby i odtwarzanie

- `Content/Effects/Thrusters/`: zapisane materiały oraz dwa modele; gotowe do użycia i gotowania przez referencje zasobów w komponencie.
- `Build/Thrusters/`: źródła OBJ/MTL w centymetrach, +X to kierunek wylotu.
- `scripts/Build-ThrusterAssets.py`: jawny generator edytorowy, uruchamiany przez `UnrealEditor-Cmd <projekt> -run=pythonscript -script=<skrypt> -unattended -nullrhi`. Nie jest migracją startową i nie zapisuje mapy ani Blueprintu.

## Referencje

- [Rolls-Royce: obrotowy moduł LiftSystem](https://www.rolls-royce.com/media/press-releases-archive/yr-2010/liftsystem-manufacturing-cell.aspx) — przekierowywanie strumienia.
- [NASA: projektowanie dysz](https://www1.grc.nasa.gov/beginners-guide-to-aeronautics/nozzle-design/) — siła zależna od przepływu, prędkości i ciśnienia wylotu.
- [NASA: badania podmuchu unoszącego pył](https://rotorcraft.arc.nasa.gov/Research/Programs/brownout.html).
- [Epic: Pixel Normal Offset](https://dev.epicgames.com/documentation/unreal-engine/refraction-using-pixel-normal-offset-in-unreal-engine) — realizacja zniekształcenia tła.

## Weryfikacja

- UE 5.8, Mac Development Editor: kompilacja zakończona sukcesem.
- Pełny pakiet w udokumentowanym trybie `-NullRHI`: **41/41**, bez niepowodzeń (`Saved/Logs/ThrusterFullNullRHI.log`, raport `Saved/Automation/ThrusterFullNullRHI`). Ostrzeżenia obejmują celowo wywołane zniszczenia, puste paliwo i czyszczenie wejścia, komunikat wydajności diagnostyki oraz zewnętrzny test łączności silnika.
- Dwa seedy InputSoak: po 3019 kroków; także osobny przebieg **2/2** w `Saved/Logs/ThrusterInputSoak.log`.
- Pierwszy pełny przebieg z `-RenderOffscreen` dał 39/41: dwa seedy soak nie przeszły w początkowych krokach. Powtórzenie w trybie przewidzianym przez dokumentację testów zaliczyło cały pakiet bez zmian produkcyjnego sterowania. Nie traktujemy trybów uruchomienia jako równoważnych dla tego fixture.
- Testy nowych efektów sprawdzają wektory, moc, dokładnie dwie dysze i strugi, brak kolizji i wpływu na prędkość/paliwo, odcięcie po pustym baku/zniszczeniu/opuszczeniu auta, reset/recovery oraz brak utrzymania starej próbki. Oddzielny test z renderowaniem zapisuje stany `Thrusters_Lift.png`, `Thrusters_Accelerate.png`, `Thrusters_Brake.png` w `Saved/Automation`.
- Przegląd chronionych plików i historycznego manifestu: opis w `INPUT_CANONICAL_BASELINE.md`. Bez zmiany silnika, mapy, Blueprintu i plików wejścia. Bez commita ani push.

Weryfikacja na macOS nie zastępuje osobnego uruchomienia na Windows ani ręcznej oceny użytkownika.
