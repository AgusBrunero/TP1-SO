#ifndef DEFS_H
#define DEFS_H
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>

#define NORTH 0
#define NORTHEAST 1
#define EAST 2
#define SOUTHEAST 3
#define SOUTH 4
#define SOUTHWEST 5
#define WEST 6
#define NORTHWEST 7

typedef struct semaphoresStruct {
    sem_t masterToView;  // El máster le indica a la vista que hay cambios por
                         // imprimir
    sem_t viewToMaster;  // La vista le indica al máster que terminó de imprimir
    sem_t masterMutex;  // Mutex para evitar inanición del máster al acceder al
                        // estado
    sem_t gameStateMutex;     // Mutex para el estado del juego
    sem_t readersCountMutex;  // Mutex para la siguiente variable
    int readers_count;        // Cantidad de jugadores leyendo el estado
    sem_t playerSems[9];      // Le indican a cada jugador que puede enviar 1
                              // movimiento
} semaphores_t;

typedef struct playerStruct {
    char name[16];             // Nombre del jugador
    unsigned int score;        // Puntaje
    unsigned int invalidReqs;  // Cantidad de solicitudes de movimientos
                               // inválidas realizadas
    unsigned int
        validReqs;  // Cantidad de solicitudes de movimientos válidas realizadas
    unsigned short x, y;  // Coordenadas x e y en el tablero
    pid_t pid;            // Identificador de proceso
    bool isBlocked;       // Indica si el jugador está bloqueado
} player_t;

typedef struct gameStateStruct {
    unsigned short width;      // Ancho del tablero
    unsigned short height;     // Alto del tablero
    unsigned int playerCount;  // Cantidad de jugadores
    player_t playerArray[9];   // Lista de jugadores
    bool finished;             // Indica si el juego se ha terminado
    int board[];  // Puntero al comienzo del tablero. fila-0, fila-1, ...,
                  // fila-n-1
} gameState_t;

#endif  // DEFS_H
