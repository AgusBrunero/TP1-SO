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

void cleanupShm(const char* name) {
    shm_unlink(name);
}
void openReadShm(unsigned short width, unsigned short height, gameState_t** gameState, semaphores_t** semaphores) {
    fprintf(stderr, "openReadShm: Iniciando con width=%d, height=%d\n", width, height);

    // Mapear memoria compartida de gameState
    size_t gameStateByteSize = sizeof(gameState_t) + width * height * sizeof(int);
    fprintf(stderr, "openReadShm: Calculé gameStateByteSize=%zu\n", gameStateByteSize);

    int gameStateShmFd = shm_open("/game_state", O_RDONLY, 644);
    if (gameStateShmFd == -1) {
        perror("shm_open game_state");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "openReadShm: shm_open game_state exitoso\n");

    *gameState = mmap(NULL, gameStateByteSize, PROT_READ, MAP_SHARED, gameStateShmFd, 0);
    if (*gameState == MAP_FAILED) {
        perror("mmap gameState failed");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "openReadShm: mmap gameState exitoso at %p\n", (void*)*gameState); fflush(stderr);
    close(gameStateShmFd);

    // Mapear memoria compartida de semaphores
    fprintf(stderr, "hello wrold\n");
    size_t semaphoresByteSize = sizeof(semaphores_t);
    fprintf(stderr, "openReadShm: semaphoresByteSize=%zu\n", semaphoresByteSize);

    int semaphoresShmFd = shm_open("/game_sync", O_RDWR, 0666);
    if (semaphoresShmFd == -1) {
        perror("shm_open game_sync");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "openReadShm: shm_open game_sync exitoso\n");

    *semaphores = mmap(NULL, semaphoresByteSize, PROT_READ | PROT_WRITE, MAP_SHARED, semaphoresShmFd, 0);
    if (*semaphores == MAP_FAILED) {
        perror("mmap semaphores failed");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "openReadShm: mmap semaphores exitoso at %p\n", (void*)*semaphores);
    fflush(stderr);
    close(semaphoresShmFd);

    fprintf(stderr, "openReadShm: Terminando exitosamente\n");
}
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


void createShms(unsigned short width, unsigned short height) {
    cleanupShm("/game_state");
    cleanupShm("/game_sync");
    // Crear memoria compartida para gameState
    size_t gameStateByteSize = sizeof(gameState_t) + width * height * sizeof(int);
    int gameStateShmFd = shm_open("/game_state", O_CREAT | O_RDWR | O_TRUNC, 0777);
    if (gameStateShmFd == -1) {
        perror("shm_open (create) game_state");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(gameStateShmFd, gameStateByteSize) == -1) {
        perror("ftruncate game_state");
        close(gameStateShmFd);
        exit(EXIT_FAILURE);
    }
    close(gameStateShmFd);

    // Crear memoria compartida para semaphores
    size_t semaphoresByteSize = sizeof(semaphores_t);
    int semaphoresShmFd = shm_open("/game_sync", O_CREAT | O_RDWR | O_TRUNC, 0777);
    if (semaphoresShmFd == -1) {
        perror("shm_open (create) game_sync");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(semaphoresShmFd, semaphoresByteSize) == -1) {
        perror("ftruncate game_sync");
        close(semaphoresShmFd);
        exit(EXIT_FAILURE);
    }
    close(semaphoresShmFd);
}

void openShms(unsigned short width, unsigned short height, gameState_t** gameState, semaphores_t** semaphores) {
    // Mapear memoria compartida de gameState
    size_t gameStateByteSize = sizeof(gameState_t) + width * height * sizeof(int);
    int gameStateShmFd = shm_open("/game_state", O_RDWR, 0666);
    if (gameStateShmFd == -1) {
        perror("shm_open game_state");
        exit(EXIT_FAILURE);
    }
    *gameState = mmap(NULL, gameStateByteSize, PROT_READ | PROT_WRITE, MAP_SHARED, gameStateShmFd, 0);
    close(gameStateShmFd);

    // Mapear memoria compartida de semaphores
    size_t semaphoresByteSize = sizeof(semaphores_t);
    int semaphoresShmFd = shm_open("/game_sync", O_RDWR, 0666);
    if (semaphoresShmFd == -1) {
        perror("shm_open game_sync");
        exit(EXIT_FAILURE);
    }
    *semaphores = mmap(NULL, semaphoresByteSize, PROT_READ | PROT_WRITE, MAP_SHARED, semaphoresShmFd, 0);
    close(semaphoresShmFd);
}


void checkMalloc(void* ptr, const char* msg, int exitCode) {
    if (ptr == NULL) {
        perror(msg);
        exit(exitCode);
    }
}

void checkPid(pid_t pid, const char* msg, int exitCode){
    if (pid == -1) {
        perror(msg);
        exit(exitCode);
    }
}