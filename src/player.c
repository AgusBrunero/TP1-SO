#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "defs.h"


/*
 * Espero 2 parametro en forma shm_open name, and byteSize
 */
/*
    char * const argv[5] = {
        binPath,
        (char *)gameStateShmName,
        arg2,
        (char *)semaphoresShmName,
        arg3,
        NULL
    };*/
int main(int argc, char* argv[]) {
    int gameStateShmFd = shm_open(argv[1], O_RDWR, 0666);
    char * gameStateShm = mmap(NULL, strtol(argv[2], NULL, 10), PROT_READ | PROT_WRITE, MAP_SHARED, gameStateShmFd, 0);
    gameState_t* gameState = (gameState_t*)gameStateShm;

    int semaphoresShmFd = shm_open(argv[3], O_RDWR, 0666);
    char * semaphoresShm = mmap(NULL, strtol(argv[4], NULL, 10), PROT_READ | PROT_WRITE, MAP_SHARED, semaphoresShmFd, 0);
    semaphores_t* semaphores = (semaphores_t*)semaphoresShm;

    printf("¡Hola! Soy un JUGADOR con PID: %d\n", getpid());
    printf("Recibí ancho: %s, alto: %s\n", argv[1], argv[2]);

    int playerIndex = 0; // TODO: obtener índice real del jugador (siempre es jugador 1)

    while (!gameState->finished) {
        /*
        sem_wait(&gameState->sems.writer);
        // int playerIndex = 0;
        // gameState->hasPendingMove[playerIndex] = 1;
        sem_post(&gameState->sems.writer);

        sem_wait(&gameState->sems.readers_count_mutex);
        if (gameState->sems.readers_count++ == 0) {
            sem_wait(&gameState->sems.mutex);
        }
        sem_post(&gameState->sems.readers_count_mutex);

        // TODO: consultar_estado();

        sem_wait(&gameState->sems.readers_count_mutex);
        if (--gameState->sems.readers_count == 0) {
            sem_post(&gameState->sems.mutex);
        }
        sem_post(&gameState->sems.readers_count_mutex);

        // TODO: decidir_siguiente_movimiento();
        // TODO: enviar_movimientos();
        */

        usleep(10000); // Pequeña pausa para evitar saturación
    }
    return 0;
}
