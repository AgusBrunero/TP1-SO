//
// Created by nacho on 8/25/2025.
//
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
void printBoard(gameState_t * gameState) {
    for (int i = 0; i < gameState->height; i++) {
        for (int j = 0; j < gameState->width; j++) {
            bool isPlayer = false;
            int playerIdx = -1;
            for (int p = 0; p < gameState->playerCount; p++) {
                if (gameState->playerArr[p].x == j && gameState->playerArr[p].y == i) {
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

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Fallo en creacion de vista, argumentos: %s <nombre_shm>, %s <tamaño de bytes del gamestate>\n", argv[0], argv[1]);
        exit(EXIT_FAILURE);
    }
    const char* shm_name = argv[1];
    const int shm_size = strtol(argv[2], NULL, 10);

    // Primero abrimos el segmento y mapeamos sólo el struct base para leer width y height
    int shm_fd = shm_open(shm_name, O_RDONLY, 0666);
    if (shm_fd == -1) {
        perror("error en shm_open en vista");
        exit(EXIT_FAILURE);
    }
    gameState_t * gameState = mmap(NULL, shm_size, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (gameState == MAP_FAILED) {
        perror("mmap base en vista");
        exit(EXIT_FAILURE);
    }

    int width = gameState->width;
    int height = gameState->height;
  //  gameState->board[width * height - 1] = -5; // Solo para probar que es readOnly escribir

    //printf("¡Hola! Soy la VISTA con PID: %d\n", getpid());
    printBoard(gameState);

    munmap(gameState, shm_size);
    close(shm_fd);
    return 0;
}