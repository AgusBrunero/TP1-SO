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


// Variables Globales

static unsigned short width, height;
static unsigned int timeout, delay;
static int seed;
static int pipes[MAXPLAYERS][2];


// Prototipos de funciones

/*
 * Inicializa los valores de los semáforos
 */
static void semaphoresInit(semaphores_t * semaphores);

/*
 * Inicializa el estado del juego
 */
static void gameStateInit (gameState_t * gameState, char* widthStr, char* heightStr, const int seed, const int playerCount, char ** playerBins);

/*
 * destruye todos los semáforos en el struct semaphores_t
 */
static void destroySemaphores(semaphores_t *semaphores);

/*
 * Inicializa el tablero int[][] con valores aleatorios entre 0 y 9
 */
static void randomizeBoard(int* board, int width, int height, int seed);

/*
 * intenta iniciar el binario de path con los argumentos dados
 * retorna -1 en error, o el pid del proceso hijo
 */
static pid_t newProc(const char * binary, char * const argv[]);

/*
 * misma función que newProc, pero redirige el stdout del hijo al pipe dado  
*/
static pid_t newPipedProc(const char * binary, int pipe_fd, char * const argv[]);


/*
 * crea un nuevo punto de spawn para el jugador playerIndex
 */
static point_t getSpawnPoint(int playerIndex, gameState_t * gamestate);

/*
 * actualiza Blocked en cada player del gameState
 */
static void checkBlockedPlayers(gameState_t * gameState);

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

/*  
 * Libera todos los recursos utilizados por chompchamps
 */
static void freeResources();

// Función para actualizar las coordenadas según la dirección
static void updatePlayerPosition(gameState_t* gamestate, player_t* player, int playerIndex, unsigned char direccion, unsigned short width, unsigned short height);

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
    sem_post(&semaphores->masterToView); // pedirle a la vista imprimir estado inicial del tablero
    sem_wait(&semaphores->viewToMaster);


    unsigned char playerChecking = 0 ; // indice del jugador a verificar 


    while (!gameState->finished) {
        sem_wait(&semaphores->masterMutex); // Bloqueo a los jugadores al inicio del loop
        sem_wait(&semaphores->gameStateMutex); // Espero a que terminen los que ya estaban leyendo

        if (!gameState->playerArray[playerChecking].isBlocked){
            unsigned char nextMove = -1;
            ssize_t bytesReaded = read(pipes[playerChecking][0], &nextMove, 1);
            if (bytesReaded == 1){
                updatePlayerPosition(gameState ,&gameState->playerArray[playerChecking],playerChecking ,nextMove, gameState->width, gameState->height);
                checkBlockedPlayers(gameState);
                sem_post(&semaphores->playerSems[playerChecking]); // le aviso al jugador que puede mandar otro movimiento.

                sem_post(&semaphores->gameStateMutex); // Liberar el estado para los jugadores (antes de hacer escribir a la vista)
                sem_post(&semaphores->masterMutex);    

                sem_post(&semaphores->masterToView);// si hubo cambios, notificar a la vista
                sem_wait(&semaphores->viewToMaster);           
            }else if (bytesReaded == -1){
                perror("Error reading from pipe");
                exit(EXIT_FAILURE);          
            }else if (bytesReaded == 0){
                gameState->playerArray[playerChecking].isBlocked = true; // EOF -> Jugador bloqueado
                sem_post(&semaphores->gameStateMutex); // Liberar el estado para los jugadores (antes de hacer escribir a la vista)
                sem_post(&semaphores->masterMutex);  
            }
        }else{
            sem_post(&semaphores->gameStateMutex); // Liberar el estado para los jugadores
            sem_post(&semaphores->masterMutex);             
        }
        if (++playerChecking >= gameState->playerCount) {
            playerChecking = 0;
        }
        usleep(delay * 1000); // Convertir delay a microsegundos
    }

    sem_post(&semaphores->masterToView); // pedir a la vista imprimir el estado final del juego.
    sem_wait(&semaphores->viewToMaster);   
    
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
    printf("Terminó master con PID: %d\n", getpid());

    freeResources();
    munmap(gameState, sizeof(gameState_t) + gameState->width * gameState->height * sizeof(int));
    destroySemaphores(semaphores);

    return 0;
}


static void gameStateInit (gameState_t * gameState, char* widthStr, char* heightStr, const int seed, const int playerCount, char ** playerBins){
    gameState->width = (unsigned short)strtoul(widthStr, NULL, 10);
    gameState->height = (unsigned short)strtoul(heightStr, NULL, 10);
    gameState->playerCount = playerCount;
    gameState->finished = false;
    randomizeBoard(gameState->board, gameState->width, gameState->height, seed);

    for (int i = 0; i < playerCount; i++) {
        point_t spawnPoint = getSpawnPoint(i, gameState);
        printf("%s\n", playerBins[i]);
        gameState->playerArray[i] = *playerFromBin(playerBins[i], i, spawnPoint.x, spawnPoint.y, widthStr, heightStr);
    }
}

static void semaphoresInit(semaphores_t * semaphores){
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

static void destroySemaphores(semaphores_t *semaphores) {
    sem_destroy(&semaphores->masterToView);
    sem_destroy(&semaphores->viewToMaster);
    sem_destroy(&semaphores->masterMutex);
    sem_destroy(&semaphores->gameStateMutex);
    sem_destroy(&semaphores->readersCountMutex);
    for (int i = 0; i < 9; i++) {
        sem_destroy(&semaphores->playerSems[i]);
    }
    munmap(semaphores, sizeof(semaphores_t));
}

static void randomizeBoard(int* board, int width, int height, int seed) {
    srand(seed);
    for (int i = 0; i < height; i++) {
       for (int j = 0; j < width; j++) {
            board[i * width + j] = rand() % 9 + 1; // Inicializar todas las celdas a 0
        }
    }
}

static pid_t newProc(const char * binary, char * const argv[]) {
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

static pid_t newPipedProc(const char * binary, int pipe_fd, char * const argv[]) {
    pid_t pid = fork();

    if (pid == -1) {
        printf("Fork error -1");
        return -1;
    }
    if (pid == 0) {
        // proceso hijo
        dup2(pipe_fd, STDOUT_FILENO);
        close(pipe_fd);
        execvp(binary, argv);
        // si se llega aquí, exec falló.
        printf("Error en execvp");
        exit(EXIT_FAILURE);
    }
    close(pipe_fd);
    return pid;
}

static point_t getSpawnPoint(int playerIndex, gameState_t * gamestate) {
    
    srand(seed + playerIndex);
    point_t toReturn = {playerIndex, playerIndex}; // valor por defecto en caso de error
    point_t usedPoints[playerIndex-1];
    for (int i = 0; i < playerIndex-1; i++) {
        usedPoints[i].x = gamestate->playerArray[i].x;
        usedPoints[i].y = gamestate->playerArray[i].y;
    }
    char used = 1;
    while (used){
        toReturn.x = (rand() % (gamestate->width-2)) + 1;   // +/-1 para evitar las esquinas
        toReturn.y = (rand() % (gamestate->height-2)) + 1;
        used = 0;
        for(int i = 0; i < playerIndex; i++){
            if (usedPoints[i].x == toReturn.x && usedPoints[i].y == toReturn.y){
                used = 1;
            }
        }
    }
    gamestate->board[toReturn.y * gamestate->width + toReturn.x] = (-1) * playerIndex; // Comer la celda inicial
    return toReturn;
}

static void checkBlockedPlayers(gameState_t * gameState){
    bool allBlocked = true;

    for (int i = 0; i < gameState->playerCount; i++) {
        player_t * player = &gameState->playerArray[i];
        if (!player->isBlocked){
            bool isBlocked = true;
            unsigned short x = player->x;
            unsigned short y = player->y;
            int directions[8][2] = {
                {0, -1},   // Norte
                {1, -1},   // Noreste
                {1, 0},    // Este
                {1, 1},    // Sureste
                {0, 1},    // Sur
                {-1, 1},   // Suroeste
                {-1, 0},   // Oeste
                {-1, -1}   // Noroeste
            };

            for (int i = 0; i < 8 && isBlocked; i++) {
                int xToTest = x + directions[i][0];
                int yToTest = y + directions[i][1];

                if (xToTest >= 0 && xToTest < gameState->width && yToTest >= 0 && yToTest < gameState->height) {
                    if (gameState->board[yToTest * gameState->width + xToTest] > 0) {
                        isBlocked = false;
                        allBlocked = false;
                        break; // No es necesario seguir verificando si ya encontramos una dirección válida
                    }
                }
            }
            player->isBlocked = isBlocked;
        }
    }
    if (allBlocked) gameState->finished = true;

}

static player_t * playerFromBin(char * binPath, int intSuffix, unsigned short x, unsigned short y, char * widthStr, char * heightStr){
    player_t * player = malloc(sizeof(player_t));
    checkMalloc(player, "malloc failed for player", EXIT_FAILURE);

    char * argv[] = {binPath, widthStr, heightStr, NULL};
    pipe(pipes[intSuffix]);
    pid_t pid = newPipedProc(binPath, pipes[intSuffix][1], argv);
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

void updatePlayerPosition(gameState_t* gamestate, player_t* player, int playerIndex , unsigned char direccion, unsigned short width, unsigned short height) {
    point_t newPosition = {player->x, player->y};
    char isValid;

    switch(direccion) {
        case NORTH:
            if (newPosition.y > 0){
                newPosition.y--;
                isValid = 1;
            }else{
                isValid = 0;
            }
            break;
        case NORTHEAST:
            if (newPosition.y > 0 && newPosition.x < width-1) {
                newPosition.y--;
                newPosition.x++;
                isValid = 1;
            }else{
                isValid = 0;
            }
            break;
        case EAST:
            if (newPosition.x < width-1){
                newPosition.x++;
                isValid = 1;
            }else{
                isValid = 0;
            }
            break;
        case SOUTHEAST:
            if (newPosition.y < height-1 && newPosition.x < width-1) {
                newPosition.y++;
                newPosition.x++;
                isValid = 1;
            }else{
                isValid = 0;
            }
            break;
        case SOUTH:
            if (newPosition.y < height-1) {
                newPosition.y++;
                isValid = 1;
            }else{
                isValid = 0;
            }
            break;
        case SOUTHWEST:
            if (newPosition.y < height-1 && newPosition.x > 0) {
                newPosition.y++;
                newPosition.x--;
                isValid = 1;
            }else{
                isValid = 0;
            }
            break;
        case WEST:
            if (newPosition.x > 0) {
                newPosition.x--;
                isValid = 1;
            }else{
                isValid = 0;
            }
            break;
        case NORTHWEST:
            if (newPosition.y > 0 && newPosition.x > 0) {
                newPosition.y--;
                newPosition.x--;
                isValid = 1;
            }else{
                isValid = 0;
            }
            break;
        default:
            isValid = 0;
            break;

    }
    
    if (gamestate->board[newPosition.y * gamestate->width + newPosition.x] <= 0) {
        isValid = 0; 
    }

    if(isValid){
        player->x = newPosition.x;
        player->y = newPosition.y;
        player->validReqs++;
        player->score += gamestate->board[player->y * gamestate->width + player->x];
        gamestate->board[player->y * gamestate->width + player->x] = (-1) * playerIndex; // Comer la celda

    }else{
        player->invalidReqs++;
    }


}

static void freeResources() {
    for (int i = 0; i < 9; i++) {
        if (pipes[i][0] != -1) close(pipes[i][0]);
        if (pipes[i][1] != -1) close(pipes[i][1]);
    }
    if (shm_unlink("/game_state") == -1) {
        perror("shm_unlink game_state");
    }
    if (shm_unlink("/game_sync") == -1) {
        perror("shm_unlink game_sync");
    }
}