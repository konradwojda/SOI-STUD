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

Implementacja w języku `C++`. System plików będzie zawarty w pojedynczym pliku na dysku fizycznym, gdzie będą zapisywane odpowiednie struktury. Struktura całego systemu będzie podobna do systemu plików w Unix V7.

### Planowane struktury

- superblock - ogólne informacje o systemie plików:
    - magic number - identyfikacja pliku jako system plików
    - wielkość bloku danych
    - ilość bloków danych
    - ilość wolnych bloków
    - informacje o i-węzłach:
        - ilość wolnych iNode
        - indeks następnego wolnego iNode
        - ilość iNode ogólnie
        - wielkość jednego iNode
    - wskaźniki na:
        - listę iNode
        - listę bloków danych
        - początek bloków iNode
        - początek bloków danych

- iNode (i-węzeł) - struktura opisująca plik:
    - typ pliku (plik, katalog)
    - czas utworzenia
    - ilość twardych dowiązań
    - jeśli plik:
        - rozmiar pliku
        - ilość zajętych bloków
        - wskaźnik na tablicę ze wskaźnikami na bloki danych
    - jeśli katalog:
        - wskaźnik na tablicę z i-węzłami plików znajdujących się w katalogu

- blok danych - zawiera dane i wskaźnik na kolejny blok danych

Na początku dysku będzie znajdowała się struktura super block, następnie tablica inode a później bloki danych.

Obsługa dysku wirtualnego będzie odbywać się poprzez linię komend, np.

- `./fs create <nazwa_dysku> <rozmiar>`

- `./fs <nazwa_dysku> <komenda> <argumenty>`

Będzie on obsługiwał wszystkie operacje, które są wymagane w treści zadania.

# Testowanie

Skrypty `.sh` wykonujące wszystkie możliwe operacje na dysku wirtualnym, na przykład:

- tworzenie plików

- kopiowanie z dysku systemu na dysk wirtualny

- rozszerzanie rozmiaru plików

- zmniejszanie rozmiaru plików

- usuwanie plików

- itd

Przy każdej operacji sprawdzanie czy zawartość plików jest taka, jakiej się spodziewaliśmy oraz czy poprawnie zmniejsza/zwiększa się zajętość dysku.

# Autor

Konrad Wojda, 310990