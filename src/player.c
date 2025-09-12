#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "defs.h"
#include "chompChampsUtils.h"

// argv[1] = widht
// argv[2] = height

int main(int argc, char* argv[]) {
    gameState_t* gameState;
    semaphores_t* semaphores;
    openShms(strtoul(argv[1], NULL, 10), strtoul(argv[2], NULL, 10), &gameState, &semaphores);

    gameState_t* savedGameState = malloc(sizeof(gameState_t) + gameState->width * gameState->height * sizeof(int));
    checkMalloc(savedGameState, "malloc failed for savedGameState", EXIT_FAILURE);


    printf("¡Hola! Soy un JUGADOR con PID: %d\n", getpid());

    while (!gameState->finished) {
        //getGameState(gameState, semaphores, savedGameState); // se guarda una captura del estado actual del juego
        
        // TODO: decidir_siguiente_movimiento();
        // TODO: enviar_movimientos();

        usleep(10000); // Pequeña pausa para evitar saturación
    }
    return 0;
}
