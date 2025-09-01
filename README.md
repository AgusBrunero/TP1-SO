# ChompChamps
### TP 1 Sistemas Operativos
El trabajo práctico consiste en aprender a utilizar los distintos tipos de IPCs presentes en un sistema POSIX. Para ello se implementará el juego ChompChamps.

## Integrantes (Grupo 18):
- Agustín Julián Brunero 62013
- Ignacio Ferrero 64399
- Nicolás Pérez Stefan 65624

### Compilar:
```make```

### Correr:
```./run <FILAS> <COLUMNAS> <DELAY> <TURNOS> <SEED>```

### Para Tests:
```make tests```
```./bin/tests```

# TO-DO:
- [ ] Los parametros de alto y ancho se pasan mal a player
- [ ] El tablero se corta por problema de carrera
- [ ] No se imprime "Terminaron todos los hijos"
---
- [x] master.c:2: warning: "_GNU_SOURCE" redefined
- [x] master.c:152:116: warning: passing argument 5 of ‘playerFromBin’ from incompatible pointer type
- [x] master.c:97:85: note: expected ‘char *’ but argument is of type ‘gameState_t *’ {aka ‘struct gameStateStruct *’}
- [ ] master.c:192:42: warning: passing argument 2 of ‘newProc’ from incompatible pointer type
- [ ] master.c:69:43: note: expected ‘char **’ but argument is of type ‘const char **’
---
- [ ] player.c:16:12: warning: unused variable ‘shmMem’
---
- [ ] view.c:100:9: warning: unused variable ‘height’
- [ ] view.c:99:9: warning: unused variable ‘width’
