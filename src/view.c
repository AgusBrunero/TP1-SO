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
static int comparePlayers(const void* a, const void* b);
static void printFinalRanking(gameState_t* gameState);
static const char * getPlayerColor(int playerIndex);

typedef struct {
    player_t* player;
    int originalIndex;
} PlayerRank;

int main(int argc, char* argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Fallo en creacion de vista, argumentos: %s <nombre_shm>, %s <tamaño de bytes del gamestate>\n", argv[0], argv[1]);
        exit(EXIT_FAILURE);
    }
    //printf("¡Hola! Soy la VISTA con PID: %d\n", getpid());
    
    gameState_t* gameState;
    semaphores_t* semaphores;
    openShms(strtoul(argv[1], NULL, 10), strtoul(argv[2], NULL, 10), &gameState, &semaphores);

    printf("\033[2J\033[H");    // limpiar pantalla
    printf("\033[?25l");        // Ocultar cursor


    while (!gameState->finished) {
        sem_wait(&semaphores->masterToView);
        printBoard(gameState);
        printFinalRanking(gameState);
        sem_post(&semaphores->viewToMaster);
    }

    printf("\033[?25h");        // Mostrar cursor
    munmap(gameState, sizeof(gameState_t) + gameState->width * gameState->height * sizeof(int));
    munmap(semaphores, sizeof(semaphores_t));

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

void printBoard(gameState_t * gameState) {
    
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



int comparePlayers(const void* a, const void* b) {
    const PlayerRank* p1 = (const PlayerRank*)a;
    const PlayerRank* p2 = (const PlayerRank*)b;
    
    // Comparar por puntaje (mayor a menor)
    if (p2->player->score != p1->player->score) {
        return p2->player->score - p1->player->score;
    }
    
    // Si hay empate, comparar por movimientos válidos (menor a mayor)
    if (p1->player->validReqs != p2->player->validReqs) {
        return p1->player->validReqs - p2->player->validReqs;
    }
    
    // Si persiste el empate, comparar por movimientos inválidos (menor a mayor)
    return p1->player->invalidReqs - p2->player->invalidReqs;
}

void printFinalRanking(gameState_t* gameState) {
    PlayerRank rankings[MAX_PLAYERS];

    // Inicializar el array de rankings
    for (int i = 0; i < gameState->playerCount; i++) {
        rankings[i].player = &gameState->playerArray[i];
        rankings[i].originalIndex = i;
    }

    // Ordenar jugadores
    qsort(rankings, gameState->playerCount, sizeof(PlayerRank), comparePlayers);

    // Imprimir resultados
    printf("\n=== RANKING FINAL ===\n");

    // Calculate max width for player names
    int nameWidth = 0;
    for (int i = 0; i < gameState->playerCount; i++) {
        int len = strlen(rankings[i].player->name);
        if (len > nameWidth) nameWidth = len;
    }
    if (nameWidth < 4) nameWidth = 4; // Minimum width for aesthetics
    nameWidth += 2; // Padding

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