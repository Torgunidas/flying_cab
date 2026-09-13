# Czytelność warstwy interakcji — propozycja 2026-09-13

**Rekomendacja:** trzy wyraźne zakresy kontrastu, spokojne otoczenie sylwetek oraz wspólny sposób oświetlenia pieszych i aut. To propozycja kolejnego kroku wizualnego, nie zmiana grafiki zawarta w paczce z 25 przystankami.

## Co widać w obecnym obrazie

Obejrzano bieżący render przy BLUE HOUR, HALO HEALTH, AFTERMARKET i APEX CAPITAL oraz eksport Web w pionowym oknie. Okna fasad tworzą jasne, regularne plamy o rozmiarze zbliżonym do postaci. Smukłe ręce i nogi przecinają podziały okien, a palety ubrań powtarzają kolory miasta. Karoseria również ma mały kontrast względem elementów fasady. W HALO HEALTH jasne słupy i podest zlewają się z fragmentami taksówki i ciałem pieszego. W APEX CAPITAL złota architektura konkuruje z żółtym autem.

Miasto powinno zachować szyldy i własną tożsamość. Trzeba uporządkować lokalne kontrasty tak, żeby ruch, kierunek auta i machający pasażer były czytelne także podczas przelotu, bez wspomagania minimapą lub stałymi ikonami nad NPC.

## Trzy plany

| Plan | Rola w obrazie | Proponowana zmiana |
| --- | --- | --- |
| Dalekie tło | Skala, głębia, atmosfera | Najniższy kontrast, mniej nasycenia, łagodne sylwetki i perspektywa atmosferyczna. Dalekie okna nie powinny wyglądać jak ostre punkty pierwszego planu. |
| Miasto i bryły platform | Orientacja, charakter dzielnic, miejsca lądowania | Zachować duże bryły, kolory dzielnic i nazwy lokali. Osłabić drobne, powtarzalne wzory okien i świecenie detali. Za pasem, po którym chodzą ludzie, stosować spokojną powierzchnię ściany. |
| Piesi, auta i użytkowa krawędź tarasu | Natychmiastowa czytelność ruchu i kontaktu | Najwyraźniejsza sylwetka, większe jednolite plamy jasności, subtelne światło na krawędziach, cienki ciemny obrys i mały cień kontaktowy na podeście. |

Platforma jest częścią architektury, ale jej przednia krawędź powinna wizualnie łączyć się z warstwą interakcji. Wąski pas lądowania może pozostać wyraźny. Jasność podpór, balustrad, ozdób i elewacji nie musi być równie wysoka.

## Pierwsza próba, którą rekomenduję

1. **Uspokoić fasadę za pieszymi.** Przy miejscach oczekiwania i dojścia tworzyć pas o małej liczbie detali, wysokości około 2–2,5 m. Użyć np. ciemnego szkła albo jednolitego panelu pasującego do lokalu. Dla powtarzalnych okien poza tym pasem rozpocząć próbę od obniżenia emisji o około 20–30%. Szyldy z nazwami pozostawić jako punkty orientacyjne. Górne dzielnice zachowują jasny charakter; regulować lokalne detale, zamiast przyciemniać cały obraz.
2. **Nadać aktorom spójne światło.** Lekko jaśniejsze barki, głowa i górne krawędzie karoserii, z bardziej neutralnym światłem niż kolorowe neony. Zachować duże, rozdzielone plamy stroju: wyraźny tułów, ciemniejsze nogi i czytelną głowę. Wygląd danej osoby pozostaje stały podczas podróży; nie zmienia koloru po przekroczeniu dzielnicy.
3. **Dodać bardzo cienki ciemny obrys.** Punkt wyjścia to około jednego piksela na rzeczywistym ekranie telefonu. Jasna krawędź pomaga w ciemnym dole miasta, a ciemny kontur utrzymuje sylwetkę na jasnym tle Edenu. Obrys obejmuje rzeczywistą sylwetkę, respektuje zasłanianie przez świat i nie tworzy świecącej aureoli ani widoczności przez ściany. Jego grubość trzeba sprawdzić w docelowym eksporcie.
4. **Wzmocnić kontakt z podłożem i gest oczekiwania.** Mały miękki cień pod stopami i pod zaparkowanym autem; bardziej czytelne uniesienie ręki z chwilą zatrzymania gestu. Najpierw ocenić tę zmianę przy obecnej skali postaci. Jeśli w ruchu nadal giną, przetestować nieco szersze barki i dłonie oraz powiększenie wizualnej sylwetki o około 10%, z zachowaniem fizycznych odstępów.

Podane wartości to punkty startowe do porównania obrazu, nie zatwierdzony balans grafiki.

## Wykonanie i koszt

Wprowadzić wspólny, edytowalny profil czytelności aktorów, używany przez pieszych i modele pojazdów, oraz osobne parametry materiałów miasta. Istniejący `facade.gdshader` już udostępnia energię okien; jasność i kolory otoczenia zależą od wysokości w `city_atmosphere.gd`. To właściwe miejsca do pierwszej próby hierarchii obrazu. Piesi używają jednej skórkowanej siatki na osobę i nie rzucają obecnie własnego cienia; efekt kontaktowy powinien być małym, tanim elementem wizualnym.

Pierwsza wersja może oprzeć się na materiałach, oświetleniu krawędzi i prostym cieniu kontaktowym. Kontur wymaga oddzielnej próby kosztu oraz jakości na małych sylwetkach. Nie należy zwiększać liczby świateł i cieni proporcjonalnie do liczby przechodniów. Ciężki blur całego miasta lub rozbudowany efekt pełnoekranowy mają niższy priorytet niż uporządkowanie kontrastu materiałów.

## Jak ocenić próbę

Porównać ten sam kadr i tę samą trasę w czterech dzielnicach, w smogu i podczas mijania jasnych reklam. Oceniać w normalnym rozmiarze gry na S25+, również w ruchu: czy od razu widać machającego człowieka, kierunek i przechył auta, krawędź podestu oraz miejsce kontaktu z platformą? Sprawdzić obraz w odcieniach szarości — rozdzielenie planów powinno działać również bez różnicy odcieni. Następnie porównać czas klatek i przycięcia przy pełnej populacji. Proponowanych zmian kontrastu, konturu ani sylwetek jeszcze nie wdrożono.
