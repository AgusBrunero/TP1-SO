#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include "../defs.h"
#include "../chompChampsUtils.h"

unsigned char getNextMovement(gameState_t* gameState, int myIndex){
    return EAST;
}
