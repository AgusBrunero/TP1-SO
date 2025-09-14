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
- [ ] COMPLETAR freeResources con todos los free y funciones para liberar recursos de mallocs, memoria compartida, etc.
- [ ] Manejar los tiempos de las demoras en vista y jugadores. (para monitorear según pide el enunciado y aplicar timeout en caso de ser necesario)
- [ ] mostrar en vista el estado COMPLETO del juego (puntajes, posición, etc. según marca el enunciado) 