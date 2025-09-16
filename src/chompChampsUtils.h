#ifndef CHOMPCHAMPSUTILS_h
#define CHOMPCHAMPSUTILS_h
#include "defs.h"


/*
 * guarda en gameStateBuffer una captura del estado actual del juego gameState,
 * verificando el estado de los semaforos para acceder a la lectura del mismo. 
 */
void getGameState(gameState_t* gameState, semaphores_t* semaphores, gameState_t* gameStateBuffer);

/*
 * Abre/crea las memorias compartidas 
 */
void createShms(unsigned short width, unsigned short height);

/*
 * Verifica que el malloc haya funcionado satisfactoriamente  
 */
void checkMalloc(void* ptr, const char* msg, int exitCode);

/* 
 * Verifica que la creacion del proceso con PID = pid haya sido exitoso, de lo contrario termina el programa.
 */
void checkPid(pid_t pid, const char* msg, int exitCode);

/*
 * Abre las memorias compartidas necesarias para el juego y las guarda en los punteros dados
 */
void openShms(unsigned short width, unsigned short height, gameState_t** gameState, semaphores_t** semaphores);

void openReadShm(unsigned short width, unsigned short height, gameState_t** gameState, semaphores_t** semaphores);

typedef struct playerRank_t{
    player_t* player;
    int originalIndex;
} playerRank_t;

/*
 * Compara dos playerRank_t de jugadores
 */
int comparePlayersRank(const void* a, const void* b);

/*
 * Guarda en un array de playerRank, el ranking (ordenado) de jugadores en gamestate.
 */
void getPlayersRanking(gameState_t * gameState, playerRank_t * playerRank);
#endif