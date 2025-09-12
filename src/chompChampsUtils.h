#ifndef CHOMPCHAMPSUTILS_h
#define CHOMPCHAMPSUTILS_h
#include "defs.h"

void getGameState(gameState_t* gameState, semaphores_t* semaphores, gameState_t* gameStateBuffer);

void openShMems(const char* gameStateShmName, size_t gameStateByteSize, gameState_t** gameState,
               const char* semaphoresShmName, size_t semaphoresByteSize, semaphores_t** semaphores);

void checkMalloc(void* ptr, const char* msg, int exitCode);
#endif