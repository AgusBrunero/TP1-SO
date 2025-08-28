//
// Created by nacho on 8/25/2025.
//

#ifndef DEFS_H
#define DEFS_H
#include <signal.h>
#include <stdbool.h>

typedef struct playerStruct {
    char name[16]; // Nombre del jugador
    unsigned int score; // Puntaje
    unsigned int invalidReqs; // Cantidad de solicitudes de movimientos inválidas realizadas
    unsigned int validReqs; // Cantidad de solicitudes de movimientos válidas realizadas
    unsigned short x, y; // Coordenadas x e y en el tablero
    pid_t pid; // Identificador de proceso
    bool blocked; // Indica si el jugador está bloqueado
} player_t;

typedef struct gameStateStruct {
    unsigned short width; // Ancho del tablero
    unsigned short height; // Alto del tablero
    unsigned int playerCount; // Cantidad de jugadores
    player_t playerArr[9]; // Lista de jugadores
    bool finished; // Indica si el juego se ha terminado
    int board[]; // Puntero al comienzo del tablero. fila-0, fila-1, ..., fila-n-1
} gameState_t;

#endif //DEFS_H
