# ChompChamps
### TP 1 Sistemas Operativos
El trabajo práctico consiste en aprender a utilizar los distintos tipos de IPCs presentes en un sistema POSIX. Para ello se implementará el juego ChompChamps.

## Integrantes (Grupo 18):
- Agustín Julián Brunero 62013
- Ignacio Ferrero 64399
- Nicolás Pérez Stefan 65624

### Compilar:
```make```

### Correr prueba con 3 jugadores:
```./run <COLUMNAS> <FILAS> <DELAY> <TIMEOUT> <SEED>```

```./bin/master -w <COLUMNAS> -h <FILAS> -d <DELAY> -s <TIMEOUT> -s <SEED> -v ./bin/view -p ./bin/player ...```