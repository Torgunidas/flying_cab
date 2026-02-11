# Przegląd systemu pilotowania pojazdu (Godot)

## Co działa teraz

Aktualne sterowanie w `scripts/car.gd` jest zbudowane na `CharacterBody2D` i ręcznie liczonej „pseudo-fizyce”:

- grawitacja i opór są dodawane ręcznie do `velocity`;
- ciąg pionowy/poziomy jest liczony na podstawie wejścia (`thrust_up`, `thrust_left`, `thrust_right`);
- jest boost przy starcie z podłoża;
- jest miękki limit wysokości (`_apply_soft_ceiling`), hamowanie, balansowanie podwozia i system obrażeń.

To podejście jest szybkie i przewidywalne, ale bywa „zero-jedynkowe” w odczuciu (mało bezwładności bocznej i mało subtelnej kontroli).

## Główne ograniczenia gameplayowe (z perspektywy frajdy)

1. **`hover` zeruje prędkość do `Vector2.ZERO`**
   - Daje to „teleportowane” zatrzymanie ruchu, co odbiera uczucie masy pojazdu.

2. **Sztywne klamrowanie prędkości (`clamp`) i szybki lerp oporu**
   - To stabilizuje sterowanie, ale ogranicza „flow” i umiejętnościowe prowadzenie.

3. **Skręcanie jest de facto ruchem lewo/prawo bez dynamiki obrotu**
   - `car_body.scale.x` tylko flipuje sprite, nie ma momentu obrotowego ani pracy nosem pojazdu.

4. **Kolizje i balans są mocno skryptowe**
   - Efekt „ślizgu” i „osuwania” (raycast + translate) działa, ale może wyglądać sztucznie przy trudnym terenie.

## Czy przepisanie w najnowszym Godocie może poprawić frajdę?

**Tak — jeśli zmienisz metodę sterowania, a nie tylko API.**

Samo przeniesienie 1:1 do nowszego Godota nie da dużego efektu. Różnicę w „fun factor” da zmiana modelu ruchu.

## Rekomendowana metoda od zera (Godot 4.x)

### Opcja A (najbardziej „fun”): `RigidBody2D` + thrusters + asysty

Przepisz pojazd na `RigidBody2D` (tryb rigid), a sterowanie opieraj o:

- `apply_central_force` dla ciągu,
- `apply_torque` dla obrotu,
- punktowe thrustry (`apply_force(force, local_offset)`),
- lekkie asysty lotu:
  - auto-level (stabilizacja roll/pitch),
  - velocity assist (tłumienie bocznego dryfu),
  - hover assist (utrzymanie wysokości nad gruntem, ale bez zerowania prędkości).

Dlaczego lepiej:

- dużo lepsze poczucie masy i bezwładności,
- większy „skill ceiling” (gracz uczy się panować nad pędem),
- kolizje są bardziej organiczne i „filmowe”.

### Opcja B (bezpieczniejsza): dalej `CharacterBody2D`, ale model „acceleration based”

Jeśli nie chcesz pełnej fizyki rigid body:

- trzymaj `CharacterBody2D`,
- zamiast bezpośrednio nadpisywać `velocity` użyj:
  - target velocity,
  - acceleration/deceleration curves,
  - oddzielne tarcie dla osi X/Y,
- `hover` zmień z „stop natychmiast” na kontrolowane wygaszanie,
- dodaj krótkie „coyote windows” dla boosta i lądowania.

Efekt będzie mniejszy niż w opcji A, ale nadal wyraźnie przyjemniejszy.

## Konkretne zmiany, które najbardziej zwiększą fun

1. **Usuń natychmiastowe `velocity = Vector2.ZERO` przy hover.**
2. **Wprowadź miękką asystę zamiast twardych limitów.**
3. **Dodaj rotację pojazdu zależną od wejścia i prędkości.**
4. **Rozdziel tryby sterowania:**
   - Arcade (silna asysta),
   - Sim-lite (większa bezwładność).
5. **Zostaw obecny model paliwa i obrażeń** (są sensowne dla pętli gameplayowej), ale podepnij pod nowy model ruchu.

## Wniosek

Jeżeli celem jest **większa frajda z pilotażu**, to warto przepisać system od zera i przejść na model:

- `RigidBody2D` + thrusters + asysty (największy zysk),

albo minimum:

- `CharacterBody2D` z miękkim, akceleracyjnym modelem sterowania.

Największą poprawę odczucia da odejście od „twardego” zatrzymywania i sztywnych clampów na rzecz pędu + inteligentnej asysty.
