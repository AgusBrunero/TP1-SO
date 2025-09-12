#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
/*
"ǁǁ"
"Ͱ"
"ඩ"
"ᒥᒧᒪᒣ"
"―‖‗‗"
"⅂	⅃"
"	⎡	⎢	⎣	⎤	⎥	⎦"
"⎸	⎹"

"┖	┗	┘	┙┚	┛"
"┣ ┫	┰ ┸ ╂"
"╬╩╦╗╚╝╔"
*/

/*
 * Asumo parametros en forma:
 * 1- nombre del proceso (vista)
 * 2- Memcompartida del gamestate
 */
#include "defs.h"
#include "chompChampsUtils.h"

#define RESET   "\033[0m"
#define PLAYER1 "\033[91m"  // Rojo brillante
#define PLAYER2 "\033[92m"  // Verde brillante
#define PLAYER3 "\033[93m"  // Amarillo brillante
#define PLAYER4 "\033[94m"  // Azul brillante
#define PLAYER5 "\033[95m"  // Magenta brillante
#define PLAYER6 "\033[96m"  // Cian brillante
#define PLAYER7 "\033[97m"  // Blanco brillante
#define PLAYER8 "\033[33m"  // Amarillo normal
#define PLAYER9 "\033[35m"  // Magenta normal
char test = 0;
void printBoard(gameState_t * gameState);

/*
char * const viewArgs[] = 
{(char *)viewBinary, 
(char *)gameStateShmName, 
gameStateByteSizeStr,
(char *)semaphoresShmName, 
semaphoresByteSizeStr, 
NULL};*/

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Fallo en creacion de vista, argumentos: %s <nombre_shm>, %s <tamaño de bytes del gamestate>\n", argv[0], argv[1]);
        exit(EXIT_FAILURE);
    }


    gameState_t* gameState;
    semaphores_t* semaphores;
    openShMems(argv[1], strtol(argv[2], NULL, 10), &gameState, argv[3], strtol(argv[4], NULL, 10), &semaphores);

    while (!gameState->finished) {
        sem_wait(&semaphores->masterToView);
        printBoard(gameState);
        sem_post(&semaphores->viewToMaster);
    }

    printf("¡Hola! Soy la VISTA con PID: %d\n", getpid());
    printBoard(gameState);

    return 0;
}

void printBoard(gameState_t * gameState) {
    printf("\033[2J\033[H");
    for (int i = 0; i < gameState->height; i++) {
        for (int j = 0; j < gameState->width; j++) {
            bool isPlayer = false;
            int playerIdx = -1;
            for (int p = 0; p < gameState->playerCount; p++) {
                if (gameState->playerArray[p].x == j && gameState->playerArray[p].y == i) {
                    isPlayer = true;
                    playerIdx = p;
                    break;
                }
            }
            if (isPlayer) {
                const char* color = RESET;
                switch (playerIdx) {
                    case 0: color = PLAYER1; break;
                    case 1: color = PLAYER2; break;
                    case 2: color = PLAYER3; break;
                    case 3: color = PLAYER4; break;
                    case 4: color = PLAYER5; break;
                    case 5: color = PLAYER6; break;
                    case 6: color = PLAYER7; break;
                    case 7: color = PLAYER8; break;
                    case 8: color = PLAYER9; break;
                    default: color = RESET; break;
                }
                printf("%s ඞ%s ", color, RESET); // 2-width: Ñ + space
            } else {
                int cellValue = gameState->board[i * gameState->width + j];
                printf("%2d ", cellValue); // 2-width: num + space
            }
        }
        printf("\n");
    }
}