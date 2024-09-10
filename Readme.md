# Cache Simulation with SystemC: Direct-Mapped and Fourway Set Associative

## Objective
Untersuchen die Unterscheide zwischen einem direkt-abgebildeten und 4-fach-assoziativen Cache.

## Theoretischer Teil - Übliche Größen und Latenzen

### Größen
- **Direkt-abgebildet** (Beispiel: COASt)
    - **L2-Cache**: 256-512 KB
- **4-fach-assoziativ** (Beispiel: Intel i7-6700 4,0GHz)<br>
Heutige Prozessoren verwenden eine Mischung von n-fach-assoziativen Caches
    - **L1-Cache**: 32 KB
    - **L2-Cache**: 256 KB
    - **L3-Cache**: 8 MB

    Latenzen in Zyklen:
    - **L1-Cache**: 4
    - **L2-Cache**: 12
    - **L3-Cache**: 42
    - **Hauptspeicher**: 42 + 51ns

Quelle: 
- https://www.7-cpu.com/cpu/Skylake.html
- https://en.wikipedia.org/wiki/Cache_on_a_stick

## Ausführung des Projekts
1. Zum `src/`-Ordner gehen
    ```
    cd src/
    ```
2. Dieses Projekt kompilieren (das auszuführende Programm befindet sich im `out/`-Ordner)
    ```
    make
    ```
3. Das Programm ausführen <br> Auf Anfrage modifizierbar
    ```
    ../out/simulation --cycles 10000 --fourway --cacheline-size 4 --cachelines 8 --cache-latency 1 --memory-latency 1 --tf=tracefile ../examples/matrix_multiplication.csv
    ```

## Implementierung

### Structs:
- `io_structs`: `Request` and `Result` struct wie in der Aufgabenstellung definiert
- `address_structs`:
    - `CacheConfig`: Anzahl von Tag-, Offset- und Index-Bits definieren
    - `CacheAddress`: `request.addr` zu Tag, Index und Offset parsen

### Direkt-abgebildet
- ein Array von `CacheEntry`-Objekten mit der Größe `cacheLines`. `CacheEntry` verhält sich als das Index und enthält: 
    - Tag, der später beim Zugriff verglichen wird
    - Array von Daten mit Größe von `numberOfOffsetBits`<sup>2</sup>


### 4-fach-assoziativ
Ersetzungsstrategie: LRU
- Das Verhalten von `FourWayLRUCache` ist ähnlich zu `DirectMappedCache` aber mit einem `LRUCache`-Array statt `CacheEntry` mit Größe von `numberOfOffsetBits`<sup>2</sup>
- `LRUCache` enthält:
    - eine HashMap mit Tag (Key) und Zeiger zum entsprechenden Node (Value)
    - eine DoublyLinkedList mit MRU $\longleftrightarrow $ LRU Struktur

## Simulation
**$4\times4$ Matrixmultiplikation von $A\times B=C$**
- Data und Adresse werden in `matrix_multiplication.csv` bestimmt
- Eine Ausführung der $4\times4$ Matrixmultiplikation in 11th Gen Intel(R) Core(TM) i7-1195G7 hat 68,242% Cache-Misses (mithilfe linux-perf)

### CacheModule
- Verwendung des polymorphischen Caches mit einer Adressgröße von 16-Bit, der sich entweder in `FourWayLRUCache` oder `DirectMappedCache` verwandelt
- Ein Zyklus für das Daten-Holen und Warten auf `cacheLatency`(zusätzlich auf `memoryLatency` bei Cache-Miss)

### run_simulation() in simulation.cpp
- Signale im `CacheModule` werden mit aktuellem `Request` bei jedem Zyklus aktualisiert<br>
- Manuell gerechnete Daten in `simulation.cpp` werden mit Daten von `matrix_multiplication.csv` verglichen, um die Korrektheit des Speicherverhaltens des Caches sicherzustellen

### Primitive Gate
Benötigte Gatteranzahl:
- 1-bit Speicher = 4 Gatter
- Der ganze Speicher = Anzahl Cachezeilen $\times$ Cachezeilengröße $\times$ 1-bit Speicher $\times$ 8 
- Control Logic = 5 $\times$ Anzahl Cachezeilen
- Tag-Comparator = 2 $\times$ Anzahl Tag-Bits $\times$ Anzahl Cachezeilen
- Summe = ganzer Speicher $+$ Control Logic $+$ Tag Comparator

Zusätzlich für 4-fach:
- 2-bit Speicher für Zähler =  2 $\times$ 1-Bit Speicher $\times$ Anzahl Cachezeilen
- Comparator = 2-bit Zähler $\times$ 2
- Update logic = 2-bit Zähler $\times$ 7
- LRU = 2-bit Zähler $+$ Comparator $+$ Update Logic 
- Summe für 4-fach = Summe $+$ LRU

## Simulationsergebnis
`result.misses` in `DirectMappedCache` > `FourWayLRUCache`

Aus diesem Grund verwenden moderne Prozessoren n-fach-assoziativen Cache statt direkt-abgebildetes Caches, da sich n-fach-assoziativer Cache als eine Alternative zwischen direkt-abgebildetem und voll-assoziativem Cache verhält, was ziemlich schnelle Suche und weniger Conflict-Misses anbietet.
