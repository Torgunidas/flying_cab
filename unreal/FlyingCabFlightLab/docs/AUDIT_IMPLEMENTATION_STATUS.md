# Status wdrożenia audytu architektury

Dokument towarzyszy raportowi `AUDYT_UNREAL_FLYINGCABFLIGHTLAB_2026-08-10.md`. Raport pozostaje historycznym opisem stanu z dnia audytu; poniższa tabela opisuje aktualny projekt po kolejnych etapach refaktoryzacji.

## Ustalenia

| ID | Status | Stan obecny |
|---|---|---|
| F-01 | zakończone | Komunikaty gracza trafiają do trwałego panelu HUD przez `ShowEventMessage`; debug overlay pozostał wyłącznie dla telemetrii `F3`. |
| F-02 | zakończone | `UFlyingCabFleetComponent` rejestruje każdy pojazd i rozróżnia recovery pojazdu aktywnego od zaparkowanego. |
| F-03 | zakończone | Interactables i pojazdy są odkrywane do cache co 1 s, a kontekst interakcji odświeżany co 0,2 s zamiast skanowania świata co klatkę. |
| F-04 | zakończone | World bootstrap, dispatch, ekonomia, fleet, run/save, HUD, traffic awareness i vitals zostały wydzielone z GameMode do wyspecjalizowanych komponentów/aktora. |
| F-05 | zakończone | Rozgrywka używa assetów Enhanced Input z `Content/Input`; legacy mappings zostały usunięte. Focus flush synchronicznie czyści także stan przechowywany przez Pawna. |
| F-06 | zakończone | Granica mapy używa tagu `EastBoundary`; heurystyka pozostała wyłącznie jako głośno logowany fallback dla starych map. |
| F-07 | zakończone | Dzielnice, stacje, trasy i bounds minimapy mają jedno źródło w `UFlyingCabCityLayoutAsset`. |
| F-08 | zakończone | HUD jest próbkowany timerem 10 Hz, a widgety porównują nowe wartości przed `SetText`/zmianą widoczności. |
| F-09 | oczekuje pomiaru | Nie zmieniono stylistyki świateł bez danych. Potrzebny jest profil GPU na fizycznym urządzeniu mobilnym i test A/B lokalnych świateł zgodnie z raportem. |
| F-10 | zakończone | PlayerController tworzy i posiada jeden trwały HUD niezależny od aktualnie posiadanego pojazdu lub postaci. |
| F-11 | zakończone | Pakiet Automation obejmuje logikę domenową i przepływy PIE; zawiera 19 testów (13 Core + 6 Functional PIE). |
| F-12 | zakończone | `UFlyingCabScoreSaveGame` zawiera jawne `SaveVersion`. |
| F-13 | zakończone | `FLIGHT_FEEL_TEST.md` opisuje aktualne miasto, tryby, flotę, HUD, Enhanced Input i zatwierdzony kadr kamery. |
| F-14 | zakończone | Menu otrzymuje skonfigurowany próg Time Attack; wartość pochodzi z assetu ekonomii. |
| F-15 | zakończone | Time Attack używa stałego seedu, a Freeroam rozpoczyna nową losową sekwencję ofert. |
| F-16 | zakończone | Stacje używają zdarzeń Begin/End Overlap i tickują tylko podczas obecności obsługiwanego pojazdu. |
| F-17 | zakończone | Look-ahead kamery jest zwykłym polem danych `CameraTrackingOffset`; Pawn nie posiada pozornej kamery ani SpringArma. |
| F-18 | świadomie odłożone | Pełna lokalizacja tekstów pozostaje etapem stabilizacji UX. Nowy tekst gracza powinien być tworzony jako `FText`, bez utrwalania logiki na porównaniach z przetłumaczonym tekstem. |
| F-19 | zakończone | Stan projektu i każdy etap audytu są zapisane w osobnych commitach na gałęzi `codex/unreal-audit-fixes`. |

## Otwarte prace

1. F-09 wymaga urządzenia mobilnego; wyniku nie da się wiarygodnie zastąpić testem desktopowym ani `NullRHI`.
2. F-18 należy rozpocząć dopiero po ustabilizowaniu treści i języków docelowych.
3. Etap 5 nie oznacza automatycznego portowania dalszych funkcji Godota. Kontrakty i kolejność decyzji znajdują się w `GODOT_MIGRATION_CONTRACTS.md`.
