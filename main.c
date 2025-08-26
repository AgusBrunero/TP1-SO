#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include "defs.h"
#define MAXPLAYERS 9
#define MAXPATHLEN 256
#define ARGLEN 8

#define MIN_MASTER_ARGC 7 //con 1 solo player: procName, w,h,d,t,seed,viewBin,playerBin1

typedef struct doublePointStruct {
    double x;
    double y;
} dPoint_t;
typedef struct PointStruct {
    unsigned short x;
    unsigned short y;
} Point_t;
//valores entre [0,1], que se expresa como porcentaje de la dimension del board
//El player i empieza en (spawnPoints[i].x * (width - 1), spawnPoints[i].y * (height - 1))
static dPoint_t spawnPointsMult[MAXPLAYERS] = {
{0, 0}, // Player 1 (esquina superior izquierda)
    {1, 0}, // Player 2 (1 en numpad)
    {0, 1}, // Player 3 (3 en numpad)
    {1, 1}, // Player 4 (9 en numpad)
    {0.5, 0.5}, // Player 5 (5 en numpad)
    {0.5, 0}, // Player 6 (2 en numpad)
    {0, 0.5}, // Player 7 (4 en numpad)
    {1, 0.5}, // Player 8 (6 en numpad)
    {0.5, 1}  // Player 9 (8 en numpad)
};
Point_t static getSpawnPoint(int playerIndex, int boardWidth, int boardHeight) {
    Point_t point;
    point.x = (unsigned short)(spawnPointsMult[playerIndex].x * (boardWidth - 1));
    point.y = (unsigned short)(spawnPointsMult[playerIndex].y * (boardHeight - 1));
    return point;
}

/*
 * Inicializa el tablero int[][] con valores aleatorios entre 0 y 9
 */
void randomizeBoard(int* board, int width, int height, int seed) {
    srand(seed);
    for (int i = 0; i < height; i++) {
       for (int j = 0; j < width; j++) {
            board[i * width + j] = rand() % 10; // Inicializar todas las celdas a 0
        }
    }
}

static int timeout, delay, seed;
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

/*
 * Crea un player_t inicializado con los parametros dados
 * binPath: path al binario del jugador
 * intSuffix: numero que se agrega al nombre del jugador para diferenciarlo
 * x, y: coordenadas iniciales del jugador
 * retorna un puntero a un player_t inicializado
 * en caso de error termina el programa
 */
static player_t * playerFromBin(char * binPath, int intSuffix, int x, int y) {
    player_t * player = malloc(sizeof(player_t));
    if (player == NULL || errno == ENOMEM) {
        printf("Malloc error\n");
        exit(EXIT_FAILURE);
    }
    snprintf(player->name, 16, "%s_%d", binPath, intSuffix);
    player->score = 0;
    player->invalidReqs = 0;
    player->validReqs = 0;
    player->x = x;
    player->y = y;
    /*
     * Parametros debug, dsp los reemplazamos por la shared mem y otros parametros utiles
     */
    char widthStr[16], heightStr[16];
    snprintf(widthStr, sizeof(widthStr), "%d", 192);
    snprintf(heightStr, sizeof(heightStr), "%d", 152);
    char* argv[4];
    argv[0] = binPath;
    argv[1] = widthStr;
    argv[2] = heightStr;
    argv[3] = NULL;
    pid_t pid = newProc(binPath, argv);
    if (pid == -1) {
        printf("Error creando proceso jugador %s with binary: %s\n", player->name, binPath);
        exit(EXIT_FAILURE);
    }
    player->pid = pid;
    player->blocked = false;
    return player;
}

/*
 * Retorna una gameState inicializado con todos los players inicializados y posicionados
 * Asume todos los parametros correctos y que hay al menos 1 player
 * Parametros de entrada: procName, width, height, delay, timeout, seed, viewBin, playerBin1, playerBin2, ...
*/
gameState_t * gameStateFromArgs(int argc, char* argv[]) {
    int width = strtol(argv[1], NULL, 10);
    int height = strtol(argv[2], NULL, 10);


    /*
     * Esto es super incomodo, pero por enunciado board no puede ser un puntero, asi que toca
     * mallocearlo con el tamaño del struct + el tamaño del board
     * :(
     */
    size_t boardByteSize = height * width * sizeof(int);
    size_t structByteSize = sizeof(gameState_t) + boardByteSize;
  //  gameState_t *gameState = malloc(structByteSize);
    gameState_t * gameState = mmap(NULL, structByteSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    gameState->width = width;
    gameState->height = height;

    delay = strtol(argv[3], NULL, 10);
    timeout = strtol(argv[4], NULL, 10);
    seed = strtol(argv[5], NULL, 10);

    //resto los primeros 6 argumentos fijos
    //procName, w,h,d,t,seed,viewBin
    gameState->playerCount = argc - MIN_MASTER_ARGC + 1;
    gameState->finished = false;
    randomizeBoard(gameState->board, width, height, seed);

    for (int i = 0; i < gameState->playerCount; i++) {
        Point_t spawnPoint = getSpawnPoint(i, width, height);
        gameState->playerArr[i] = *playerFromBin(argv[MIN_MASTER_ARGC - 1 + i], i + 1, spawnPoint.x, spawnPoint.y);
    }
    return gameState;
}


int main(int argc, char *argv[]) {
    int width = 10;
    int height = 10;

    gameState_t* globalGameState = gameStateFromArgs(argc, argv);
    char* viewBinary = argv[6];

    char widthStr[ARGLEN], heightStr[ARGLEN];
    snprintf(widthStr, ARGLEN, "%d", width);
    snprintf(heightStr, ARGLEN, "%d", height);


    char * sharedMemPtr[32];
    snprintf(sharedMemPtr, "%p", (void*)globalGameState);
    char * viewArgs[] = {sharedMemPtr, NULL};
    pid_t view_pid = newProc(viewBinary, viewArgs);
    if (view_pid == -1) {
        printf("Error creando proceso vista\n");
        return EXIT_FAILURE;
    }
    printf("Creado proceso vista con PID: %d\n", view_pid);


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




