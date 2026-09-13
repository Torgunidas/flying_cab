# Naprawa płaszczyzn w eksporcie — 2026-09-13

Zgłoszenie: w [publikacji itch.io](https://torgerd.itch.io/newcab) wjazd w smog oraz dolna zabudowa wyglądały jak przenikanie przez płaskie plansze. Odtworzono ogromne ukośne powierzchnie zasłaniające auto i miasto. Użytkownik potwierdził dolną część miasta.

## Przyczyna

Błąd był w dodanym podczas optymalizacji narzędziu `tools/compile_city.gd`. Kompilator wywoływał `MultiMesh.set_instance_transform()` dla każdego detalu, również przy uruchomieniu `--headless`. W Godocie 4.7.2 renderer zastępczy używany bez okna pomija to wywołanie. Zapisane grupy zawierały liczbę instancji i siatkę, ale **nie zawierały bufora ich transformacji**. W pobranej paczce dotyczyło to wszystkich 166 grup, obejmujących 1226 detali.

Przy uruchomieniu z rendererem niepełne dane dawały rozciągnięte, ukośne powierzchnie. Efekt mógł być różny między procesami; lokalne źródło potrafiło wyglądać poprawnie, choć detale nie znajdowały się na swoich miejscach. Samo pomyślne otwarcie sceny i testy kolizji nie wykrywały tego problemu. Wcześniejsza weryfikacja była niewystarczająca, a wcześniejsze liczby wydajności po grupowaniu detali wymagają zastąpienia.

Potwierdzenie w kodzie użytej wersji silnika: [renderer headless](https://github.com/godotengine/godot/blob/4.7.2-stable/servers/rendering/dummy/storage/mesh_storage.h) ma pustą implementację ustawiania pojedynczej transformacji oraz działający zapis/odczyt całego bufora. Układ bufora odpowiada [implementacji GLES3](https://github.com/godotengine/godot/blob/4.7.2-stable/drivers/gles3/storage/mesh_storage.cpp).

Nie zmieniono shadera smogu, geometrii źródłowego miasta ani kolizji. Ukrycie samych grup detali w diagnostycznym egzemplarzu pobranej paczki usuwało płaszczyzny; shader smogu nadal działał.

## Zmiana

- Kompilator buduje pełny bufor transformacji na CPU i przekazuje go przez `MultiMesh.buffer`. Ta ścieżka zachowuje dane zarówno bez okna, jak i z rendererem.
- Każda grupa zapisuje mapowanie instancji na oryginalne węzły. Walidator porównuje wszystkie pozycje, obroty i skale z edytowalną sceną, sprawdza długość bufora oraz brak zaginionej/podwójnej geometrii. Przy działającym rendererze porównuje także odczyt instancji z danymi bufora.
- Scena jest najpierw zapisywana do pliku tymczasowego, ponownie odczytywana i sprawdzana. Dopiero poprawny wynik zastępuje `city_runtime.tscn`. Wersja kompilatora: 2.
- Eksporter ponownie otwiera **gotowy `index.pck`** i sprawdza geometrię przed utworzeniem ZIP. Testowanie wyłącznie sceny przed eksportem nie wystarcza.
- Pakowanie obejmuje jawnie określone pliki gry. Nie dołącza dodatkowych archiwów pozostawionych ręcznie w `build/web/`; istniejące pliki użytkownika pozostają na dysku.

## Weryfikacja

Pobrana paczka publikacji `html/19213297/index.pck` miała 642 892 bajty i SHA-256 `55854363ab91d74bb7213efceb600dc491c5c071c4de328214cfe835dd1557d9`. Była identyczna z lokalnym eksportem zastanym na początku tej naprawy. Zniekształcenia odtworzono w WebGL2 i przy lokalnym renderowaniu tej samej paczki.

Nowy test celowo uruchomiony na starej paczce zakończył się błędem. Następnie zaliczył ponownie wczytaną scenę po kompilacji headless i gotowy poprawiony PCK. Powtórzone testy miasta **48/48** oraz smogu i reflektorów **24/24** przeszły.

W poprawionej paczce Web, uruchomionej lokalnie w przeglądarce Chromium/WebGL2, sprawdzono start, opuszczenie depotu, zejście na dno, wznoszenie pomiędzy budynkami LowLife oraz wyjście ponad granicę smogu. Nie ma ukośnych plansz; widać detale na fasadach i prawidłowo przełączające się reflektory. Konsola nie zgłosiła błędów ani ostrzeżeń. To test przeglądarki na Macu; Samsung S25+ nadal wymaga próby autora.

## Zaktualizowany pomiar wydajności

Mac / Apple M5, Godot 4.7.2, Compatibility, 540 × 960, profil desktop, bez VSync i bez sztucznego kroku FPS. Pozostałe karty z grą zamknięto przed pomiarem. Pomiar obejmuje sześć stacjonarnych kadrów po przygotowaniu grafiki, nie lot na telefonie ani sam czas GPU.

| Kadr | Wywołania rysowania: baza audytu → poprawna geometria | Maksimum pierwszego przejścia po przygotowaniu | p95 ustalonego kadru, pierwszy/drugi przebieg |
| --- | ---: | ---: | ---: |
| Depot | 911 → 217 | 11,80 ms | 6,43 / 6,58 ms |
| Smog | 736 → 232 | 9,02 ms | 6,97 / 6,62 ms |
| Eden | 501 → 212 | 45,32 ms | 6,89 / 6,50 ms |

Przygotowanie 119 widoków trwało 1873 ms bez limitu FPS. Największy odstęp podczas wszystkich przejść wyniósł **45,32 ms**; tego skoku nie pominięto. Maksima ustalonych kadrów wynosiły 8,52–10,20 ms. Zapis: `build/performance-geometry-fixed.json` i `.log`. Wcześniejsze wartości 204/220/209 wywołań oraz maksima około 9 ms dla wszystkich wejść nie opisują kompletnej sceny i nie powinny być używane jako aktualny wynik.

## Paczka do publikacji

Zbudowano i sprawdzono **`city03-8a443ef67981`**:

- `build/city03-8a443ef67981-itch.zip` — paczka z jednoznacznym ID;
- `build/FlyingCabFlight01-itch.zip` — ta sama zawartość pod dotychczasową nazwą;
- `build/web-manifest.json` — hashe plików pakietu;
- poprawiony PCK SHA-256: `fa2b06980f3c5371300355af8b690bd6157f1e8d94f898c76d700a079063859f`.

**Nie przesłano nowego ZIP na itch.io.** Opisaną naprawę zweryfikowano w lokalnym eksporcie; publikacja wymaga podmiany paczki przez autora lub osobnego zlecenia publikacji. Bieżące uprawnienie do eksportu zostało udzielone podczas tej naprawy; odrzucenie z poprzedniego zadania nie jest już aktualnym statusem lokalnej paczki.

Z katalogu prototypu: `bash tools/export-web.sh` wykonuje kompilację, walidację zapisanego miasta, eksport, kontrolę gotowego PCK i pakowanie. `bash tools/verify.sh compilation` sprawdza zapisane miasto bez ponownej kompilacji. Ponowny eksport przez sam edytor musi korzystać z poprawnie skompilowanego miasta; zalecana ścieżka to skrypt obejmujący kontrolę paczki.
