# Treść zadania

Proszę zrealizować algorytm szeregowania dzielący procesy użytkownika na dwie grupy: A i B. 

Dodatkowo, proszę opracować funkcję systemową umożliwiającą przenoszenie procesów pomiędzy grupami.

Procesy w grupie B otrzymują dwa razy więcej czasu niż procesy z grupy A. 

Grupa B może zawierać maksymalnie 3 procesy, grupa A maksymalnie 5, dodawanie kolejnych procesów powoduje usunięcie najwcześniej dodanego. 

Zakładamy, że nowy proces domyślnie znajduje się w grupie A oraz że w grupie A znajduje się co najmniej 1 proces. 

Opracować również łatwą metodę weryfikacji poprawności rozwiązania.

# Interpretacja

Sam podział na grupy A i B jest zrozumiały, lecz problem pojawia się przy usuwaniu najwcześniej dodanego po wykorzystaniu limitów.
Aby nie ograniczać procesów użytkownika do 8 (i tym samym uniknąć przypadkowego zabicia ważnego procesu), założyłem, że usunięcie 
oznacza przydzielenie czasu procesora równego 0. Tym samym tak naprawdę będą 3 grupy:

B - mająca dwa razy więcej czasu niż grupa A  
A - mająca "standardową" ilość czasu  
C - mająca zerową ilość czasu  

# Przydział do grup

Aby rozróżnić grupy dodam pole 'proc_grp' do struktury 'proc' w jądrze. Będzie ona przyjmowała różne wartości w zależności od przynależności
procesu do grupy.

- proc_grp = 2 dla grupy B  
- proc_grp = 1 dla grupy A  
- proc_grp = 0 dla grupy C  

Oznaczenie jest zgode z założonymi proporcjami kwantów czasu dla danych grup.

# Algorytm

Aby odpowiednio zmienić szeregowanie zmodyfikuję nieco funkcję sched() w taki sposób aby sprawdzała przez ile kwantów czasu wykonywał się dany proces.
Jeśli osiągnął on swój limit (zależny od grupy) - wrzucamy go na koniec kolejki. Jeśli nie - na początek.
Potrzeba do tego jeszcze jednego pola w strukturze proc - 'proc_cnt', która będzie przechowywać ile razy wykonał się proces.
Jeśli 'proc_cnt' zwiększone o 1 będzie mniejsze lub równe od 'proc_grp' - wrzucamy proces na początek kolejki i inkrementujemy 'proc_cnt'.
W przeciwnym wypadku - na koniec.
W tak zaimplementowanym algorytmie procesy z grupy B powinny otrzymywać dwa kwanty czasu, z A jeden a z grupy C powinny być wrzucane zawsze na koniec kolejki.

# Domyślne wartości

Po utworzeniu procesu będzie on miał domyślnie ustawioną grupę A - `proc_grp = 1`.

# Przenoszenie procesów między grupami

Aby dobrze zaimplementować przenoszenie i uwzględnić limity będziemy potrzebować zmiennych trzymających liczbę procesów w danej grupie.

### Między grupą A i B

Jeśli chcemy przenieść proces z grupy A do grupy B 

