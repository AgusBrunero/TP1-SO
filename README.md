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
- [ ] Corregir la posición inicial de los players (ningun jugador debe tener ventajas de movimiento según enunciado, en lugar de ponerlos en las esquinas usar rand)
- [ ] El primer ciclo de master no detecta el movimiento pendiente de player