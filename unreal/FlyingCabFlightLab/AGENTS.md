# Flying Cab — chroniony punkt odniesienia sterowania

Przed zmianami w sterowaniu, przejściach UI, possession, resecie lub recovery przeczytaj w całości `docs/INPUT_CANONICAL_BASELINE.md` i uruchom `scripts/Verify-InputBaseline.ps1`.

- Kanoniczny wariant to `input-canonical-2026-09-04`, UE 5.8, `flyingcab.UseControlFrame=0`, potwierdzony ręcznie przez użytkownika po długiej sesji przez Parsec.
- Nie włączaj domyślnie eksperymentalnej ramki, nie usuwaj działającej ścieżki ani nie zmieniaj kolejności przetwarzania wejścia przy okazji innych prac.
- Nowe zachowanie wejścia wymaga osobnego, wyraźnie uzgodnionego zakresu; sama prośba o kontynuację audytu nie oznacza zgody na zmianę wariantu kanonicznego.
- Najpierw dowód/test, potem mała zmiana, pełne testy i ręczna akceptacja użytkownika. Zielona automatyzacja sama nie zatwierdza nowej wersji.
- Niezgodność sum kontrolnych oznacza konieczność przeglądu różnic, nie zgodę na cofnięcie zmian użytkownika. Nie aktualizuj manifestu tylko po to, żeby kontrola przeszła.
- Nie wykonuj commita, tagu, push ani zmiany UE bez polecenia użytkownika. Stary projekt Godot pozostaje referencją.
