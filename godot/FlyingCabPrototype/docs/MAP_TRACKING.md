# Śledzenie punktu na mapie

Wdrożenie: 2026-09-16.

- Krótki klik / dotknięcie platformy lub questhubu zachowuje podgląd nazwy i informacji.
- Przytrzymanie przez 0,65 s przełącza śledzenie. Zielony pasek obiega ikonę zgodnie z ruchem wskazówek zegara, od góry. Pełny okrąg zatwierdza akcję raz na przytrzymanie.
- Kolejne przytrzymanie tego samego punktu wyłącza śledzenie; innego punktu zastępuje cel. Śledzony punkt ma zieloną obwódkę.
- Zwolnienie przed końcem, odsunięcie palca/kursora, utrata fokusu, zmiana rozmiaru i zamknięcie mapy anulują nieukończone przytrzymanie. Drugi palec nie przejmuje gestu.
- Zielona strzałka korzysta z tego samego kształtu, projekcji i położenia co strzałka kursu. Obie mogą się nakładać. Śledzenie działa w całym mieście, w aucie i pieszo; wskazuje bezpośredni kierunek do punktu, bez wyznaczania trasy. Mapa, opcje i dialogi ukrywają strzałki.
- Questhub bez dostępnego zadania zachowuje znacznik śledzonej lokalizacji do ręcznego wyłączenia. Śledzona jest pozycja punktu wybrana na mapie.
- Zaznaczenie pozostaje po zamknięciu mapy i zmianie pojazdu. Jest lokalnym stanem aktualnie wczytanej mapy, bez zapisu do autosave; ponowne wczytanie poziomu je zeruje.

Implementacja: `scripts/taxi/city_reference_map.gd`, `taxi_hud.gd`, `taxi_guidance_arrow.gd`. Testy: `bash tools/verify.sh map-tracking`, `map-tracking-render`, `quest-map`, `taxi-guidance`, `taxi-ui`. Wariant graficzny zapisuje ujęcia pierścienia, obu strzałek i mapy w pionie oraz poziomie w `build/tracking-*.png`.
