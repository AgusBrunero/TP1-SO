#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include "defs.h"
#include "chompChampsUtils.h"

#include <sys/mman.h>    // mmap, munmap, MAP_SHARED, PROT_READ, PROT_WRITE
#include <sys/shm.h>     // shm_open, shm_unlink
#include <fcntl.h>       // O_CREAT, O_RDWR, O_TRUNC
#include <unistd.h>      // ftruncate, close

#define NORTH 0
#define NORTHEAST 1
#define EAST 2
#define SOUTHEAST 3
#define SOUTH 4
#define SOUTHWEST 5
#define WEST 6
#define NORTHWEST 7

#define MAXPLAYERS 9
#define MAXPATHLEN 256
#define ARGLEN 8

#define MIN_MASTER_ARGC 7 //con 1 solo player: procName, w,h,d,t,seed,viewBin,playerBin1

typedef struct doublePointStruct {
    double x;
    double y;
} dPoint_t;
typedef struct pointStruct {
    unsigned short x;
    unsigned short y;
} point_t;



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
point_t static getSpawnPoint(int playerIndex, int boardWidth, int boardHeight) {
    point_t point;
    point.x = (unsigned short)(spawnPointsMult[playerIndex].x * (boardWidth - 1));
    point.y = (unsigned short)(spawnPointsMult[playerIndex].y * (boardHeight - 1));
    return point;
}

// Variables Globales

static unsigned short width, height;
static unsigned int timeout, delay;
static int seed;


// Prototipos de funciones

/*
 * Inicializa los valores de los semáforos
 */
void semaphoresInit(semaphores_t * semaphores);

/*
 * Inicializa el estado del juego
 */
static void gameStateInit (gameState_t * gameState, char* widthStr, char* heightStr, const int seed, const int playerCount, char ** playerBins);

/*
 * destruye todos los semáforos en el struct semaphores_t
 */
void destroySemaphores(semaphores_t *semaphores);

/*
 * Inicializa el tablero int[][] con valores aleatorios entre 0 y 9
 */
void randomizeBoard(int* board, int width, int height, int seed);

/*
 * intenta iniciar el binario de path con los argumentos dados
 * retorna -1 en error, o el pid del proceso hijo
 */
pid_t newProc(const char * binary, char * const argv[]);

/*
 * Crea un nuevo player_t inicializado con los parametros dados
 * binPath: path al binario del jugador
 * intSuffix: numero que se agrega al nombre del jugador para diferenciarlo
 * x, y: coordenadas iniciales del jugador
 * gameState: puntero al gameState compartido
 * shmSize: tamaño en bytes del gameState compartido
 * retorna un puntero a un player_t inicializado
 * en caso de error termina el programa
 */
static player_t * playerFromBin(char * binPath, int intSuffix, unsigned short x, unsigned short y, char * widthStr, char * heightStr);

// Función para actualizar las coordenadas según la dirección
void actualizarPosicion(player_t* player, unsigned char direccion, unsigned short width, unsigned short height);

int main(int argc, char *argv[]) {
    // recepción de parámetros
    printf("Master PID: %d\n", getpid());
    char * widthStr = argv[1];
    char * heightStr = argv[2];
    char * delayStr = argv[3];
    char * timeoutStr = argv[4];
    char * seedStr = argv[5];
    char * viewBinary = argv[6];

    width = strtoul(widthStr, NULL, 10);
    height = strtoul(heightStr, NULL, 10);
    delay = strtoul(delayStr, NULL, 10);
    timeout = strtoul(timeoutStr, NULL, 10);
    seed = strtol(seedStr, NULL, 10);

    // creación memorias compartidas
    createShms(width, height);
    // apertura memorias compartidas
    gameState_t * gameState;
    semaphores_t * semaphores;
    openShms(width, height, &gameState, &semaphores);

    semaphoresInit(semaphores);
    gameStateInit(gameState, widthStr, heightStr, seed, argc - MIN_MASTER_ARGC, &argv[MIN_MASTER_ARGC]);


    // Crear proceso de vista
    char * const viewArgs[] = {(char *)viewBinary, widthStr, heightStr, NULL};
    pid_t view_pid = newProc(viewBinary, viewArgs);
    checkPid(view_pid, "Error creando proceso vista", EXIT_FAILURE);
    printf("Creado proceso vista con PID: %d\n", view_pid);





    while (!gameState->finished) {
        // Permitir a todos los jugadores enviar un movimiento
        for (int i = 0; i < gameState->playerCount; i++) {
            sem_post(&semaphores->playerSems[i]);
        }

        // Esperar y procesar los movimientos de cada jugador
        sem_wait(&semaphores->gameStateMutex);
        for (int i = 0; i < gameState->playerCount; i++) {
            // TODO: leer la dirección enviada por el jugador i
            unsigned char direccion = rand() % 8; // Dirección aleatoria para pruebas
            actualizarPosicion(&gameState->playerArray[i], direccion, gameState->width, gameState->height);
        }
        sem_post(&semaphores->gameStateMutex);

        // Notificar a la vista y esperar que termine
        sem_post(&semaphores->masterToView);
        sem_wait(&semaphores->viewToMaster);

        usleep(delay * 1000); // Convertir delay a microsegundos
    }
    

    destroySemaphores(semaphores);

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

    printf("Terminaron todos los hijos :)\n");

    // Agregar limpieza del shared memory
    // munmap(gameState, gameStateByteSize);
    // shm_unlink(gameStateShmName);
    // munmap(semaphores, semaphoresByteSize);
    // shm_unlink(semaphoresShmName);
    printf("Terminó master con PID: %d\n", getpid());

    return 0;
}


static void gameStateInit (gameState_t * gameState, char* widthStr, char* heightStr, const int seed, const int playerCount, char ** playerBins){
    gameState->width = (unsigned short)strtoul(widthStr, NULL, 10);
    gameState->height = (unsigned short)strtoul(heightStr, NULL, 10);
    gameState->playerCount = playerCount;
    gameState->finished = false;
    randomizeBoard(gameState->board, gameState->width, gameState->height, seed);

    for (int i = 0; i < playerCount; i++) {
        point_t spawnPoint = getSpawnPoint(i, gameState->width, gameState->height);
        printf("%s\n", playerBins[i]);
        gameState->playerArray[i] = *playerFromBin(playerBins[i], i, spawnPoint.x, spawnPoint.y, widthStr, heightStr);
    }
}

void semaphoresInit(semaphores_t * semaphores){
    if (sem_init(&semaphores->masterToView, 1, 0) == -1) {
        perror("sem_init failed for masterToView");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&semaphores->viewToMaster, 1, 0) == -1) {
        perror("sem_init failed for viewToMaster");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&semaphores->masterMutex, 1, 1) == -1) {
        perror("sem_init failed for masterMutex");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&semaphores->gameStateMutex, 1, 1) == -1) {
        perror("sem_init failed for gameStateMutex");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&semaphores->readersCountMutex, 1, 1) == -1) {
        perror("sem_init failed for readersCountMutex");
        exit(EXIT_FAILURE);
    }

    // Inicializar contador de lectores
    semaphores->readers_count = 0;

    // Inicializar semáforos de jugadores
    for (int i = 0; i < 9; i++) {
        if (sem_init(&semaphores->playerSems[i], 1, 1) == -1) {
            perror("sem_init failed for playerSems");
            exit(EXIT_FAILURE);
        }
    }

}

void destroySemaphores(semaphores_t *semaphores) {
    sem_destroy(&semaphores->masterToView);
    sem_destroy(&semaphores->viewToMaster);
    sem_destroy(&semaphores->masterMutex);
    sem_destroy(&semaphores->gameStateMutex);
    sem_destroy(&semaphores->readersCountMutex);
    for (int i = 0; i < 9; i++) {
        sem_destroy(&semaphores->playerSems[i]);
    }
}

void randomizeBoard(int* board, int width, int height, int seed) {
    srand(seed);
    for (int i = 0; i < height; i++) {
       for (int j = 0; j < width; j++) {
            board[i * width + j] = rand() % 10; // Inicializar todas las celdas a 0
        }
    }
}

pid_t newProc(const char * binary, char * const argv[]) {
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

static player_t * playerFromBin(char * binPath, int intSuffix, unsigned short x, unsigned short y, char * widthStr, char * heightStr){
    player_t * player = malloc(sizeof(player_t));
    checkMalloc(player, "malloc failed for player", EXIT_FAILURE);

    char * argv[] = {binPath, widthStr, heightStr, NULL};
    pid_t pid = newProc(binPath, argv);
    if (pid == -1) {
        printf("Error creando proceso jugador %s with binary: %s\n", player->name, binPath);
        exit(EXIT_FAILURE);
    }

    snprintf(player->name, 16, "%s_%d", binPath, intSuffix);
    player->score = 0;
    player->invalidReqs = 0;
    player->validReqs = 0;
    player->x = x;
    player->y = y;
    player->pid = pid;
    player->isBlocked = false;

    return player;
}

void actualizarPosicion(player_t* player, unsigned char direccion, unsigned short width, unsigned short height) {
    switch(direccion) {
        case NORTH:
            if (player->y > 0) player->y--;
            break;
        case NORTHEAST:
            if (player->y > 0 && player->x < width-1) {
                player->y--;
                player->x++;
            }
            break;
        case EAST:
            if (player->x < width-1) player->x++;
            break;
        case SOUTHEAST:
            if (player->y < height-1 && player->x < width-1) {
                player->y++;
                player->x++;
            }
            break;
        case SOUTH:
            if (player->y < height-1) player->y++;
            break;
        case SOUTHWEST:
            if (player->y < height-1 && player->x > 0) {
                player->y++;
                player->x--;
            }
            break;
        case WEST:
            if (player->x > 0) player->x--;
            break;
        case NORTHWEST:
            if (player->y > 0 && player->x > 0) {
                player->y--;
                player->x--;
            }
            break;
    }
}