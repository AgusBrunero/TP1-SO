#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "defs.h"
#include "chompChampsUtils.h"

int main(int argc, char* argv[]) {

    printf("%d %s %s %s %s %s %s\n", argc ,argv[0],argv[1],argv[2],argv[3],argv[4],argv[5]);
    gameState_t* gameState;
    semaphores_t* semaphores;
    openShMems(argv[1], strtol(argv[2], NULL, 10), &gameState, argv[3], strtol(argv[4], NULL, 10), &semaphores);

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
