# Audyt architektury i wydajności — 2026-09-12

**Raport historyczny, sprzed wdrożenia.** Poniższe pomiary i odwołania do kodu opisują stan audytowany. [Architektura i wynik wdrożenia](ARCHITECTURE.md) zawierają aktualne moduły, porównanie pomiarów, rozszerzony zakres autora i status eksportu. Autor następnie podał [adres publikacji](https://torgerd.itch.io/newcab); starszy pokaz uruchomiono w przeglądarce. Nie podmieniono go na nową wersję.

## Wniosek

**Obecny projekt jest dobrą podstawą próby lotu, ale przed dołożeniem żyjącego miasta potrzebuje etapu stabilizacji renderowania i rozdzielenia odpowiedzialności.** Nie ma podstaw do zmiany silnika ani przepisywania całego prototypu. Są natomiast mierzalne koszty, które już teraz ograniczają zapas na NPC, kolejne pojazdy i kampanię.

W lokalnej próbie pierwsze pokazanie smogu spowodowało odstęp między klatkami **151 ms**, a pierwsze pokazanie Edenu **122 ms**. Przy drugim pokazaniu tych samych miejsc maksima podczas przejścia wyniosły około **8 ms**. Oddzielnym problemem jest stały koszt renderowania: depot generował **911 wywołań rysowania na klatkę**, z czego **709 znikało po wyłączeniu cieni**. Te obserwacje wskazują na dwa osobne zadania: przygotowanie grafiki przed oddaniem kontroli oraz obniżenie kosztu kolejnych klatek.

Zgłoszenie autora: **itch.io, Chrome, Samsung S25+**, kilka zatrzymań przypominających doczytywanie. To potwierdzenie występowania problemu u autora, nie wykonany przez audytora pomiar na telefonie. Nie dostarczono adresu publikacji ani śladu wydajności z urządzenia; zgodność opublikowanej paczki z lokalnymi plikami nie została potwierdzona.

Zakres: aktualny `godot/FlyingCabPrototype`, kod, sceny, materiały, ustawienia, eksport lokalny, dokumentacja i pomiar prawdziwego renderera. Docelowy zakres według [GAME_VISION.md](../../../docs/GAME_VISION.md): lot, pieszy Ari, NPC chodzący i korzystający z aut, kradzież tych aut, zlecenia, zasoby i dostarczanie leku przedłużające globalny czas życia siostry. Walka pozostaje nieustalona. Nie audytowano ponownie całego C++ Unreal; poniższe kontrakty można zachować między silnikami.

## Co zostało zmierzone

Narzędzie: [tests/performance_audit.gd](../tests/performance_audit.gd). Godot **4.7.2.stable.official.ed1daf0bf**, macOS, Apple M5, **OpenGL 4.1 Metal / Compatibility**. Bez VSync i bez sztucznego kroku `--fixed-fps`. Żaden test renderowania nie działał równolegle z innym.

Scena jest prawdziwa, ale pojazd jest zamrożony, a kamera przestawiana między trzema ustalonymi punktami: depot, smog i Eden. Każdy punkt odwiedzany jest dwukrotnie, z fazą przejścia 0,75 s i około 2 s pomiaru ustalonego kadru. HUD i atmosfera działają. Test nie mierzy lotu, dotyku, hamowania, smug express, przeglądarki ani termiki telefonu. Nie kasowano pamięci podręcznej sterownika. Odstępy czasu obejmują całą klatkę i pracę systemu; **nie są pomiarem samego GPU**. Krótkie próby nadają się do wykrycia dużych skoków i porównania liczby wywołań, nie do obiecywania konkretnego przyrostu FPS na Androidzie.

### Pierwsza i kolejna prezentacja — wariant bazowy 540 × 960

| Kadr | Maksymalny odstęp podczas pierwszego przejścia | Podczas drugiego przejścia | p95 klatki po drugim przejściu |
| --- | ---: | ---: | ---: |
| Depot | 617,75 ms, obejmuje uruchomienie renderowania sceny | 9,38 ms | 7,09 ms |
| Smog | 151,14 ms | 8,32 ms | 7,64 ms |
| Eden | 121,61 ms | 8,29 ms | 7,76 ms |

Powtórzenie bazowej próby z żądaniem większego okna dało analogiczny wzorzec: pierwsze przejścia smog/Eden **165,64/119,78 ms**, kolejne **8,09/8,81 ms**. System ograniczył żądane 1080 × 1920 do rzeczywistego okna **1080 × 1880**; nie jest to emulacja ekranu S25+. Bazowy logiczny obszar UI nadal wynosił 540 × 960, skala 3D 1,0, MSAA 2×.

Skok przy natychmiastowej zmianie kadru nie dowodzi takiej samej długości przycięcia podczas płynnego lotu. Dowodzi kosztu pierwszej prezentacji istniejącego już w pamięci fragmentu sceny. Kompilacja wariantów shaderów lub przygotowanie zasobów graficznych jest mocną hipotezą; bez śladu sterownika nie można przypisać całego czasu jednej operacji.

### Porównania wariantów — 540 × 960, drugi przebieg

Poniżej mediana licznika `Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME`. Obejmuje raportowane przez silnik rysowanie, w tym dodatkowe przebiegi; liczba obiektów w scenie nie jest liczbą wywołań na ekranie.

| Wariant diagnostyczny | Depot | Smog | Eden |
| --- | ---: | ---: | ---: |
| Baza | 911 | 736 | 501 |
| Bez cieni wszystkich świateł | 202 | 165 | 153 |
| Bez cieni samych reflektorów | 911 | 727 | 501 |
| Bez trzech geometrycznych warstw smogu | 911 | 733 | 501 |
| Bez SSAO i glow | 911 | 736 | 501 |

**Największa nadwyżka wywołań pochodzi z cieni światła kierunkowego.** Wyłączenie wszystkich cieni obniża liczbę wywołań przy depocie o około 78%. To nie oznacza 78% krótszej klatki. Mediana jej czasu w tej próbie spadła z 5,26 do 3,45 ms, natomiast p95 z 7,09 do 6,74 ms; rozrzut i koszty systemu nadal są istotne. Nie zalecamy usunięcia wszystkich cieni z gotowej gry — próba wyznacza pole do selektywnej optymalizacji.

Same reflektory dokładają w badanym kadrze tylko dziewięć wywołań cieni. Ich lokalny koszt nie uzasadnia uznania ich za głównego winowajcę. Usunięcie warstw smogu również **nie usunęło pierwszego przycięcia**: pierwsze wejście w dolny kadr nadal osiągnęło około 160 ms. Efekty ekranowe mogą kosztować czas mimo braku zmiany tego licznika; równe liczby wywołań nie oznaczają równego kosztu.

Surowe wyniki i logi znajdują się w ignorowanym `build/performance-*.json` i `build/performance-*.log`. Narzędzie zapisuje p50/p95/p99, maksimum, liczbę klatek ponad 50/100 ms, liczbę wywołań i prymitywów, parametry okna oraz inwentarz. Poprawiono przypisywanie interwału przecinającego granicę 0,75 s do fazy przejścia; tę sytuację zweryfikowano ponowną próbą `no-post`. Tabele bazowe zachowują wyniki pierwotnych pomiarów, bez usuwania niekorzystnych próbek.

## Ustalenia według priorytetu

P1: przed kolejnym pokazem ocenianym na telefonie. P2: przed dodaniem systemu, którego problem dotyczy. Priorytet nie oznacza, że każda hipoteza jest potwierdzonym źródłem zgłoszonego zatrzymania.

### P1 — Brakuje przygotowania pierwszego użycia grafiki

**Dowód:** `scenes/flight_lab.tscn:55` instancjonuje całe miasto na starcie. W skryptach gry nie ma `ResourceLoader`, żądań HTTP ani ładowania nowych scen podczas lotu. Generator miasta nie uruchamia się przy Play. Reflektory, płomienie i smugi są już instancjami, lecz stają się widoczne dopiero zależnie od stanu: `scripts/cab_lights.gd:31–38`, `scripts/cab_flight_fx.gd:30–34,60,81`. Nie ma etapu renderowania reprezentatywnych materiałów/efektów przed rozpoczęciem gry. Pomiar pierwszego i drugiego odwiedzenia pokazuje duży koszt pierwszej prezentacji także poza mgłą.

**Znaczenie:** wczytanie zasobu do pamięci nie gwarantuje przygotowania wszystkich operacji potrzebnych do narysowania go. Compatibility korzysta z innej ścieżki niż Forward+/Mobile; dokumentacja zaleca wcześniejsze faktyczne pokazanie materiałów i efektów przed kamerą. Samo `preload()`, ukryty węzeł albo obiekt poza kadrem nie realizuje tego celu. [Godot: kompilacja shaderów w Compatibility](https://docs.godotengine.org/en/4.6/tutorials/performance/pipeline_compilations.html).

**Zalecenie:** krótki ekran startowy zasłaniający scenę, podczas którego renderują się reprezentatywne kombinacje materiałów, świateł, cieni, smogu, płomieni i express. Zrobić to w docelowym viewportcie i ustawieniach jakości; nie tworzyć kombinacji wszystkich uniformów, bo różna wartość koloru nie oznacza automatycznie nowego shadera. Dopiero potem reset stanu próby i oddanie sterowania. Powrót z tła/utrata kontekstu graficznego wymagają osobnego testu. Nowe rodzaje NPC i pojazdów powinny dodawać swoje warianty do tego etapu. Przygotowanie grafiki trzeba zweryfikować na świeżym uruchomieniu Chrome na S25+; nie wystarczy lokalny wynik po rozgrzaniu.

### P1 — Za dużo drobnych obiektów uczestniczy w dynamicznych cieniach

**Dowód:** samo `city.tscn` zawiera **2390 węzłów**, w tym **2090 MeshInstance3D, 137 Label3D, 58 OmniLight3D i 39 brył statycznych**. W uruchomionej całej scenie **2104 elementy MeshInstance3D mają włączone rzucanie cieni**. Miasto wyłącza je jawnie jedynie na dziewięciu elementach pasów i smogu. `tools/build_city.py:45–58,70–76` pozostawia domyślne cienie także na wentylacji, podziałach, cienkich listwach, rurach i neonach. `scenes/flight_lab.tscn:46–53` włącza cień kierunkowy o zasięgu 65 m. Wynik porównania: 911 → 202 wywołań przy depocie.

**Zalecenie:** oddzielić geometrię widoczną od uproszczonych brył rzucających cień. Zachować cień auta, tarasów i istotnych brył budynków; wyłączyć go na detalach, które nie zmieniają czytelnie obrazu. Powtarzalne detale łączyć materiałami w małych sektorach albo grupować w `MultiMesh`. Nie tworzyć jednego `MultiMesh` na całe miasto: pogorszyłoby to odrzucanie niewidocznych fragmentów. Wydzielić sektory również pionowo, zgodnie z wąskim kadrem lotu.

Godot wykonuje automatyczne odrzucanie obiektów spoza kadru; scena nie rysuje wszystkich 2090 elementów naraz. Jednak współdzielenie zasobów nie daje automatycznego instancingu w Compatibility. Obecny generator dobrze współdzieli zasoby, a mimo tego pozostaje koszt oddzielnych węzłów i przebiegów cieni. [Godot: optymalizacja scen 3D](https://docs.godotengine.org/en/latest/tutorials/performance/optimizing_3d_performance.html).

### P1 — Rozdzielczość renderowania nie ma mobilnego budżetu

**Dowód:** `project.godot:20–24` ustawia bazę 540 × 960 i `canvas_items`, a `export_presets.cfg:40` automatycznie dopasowuje canvas do okna. Lokalny wygenerowany `build/web/index.js` oblicza jego wymiary jako `window.innerWidth/innerHeight × devicePixelRatio` przy włączonym HiDPI. Skala 3D nie jest ograniczana przez kod gry. Sama wpisana baza 540 × 960 nie jest limitem liczby pikseli.

**Znaczenie:** wzrost obu wymiarów obrazu dwa razy oznacza cztery razy więcej pikseli. Na telefonie dotyczy to również przezroczystości, wygładzania i efektów ekranowych. Nie zakładamy konkretnej rozdzielczości ani DPR sesji autora — potrzebny jest odczyt z działającego eksportu.

**Zalecenie:** profil jakości Web z limitem budżetu pikseli 3D, przy zachowaniu ostrego UI i obecnego multitouch. Początkowo przetestować skalowanie świata w zakresie 0,5–1,0 i obrazy około 540 × 960 / 720 × 1280, zachowując rzeczywiste proporcje telefonu. To propozycje do porównania, nie ustalony wygląd. Profile mogą regulować cienie, SSAO, glow, MSAA i liczbę lokalnych świateł. Zmianę kosztownych wariantów grafiki wykonywać przy ładowaniu; późniejsze automatyczne skalowanie rozdzielczości powinno reagować powoli, z histerezą. [Godot: bazowy rozmiar i skalowanie 3D niezależnie od UI](https://docs.godotengine.org/en/latest/tutorials/rendering/multiple_resolutions.html).

### P1/P2 — Przezroczystości i lokalne światła wymagają budżetu dla wielu aut

**Dowód:** `tools/build_city.py:287–292` tworzy trzy warstwy smogu po **174 × 64 m**. `shaders/smog_bank.gdshader:10–21` oblicza dwa poziomy szumu proceduralnego dla piksela, z przezroczystością i bez zapisu głębokości. Przednia warstwa może zasłaniać duży obszar ekranu. Pasy także wykorzystują przezroczyste quady. Do 58 miejskich świateł dochodzą dwa reflektory i jedno światło rozproszone auta.

**Zalecenie:** sprawdzić na telefonie wariant jednej/dwóch warstw, tańszy szum z tekstury i renderowanie przy mniejszej skali 3D. Zachować kontrast i czytelność LowLife; pomiary nie uzasadniają usuwania mgły. Statyczne światła mają już wygaszanie z odległością (`build_city.py:64–65`) — to warto zachować. Zdefiniować wspólny limit realnych świateł w kadrze: pobliski aktywny pojazd może mieć bogaty efekt, odległe auta emisyjne lampy i uproszczone snopy. Nie kopiować pełnego zestawu dwóch dynamicznych cieni na każdego NPC. Materiały i geometria współdzielone, stan efektu prywatny dla auta.

SSAO i glow są włączone w scenie. Dokumentacja obecnej gałęzi silnika wymienia ich obsługę w Compatibility, a glow ma własną implementację; nie należy opierać audytu na dawnym założeniu, że wszystkie te opcje są ignorowane. Dokładny koszt trzeba mierzyć w użytym eksporcie. [Godot: zestawienie rendererów](https://docs.godotengine.org/en/latest/tutorials/rendering/renderers.html). Dokumentacja `latest` może opisywać nowszy stan — rozstrzygający dla projektu jest uruchomiony silnik 4.7.2.

### P2 — Pojazd, aktywny kierowca i prezentacja są połączone na sztywno

**Dowód:** `scripts/flight_lab.gd:3–5,41–59` steruje zawsze `$Cab`, śledzi go kamerą i zasila HUD jego stanem. Atmosfera odszukuje węzeł o nazwie `Cab` (`city_atmosphere.gd:4`). `cab.gd` łączy integrację, paliwo, tankowanie, kontakt z podłożem, autopilota, wykrywanie autostrad, reset i przechył modelu. `FlightTuning` zawiera równocześnie parametry auta, kamery i granic całego miasta.

**Skutek przy rozbudowie:** łatwo powstaną dwa rozbieżne typy aut, oddzielnie dla NPC i gracza; kradzież będzie wtedy wymagała zastępowania obiektu lub kopiowania stanu. HUD/kamera/atmosfera mogą nadal śledzić stare auto. Ukrywanie zaparkowanego auta nie wyłącza jego `_physics_process`, animacji dysz ani pracy świateł.

**Zalecenie:** przed NPC wydzielić sesję gracza, źródło poleceń i stan pojazdu. Wszystkie auta powinny przyjmować ten sam kontrakt polecenia. Posiadacz, aktualny kierowca i pasażerowie są odrębnymi relacjami. Kradzież zmienia kierowcę/kontroler tego samego pojazdu, zachowując jego identyfikator, pozycję, prędkość, paliwo i użytkowników. Odebranie kontroli NPC musi anulować jego komendy i rezerwację podróży, a następnie uruchomić odpowiednią reakcję. Po oddaniu sterowania nadal obowiązuje sprawdzona reguła puszczenia trzymanych przycisków.

### P2 — Stan świata jest odczytywany z drzewa i powielonych współrzędnych

**Dowód:** auto zapisuje listę autostrad raz w `_ready()` (`cab.gd:40`), a HUD raz pobiera autostrady i lądowiska (`flight_controls.gd:44–48`). Nowe sektory dodane później nie trafią do tych list. Nazwy dzielnic i granice są zakodowane ponownie w HUD (`flight_controls.gd:50–62,231–257`), podczas gdy generator i tuning mają własne dane. Generator nadpisuje całą scenę; ręczna edycja i regeneracja nie mają wspólnego źródła zmian.

**Zalecenie:** `CityDefinition` i stabilne identyfikatory dzielnic, tarasów, drzwi oraz odcinków tras. Rejestr świata zgłasza rejestrację/usunięcie sektora i udostępnia dane stref, usług i tras. Graf przejść pieszych i lotów opisuje logiczne połączenia, punkty wejścia/wyjścia i rezerwację miejsca. Taras otrzymuje jawne usługi; etykieta „FUEL” lub grupa testowa nie zastępuje ekonomii kampanii. HUD korzysta z tych samych danych, co rozgrywka.

Sektory najpierw służą organizacji i ograniczaniu kosztu, nie obowiązkowemu doczytywaniu z dysku. Aktualna scena miasta zajmuje około **481 KiB**, a lokalny plik gry `index.pck` **328 KiB**; duży `index.wasm` ma około **37,7 MiB** i dotyczy głównie uruchomienia silnika. To nie jest uzasadnienie wprowadzania teraz złożonego streamingu zasobów. Rozmiar PCK nie jest pomiarem pamięci RAM/VRAM po uruchomieniu.

### P2 — Testy poprawności nie są bramką wydajności

**Dowód:** dotychczasowe testy używają `--fixed-fps`, część działa headless. Są wartościowe dla reguł i interpolacji, ale nie mierzą realnej długości klatki. README słusznie zaznaczało to przy teście kamery. Brakuje powtarzalnej trasy mobilnej, identyfikatora publikacji i zapisu skoków czasu klatki.

**Zalecenie:** osobny test wydajności z czasem rzeczywistym i wynikami dla konkretnego urządzenia, przeglądarki, rozdzielczości i wersji paczki. Nie zastępować testów fizyki benchmarkiem. Bazowy profil lotu powinien zachować dotychczasowe sprawdzone sterowanie, interpolację i zerowe spalanie podczas postoju.

## Docelowy podział odpowiedzialności

Poniżej projekt kontraktów, **nie wykaz wdrożonych systemów**. Wystarczą zasoby danych, niewielkie komponenty i jawne sygnały. Pełny ECS, rozbudowany framework i globalna magistrala wszystkich zmian nie są wymagane.

| Element | Odpowiada za | Ważna granica |
| --- | --- | --- |
| `GameSession` / `CampaignState` | Czas siostry, zasoby Ariego, postęp zleceń, zapis | Żyje niezależnie od auta, HUD i wczytania sektora |
| `PlayerSession` | Ari, aktywne ciało/pojazd, przejęcie i zwrot kontroli | Kamera i interfejs subskrybują zmianę celu |
| `VehicleState` + silnik lotu | ID, paliwo, ruch, kontakt i wykonanie polecenia | Ten sam pojazd i model prowadzenia dla gracza i NPC |
| Kontroler gracza / ruchu / powrotu | Wytworzenie polecenia z wejścia lub trasy | Jeden rozstrzygnięty autor polecenia na krok fizyki |
| `VehiclePresentation` | Model, światła, dysze, smugi, przyszły dźwięk | Czyta stan; nie zmienia sił, paliwa ani kampanii |
| `WorldRegistry` + `CityDefinition` | Sektory, strefy lotu, tarasy, usługi, graf tras | Brak ukrytej zależności od nazwy `$Cab` lub listy z pierwszego `_ready()` |
| `PopulationDirector` | Cykl NPC, aktywacja, rezerwacje auta i miejsca | Oddziela logiczną podróż od kosztownej fizycznej reprezentacji |
| Zlecenia i interakcje | Oferty, warunki, skutki działań Ariego | Korzystają z ID i zdarzeń domenowych, nie z napisów HUD |

Przykładowy przepływ: kontroler dostarcza polecenie → arbitraż kontroli wybiera kierowcę/powrót → pojazd wykonuje lot i rozlicza paliwo → prezentacja odczytuje wynik. Zdarzenia takie jak `vehicle_control_changed`, `trip_completed`, `medicine_delivered` wystarczają do powiadomień; nie wysyłać całego stanu świata globalnie co klatkę.

**NPC:** pierwszy etap to mała populacja i pełny cykl: dojście → rezerwacja auta → wejście → lot → lądowanie → wyjście. Blisko Ariego pełna symulacja i interakcje; dalej uproszczona podróż po trasie; poza aktywnym obszarem przede wszystkim stan logiczny. Orientacyjne częstotliwości decyzji 5–10 Hz i odległej symulacji 1–2 Hz są propozycją do pomiaru. Fizyczny ruch aktywnych aut pozostaje płynny przy kroku 60 Hz. Nie wyszukiwać pełnej trasy dla każdego NPC w każdej klatce i nie włączać wszystkich postaci naraz na granicy sektora.

Przełączanie reprezentacji musi zachowywać ID, zajętość miejsc, kierowcę, pasażerów i reakcję na kradzież. Obiekt obserwowany lub biorący udział w zadaniu nie może zniknąć tylko dlatego, że wyszedł o kilka pikseli poza ekran. Granice aktywacji powinny mieć zapas i histerezę.

**Kampania:** czas siostry jest stanem sesji, nie lokalnym timerem sceny z siostrą ani HUD. Dopiero poprawnie rozliczona dostawa dawki przedłuża odliczanie; zakup i zarobek nie wystarczają. Jedna dostawa musi być rozliczona dokładnie raz, także po wczytaniu zapisu. Zapis potrzebuje wersji schematu, stabilnych ID i stanu własności/obsady aut. Zmiana pojazdu, przeładowanie sektora lub odtworzenie grafiki nie resetują pieniędzy ani czasu. Zachowanie czasu podczas pauzy, dialogu, ukrycia karty i po zamknięciu gry wymaga decyzji autora; nie zakładamy upływu czasu offline. Techniczny powrót aplikacji nie może niejawnie doliczyć całego czasu nieaktywnej karty.

## Kolejność prac i kryteria zakończenia

1. **Stabilny pokaz na telefonie.** Dodać odczyt numeru paczki, rzeczywistego canvas/DPR/skali 3D i czasów klatek, przygotowanie pierwszego użycia grafiki oraz profil jakości Web. Najpierw ograniczyć rzucanie cieni przez detale i zmierzyć warianty; potem grupować powtarzalną geometrię w sektorach. Zachować wygląd porównując te same kadry. Nie zmieniać na ślepo fizyki ani liczby jej kroków.
2. **Kontrakt auta i przejęcia.** Odłączyć kontroler i cel kamery/HUD od `$Cab`, wydzielić konfigurację świata oraz rejestr obiektów. Próba z dwoma autami: przejęcie auta NPC zachowuje stan; poprzedni kontroler natychmiast przestaje nim sterować; kamera, HUD, reflektory i atmosfera podążają za aktualnym celem.
3. **Żyjący fragment miasta.** Wpiąć małą populację w graf tarasów i tras, z rezerwacjami i uproszczonym stanem poza okolicą gracza. Potwierdzić pełny cykl życia i widoczny skutek kradzieży, a następnie ponownie mierzyć telefon. Liczbę mieszkańców zwiększać na podstawie wyników.
4. **Wycinek kampanii.** Zlecenie → zarobek → zdobycie dawki → dostarczenie → wydłużenie globalnego czasu; zapis i ponowne uruchomienie zachowują stan. Testować również przypadek śmierci siostry/wyłączenia Ariego. Nie dodawać walki jako domniemanego wymogu tej przebudowy.

Proponowany cel techniczny do oceny: stabilne **60 FPS**, czyli budżet około **16,7 ms/klatkę**, z mierzalnym zapasem na NPC. Jeśli wybrany profil urządzeń tego nie utrzymuje, świadomie ustalić profil 30 FPS zamiast tolerować niekontrolowane wahania. Dla profilu 60 FPS mierzyć p95 ≤ 16,7 ms i p99 ≤ 25 ms oraz osobno wszystkie zatrzymania ponad 50/100 ms; to cele robocze, nie osiągnięte wyniki mobilne. Na trasie po przygotowaniu grafiki żadna klatka powyżej 100 ms nie powinna pozostać bez wyjaśnienia.

Trasa telefonu: świeże uruchomienie → start z depotu → wejście i wyjście ze smogu z reflektorami → express → górne dzielnice → te same odcinki ponownie → minimum 10 minut gry dla sprawdzenia nagrzania. Osobno pełny ekran/iframe itch.io, zmiana rozmiaru, utrata i odzyskanie fokusu, szybkie przejęcie pojazdu, gdy zostanie wdrożone. Zapisywać zimne pierwsze przejście i ciepłe powtórzenia oddzielnie, bez ukrywania skoków za średnim FPS.

Web pozostaje obecnie jednowątkowy (`variant/thread_support=false`). Wartości puli 8/4 nie włączają wątków. Przełączenie na wielowątkowość nie jest automatyczną poprawką na koszty GPU i wymaga sprawdzenia izolacji przeglądarki oraz hostingu. Jednowątkowy eksport jest wspieranym rozwiązaniem dla itch.io. [Godot: eksport Web](https://docs.godotengine.org/en/latest/tutorials/export/exporting_for_web.html).

## Odtworzenie audytu lokalnie

Z katalogu prototypu, z zainstalowanym Godotem wskazywanym przez istniejący skrypt środowiska:

```bash
source tools/godot-env.sh
"$FC_GODOT_BIN" --path "$FC_PROJECT_DIR" --resolution 540x960 --disable-vsync \
  --script tests/performance_audit.gd --log-file "$FC_PROJECT_DIR/build/performance-baseline.log" \
  -- --variant=baseline --seconds=2 --output=res://build/performance-baseline.json
```

Pozostałe warianty: `no-shadows`, `no-headlight-shadows`, `no-smog-banks`, `no-post`. Pomocniczo dostępne są `half-resolution` i `no-local-lights`; nie były podstawą wniosków liczbowych w tym raporcie. Każdy wariant uruchamiać osobno i zapisywać do odrębnego pliku. `inventory` może działać z `--headless` i nie generuje pomiaru wydajności. Narzędzie odrzuca próbę mierzenia klatek headless. **Nie dodawać `--fixed-fps`**: silnik zużywa tę flagę przed udostępnieniem argumentów skryptowi, więc skrypt nie potrafi jej wykryć przez `OS.get_cmdline_args()`; potwierdzono to osobną próbą. Używać powyższego polecenia, nie wywołania z dotychczasowego `verify.sh`.

Wywołania benchmarku **nie zmieniają zapisanych ustawień ani scen gry**. Zmiany istnieją tylko w testowym procesie. Skrypt znajduje się w katalogu `tests/`, który obecny eksport wyklucza. Dotychczasowe testy lotu pozostają właściwymi testami zachowania; ten audyt nie deklaruje ich ponownego pełnego wykonania, ponieważ nie zmieniał kodu gry.

Identyfikacja lokalnego stanu z pomiaru, SHA-256:

| Plik | SHA-256 |
| --- | --- |
| `project.godot` | `d6ecc1b17d3e49779d01e92bcd715c01cbb730630856895cf0acf0e2d074456b` |
| `scenes/city.tscn` | `480d18a4faf1b8800f8af2a00c3cfa488d752adb3b6c8a2d682a254f87c52e1f` |
| `scenes/flight_lab.tscn` | `7e2add5f7c4a7815fade78146eb0f6f92e883705b6b011a9971d27356ba7b2ad` |
| lokalne `build/web/index.pck` | `4a353aec4c7f2f0962726e4900b7c9105722a73a30af9172faa8118713c6a538` |

Raport i narzędzie są wynikiem przeglądu. Optymalizacje, nowe komponenty, przygotowanie shaderów przed grą i telemetria telefonu opisane wyżej pozostają zadaniami do wdrożenia. Żadna z tych propozycji nie jest deklarowana jako ukończona poprawka zgłoszonego problemu.
