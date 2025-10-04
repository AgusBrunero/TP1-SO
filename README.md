# ChompChamps
### TP 1 Sistemas Operativos

El trabajo práctico consiste en aprender a utilizar los distintos tipos de IPC presentes en un sistema POSIX. Para ello, se implementará el juego ChompChamps.

## Integrantes (Grupo 18):
- Agustín Julián Brunero 62013
- Ignacio Ferrero 64399
- Nicolás Pérez Stefan 65624

## Docker

Se utiliza la imagen `agodio/itba-so-multi-platform:3.0`.
Para facilitar su uso, se proveen los siguientes comandos:

**Descargar la imagen:**

```make docker-pull```

**Abrir una shell en el contenedor:**

```make docker-run```

**Instalar dependencias y configurar el entorno (ncurses y locales UTF-8 para la vista):**

```make setup```  

**Abrir una shell, instalar dependencias y configurar el entorno en el contenedor (ncurses y locales UTF-8 para la vista):**

```make docker-setup```

Nota: Las configuraciones del sistema no persisten entre ejecuciones del contenedor. 
Ejecutar ```make docker-setup``` cada vez que se inicie un nuevo contenedor.

### Compilación:

**Compilar el proyecto:**  

```make```  

**Limpiar archivos compilados:**  

```make clean```  

**Ejecutar prueba rápida con 2 jugadores y vista:**  

```make runtest```  

### Ejecución  

```./bin/master -w <COLUMNAS> -h <FILAS> -d <DELAY> -t <TIMEOUT> -s <SEED> -v ./bin/view -p ./bin/player ...```  

**Parámetros:**  

```-w:``` Ancho del tablero (default y mínimo: 10)  

```-h:``` Alto del tablero (default y mínimo: 10)  

```-d:``` Delay entre turnos en ms (default: 200)  

```-t:``` Timeout por turno en s (default: 10)  

```-s:``` Seed para generación aleatoria (default: time(NULL))  

```-v:``` Path al binario de la vista (opcional)  

```-p:``` Ejecutables de los jugadores (mínimo 1 máximo 9)  

## Ejemplo

**Con los siguientes comandos, en este orden, se obtiene la imagen de Docker utilizada, se abre el contenedor, se realiza la instalación de dependencias y configuración de caracteres, se compila el proyecto y se ejecuta un test en un tablero de 20×15 con 2 jugadores, vista y un delay de 50 ms, dejando el resto de los parámetros en sus valores por defecto:**

```make docker-pull```  

```make docker-setup```

```make```

```./bin/master -p ./bin/player ./bin/player -v ./bin/view -w 20 -h 15 -d 50```