# Kontrakty dalszej migracji z Godota

Te kontrakty realizują Etap 5 audytu. Opisują zachowanie, które można świadomie odtworzyć, ale nie nakazują portu 1:1 ani kopiowania wartości wyrażonych w pikselach. Każda implementacja musi zachować aktualne granice odpowiedzialności i otrzymać test akceptacyjny przed rozbudową GameMode.

## Zasady wspólne

- Unreal używa centymetrów, aktualnego modelu fizyki i danych z assetów; wartości Godota są wyłącznie odniesieniem zachowania.
- Nowy stan domenowy trafia do istniejącego komponentu lub subsystemu, nie bezpośrednio do GameMode.
- Wejście korzysta z Enhanced Input. Nie wracamy do legacy mappings ani drugiego równoległego stanu klawiatury.
- Zmiany fizyki muszą być domyślnie wyłączalne i przejść pełną procedurę `FLIGHT_FEEL_TEST.md`.
- Funkcja nie jest ukończona bez automatycznego testu logiki i wskazanego testu manualnego, jeśli zachowanie jest wizualne.

## 1. Zdarzeniowy HUD — kontrakt zrealizowany

Stan docelowy jest już osiągnięty przez `UFlyingCabHudPresenterComponent`, timer interfejsu w PlayerControllerze i porównania wartości w `UFlyingCabTouchControls`.

- Właściciel UI: PlayerController.
- Źródła: komponenty domenowe i aktualnie sterowany Pawn/Character.
- Częstotliwość dynamicznych odczytów: maksymalnie 10 Hz, chyba że pojedyncze zdarzenie wymaga natychmiastowego komunikatu.
- Tekst i layout nie są ponownie ustawiane, gdy wartość prezentacyjna się nie zmieniła.
- Zmiana posiadanego obiektu nie tworzy ani nie przenosi widgetu.

Test akceptacyjny: przejście pojazd → pieszo → drugi pojazd zachowuje jeden HUD, aktualne zasoby i prawidłowe przyciski; `stat slate` nie pokazuje stałej inwalidacji tekstów co klatkę.

## 2. Trwałość stanu gry z wersją i kopią zapasową

Status: oczekuje decyzji produktowej o zakresie trwałości.

### Wejścia i wyjścia

- Wejście zapisu: jawna, typowana struktura stanu; co najmniej wersja, timestamp oraz wybrany zakres kredytów i licencji.
- Wyjście odczytu: zwalidowany stan bieżącej wersji albo bezpieczny stan domyślny.
- Leaderboard może pozostać osobnym, małym zapisem, dopóki pełny save nie zostanie zatwierdzony.

### Stany brzegowe

- Brak pliku nie jest błędem.
- Przed nadpisaniem poprawnego pliku powstaje jedna odzyskiwalna kopia zapasowa.
- Uszkodzony zapis główny próbuje odczytać backup; uszkodzenie obu nie może zatrzymać startu gry.
- Nieznana nowsza wersja nie jest interpretowana jak bieżąca.
- Migracje są jawne i monotoniczne; kod nie zgaduje brakujących pól na podstawie tekstów UI.

### Miejsce docelowe

Dedykowany `USaveGame` i warstwa koordynująca przy GameInstance/Run, bez serializacji w GameMode i bez refleksyjnych nazw właściwości.

Test akceptacyjny: zapis → ponowne uruchomienie → odtworzenie zatwierdzonych pól; następnie uszkodzenie pliku głównego → poprawny odczyt backupu; nieznana wersja → kontrolowana odmowa i stan domyślny.

## 3. Fizyczny pasażer NPC

Status: oczekuje decyzji, czy zastępuje obecną sylwetkę, czy jest wariantem jakościowym.

### Kontrakt zachowania

- Oferta może posiadać najwyżej jednego oczekującego pasażera.
- Oczekujący NPC należy do przepływu oferty; wygaśnięcie lub anulowanie oferty zawsze go usuwa.
- Podczas `CURBSIDE LINK` NPC porusza się w stronę rzeczywistej pozycji taksówki, niezależnie od strony podejścia.
- Po zakończeniu odbioru pasażer znika ze świata, a komponent dispatch pozostaje jedynym właścicielem stanu „na pokładzie”.
- Przy dropoffie NPC pojawia się przy pojeździe, wybiera bliższą bezpieczną krawędź strefy i znika dopiero po zakończeniu `CURBSIDE EXIT`.
- Przerwanie linku/exit przywraca stan właściwy dla bieżącej fazy bez duplikowania NPC.
- Zniszczenie pojazdu i abort kursu usuwają wszystkie aktory pasażera związane z kursem.

### Miejsce docelowe

Nowy lekki aktor prezentacyjny sterowany przez `UFlyingCabDispatchComponent` i `AFlyingCabDeliveryZone`. Nie przechowuje opłaty, celu ani stanu kursu i nie dodaje odpowiedzialności do GameMode.

Test akceptacyjny: odbiór i dowóz z obu stron strefy; przerwanie każdej fazy; wygaśnięcie oferty; zniszczenie auta. W każdej próbie liczba aktywnych aktorów pasażera wynosi dokładnie 0 albo 1 zgodnie z fazą.

## 4. Ground boost i miękki sufit

Status: oczekuje decyzji dotyczącej game feel. Nie implementować przy okazji innego refaktoru.

### Ground boost

- Boost rozpoczyna wyłącznie nowe naciśnięcie kierunku lub ciągu pionowego, gdy pojazd ma kontakt z podłożem i może używać silników.
- Ma ograniczony czas, nie odnawia się od trzymanego klawisza i kończy się po utracie kontaktu lub upływie czasu.
- Podlega zużyciu paliwa i końcowym limitom prędkości; nie omija zniszczenia ani blokady wejścia.
- Parametry pionowe i poziome są oddzielne oraz edytowalne w istniejącym workflow strojenia pojazdu.

### Miękki sufit

- Działa tylko powyżej skonfigurowanej wysokości ostrzegawczej i wyłącznie na ruch skierowany w górę.
- Płynnie tłumi dodatnią prędkość Z wraz ze zbliżaniem się do granicy; nie teleportuje i nie wpływa na opadanie.
- Położenie granicy używa współrzędnych Unreal i danych aktualnego miasta, a nie liczby pikselowej z Godota.

### Test akceptacyjny

Porównanie A/B z funkcją wyłączoną i włączoną: start z podłoża, trzymanie klawisza bez ponownego boostu, brak paliwa, wejście w sufit z różnymi prędkościami oraz swobodne opadanie. Następnie pełny test lotu, kamery, paliwa i uszkodzeń.

## 5. Cofanie dostępu do pojazdu

Status: kontrakt zaimplementowany w `UFlyingCabProgressionSubsystem` i pokryty testem `FlyingCab.Core.Progression.AccessLifecycle`; użycie przez przyszłą mechanikę progresji pozostaje opcjonalne.

### Kontrakt zachowania

- `RevokeAccess(None)` zwraca `false` i niczego nie zmienia.
- Cofnięcie istniejącego ID zwraca `true`; ponowne cofnięcie zwraca `false`.
- `HasAccess` odzwierciedla zmianę natychmiast.
- Cofnięcie licencji nie wyrzuca gracza z aktualnie prowadzonego pojazdu, ale blokuje następne wejście po jego opuszczeniu.
- `ResetAccess` nadal usuwa wszystkie przyznane prawa na początku konkurencyjnego runu.
- Przyszły zapis trwały serializuje wynikowy zbiór, nie historię operacji grant/revoke.

### Miejsce docelowe

`UFlyingCabProgressionSubsystem`; ewentualne komunikaty prezentuje PlayerController/HUD, nie subsystem.

Test akceptacyjny: grant → wejście do service cab → revoke podczas prowadzenia → wyjście możliwe → ponowne wejście odrzucone; osobny test jednostkowy sprawdza idempotencję i `None`.

## Funkcje, których nie przenosimy 1:1

- Globalny „god subsystem” odpowiadający autoloadom Godota.
- `VehiclePersistence` między poziomami, dopóki gra pozostaje na jednym trwałym poziomie.
- Dialogowa stacja paliw; obecny hold-to-service pozostaje właściwy dla mobile portrait.
- Refleksyjne `_get_prop/_set_prop` i serializacja po nazwach tekstowych.
- Timer Mai oraz równoległy reset gry; presję czasową zapewnia Time Attack.
- Pikselowe stałe fizyki i wysokości świata 2D.

## Zalecana kolejność po audycie

1. `RevokeAccess` — ukończone wraz z testem jednostkowym.
2. Fizyczny pasażer — największy widoczny przyrost jakości, ale dopiero po decyzji produktowej.
3. Trwały save — dopiero po wyborze zakresu kredytów/licencji/stanu świata.
4. Ground boost i miękki sufit — wyłącznie jako osobny etap strojenia game feel.
