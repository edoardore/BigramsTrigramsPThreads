# BigramsTrigramsPThreads
Progetto finale di Parallel Computing, versione parallela.

## Utilizzo
Eseguire il file main.c per il programma in parallelo, mentre per la versione sequenziale visitare la repo [BigramsTrigramsC](https://github.com/edoardore/BigramsTrigramsC).


## Esecuzione
Il programma leggerà i file all'interno della directory Gutenberg\txt e ne calcolerà i bigrammi ed i trigrammi.
È possibile impostare il numero di Thread Produttori e Consumatori nel file main.c
```c
int nProd = 4;
int nCons = 2;
```


## Esempio di output del file ```main.c```
```
aa: 439
ab: 104371
ac: 163436
...
zzi: 1362
zzl: 1147
zzo: 14
zzy: 94
zzz: 10
Tempo totale di esecuzione: ms 8059

Process finished with exit code 0
```

## Librerie utilizzate
```c
#include <pthread.h>
#include <assert.h>
#include <malloc.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdatomic.h>
```

## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.
Please make sure to update tests as appropriate.


## License
[Edoardo Re](https://github.com/edoardore), 2020
