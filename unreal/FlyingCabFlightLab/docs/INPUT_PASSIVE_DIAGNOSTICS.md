# Pasywna diagnostyka wejścia — 2026-09-04

Osobny `UFlyingCabInputDiagnosticsSubsystem` obserwuje działającą grę. Nie zmienia 32 plików wersji kanonicznej, nie podłącza nowych klawiszy, nie wykonuje flushu, nie wstrzykuje wejścia i nie zeruje ciągu. `flyingcab.UseControlFrame=0` pozostaje domyślne. Subsystem jest tworzony tylko w światach Game/PIE, poza buildem Shipping.

## Jak korzystać

Diagnostyka jest domyślnie włączona: `flyingcab.InputDiagnostics 1`. Można grać normalnie. Zrzuty powstają przy zniszczeniu sterowanego auta, ukończeniu recovery, ręcznym resecie, przejściu do braku paliwa i wykrytej rozbieżności wejścia/napędu.

Pliki: `Saved/Logs/InputDiagnostics/<czas_UTC>_<unikalny_id>.log`. Ścieżkę potwierdza też główny log komunikatem `Input evidence saved`. To dane lokalne, ignorowane przez Git; nic nie jest wysyłane. Przy zgłoszeniu należy zachować plik incydentu oraz główny log sesji.

Jeśli wystąpi podejrzane zachowanie bez automatycznego zrzutu, polecenie konsoli gry `flyingcab.DumpInput` zapisuje ostatnie pięć sekund. Samo otwarcie konsoli może zmienić focus zgodnie z istniejącym zachowaniem Unreal; polecenie diagnostyczne nie czyści wejścia. Gdy wszystkie warstwy gry zgodnie widzą trzymany klawisz, obserwator nie potrafi udowodnić, że użytkownik faktycznie go puścił po stronie Parsec.

Wyłączenie: `flyingcab.InputDiagnostics 0` — czyści wyłącznie historię i zrzuty jeszcze nieprzekazane do zapisu; rozpoczęty zapis może się dokończyć. Istniejący `flyingcab.InputTrace` jest osobnym przełącznikiem; pozostawienie go na `1` jest potrzebne do szczegółowego zapisu zdarzeń i siły. Gdy trace jest wyłączony, siła i cache auta są oznaczane jako nieznane, nie jako zerowe.

## Co zawiera zapis

- Ostatnie pięć sekund według zegara rzeczywistego, również podczas pauzy. Pełne próbki 10 Hz, zdarzenia i zmiany trybów rejestrowane dodatkowo. Czasy są względne wobec pierwszego incydentu w pliku.
- Dostarczone `KEY Pressed/Released/Repeat` z istniejącej telemetrii. Powtórzenia są już współdzielone przez źródło: logowany jest pierwszy repeat w serii, nie każdy.
- Surowe stany mapowanych klawiszy, wartości i stany triggerów akcji Horizontal/Thrust/Service, obecność kontekstu mapowania, aktywny wariant sterowania, obiekty komponentów wejścia.
- Pawn, tryb postaci, tryb wejścia Unreal, flagi menu/dziennika/observera, blokada kontrolera, pauza, widoczność kursora.
- Focus viewportu i aktywność aplikacji próbkowane co klatkę; dodatkowo zdarzenia aktywacji Slate. `-1` oznacza brak dostępnej informacji. Bardzo krótkie zmiany focusu między próbkami mogą nie zostać zarejestrowane.
- Paliwo, hull, stan fizyki/uśpienia, prędkość i ostatni wektor ruchu Pawna. Ostatnia ukończona próbka auta zachowuje jego wejście klawiatury/dotyku i siłę również wtedy, gdy przez pięć sekund nic się nie zmieniało.
- Jawne zmiany `PROPULSION_GATE`: brak paliwa, wrak, wyłączona fizyka, menu/dziennik/observer/przejście, pauza, brak pojazdu. Strażnik `Transition` może pozostać widoczny przez pauzę w dzienniku; osobne `journal=1` nie pozwala pomylić tego ze zwykłym lotem.

`last_cab_trace_known=0` / `last_force_known=0` oznacza, że nie można używać ostatniej próbki auta jako aktualnego dowodu (np. zmieniono Pawn, wyłączono trace lub trwa przejście). Snapshoty `STATE reason=...` przy przejściach mogą zawierać siłę sprzed czyszczenia; nie służą do detekcji rozbieżności siły. Paliwo w istniejącej telemetrii jest liczone po zużyciu, więc ostatnia opłacona klatka może mieć jednocześnie paliwo 0 i dodatnią siłę.

## Detektory nie są naprawą wejścia

- `ACTION_WITHOUT_MAPPED_KEY`: akcja ma niezerową wartość, ale żaden jej mapowany klawisz nie jest aktywny. Raport po co najmniej dwóch próbkach i 0,1 s; ponowne uzbrojenie dopiero po zaniku rozbieżności. Może być poprawną obserwacją przy celowym wstrzykiwaniu akcji w testach — nie jest samodzielnym dowodem błędu Enhanced Input.
- `THRUST_REQUESTED_NOT_APPLIED`: ostatnia ukończona próbka auta pokazuje żądanie osi bez odpowiedniej siły, mimo braku znanej blokady. Ten sam pasywny próg czasu. Prawidłowe odcięcie pustego baku jest opisywane jako `NoFuel`, nie jako awaria wejścia.
- Detektory są wyciszone przez pauzę/blokadę i przez 0,25 s po zmianie trybu, aby nie mylić różnych faz klatki z trwałym błędem. Ich jedynym skutkiem jest zapis diagnostyczny. Nie implementujemy timeoutu kasującego klawisz ani nie traktujemy braku repeat jako puszczenia.

## Koszt i ograniczenia

Bufor ma limit 2048 wpisów po 2048 znaków. Przepełnienie jest ujawniane jako `capacity_drops_total`, długi wpis jako `[truncated]`; przy skrajnej liczbie zdarzeń historia może być krótsza niż pięć sekund. Zapis nie częściej niż raz na sekundę, zdarzenia z tego okresu są łączone z zachowaniem pierwszego preludium i kolejnych obserwacji. Oczekujący tekst również ma limit rozmiaru z jawnym znacznikiem obcięcia.

Dysk zapisuje wątek roboczy, bez UObjects. W procesie mogą działać najwyżej dwa zapisy naraz. Limit automatyczny: 64 pliki na instancję świata; ręczne żądanie nadal jest możliwe. Pliki nie są automatycznie usuwane — kolejne sesje mogą zwiększać rozmiar katalogu. Nagłe zamknięcie procesu może utracić niezapisany bufor/zlecenie; to nie jest rejestrator odporny na crash procesu.

Zakres to pierwszy lokalny gracz. Istniejące komunikaty KEY nie mają identyfikatora kontrolera, dlatego są przypisywane tylko przy jednym lokalnym świecie gry w procesie. Nie zgadujemy źródła przy wielu światach PIE. Nie jest to pełna telemetria split-screen/multiplayer.

Pozostają poza tym etapem: fizyczna klawiatura klienta Parsec, transport Windows/Parsec, rzeczywisty multitouch/gamepad, pełne prywatne stany palców/widgetu i cache pieszego. Odczyt pieszego obejmuje akcje, surowe klawisze i wektor ruchu. Ich pełna unifikacja wymagałaby osobno uzgodnionej zmiany chronionych plików. Diagnostyka nie ustala przyczyny historycznych blokad.

## Weryfikacja

- `FlyingCab.Core.Input.DiagnosticsHistory`: retencja pięciu sekund, kolejność po zawinięciu, limit pojemności, obcinanie wpisu i reset.
- `FlyingCab.Core.Input.DiagnosticsPassiveLatch`: tolerancja krótkiej rozbieżności, pojedynczy raport, brak przepełnienia i ponowne uzbrojenie.
- `FlyingCab.Functional.PIE.PassiveInputDiagnostics`: przechwycenie dostarczonego klawisza i siły, ręczny zapis na dysk bez puszczenia ciągu, próbki podczas pauzy w dzienniku, celowo wstrzyknięta akcja bez klawisza wykryta bez neutralizacji, automatyczny zrzut recovery oraz włączenie/wyłączenie obserwatora bez zmiany trzymanego ciągu. Wstrzykiwanie istnieje wyłącznie w fixture testowym.

Test PIE wymaga kanonicznego `UseControlFrame=0`, `InputTrace=1` i `InputDiagnostics=1`. Nie testuje fizycznego przełączania aplikacji ani klienta Parsec. Pełny pakiet należy uruchamiać jak w [INPUT_RELIABILITY_TESTS.md](INPUT_RELIABILITY_TESTS.md).

Wynik końcowy: build UE 5.8 Win64 Development poprawny; **32/32** testy, kod 0 (`Saved/Logs/PassiveDiagnosticsVerifiedTests.log`). Wszystkie 32 pliki manifestu kanonicznego zgodne. Wynik i listę plików etapu zapisano w [AUDIT_IMPLEMENTATION_STATUS.md](AUDIT_IMPLEMENTATION_STATUS.md).
