#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include "defs.h"
#include "chompChampsUtils.h"

/*
 * Decide el siguiente movimiento a partir del estado del juego
 */
unsigned char getNextMovement(gameState_t* gameState, int myIndex);

void sendChar(unsigned char c);

short getX(unsigned char direction);

short getY(unsigned char direction);

int main(int argc, char* argv[]) {
    srand(time(NULL));
    gameState_t* gameState;
    semaphores_t* semaphores;
    openReadShm(strtoul(argv[1], NULL, 10), strtoul(argv[2], NULL, 10), &gameState, &semaphores);

    gameState_t* savedGameState = malloc(sizeof(gameState_t) + gameState->width * gameState->height * sizeof(int));
    checkMalloc(savedGameState, "malloc failed for savedGameState", EXIT_FAILURE);

    pid_t myPid = getpid();

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

    while (!gameState->playerArray[myIndex].isBlocked) {
        sem_wait(&semaphores->playerSems[myIndex]);
        getGameState(gameState, semaphores, savedGameState);
        unsigned char direction = getNextMovement(savedGameState, myIndex);
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


unsigned char getNextMovement(gameState_t* gameState, int myIndex){
    unsigned short x = gameState->playerArray[myIndex].x;
    unsigned short y = gameState->playerArray[myIndex].y;

    unsigned char direcciones_validas[8];
    int num_direcciones = 0;

    if (y > 0)
        direcciones_validas[num_direcciones++] = NORTH;
    if (y < (gameState->height - 1))
        direcciones_validas[num_direcciones++] = SOUTH;
    if (x > 0)
        direcciones_validas[num_direcciones++] = WEST;
    if (x < (gameState->width - 1))
        direcciones_validas[num_direcciones++] = EAST;
    if (y > 0 && x > 0)
        direcciones_validas[num_direcciones++] = NORTHWEST;
    if (y > 0 && x < (gameState->width - 1))
        direcciones_validas[num_direcciones++] = NORTHEAST;
    if (y < (gameState->height - 1) && x > 0)
        direcciones_validas[num_direcciones++] = SOUTHWEST;
    if (y < (gameState->height - 1) && x < (gameState->width - 1))
        direcciones_validas[num_direcciones++] = SOUTHEAST;
    
    short bestX = 0;
    short bestY = 0;
    short bestValue = -10;

    for (int i = 0 ; i < num_direcciones ; i++){
        short newX = getX(direcciones_validas[i]);
        short newY = getY(direcciones_validas[i]);
        short newValue = gameState->board[gameState->width * (y + newY) + x + newX];
        if (newValue > bestValue){
            bestX = newX;
            bestY = newY;
            bestValue = newValue;
        }
    }
    unsigned char move [3][3] ={{NORTHWEST  , NORTH , NORTHEAST },
                                {WEST       , 254   , EAST      },
                                {SOUTHWEST  , SOUTH , SOUTHEAST }};  
                                
    
    return move[1 + bestY][1 + bestX];
    
}

short getX(unsigned char direction){
    switch (direction){
        case EAST:  
        case NORTHEAST:
        case SOUTHEAST:
            return 1;
        case WEST: 
        case NORTHWEST:
        case SOUTHWEST:
            return -1;
        case NORTH:
        case SOUTH:
        default:
            return 0;
    }
}

short getY(unsigned char direction){
    switch (direction){
        case NORTH:  
        case NORTHEAST:
        case NORTHWEST:
            return -1;
        case SOUTH: 
        case SOUTHEAST:
        case SOUTHWEST:
            return 1;
        case EAST:
        case WEST:
        default:
            return 0;
    }
}