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

void printBoard(gameState_t * gameState);
int comparePlayers(const void* a, const void* b);
void printFinalRanking(gameState_t* gameState);

typedef struct {
    player_t* player;
    int originalIndex;
} PlayerRank;

int main(int argc, char* argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Fallo en creacion de vista, argumentos: %s <nombre_shm>, %s <tamaño de bytes del gamestate>\n", argv[0], argv[1]);
        exit(EXIT_FAILURE);
    }
    printf("¡Hola! Soy la VISTA con PID: %d\n", getpid());
    
    gameState_t* gameState;
    semaphores_t* semaphores;
    openShms(strtoul(argv[1], NULL, 10), strtoul(argv[2], NULL, 10), &gameState, &semaphores);

    while (!gameState->finished) {
        sem_wait(&semaphores->masterToView);
        printBoard(gameState);
        sem_post(&semaphores->viewToMaster);
    }

<<<<<<< HEAD
    munmap(gameState, sizeof(gameState_t) + gameState->width * gameState->height * sizeof(int));
    munmap(semaphores, sizeof(semaphores_t));
=======
    printFinalRanking(gameState);
>>>>>>> a4387f54d69e5fad1c2f967d92dc8ed39a0334b1
    return 0;
}

void printBoard(gameState_t * gameState) {
    printf("\033[2J\033[H");    // limpiar pantalla
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
                switch (cellValue) {
                    case 0: color = PLAYER1; break;
                    case -1: color = PLAYER2; break;
                    case -2: color = PLAYER3; break;
                    case -3: color = PLAYER4; break;
                    case -4: color = PLAYER5; break;
                    case -5: color = PLAYER6; break;
                    case -6: color = PLAYER7; break;
                    case -7: color = PLAYER8; break;
                    case -8: color = PLAYER9; break;
                    default: color = RESET; break;
                }
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
    
    for (int i = 0; i < gameState->playerCount; i++) {
        const char* color;
        switch (rankings[i].originalIndex) {
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
        
        // Verificar si hay empate con el jugador anterior
        bool isEqual = i > 0 && 
                      rankings[i].player->score == rankings[i-1].player->score &&
                      rankings[i].player->validReqs == rankings[i-1].player->validReqs &&
                      rankings[i].player->invalidReqs == rankings[i-1].player->invalidReqs;
        
        printf("%s%d%s. %s%s%s\n", 
               color,
               isEqual ? i : i + 1,  // Si hay empate, usar la misma posición
               RESET,
               color,
               rankings[i].player->name,
               RESET);
        printf("   Puntaje: %d\n", rankings[i].player->score);
        printf("   Movimientos válidos: %d\n", rankings[i].player->validReqs);
        printf("   Movimientos inválidos: %d\n", rankings[i].player->invalidReqs);
        printf("\n");
    }
}