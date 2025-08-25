#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>

#define MAXPLAYERS 9
#define MAXPATHLEN 256
#define ARGLEN 8

#define MIN_MASTER_ARGC 6 //con 1 solo player: w,h,d,t,s,viewBin,playerBin1

typedef struct gameState {
    int width;
    int height;
    int board;
    int playerCount;
    //pid_t es temporal mas adelante hay que reemplazarlo con el struct de players
    pid_t players[MAXPLAYERS];
    bool gameOver;

    //temporal (irian en otro struct)
    int delay;
    int timeOut;
    char * viewBin;
    char * playerBins[MAXPLAYERS];
} gameState_t;


void gameStateFromArgs(gameState_t * blankState,int argc, char * argv[]) {
    //w ,h, d, t, s, viewBin, playerBins...
    blankState->width = argv[0];
    blankState->height = argv[1];
    blankState->delay = argv[2];
    blankState->timeOut = argv[3];
    blankState->viewBin = argv[4];
    int playerCount = argc - MIN_MASTER_ARGC + 1;
    blankState->playerCount = playerCount;
    for (int i = 0; i < playerCount; i++) {
        blankState->playerBins[i] = argv[MIN_MASTER_ARGC + i];
    }
}

/*
 * intenta iniciar el binario de path con los argumentos dados
 * retorna -1 en error, o el pid del proceso hijo
 */
pid_t newProc(char * binary, char * argv[]) {
    pid_t pid = fork();

    if (pid == -1) {
        printf("Fork error -1");
        return -1;
    }
    //pid 0 si es el hijo
    if (pid == 0) {
        //como soy el hijo me reemplazo por el binario de un player
        execvp(binary, argv);
        //aca nunca se llega porque exec no crea un nuevo proceso, sino que reemplaza el actual
        printf("Error en execvp");
        exit(EXIT_FAILURE);
    }
    return pid;
}

int main(int argc, char *argv[]) {
    int width = 10;
    int height = 10;

    gameState_t globalGameState;
    //ignoro primer arg que es el nombre del programa
    gameStateFromArgs(&globalGameState, argc - 1, &argv[1]);

    char widthStr[ARGLEN], heightStr[ARGLEN];
    snprintf(widthStr, ARGLEN, "%d", width);
    snprintf(heightStr, ARGLEN, "%d", height);

    printf("Hello, World!\n");


    char * viewArgs[] = {globalGameState.viewBin, widthStr, heightStr, NULL};
    pid_t view_pid = newProc(globalGameState.viewBin, viewArgs);
    if (view_pid == -1) {
        printf("Error creando proceso vista\n");
        return EXIT_FAILURE;
    }
    printf("Creado proceso vista con PID: %d\n", view_pid);

    char* player1_argv[] = {
        globalGameState.playerBins[0],
        widthStr,
        heightStr,
        NULL
    };
    pid_t player1_pid = newProc(globalGameState.playerBins[0], player1_argv);
    if (player1_pid == -1) {
        printf("Error creando proceso jugador 1\n");
        return EXIT_FAILURE;
    }
    printf("Creado proceso jugador 1 con PID: %d\n", player1_pid);

    //aca wait escribe el codigo con el que termino el proceso hijo
    int status;
    //aca wait me retorna el pid del proceso que termino
    pid_t finished_pid;

    while ( (finished_pid = wait(&status)) > 0 ) {
        if (WIFEXITED(status)) {
            printf("Proceso con PID %d terminó con estado %d\n", finished_pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Proceso con PID %d terminó por señal %d\n", finished_pid, WTERMSIG(status));
        } else {
            printf("Proceso con PID %d terminó de manera inesperada\n", finished_pid);
        }
    }

    printf("Terminaron todos los hijos :)");
    return 0;
}


