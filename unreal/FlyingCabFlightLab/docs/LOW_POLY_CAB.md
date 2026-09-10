# Prosty model pojazdu — MVP

Model `Content/Vehicles/LowPolyCab/SM_LowPolyCab.uasset` zastępuje kostkę wizualną. Ma 48 trójkątów: kanciastą karoserię, trapezową kabinę oraz płaskie oznaczenia świateł. Niska, długa maska i jasne lampy oznaczają przód (+X); krótki, pionowy tył ma czerwone lampy. Szyby są ciemne i nieprzezroczyste. Brak tekstur i animowanego szkieletu.

Bryła mieści się w dotychczasowych 220 × 90 × 70 cm; płaskie lampy wystają o 0,15 cm. Istniejący `CollisionBody` pozostaje bez zmian i jest jedyną kolizją pojazdu. Materiał karoserii przyjmuje dotychczasowy parametr `Color`, więc kolory floty i sygnalizacja uszkodzeń nadal sterują lakierem.

`ApplyVisualBody()` podmienia siatkę podczas konstrukcji aktora i na początku gry, również dla istniejącego Blueprintu. Referencja miękka uwzględnia zasób przy gotowaniu. Domyślna kostka pozostaje tylko awaryjnym zasobem, gdy model jest niedostępny.

Przy locie w lewo odwracana jest wyłącznie skala X siatki wizualnej. Kierunek wynika z prędkości, a poniżej 5 cm/s z już zastosowanej siły poziomej. Bez nowego wejścia, obrotu fizycznego nadwozia ani zmiany dotychczasowego pochylenia. Osie, położenie i zachowanie strumieni pozostają nienaruszone.

Źródła OBJ/MTL znajdują się w `Build/LowPolyCab`. Generator `scripts/Build-LowPolyCab.py` uruchomiony przez Python z `--source-only` odtwarza geometrię; uruchomiony jako skrypt edytorowy Unreal importuje siatkę i cztery materiały do `/Game/Vehicles/LowPolyCab`. Nie zapisuje map ani Blueprintów.

Zakres weryfikacji: import zasobów, przegląd zmian i kompilacja. Zgodnie z decyzją użytkownika testy w grze i ocena wizualna należą do niego; nie renderujemy podglądów.

## Kolory i pojazdy NPC

`AFlyingCabTrafficVehicle` korzysta z tej samej siatki i materiałów. Skala dopasowuje ją do istniejących gabarytów ruchu NPC (260 × 100 × 84 cm). Odwracana jest tylko skala X bryły, zgodnie z kierunkiem trasy. Trasy, postoje, prędkości, czujniki przeszkód i kolizje pozostają bez zmian.

Oba generatory ruchu miejskiego przydzielają kolejno sześć lakierów z `FlyingCabVehiclePaint.h`: musztardowy `#E5B849`, koralowy `#D8644C`, morski `#42969A`, niebieski `#536C9D`, wiśniowy `#914B69` i kremowy `#D8D3BF`. Paleta zastępuje poprzednie kolory tras przy tworzeniu standardowego ruchu. Warianty zmieniają tylko parametr materiału karoserii; szyby i lampy zachowują własne kolory, a siatka jest współdzielona. Bez kopiowania geometrii i tworzenia nowych shaderów.

Obwódka `PlayerFocusHalo` i światło `PlayerFocusLight` są stale niewidoczne, z zerową mocą światła. Ich komponenty pozostają nieaktywne dla zgodności z istniejącym Blueprintem. Strumienie i światła samochodu nie są częścią tego oznaczenia.
