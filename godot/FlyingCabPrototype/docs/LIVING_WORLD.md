# Living world — pierwsza grywalna populacja

Wdrożenie 2026-09-13 w Godot 4.7.2. Domyślna gra i F6 poziomu tworzą populację przed rozpoczęciem rozgrywki. Nie trzeba otwierać sceny przeglądowej pojazdów ani włączać trybu debugowania.

## Aktualizacja 2026-09-16

Środkowa pionowa autostrada ma ruch prawostronny: wznoszenie po X = 3,5, opadanie po X = −2,5. Jej połączenia z obwodnicą używają górnego pasa Y = 306,5 dla ruchu w lewo i dolnego Y = 13,5 dla ruchu w prawo. Pozostałe autostrady zachowują poprawne strony. Zmieniono zasób tras, bez przebudowy geometrii miasta. ID aut i indeksy punktów pozostają zgodne; samochody ze starszego zapisu dołączają do nowych punktów swojej pętli.

Wsiadanie/przejęcie wymaga teraz stania przy kabinie kierowcy, z uwzględnieniem obrócenia modelu. Wysiadanie działa również w locie, a upadek może zranić lub zabić Ariego. Szczegóły i zapis zdrowia: [tryb pieszy](ON_FOOT.md).

## Zawartość miasta

| Populacja w nowej grze | Liczba | Zachowanie |
| --- | ---: | --- |
| Auta na autostradach | 30 | Dwa przeciwne obiegi zewnętrzne, środkowa oś i poprzeczna autostrada |
| Auta w dzielnicach | 8 | Po dwa na pętli Velvet, Foundry, Eden i Aurelia |
| Mieszkańcy z regularną podróżą | 4 | Budynek → pieszo do auta → wejście → lot → lądowanie → wyjście → budynek; potem podróż powrotna |
| Zaparkowane auta i ich właściciele | 8 | Właściciel spaceruje i odwiedza budynek; auto pozostaje do przejęcia |
| Niezależni spacerowicze | 25 | Po jednym na każdej platformie, pomiędzy budynkiem a krawędzią tarasu |

Łącznie w populacji: **50 aut NPC**, własny cab Ariego i **37 reprezentacji pieszych living world**. Dodatkowo osobna platforma **FOUNDRY / TEST FLEET** zawiera 12 aut do ręcznych prób, czyli cała mapa ma obecnie 63 pojazdy. [Położenie i instrukcja](CITY_02.md#platforma-testowa-foundry--2026-09-13). Kierowcy są ukryci, kiedy siedzą w samochodzie lub przebywają w budynku. Niezależnie działa dotychczasowa pula pasażerów taxi: nadal najwyżej **jeden oczekujący na kurs na platformie**. Spacerowicz i właściciel nie są dodatkowym zleceniem taxi.

Ruch wykorzystuje lorry, supercar, limousine, luxury car, police car, poor car, tow car oraz trzy kolory normal car. Na autostradach prędkość docelowa wynosi 11,5–12,5 jednostki/s, w dzielnicach 6, w podróży mieszkańca 5; podejście do lądowania zwalnia do 1,3. Piesi idą 1,5 jednostki/s, z animacją zależną od rzeczywistego dystansu.

Regularne podróże: Depot ↔ Velvet Club, Foundry Market ↔ Foundry Docks, Eden 01 ↔ Eden 02, Aurelia 01 ↔ Aurelia 02. Auta pozostawione na dłużej stoją na Velvet 03/04, Foundry 04/06, Eden 04/06 i Aurelia 04/06. Postój jest przy boku platformy, zostawiając miejsce na lądowanie taksówki.

## Ari i przejmowanie pojazdów

Ari chodzi na **Z = 1,2**, tak jak pasażerowie i mieszkańcy. Samochody latają na Z = 0. Ludzie nie blokują się wzajemnie i przechodzą przed zaparkowanymi autami bez skakania. Nadal zatrzymują ich ściany i podłoże; kolizje samochodów z samochodami oraz obrażenia pozostają aktywne.

Przy pustym, zaparkowanym aucie NPC pojawia się **PRZEJMIJ / Q**. Dotyk używa tego samego przycisku kontekstowego. Trzeba stać blisko wejścia; pojazd musi rzeczywiście spoczywać na platformie. Nie ma wyciągania kierowcy z lecącego auta. Można przejąć zarówno samochód stojący od początku, jak i auto mieszkańca po zakończeniu jego podróży.

Przejęcie zachowuje ten sam obiekt, model, zbiornik, uszkodzenia i identyfikator. `owner_id` nadal oznacza właściciela NPC, a `driver_id` zmienia się na Ariego. Właściciel traci plan odlotu, wraca do budynku i kontynuuje chodzenie; nie odzyskuje zdalnie kierownicy. Stan przejęcia jest trwały również po zapisie. Dotychczasowy samochód Ariego pozostaje w mieście.

Holowanie wybiera wolne miejsce w depocie z uwzględnieniem rozmiaru aktualnego modelu i innych aut. Gdy depot jest pełny, sprawdza pozostałe tarasy i podaje faktyczny cel w dymku. Przy braku wolnego miejsca nie pobiera opłaty. Nie teleportuje przejętego auta do zapisanej pozycji lotu ani do środka innego pojazdu.

## Podział odpowiedzialności i autorowanie

| Element | Odpowiedzialność |
| --- | --- |
| `LivingWorldProfile`, `LivingRoute`, `ResidentSchedule` | Edytowalne dane mapy, modele, kierunki pętli, liczebność i plany mieszkańców |
| `LivingWorldState` w `RuntimeContext` | Trwałe tożsamości, fazy planów, punkt trasy, postój, pozycje pieszych i przejęcie |
| `LivingWorldDirector` na mapie | Reprezentacje aut/ludzi, cykle podróży, wolne miejsca, kolejki skrzyżowań i podłączenie kontrolerów |
| `TrafficDriver` | Polecenie ciągu przez wspólny kontrakt kierowcy `FlightCab`, z hamowaniem przed celem |
| `OnFootInteraction` | Dostępność wejścia, kontekstowy przycisk i przekazanie kontroli przez `PlayerSession` |
| `WorldLayers` | Kategorie kolizji i wspólna płaszczyzna pieszych |

W Inspectorze otwórz **`resources/living_world/city_02.tres`**. Każda pętla ma `points`, `population`, `speed` i listę `models`. Każdy mieszkaniec ma `model_id`, `stops`, `berth_x`, `corridor_x`, `dwell_seconds`, wygląd i `parked_only`. Parametry fizyczne auta pozostają w jego `VehicleDefinition`, zgodnie z biblioteką pojazdów. Kontroler NPC nie ma osobnego modelu fizyki.

Punkty pętli są kierunkowe i zamykane automatycznie. Przeciwne pasy mają odstęp 5–6 m. Przed dziewięcioma skrzyżowaniami działa kolejka z wyłącznością przejazdu; auto zatrzymuje się również przed pojazdem wykrytym krótkim sprawdzeniem swojej pełnej bryły. Mieszkaniec czeka nad zajętym miejscem lądowania. Graf tras gracza i mapa odniesienia nie otrzymują z tego prowadzenia GPS.

Nową mapę można wyposażyć we własny profil, zarejestrować go w `RuntimeContext.living_profiles` i przypisać do jej dyrektora. IDs tras, mieszkańców i aut muszą być stabilne oraz unikalne w całej sesji. Dane zawierają identyfikatory, bez ścieżek do dowolnego skryptu z zapisu. Przed zwiększeniem liczebności lub zmianą tras trzeba sprawdzić fizyczną przestrzeń dla największego dopuszczonego modelu, postoje i skrzyżowania. Walidacja profilu wykrywa m.in. brak modelu/platformy, błędne punkty i auto wystające poza swój taras.

Pierwsze przygotowanie mapy tworzy populację tylko raz. Po edycji liczebności istniejący zapis zachowuje swoje wcześniej utworzone tożsamości; nową obsadę sprawdza się w nowej grze. Zmiana lub usunięcie używanego ID wymaga migracji zapisu. Przy migracji starszej gry NPC sprawdza miejsce zajmowane przez zapisane auto gracza i wybiera drugą stronę swojej platformy; jeśli wszystkie miejsca są zajęte, dany nowy mieszkaniec nie jest tworzony.

## Zapis i koszt działania

Schemat **4** zapisuje populację obok stanu wszystkich samochodów i Ariego. Schematy 1–3 nadal są obsługiwane, a stara pozycja pieszego na Z = 0 przechodzi na Z = 1,2. Wczytanie sprawdza dane przed zmianą sesji. Dynamiczne auta są odtwarzane przed przywróceniem kontroli gracza, dlatego zapis działa również za kierownicą samochodu NPC. Kolejki skrzyżowań są odtwarzane z rzeczywistych pozycji, bez teleportowania ruchu na początek pętli.

Populacja ma stałą wielkość: nie pojawiają się nowe auta podczas przelotu kamery. Modele, sylwetki i efekty są przygotowywane przed oddaniem kontroli. Wszystkie auta są na ten czas zamrożone. Menu zatrzymuje grę, a blokady sesji zatrzymują dodatkowo fizyczne auta NPC i ich harmonogramy; zwolnienie jednej z kilku blokad nie wznawia świata przedwcześnie.

Piesi mają wspólny lekki model i centralny krok symulacji, bez osobnego `CharacterBody3D` i bez wyszukiwania ścieżki dla każdego człowieka. Samochody pozostają rzeczywistymi ciałami fizycznymi, także poza kamerą. Obecny etap nie ma jeszcze różnych poziomów szczegółowości symulacji; zwiększanie populacji wymaga kolejnego pomiaru na telefonie.

**Celowe uproszczenia:** podczas prowadzenia przez NPC nie jest zużywany jego zapisany zbiornik paliwa; po przejęciu natychmiast wraca normalne spalanie. Nie jest to gotowa ekonomia paliwa mieszkańców ani system tankowania NPC. Zniszczone samochody nie odradzają się automatycznie. Brakuje usuwania wraków, omijania dowolnej blokady, policyjnego pościgu, walki, dialogowej reakcji na kradzież i grywalnych wnętrz. Wejście NPC do budynku oznacza ukrycie reprezentacji przy drzwiach. Laweta nadal ma tylko model i miejsce na ładunek.

## Weryfikacja

macOS / Godot 4.7.2: **777/777 kontroli** w tej iteracji.

| Zestaw | Wynik |
| --- | --- |
| Living world, dwie kradzieże, migracja, odtworzenie mapy i dalszy ruch | 82/82 |
| Living world z rendererem | 5/5 |
| Tryb pieszy z rendererem i pełnym wczytaniem | 12/12 |
| Start z przygotowaniem grafiki, profil balanced | 8/8 |
| Chód i tryb pieszy | 85/85 + 46/46 |
| Modele, masa, zderzenia i warsztaty | 178/178 + 45/45 |
| Lot i narastanie ciągu | 23/23 + 28/28 |
| Architektura i zapis | 41/41 |
| Taxi, interakcje, populacja platform, UI i strzałka celu | 39/39 + 55/55 + 61/61 + 21/21 + 48/48 |

`bash tools/verify.sh living-world` sprawdza całą mapę przez sześć minut symulacji, każdy obieg w drugiej połowie próby, pełne cykle czterech mieszkańców, przejście Ariego przez zaparkowane auta, blokady sesji, dwie kradzieże, ponowne utworzenie mapy z JSON i dalsze pięć minut ruchu, holowanie oraz migrację starszej gry. Izolowane testy taxi wyłączają niezależną populację; ten zestaw sprawdza ją wspólnie z platformami i pulą taxi.

`bash tools/verify.sh living-world-render` uruchamia zwykły punkt wejścia gry, kontroluje wznowienie aut po przygotowaniu grafiki i przejście Ariego przed samochodem. Zapisuje widoki depotu, autostrady i dzielnicy do `build/living-world-*.png`. Testy `vehicles` obejmują rzeczywiste zderzenie dwóch aut o różnych masach oraz oba warsztaty.

Czysty import w odizolowanej kopii: PASS, bez błędów skryptów. Test pełnego startu z profilem balanced: przygotowanie grafiki około 5,94 s.

Lokalna obserwacja z rendererem: macOS / Apple M5, Compatibility, 540 × 960, profil desktop, limit 60 FPS. W krótkiej próbce 590 klatek po przygotowaniu: p95 17,52 ms, p99 18,63 ms, maksimum 43,73 ms, bez klatki ponad 50 ms. To próba funkcjonalna kilku kadrów, **nie benchmark telefonu** ani gwarancja płynności całej rozgrywki. Windows, Web i Samsung S25+ wymagają sprawdzenia tej populacji. Nie przygotowano eksportu ani paczki itch.io.
