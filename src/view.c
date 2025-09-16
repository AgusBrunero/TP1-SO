#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
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

#define MAX_PLAYERS 9

static void printBoard(gameState_t * gameState);
static void printRanking(gameState_t* gameState);
static const char * getPlayerColor(int playerIndex);


int main(int argc, char* argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Fallo en creacion de vista, argumentos: %s <nombre_shm>, %s <tamaño de bytes del gamestate>\n", argv[0], argv[1]);
        exit(EXIT_FAILURE);
    }
    
    gameState_t* gameState;
    semaphores_t* semaphores;
    openReadShm(strtoul(argv[1], NULL, 10), strtoul(argv[2], NULL, 10), &gameState, &semaphores);

    printf("\033[2J\033[H");    // limpiar pantalla
    printf("\033[?25l");        // Ocultar cursor


    while (!gameState->finished) {
        sem_wait(&semaphores->masterToView);
        printBoard(gameState);
        printRanking(gameState);
        sem_post(&semaphores->viewToMaster);
    }

    printf("\033[?25h");        // Mostrar cursor
    if (gameState != NULL) {
        size_t gameStateSize = sizeof(gameState_t) + (size_t)gameState->width * gameState->height * sizeof(int);
        if (munmap(gameState, gameStateSize) == -1) {
            perror("view: munmap(gameState) failed");
        }
     }
     if (semaphores != NULL) {
        if (munmap(semaphores, sizeof(semaphores_t)) == -1) {
            perror("view: munmap(semaphores) failed");
        }
     }

     return 0;
}


static const char * getPlayerColor(int playerIndex) {
    switch (playerIndex) {
        case 0: return PLAYER1;
        case 1: return PLAYER2;
        case 2: return PLAYER3;
        case 3: return PLAYER4;
        case 4: return PLAYER5;
        case 5: return PLAYER6;
        case 6: return PLAYER7;
        case 7: return PLAYER8;
        case 8: return PLAYER9;
        default: return RESET;
    }
}

static void printBoard(gameState_t * gameState) {
    
    printf("\033[H"); // Mover el cursor a la posición (0,0)
    printf("CHOMPCHAMPS: Mapa %dx%d | Estado: %s\n\n", gameState->width, gameState->height, gameState->finished ? "Finalizado" : "En juego");
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
            const char* color = RESET;
            if (isPlayer) {
                color = getPlayerColor(playerIdx);
                printf("%s ඞ%s ", color, RESET); // 2-width: Ñ + space
            } else {
                int cellValue = gameState->board[i * gameState->width + j];
                color = getPlayerColor(cellValue*(-1));
                if (cellValue > 0)
                    printf("%2d ", cellValue); // 2-width: num + space
                else
                    printf("%s ■%s ", color, RESET);
            }
        }
        printf("\n");
    }
}

static void printRanking(gameState_t* gameState) {
    playerRank_t rankings[MAX_PLAYERS];

    getPlayersRanking(gameState, rankings);

    if (gameState->finished){
        printf("\n=== RANKING FINAL ===\n"); 
    }else{
        printf("\n=== RANKING ===\n");
    }
    
    // Ancho maximo para los nombres de jugadores
    int nameWidth = 0;
    for (int i = 0; i < gameState->playerCount; i++) {
        int len = strlen(rankings[i].player->name);
        if (len > nameWidth) nameWidth = len;
    }
    if (nameWidth < 4) nameWidth = 4;
    nameWidth += 2;

    // Imprimir nombres en una sola línea, separados por |
    printf("Jugador:    ");
    for (int i = 0; i < gameState->playerCount; i++) {
        const char* color = getPlayerColor(rankings[i].originalIndex);
        printf("%s%-*s%s", color, nameWidth, rankings[i].player->name, RESET);
        if (i < gameState->playerCount - 1) printf(" | ");
    }
    printf("\nPuntaje:    ");
    for (int i = 0; i < gameState->playerCount; i++) {
        printf("%*d", nameWidth, rankings[i].player->score);
        if (i < gameState->playerCount - 1) printf(" | ");
    }
    printf("\nVálidos:    ");
    for (int i = 0; i < gameState->playerCount; i++) {
        printf("%*d", nameWidth, rankings[i].player->validReqs);
        if (i < gameState->playerCount - 1) printf(" | ");
    }
    printf("\nInválidos:  ");
    for (int i = 0; i < gameState->playerCount; i++) {
        printf("%*d", nameWidth, rankings[i].player->invalidReqs);
        if (i < gameState->playerCount - 1) printf(" | ");
    }
    printf("\nBloqueado:  ");
    for (int i = 0; i < gameState->playerCount; i++) {
        printf("%*s", nameWidth, rankings[i].player->isBlocked ? "Si" : "No");
        if (i < gameState->playerCount - 1) printf(" | ");
    }

    printf("\n\n");
}