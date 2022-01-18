# Treść zadania

System plików - (aplikacja w C++/Python itp symulująca system plików) - W pliku na dysku należy zorganizować system plików z wielopoziomowym katalogiem.
Należy zrealizować aplikację konsolową, przyjmującą polecenia, wywoływaną z nazwą
pliku implementującego dysk wirtualny.
Należy zaimplementować następujące operacje, dostępne dla użytkownika tej aplikacji:
- tworzenie wirtualnego dysku (gdy plik wirtualnego dysku będący parametrem nie
istnieje to pytamy się o utworzenie przed przejściem do interakcji) - jak odpowiedź
negatywna to kończymy program. Parametrem polecenia powinien być rozmiar
tworzonego systemu plików w bajtach. Dopuszcza się utworzenie systemu
nieznacznie większego lub mniejszego, gdy wynika to z przyjętych założeń
dotyczących budowy.
- kopiowanie pliku z dysku systemu na dysk wirtualny,
- utworzenie katalogu na dysku wirtualnym (katalogi mogą być zagnieżdżane -
jednym poleceniem mkdir a/b/c tworzymy 3 katalogi)
- usunięcie katalogu z dysku wirtualnego
- kopiowanie pliku z dysku wirtualnego na dysk systemu,
- wyświetlanie katalogu dysku wirtualnego z informacją o rozmiarze (sumie) plików
w katalogu, rozmiarze plików w katalogu razem z podkatalogami (suma), oraz ilości
wolnej pamięci na dysku wirtualnym
- tworzenie twardego dowiązania do pliku lub katalogu
- usuwanie pliku lub dowiązania z wirtualnego dysku,
- dodanie do pliku o zadanej nazwie n bajtów
- skrócenie pliku o zadanej nazwie o n bajtów
- wyświetlenie informacji o zajętości dysku.

# Planowana implementacja

Implementacja w języku `C++`. System plików będzie zawarty w pojedynczym pliku na dysku fizycznym, gdzie będą zapisywane odpowiednie struktury.

### Planowane struktury

- super block - zawierający informacje ogólne o systemie plików (rozmiar, maksymalna dostępna pamięć, ilość inode i bloków danych, wskaźniki do nich, itd)

- inode (i-węzeł) - struktura opisująca plik - zawiera jego nazwę, tryb (czy katalog), rozmiar, wskaźnik na pierwszy blok danych

- blok danych - zawiera dane oraz wskaźnik na kolejny blok.


Na początku dysku będzie znajdowała się struktura super block, następnie tablica inode a później bloki danych.

Obsługa dysku wirtualnego będzie odbywać się poprzez linię komend, np.

- `./fs create <nazwa_dysku> <rozmiar>`

- `./fs <nazwa_dysku> <komenda> <argumenty>`

Będzie on obsługiwał wszystkie operacje, które są wymagane w treści zadania.

# Testowanie

Skrypty `.sh` wykonujące wszystkie możliwe operacje na dysku wirtualnym, na przykład:

- tworzenie plików

- kopiowanie z dysku systemu na dysk wirtualny

- usuwanie plików

- itd

Przy każdej operacji sprawdzanie czy zawartość plików jest taka, jakiej się spodziewaliśmy oraz czy poprawnie zmniejsza/zwiększa się zajętość dysku.

# Autor

Konrad Wojda, 310990