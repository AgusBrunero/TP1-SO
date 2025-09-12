#include <errno.h>
#include <signal.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include "defs.h"
#include "chompChampsUtils.h"
#include <string.h>

#include <sys/mman.h>    // mmap, munmap, MAP_SHARED, PROT_READ, PROT_WRITE
#include <sys/shm.h>     // shm_open, shm_unlink
#include <fcntl.h>       // O_CREAT, O_RDWR, O_TRUNC
#include <unistd.h>      // ftruncate, close
    

void getGameState(gameState_t* gameState, semaphores_t* semaphores, gameState_t* gameStateBuffer) {

    sem_wait(&semaphores->masterMutex); // Si el master está escribiendo espero
    
    sem_wait(&semaphores->readersCountMutex);
    if (semaphores->readers_count++ == 0) {
        sem_wait(&semaphores->gameStateMutex); // paso a modo lectura
    }
    sem_post(&semaphores->readersCountMutex);

    // Copiar el estado del juego al buffer
    memcpy(gameStateBuffer, gameState, sizeof(gameState_t) + gameState->width * gameState->height * sizeof(int));

    if (semaphores->readers_count-- == 1) {
        sem_post(&semaphores->gameStateMutex); // paso a modo escritura
    }

    sem_post(&semaphores->masterMutex);
}


void openShMems(const char* gameStateShmName, size_t gameStateByteSize, gameState_t** gameState,
               const char* semaphoresShmName, size_t semaphoresByteSize, semaphores_t** semaphores){

    int gameStateShmFd = shm_open(gameStateShmName, O_RDWR, 0666);
    char * gameStateShm = mmap(NULL, gameStateByteSize, PROT_READ | PROT_WRITE, MAP_SHARED, gameStateShmFd, 0);
    *gameState  = (gameState_t*)gameStateShm;

    int semaphoresShmFd = shm_open(semaphoresShmName, O_RDWR, 0666);
    char * semaphoresShm = mmap(NULL, semaphoresByteSize, PROT_READ | PROT_WRITE, MAP_SHARED, semaphoresShmFd, 0);
    *semaphores = (semaphores_t*)semaphoresShm;
}

void checkMalloc(void* ptr, const char* msg, int exitCode) {
    if (ptr == NULL) {
        perror(msg);
        exit(exitCode);
    }
}