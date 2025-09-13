#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include "defs.h"
#include "chompChampsUtils.h"

#define NORTH 0
#define NORTHEAST 1
#define EAST 2
#define SOUTHEAST 3
#define SOUTH 4
#define SOUTHWEST 5
#define WEST 6
#define NORTHWEST 7

unsigned char decidir_siguiente_movimiento(gameState_t* gameState, int myIndex) {
    unsigned short x = gameState->playerArray[myIndex].x;
    unsigned short y = gameState->playerArray[myIndex].y;
    
    unsigned char direcciones_validas[8];
    int num_direcciones = 0;
    
    // Norte
    if (y > 0 && gameState->board[y-1 * gameState->width + x] == 1)
        direcciones_validas[num_direcciones++] = NORTH;
    
    // Noreste
    if (y > 0 && x < gameState->width-1 && gameState->board[(y-1) * gameState->width + (x+1)] == 1)
        direcciones_validas[num_direcciones++] = NORTHEAST;
    
    // Este
    if (x < gameState->width-1 && gameState->board[y * gameState->width + (x+1)] == 1)
        direcciones_validas[num_direcciones++] = EAST;
    
    // Sureste
    if (y < gameState->height-1 && x < gameState->width-1 && gameState->board[(y+1) * gameState->width + (x+1)] == 1)
        direcciones_validas[num_direcciones++] = SOUTHEAST;
    
    // Sur
    if (y < gameState->height-1 && gameState->board[(y+1) * gameState->width + x] == 1)
        direcciones_validas[num_direcciones++] = SOUTH;
    
    // Suroeste
    if (y < gameState->height-1 && x > 0 && gameState->board[(y+1) * gameState->width + (x-1)] == 1)
        direcciones_validas[num_direcciones++] = SOUTHWEST;
    
    // Oeste
    if (x > 0 && gameState->board[y * gameState->width + (x-1)] == 1)
        direcciones_validas[num_direcciones++] = WEST;
    
    // Noroeste
    if (y > 0 && x > 0 && gameState->board[(y-1) * gameState->width + (x-1)] == 1)
        direcciones_validas[num_direcciones++] = NORTHWEST;
    
    // Si no hay direcciones válidas, retornar una dirección aleatoria
    if (num_direcciones == 0)
        return rand() % 8;
    
    // Seleccionar una dirección válida aleatoria
    return direcciones_validas[rand() % num_direcciones];
}

int main(int argc, char* argv[]) {
    srand(time(NULL));
    gameState_t* gameState;
    semaphores_t* semaphores;
    openShms(strtoul(argv[1], NULL, 10), strtoul(argv[2], NULL, 10), &gameState, &semaphores);

    gameState_t* savedGameState = malloc(sizeof(gameState_t) + gameState->width * gameState->height * sizeof(int));
    checkMalloc(savedGameState, "malloc failed for savedGameState", EXIT_FAILURE);

    pid_t myPid = getpid();
    printf("¡Hola! Soy un JUGADOR con PID: %d\n", myPid);

    int myIndex = -1;
    for (int i = 0; i < gameState->playerCount; i++) {
        if (gameState->playerArray[i].pid == myPid) {
            myIndex = i;
            break;
        }
    }

    if (myIndex == -1) {
        fprintf(stderr, "Error: No se encontró el jugador en el gameState\n");
        exit(EXIT_FAILURE);
    }

    while (!gameState->finished) {
        // Esperar que master nos permita enviar un movimiento
        sem_wait(&semaphores->playerSems[myIndex]);
        
        // Tomar mutex para leer el estado
        sem_wait(&semaphores->readersCountMutex);
        semaphores->readers_count++;
        if (semaphores->readers_count == 1) {
            sem_wait(&semaphores->gameStateMutex);
        }
        sem_post(&semaphores->readersCountMutex);

        unsigned char direccion = decidir_siguiente_movimiento(gameState, myIndex);
        
        // Liberar mutex de lectura
        sem_wait(&semaphores->readersCountMutex);
        semaphores->readers_count--;
        if (semaphores->readers_count == 0) {
            sem_post(&semaphores->gameStateMutex);
        }
        sem_post(&semaphores->readersCountMutex);

        // Esperar a que el master esté listo para recibir movimientos
        sem_wait(&semaphores->masterMutex);
        // TODO: Actualizar la posición del jugador basado en la dirección
        sem_post(&semaphores->masterMutex);

        usleep(10000); // Pequeña pausa para evitar saturación
    }
    free(savedGameState);
    return 0;
}
