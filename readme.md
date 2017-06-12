# Dokumentacja
Autorem projektu jest Mateusz Grzegorczyk.

## Uruchomienie projektu
Aby skompilować należy skorzystać z `CMakeLists.txt`

Należy kolejno wywołać:  
`cmake CMakeLists.txt`  
Następnie, gdy pliki make zostaną poprawnie wygenerowane wywołać komendę  
`make`


Po uruchomieniu komendy `make` powinnien zostać stworzony plik wykonywalny: **SyncDaemon**.

Aby przetestować działanie programu, należy go odpalić z poziomu konsoli oraz podać dwa wymagane argumenty, które są ścieżkami do katalogów.
```
./SyncDaemon [folder źródłowy] [folder docelowy]
```
Przykładowe wywołanie.
```
./SyncDaemon katalogA katalogB
```

## Funkcjonalności
- Odpalenie programu jako daemon.
- Sprawdzenie czy katalogi podane jako argumenty istnieją.
- Synchronizowanie katalogów
- Kopiowanie plików
- Sprawdzanie czasu modyfikacji pliku.
- Ustawianie czasu modyfikacji pliku.
- Logowanie wszelkich akcji w logu systemowym (/var/log/syslog/)
- Wybudzanie daemona poprzez sygnał SIGUSR1.