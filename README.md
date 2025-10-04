# ChompChamps
### TP 1 Sistemas Operativos

El trabajo práctico consiste en aprender a utilizar los distintos tipos de IPCs presentes en un sistema POSIX. Para ello se implementará el juego ChompChamps.

## Integrantes (Grupo 18):
- Agustín Julián Brunero 62013
- Ignacio Ferrero 64399
- Nicolás Pérez Stefan 65624

## Docker

Se utiliza la imagen `agodio/itba-so-multi-platform:3.0`. Para facilitar su uso se proveen los siguientes comandos:

**Descargar la imagen:**

```make docker-pull```

**Abrir una shell en el contenedor:**

```make docker-run```

**Abrir una shell y configurar el entorno en el contenedor (instala ncurses y locales UTF-8 para la vista):**

```make docker-setup```

Nota: Las configuraciones del sistema no persisten entre ejecuciones del contenedor. Ejecutar make docker-setup cada vez que se inicie un nuevo contenedor.

### Compilar:

**Instalar dependencias (solo si no se usa Docker):**  

```make setup```  

**Compilar el proyecto:**  

```make```  

**Limpiar archivos compilados:**  

```make clean```  

**Ejecutar prueba con 2 jugadores y vista:**  

```make runtest```  

### Ejecución  

```./bin/master -w <COLUMNAS> -h <FILAS> -d <DELAY> -s <TIMEOUT> -s <SEED> -v ./bin/view -p ./bin/player ...```  

**Parámetros:**  

```-w:``` Ancho del tablero (default y mínimo: 10)  

```-h:``` Alto del tablero (default y mínimo: 10)  

```-d:``` Delay entre turnos en ms (default: 200)  

```-t:``` Timeout por turno en s (default: 10)  

```-s:``` Seed para generación aleatoria (default: time(NULL))  

```-v:``` Path al binario de la vista (opcional)  

```-p:``` Ejecutables de los jugadores (mínimo 1 máximo 9)  
