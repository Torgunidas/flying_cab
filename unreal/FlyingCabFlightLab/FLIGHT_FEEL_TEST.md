# FlightLab — procedura testowa lotu

## Przygotowanie

1. Otwórz projekt i uruchom mapę `FlightLab` w trybie **Selected Viewport**.
2. HUD telemetryczny jest domyślnie ukryty; `F3` przełącza jego widoczność.
3. Przed każdym testem naciśnij `R`, aby wyzerować pozycję, prędkość i wejścia.

## Szybka iteracja parametrów

- W **Content Browser** otwórz `Content/Blueprints/BP_FlyingCabPawn` i wybierz **Class Defaults**.
- Parametry znajdują się w kategoriach `Flying Cab | Flight`, `Flying Cab | Presentation` oraz `Flying Cab | Debug`.
- Po zmianie wartości kliknij **Compile** i uruchom Play ponownie, aby zachować i sprawdzić ustawienie.
- Do tymczasowego strojenia podczas Play użyj `Shift+F1`, zaznacz uruchomionego `BP_FlyingCabPawn` w **World Outliner** i zmieniaj wartości w panelu **Details**. Takie zmiany działają od razu, ale po zakończeniu Play zostaną cofnięte.
- Za płynność powrotu do poziomu odpowiada `Visual Pitch Return Speed`: mniejsza wartość oznacza wolniejsze wyrównanie. Szybkość wejścia w przechylenie kontroluje osobno `Visual Pitch Response Speed`.

## Testy bazowe

### 1. Opadanie i ciąg pionowy

- Bez wejścia obserwuj swobodne opadanie przez około sekundę.
- Przytrzymaj `W` lub `Spację` przez sekundę, następnie puść.
- Sprawdź, czy `effective` wraca do `0.00`, a prędkość Z przechodzi płynnie z wznoszenia do opadania.

### 2. Przyspieszenie i dryf poziomy

- Przytrzymaj `D` przez sekundę i puść.
- `Horizontal effective` powinno natychmiast wrócić do `0.00`.
- Prędkość X może zanikać stopniowo — to zamierzony dryf sterowany przez `HorizontalCoastDamping`.
- Powtórz test w lewo klawiszem `A`.

### 3. Zmiana kierunku

- Rozpędź pojazd klawiszem `D`, puść go i od razu naciśnij `A`.
- Oceń czas potrzebny na zatrzymanie i rozpoczęcie ruchu w przeciwną stronę.

### 4. Precyzja

- Wyląduj kolejno na trzech platformach.
- Przeleć przez górną bramkę bez dotykania jej boków.
- Zanotuj, czy korekty są przewidywalne, czy pojazd regularnie przestrzeliwuje cel.

### 5. Reakcja wizualna i kamera

- Naciśnij krótko `A` i `D`; bryła pojazdu powinna reagować przechyleniem na rozpędzanie i hamowanie.
- Przytrzymaj kierunek aż pojazd osiągnie stałą prędkość; mimo nadal wciśniętego klawisza powinien płynnie wrócić do poziomu.
- Zmień kierunek lotu z `D` na `A`; przechylenie powinno czytelnie zaakcentować hamowanie i ponowne rozpędzanie.
- Rozpędź pojazd poziomo, a następnie rozpocznij wznoszenie i opadanie.
- Kamera powinna łagodnie przesuwać kadr w kierunku ruchu, bez szarpnięcia przy zmianie kierunku.
- W HUD sprawdź wartości `Presentation accel X`, `pitch` oraz `camera X/Z`; przy stałej prędkości `accel X` i `pitch` powinny dążyć do zera. Po naciśnięciu `R` powinny wrócić do zera natychmiast.

### 6. Regresja utraty wejścia

- Przytrzymaj `D`, użyj `Shift+F1` albo `Alt+Tab`, puść `D` poza viewportem i wróć do gry.
- `Horizontal effective` powinno wrócić do `0.00` najpóźniej po jednej klatce.
- Włącz telemetrię `F3`. Wiersz `Keys` pokazuje teraz osobno `A`, `D`, `LEFT`, `RIGHT`, `W`, `UP`, `SPACE` i `E`; po puszczeniu każdy klawisz musi natychmiast wrócić do `0`.
- Przytrzymaj `D`, kliknij jeden z przycisków ekranowych i puść `D`, gdy kursor znajduje się nad HUD-em. Przycisk ekranowy nie może przejąć fokusu klawiatury, a `D` musi wrócić do `0`.
- Wciśnij jednocześnie `A` i `D`, potem puszczaj je pojedynczo w różnej kolejności. Kierunek powinien być neutralny przy obu wciśniętych, a po puszczeniu jednego odpowiadać drugiemu bez pozostawania aktywnym po puszczeniu obu.
- Przytrzymaj `W`, `Spację`, `A` albo `D` i w tym samym czasie wykonaj reset klawiszem `R`. Po resecie sterowanie musi pozostać neutralne, dopóki nie puścisz wszystkich klawiszy danej osi. Ponowne sterowanie powinno zadziałać dopiero po puszczeniu i kolejnym naciśnięciu.
- Powtórz próbę przez zniszczenie auta przy aktywnym ciągu. Po holowaniu thrust i kierunek muszą pozostać wyłączone nawet wtedy, gdy Unreal nie zarejestrował wcześniejszego puszczenia klawisza.
- Włącz telemetrię `F3`. Pole `forced resets` powinno zwiększyć licznik po wymuszonym czyszczeniu wejścia; wszystkie wartości `effective`, `keyboard` i `touch` muszą pozostać neutralne aż do ponownego naciśnięcia.

### 7. Sterowanie dotykowe

- Po uruchomieniu Play powinny być widoczne przyciski `LEFT`, `RIGHT`, `THRUST` i `RESET`; `F4` przełącza cały interfejs.
- Przytrzymaj myszą `LEFT` lub `RIGHT`. W HUD wartość `touch` powinna wynosić odpowiednio `-1.00` albo `+1.00`, a po puszczeniu natychmiast wrócić do `0.00`.
- Przytrzymaj `THRUST`; wiersz ciągu powinien pokazać `touch:+1.00`, a po puszczeniu `touch:+0.00`.
- Przeciągnij kursor poza trzymany przycisk, a następnie puść mysz. `touch` musi wrócić do zera również wtedy, gdy puszczenie nie nastąpiło dokładnie nad przyciskiem.
- Sprawdź, czy klawiatura nadal działa przy widocznym interfejsie oraz czy `RESET` zeruje pozycję, prędkość i wszystkie wejścia.
- Panel `CREDITS / FUEL / HULL / FARE` powinien mieć większy tekst i ciemne, półprzezroczyste tło. Przeleć przed jasnymi platformami, neonami i ciemnym tłem; cały tekst powinien pozostać czytelny.
- Rzeczywiste jednoczesne użycie kierunku i ciągu wymaga końcowego testu wielodotykowego na urządzeniu mobilnym.

### 8. Pierwsza pętla kursu taxi

- Po starcie wybierz jedną z widocznych bursztynowych ofert pasażerów i odszukaj jej turkusową bramkę `PICKUP` w jednej z dziesięciu dzielnic; górny komunikat pokazuje nazwę punktu, cel podróży, szacowaną opłatę, czas ważności i dystans.
- Wleć do bramki z prędkością większą niż `180 cm/s`. Zadanie nie powinno zostać zaliczone, a komunikat powinien zmienić się na `SLOW DOWN`.
- Zwolnij pozostając w bramce. Po zakończeniu linku turkusowa strefa powinna zniknąć, a w innej dzielnicy aktywować się pomarańczowa bramka `DROPOFF`.
- Zatrzymaj pojazd w bramce `DROPOFF`. Licznik `DELIVERIES` powinien wzrosnąć, a po krótkiej pracy dispatchera pojawić się nowe, losowane zlecenie.
- Ukończ kilka kursów, aby potwierdzić, że trasy nie działają już w stałej rotacji.

### 9. Minimapa i nauka topografii

- Telemetria jest domyślnie ukryta; `F3` nadal pozwala ją włączyć na czas diagnostyki.
- Na stałej minimapie `CITY GRID` biały punkt powinien śledzić pozycję auta bez obracania całej mapy.
- Szare punkty `YP`, `ME`, `ST`, `AM`, `ND` i `ZS` oznaczają odpowiednio `Yellow Projects`, `Midtown Exchange`, `Skyline Terraces`, `Ashline Market`, `Neon Docks` i `Zenith Spire`.
- Aktywny cel powinien być turkusowy podczas odbioru i pomarańczowy podczas dowozu.
- Sprawdź, czy nazwa w zleceniu odpowiada punktowi na minimapie oraz czy sam dystans wystarcza do odnalezienia celu bez strzałki kierunkowej.

### 10. Curbside link

- W aktywnej bramce `PICKUP` powinna być widoczna prosta świetlna sylwetka pasażera.
- Zwolnij poniżej limitu i pozostań w strefie przez około `0,65 s`. Napis bramki i HUD powinny pokazywać rosnący procent `CURBSIDE LINK`, światło powinno pulsować, a sylwetka przesuwać się w stronę auta.
- Rozpocznij link, ale przed końcem wyjedź ze strefy albo przyspiesz ponad limit. Postęp powinien natychmiast wrócić do zera, a pasażer do pozycji początkowej.
- Ukończ link. Powinien pojawić się krótki komunikat `PASSENGER SECURED`, sylwetka i bramka odbioru powinny zniknąć, a aktywować się cel dowozu.
- Oceń, czy `0,65 s` daje wiarygodne potwierdzenie odbioru, ale nie psuje dotychczasowego tempa kursu.

### 11. Kierunek podejścia i wysiadanie

- Przy odbiorze zatrzymaj taksówkę najpierw po lewej, a potem po prawej stronie pasażera. W obu przypadkach pasażer powinien iść w stronę rzeczywistego położenia auta, a nie zawsze w prawo.
- Dojedź do `DROPOFF` i zatrzymaj się poniżej limitu prędkości. HUD powinien przez około `0,55 s` pokazywać `CURBSIDE EXIT`, a napis bramki rosnący procent `EXIT`.
- Pasażer powinien pojawić się przy taksówce, przejść do bliższej krawędzi strefy i zniknąć po ukończeniu sekwencji. Dopiero wtedy powinien wzrosnąć licznik dostaw.
- Powtórz wysiadanie z autem ustawionym po przeciwnej stronie strefy, aby potwierdzić zmianę kierunku ruchu pasażera.
- Przerwij wysiadanie przez wyjazd ze strefy albo przekroczenie limitu prędkości. Postęp powinien wrócić do zera, pasażer zniknąć, a pełna sekwencja rozpocząć się ponownie po prawidłowym zatrzymaniu.

### 12. Opłata za kurs i kredyty

- Po starcie HUD powinien pokazywać `100` kredytów. Po odebraniu pasażera opłata zaczyna się od `20 CR`.
- Leć w stronę celu. `FARE` powinno rosnąć wraz ze zmniejszaniem dystansu do punktu docelowego.
- Zawróć na kilka sekund. Opłata powinna maleć wolniej, niż wcześniej rosła, i nigdy nie spaść poniżej stawki bazowej.
- Ukończ wysiadanie. Naliczona opłata powinna zostać dodana do salda dopiero po zakończeniu `CURBSIDE EXIT`.

### 13. Paliwo i stacje

- Początkowy poziom paliwa powinien wynosić `65%`. Ciąg pionowy powinien zużywać paliwo szybciej niż sterowanie poziome.
- Puść wszystkie przyciski podczas opadania. Paliwo powinno regenerować się bardzo powoli, proporcjonalnie do prędkości opadania.
- Odszukaj zielony punkt `F` przy `Midtown Exchange` albo `Ashline Market`, zatrzymaj się w zielonej strefie i przytrzymaj `E` albo przycisk `REFUEL`.
- Tankowanie powinno płynnie zwiększać paliwo i pobierać `2 CR` za jednostkę. Powinno zatrzymać się po puszczeniu przycisku, zapełnieniu zbiornika albo wyczerpaniu kredytów.
- Przy zerowym paliwie normalny ciąg powinien przestać działać. Kontrolowane opadanie powinno powoli odzyskać minimalną rezerwę energii.
- Zużyj część paliwa i lekko uszkodź pojazd, a następnie naciśnij `R`. Reset powinien przywrócić pozycję testową, `65%` paliwa i `100%` kadłuba; saldo kredytów oraz aktywny kurs nie powinny się zmienić.

### 14. Uszkodzenia i zniszczenie

- Delikatnie dotknij platformy. Lekkie kontakty nie powinny zmieniać `HULL`.
- Uderz w platformę lub bok bramki ze znaczną prędkością. HUD powinien pokazać utratę kadłuba i krótki komunikat `IMPACT`.
- Powtarzaj mocne uderzenia do zera HP. Taksówka powinna wejść w czerwony stan zniszczenia, kurs zostać przerwany, a z salda pobrana opłata za holowanie do `35 CR`.
- Po około `2,5 s` auto powinno wrócić na pozycję startową z pełnym kadłubem i co najmniej `25%` paliwa. Nowy kurs powinien ponownie czekać na odbiór pasażera.

### 15. Dynamiczny dispatcher

- Bez pasażera system powinien utrzymywać od jednej do czterech równoległych ofert. Każda oferta ma własny bursztynowy znacznik minimapy i odliczany czas ważności.
- Gdy oferta wygaśnie poza trwającym `CURBSIDE LINK`, jej strefa i znacznik powinny zniknąć, a po losowym opóźnieniu powinna pojawić się kolejna oferta.
- Rozpocznij `CURBSIDE LINK`. Czas wybranej oferty powinien przestać maleć w trakcie potwierdzania; przerwanie linku wznawia odliczanie.
- Po odebraniu pasażera wszystkie pozostałe oferty powinny przestać przyjmować taksówkę, ale mogą pozostać widoczne do końca kursu.
- Zapisz sześć kolejnych par odbiór–dowóz. Odbiór i dowóz jednego kursu zawsze muszą być różnymi punktami. `TIME ATTACK` z seedem `1977` powinien dawać tę samą sekwencję po restarcie; `FREE ROAM` powinien rozpoczynać się od nowej losowej sekwencji.
- Zniszcz taksówkę w trakcie kursu. Kurs ma zostać anulowany, oferty zablokowane na czas holowania, a po odzyskaniu pojazdu rynek ofert ponownie aktywny.

### 16. Rozbudowane miasto, paliwo i nowa krzywa obrażeń

- Minimapa powinna pokazywać dziesięć dzielnic: `YP`, `ME`, `ST`, `AM`, `ND`, `ZS`, `GT`, `RB`, `CH` i `OG`. Zielone punkty `F` powinny znajdować się przy Midtown Exchange, Ashline Market i Rainline Bazaar.
- Wykonaj co najmniej trzy kursy obejmujące nowe dzielnice. Na długiej trasie pojazd powinien mieć czas dojść do prędkości maksymalnej, a hamowanie przed bramką powinno wymagać wyczucia.
- Początkowy poziom paliwa wynosi `65%`. Zanotuj paliwo na początku i końcu każdego kursu; celem jest zauważalny koszt długiej trasy i potrzeba tankowania po kilku kursach, a nie po każdym zleceniu.
- Sprawdź trzy stacje: `MIDTOWN FUEL`, `ASHLINE CHARGE` i `RAINLINE ENERGY`. Zatrzymanie i koszt tankowania powinny działać identycznie.
- Wyląduj zwyczajnie obok pasażera kilka razy. Kontakty do około `700 cm/s` zmiany prędkości normalnej nie powinny uszkadzać kadłuba.
- Następnie wykonaj jedno średnie i jedno mocne uderzenie. Obrażenia powinny rosnąć łagodnie powyżej bezpiecznego progu, ale wyraźnie przy poważnym zderzeniu.
- Oceń, czy niższe przyspieszenie i limity prędkości uspokoiły pojazd bez utraty dotychczasowego dryfu i płynności kamery.

### 17. Nightshift Repair

- Na minimapie powinien być widoczny fioletowy punkt `R` w górnej, centralnej części miasta.
- Lekko uszkodź pojazd, a następnie wyląduj na osobnej platformie `NIGHTSHIFT REPAIR` i zatrzymaj się w fioletowej strefie.
- HUD i przycisk serwisowy powinny zmienić opis na `REPAIR`. Przytrzymaj `E` albo przycisk dotykowy; kadłub powinien rosnąć, a saldo maleć o `1 CR` za każdy naprawiony punkt.
- Puść przycisk przed pełną naprawą. Proces powinien zatrzymać się natychmiast i dać się wznowić.
- Przy pełnym kadłubie stacja powinna pokazać `HULL FULL`, a przy braku pieniędzy odmówić dalszej naprawy.

### 18. Prosty ruch uliczny

- Na trasach miasta powinno poruszać się łącznie osiem kolorowych aut. Ich ruch, teleport na końcu pasa i fazy początkowe powinny być powtarzalne po restarcie.
- Obserwuj jeden pojazd do końca pasa. Powinien pojawić się ponownie po przeciwnej stronie bez przejazdu przez całą mapę podczas teleportu.
- Auta nie powinny zatrzymywać się na platformach ani zderzać ze sobą. Powinny jednak blokować taksówkę gracza i zatrzymać się chwilowo, jeżeli gracz zajmie ich tor.
- Przeleć przez każdy pas kilka razy. Oceń, czy prędkości są czytelne i dają czas na reakcję, ale wymagają obserwowania ruchu przed przecięciem korytarza.
- Sprawdź, czy ruch nie blokuje spawnu, bramek odbioru/dowozu, obu stacji paliw ani punktu napraw.

### 19. Ostrzeganie o ruchu i near miss

- Zatrzymaj się w pobliżu jednego z trzech pasów. Jeżeli auto ruchu przecina przewidywany tor taksówki w ciągu około `1,5 s`, na górze HUD-u powinien pojawić się bursztynowy komunikat `TRAFFIC` z czasem i stroną, z której nadjeżdża pojazd.
- Przy czasie poniżej około `0,65 s` ostrzeżenie powinno zmienić kolor na czerwony. Komunikat nie powinien pojawiać się, gdy pojazd już się oddala albo minie taksówkę na wyraźnie innym pułapie.
- Przepuść auto z niewielkim, ale bezpiecznym odstępem nad lub pod taksówką. Po rzeczywistym przecięciu torów HUD powinien pokazać `CLEAN NEAR MISS // +3 CR`, a saldo wzrosnąć o `3`.
- Powtórz przejazd ze zbyt dużym odstępem. Nie powinno być nagrody.
- Doprowadź do kontaktu z autem ruchu. Kolizja może uszkodzić kadłub, ale nie może jednocześnie naliczyć nagrody za near miss.
- Sprawdź ostrzeżenie i nagrodę zarówno podczas lotu poziomego, jak i podczas pionowego przecinania pasa.

### 20. Tryby rozgrywki i Time Attack

- Po uruchomieniu poziomu gra powinna być wstrzymana, a ekran wyboru pozwalać rozpocząć `FREE ROAM` albo `TIME ATTACK`.
- Wybierz `FREE ROAM`. Ekran ma zniknąć, wejścia zostać wyzerowane, a rozgrywka wznowiona z saldem `100 CR`.
- Uruchom ponownie poziom i wybierz `TIME ATTACK`. Opis trybu i HUD powinny pokazywać ten sam skonfigurowany cel salda (`1000 CR` w ustawieniach domyślnych).
- Zdobywaj kredyty przez kursy i near miss, a wydawaj je na paliwo, naprawy i holowanie. Wynik powinien uwzględniać wszystkie te operacje.
- Po osiągnięciu `1000 CR` gra ma się zatrzymać i pokazać wynik wraz z maksymalnie pięcioma najlepszymi czasami.
- `RETRY` powinno ponownie uruchomić Time Attack bez powrotu do wyboru trybu; `FREE ROAM` powinno otworzyć ten tryb bezpośrednio.
- Zamknij i ponownie uruchom grę. Leaderboard ma pozostać zapisany, natomiast saldo bieżącej sesji i licencje nie muszą przetrwać restartu aplikacji.

### 21. Tryb pieszy, portale i dostęp do pojazdu serwisowego

- Bez pasażera i bez trwającego `CURBSIDE LINK` naciśnij `Q` albo `EXIT`. Postać powinna pojawić się po bezpiecznej stronie auta, przejąć kamerę i zachować prędkość pojazdu przy wyjściu w powietrzu.
- Podczas kursu lub aktywnego linku próba wyjścia ma zostać odrzucona z czytelnym komunikatem w HUD.
- W trybie pieszym `A/D` sterują ruchem, `SPACE`/`JUMP` skokiem, a `Q`/`ENTER` najbliższą interakcją. HUD powinien pokazywać zdrowie, odległość od ostatniego auta i aktualny kontekst interakcji.
- Wejdź portalem do Nightshift Office i wróć do miasta. Kamera powinna przeskoczyć do postaci bez długiego przelotu przez pustą przestrzeń.
- Podejdź do terminala dostępu i aktywuj go. Po przyznaniu `Vehicle.Service` wejście do `NIGHTSHIFT SERVICE CAB` powinno być możliwe; przed przyznaniem licencji wejście ma być odrzucone.
- Po wejściu do dowolnego auta postać piesza powinna zniknąć, kamera przejąć nowy pojazd, a ten sam HUD przełączyć przyciski z `JUMP/ENTER` na `THRUST/EXIT`.
- Doprowadź zdrowie postaci do zera. Po około `1,4 s` poziom powinien przeładować się w tym samym trybie runu.

### 22. Wiele pojazdów i odzyskiwanie wraków

- Po uzyskaniu licencji przełączaj się pomiędzy podstawową taksówką i pojazdem serwisowym. Ekonomia, dispatcher i kamera powinny zawsze śledzić aktualnie prowadzony pojazd.
- Zniszcz aktualnie prowadzony pojazd. Kurs ma zostać anulowany, opłata za holowanie pobrana, a pojazd odzyskany po około `2,5 s`.
- Zniszcz pojazd zaparkowany, gdy gracz kontroluje drugi. Zaparkowany pojazd również musi zostać odzyskany po czasie, ale nie powinien anulować bieżącego kursu ani naliczać graczowi opłaty za holowanie.
- Po odzyskaniu jednego pojazdu drugi nie może utracić obsługi własnego zniszczenia. Powtórz próbę w odwrotnej kolejności.
- Sprawdź, czy wejście do wraku jest blokowane do chwili zakończenia odzyskiwania.

### 23. Komunikaty w buildzie docelowym

- Uruchom build `Development` lub `Shipping` na urządzeniu albo poza edytorem.
- Potwierdź, że odmowa wyjścia, brak dostępu, `PASSENGER SECURED`, wypłata, brak kredytów, `IMPACT`, pusty bak, śmierć postaci i holowanie pojawiają się w panelu komunikatów HUD, a nie wyłącznie jako debug overlay silnika.
- Kolejny komunikat powinien zastąpić poprzedni, pozostać czytelny na jasnym i ciemnym tle oraz zniknąć po swoim czasie życia.

## Co zapisać po teście

- Czy wznoszenie i opadanie są zbyt szybkie lub zbyt powolne.
- Czy dryf poziomy zanika za wcześnie albo za późno.
- Czy zmiana kierunku jest zbyt ostra lub zbyt bezwładna.
- Czy kamera pomaga przewidzieć ruch.
- Czy przechylenie pojazdu jest czytelne, ale nie przesadzone.
- Ile paliwa zużywa typowy kurs i czy wizyta na stacji jest potrzebna w dobrym rytmie.
- Czy przychód z kursu, cena paliwa i opłata za holowanie tworzą odczuwalną, ale nie frustrującą presję.
- Czy próg obrażeń odróżnia zwykłe lądowanie od wyraźnie niebezpiecznego uderzenia.
- Wartości HUD w chwili zachowania, które wygląda nieprawidłowo.
