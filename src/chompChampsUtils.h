#ifndef CHOMPCHAMPSUTILS_h
#define CHOMPCHAMPSUTILS_h
#include "defs.h"

void getGameState(gameState_t* gameState, semaphores_t* semaphores, gameState_t* gameStateBuffer);

//void openShMems(const char* gameStateShmName, size_t gameStateByteSize, gameState_t** gameState,
//               const char* semaphoresShmName, size_t semaphoresByteSize, semaphores_t** semaphores);

void createShms(unsigned short width, unsigned short height);

void checkMalloc(void* ptr, const char* msg, int exitCode);

void checkPid(pid_t pid, const char* msg, int exitCode);

/*
 * Abre las memorias compartidas necesarias para el juego y las guarda en los punteros dados
 */
void openShms(unsigned short width, unsigned short height, gameState_t** gameState, semaphores_t** semaphores);

#endif