#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include "../defs.h"
#include "../chompChampsUtils.h"

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
}
