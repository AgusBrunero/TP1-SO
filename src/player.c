#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include "defs.h"
#include "chompChampsUtils.h"

unsigned char getNextMovement(gameState_t* gameState, int myIndex);

void sendChar(unsigned char c);

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
        getGameState(gameState, semaphores, savedGameState);
        unsigned char direction = getNextMovement(savedGameState, myIndex);
        sem_wait(&semaphores->playerSems[myIndex]);
        sendChar(direction);
    }
    free(savedGameState);

    munmap(gameState, sizeof(gameState_t) + gameState->width * gameState->height * sizeof(int));
    munmap(semaphores, sizeof(semaphores_t));
    return 0;
}

void sendChar(unsigned char c) {
    write(STDOUT_FILENO, &c, 1);
}


// DEFINIR EN UN ARCHIVO APARTE
unsigned char getNextMovement(gameState_t* gameState, int myIndex){
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
