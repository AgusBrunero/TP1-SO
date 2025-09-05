//
// Created by nacho on 8/25/2025.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "defs.h"


/*
 * Espero 2 parametro en forma shm_open name, and byteSize
 */
int main(int argc, char* argv[]) {
    int shrMemFd = shm_open(argv[1], O_RDWR, 0666);
    char * shmMem = mmap(NULL, strtol(argv[2], NULL, 10), PROT_READ | PROT_WRITE, MAP_SHARED, shrMemFd, 0);
    gameState_t* gameState = (gameState_t*)shmMem;


    printf("¡Hola! Soy un JUGADOR con PID: %d\n", getpid());
    printf("Recibí ancho: %s, alto: %s\n", argv[1], argv[2]);
    while (!gameState->finished) {
        sem_wait(&gameState->sems.writer);
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

        usleep(10000); // Pequeña pausa para evitar saturación
    }
    return 0;
}
