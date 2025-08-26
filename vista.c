//
// Created by nacho on 8/25/2025.
//
#include <stdio.h>
#include <unistd.h>
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
    // Print top border
    printf("╔");
    for (int j = 0; j < gameState->width - 1; j++) {
        printf("══╦");
    }
    printf("══╗\n");

    for (int i = 0; i < gameState->height; i++) {
        printf("║");
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
            const char* color = RESET;
            if (isPlayer) {
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
                printf("%s Ñ %s", color, RESET);
            } else {
                printf("   "); // Unoccupied cell: white/empty
            }
            if (j < gameState->width - 1)
                printf("║");
        }
        printf("║\n");
        if (i < gameState->height - 1) {
            printf("╠");
            for (int j = 0; j < gameState->width - 1; j++) {
                printf("══╬");
            }
            printf("══╣\n");
        }
    }
    // Print bottom border
    printf("╚");
    for (int j = 0; j < gameState->width - 1; j++) {
        printf("══╩");
    }
    printf("══╝\n");
}

int main(int argc, char* argv[]) {
    gameState_t * gameState = NULL;
    sscanf(argv[1], "%p", (void**)&gameState);
    printf("¡Hola! Soy la VISTA con PID: %d\n", getpid());
    // printf("Recibí ancho[%d]: %s, alto: %s\n", 2, argv[0], argv[1]);
    sleep(1);  // Simular trabajo
    printBoard(gameState);
    printf("⠀⠀⠀⠀⠀⠀⠀⠈⠀⠀⠀⠀⠀⠀⠀⠈⠈⠉⠉⠈⠈⠈⠉⠉⠉⠉⠉⠉⠉⠉⠙⠻⣄⠉⠉⠉⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n"
           "Vista terminando...\n");
    return 0;
}