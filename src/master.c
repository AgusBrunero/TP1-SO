#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>  // O_CREAT, O_RDWR, O_TRUNC
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/mman.h>  // mmap, munmap, MAP_SHARED, PROT_READ, PROT_WRITE
#include <sys/select.h>
#include <sys/shm.h>  // shm_open, shm_unlink
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>  // ftruncate, close

#include "chompChampsUtils.h"
#include "defs.h"

#define MINPLAYERS 1
#define MAXPLAYERS 9
#define MAXPATHLEN 256
#define ARGLEN 8

#define DEFAULTWIDTH 10
#define DEFAULTHEIGHT 10
#define DEFAULTDELAY 200
#define DEFAULTTIMEOUT 10
#define DEFAULTSEED time(NULL)

typedef struct doublePointStruct {
    double x;
    double y;
} dPoint_t;

typedef struct pointStruct {
    unsigned short x;
    unsigned short y;
} point_t;

typedef struct masterDataStruct {
    unsigned short width;
    unsigned short height;
    unsigned int delay;
    unsigned int timeout;
    int seed;
    char *viewBinary;
    char *playerBinaries[MAXPLAYERS];
    int playerCount;
    unsigned long long savedTime;
    char *widthStr;
    char *heightStr;
} masterData_t;

// Variables Globales
static masterData_t masterData = {DEFAULTWIDTH, DEFAULTHEIGHT, DEFAULTDELAY, DEFAULTTIMEOUT, 0, NULL, {NULL}, 0, 0, "10", "10"};
static int pipes[MAXPLAYERS][2];

// Prototipos de funciones

/*
 * Inicializa los valores de los semáforos
 */
static void semaphoresInit(semaphores_t *semaphores);

/*
 * Inicializa el gameState, variables y crea los procesos de los jugadores
 */
static void gameStateInit(gameState_t *gameState, unsigned short width, unsigned short height, const int seed, const int playerCount, char **playerBins);

/*
 * destruye todos los semáforos en el struct semaphores_t
 */
static void destroySemaphores(semaphores_t *semaphores);

/*
 * Inicializa el tablero int[][] con valores aleatorios entre 0 y 9
 */
static void randomizeBoard(int *board, int width, int height, int seed);

/*
 * intenta iniciar el binario de path con los argumentos dados
 * retorna -1 en error, o el pid del proceso hijo
 */
static pid_t newProc(const char *binary, char *const argv[]);

/*
 * misma función que newProc, pero redirige el stdout del hijo al pipe dado
 */
static pid_t newPipedProc(const char *binary, int pipe_fd, char *const argv[]);

/*
 * crea un nuevo punto de spawn para el jugador playerIndex
 */
static point_t getSpawnPoint(int playerIndex, gameState_t *gamestate);

/*
 * actualiza Blocked en cada player del gameState
 */
static void checkBlockedPlayers(gameState_t *gameState);

/*
 * Crea un nuevo player_t inicializado con los parametros dados
 * en caso de error termina el programa
 */
static player_t *playerFromBin(char *binPath, int intSuffix, unsigned short x, unsigned short y, char *widthStr, char *heightStr);

/*
 * utiliza clock_gettime() y retorna el tiempo en ms
 */
static long long getTimeMs();

// Función para actualizar las coordenadas según la dirección
static void updatePlayerPosition(gameState_t *gamestate, player_t *player, int playerIndex, unsigned char direccion);

static void saveParams(int argc, char *argv[]);

static void finishGame(gameState_t *gameState);

static void printEndGame(gameState_t *gameState);

/*
 * Libera todos los recursos utilizados por chompchamps
 */
static void freeResources();

int main(int argc, char *argv[]) {
    saveParams(argc, argv);

    createShms(masterData.width, masterData.height);
    gameState_t *gameState;
    semaphores_t *semaphores;
    openShms(masterData.width, masterData.height, &gameState, &semaphores);

    semaphoresInit(semaphores);
    gameStateInit(gameState, masterData.width, masterData.height, masterData.seed, masterData.playerCount, masterData.playerBinaries);

    if (!(masterData.viewBinary == NULL)) {
        char *const viewArgs[] = {(char *)masterData.viewBinary, masterData.widthStr, masterData.heightStr, NULL};
        pid_t view_pid = newProc(masterData.viewBinary, viewArgs);
        checkPid(view_pid, "Error creando proceso vista", EXIT_FAILURE);
        sem_post(&semaphores->masterToView);
        sem_wait(&semaphores->viewToMaster);
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    int maxfd = -1;
    for (int i = 0; i < masterData.playerCount; i++) {
        if (pipes[i][0] > maxfd) maxfd = pipes[i][0];
        FD_SET(pipes[i][0], &readfds);
    }
    maxfd++;

    unsigned char playerChecking = 0;

    while (!gameState->finished) {
        struct timeval tv = {.tv_sec = masterData.timeout, .tv_usec = 0};
        int ready = select(maxfd, &readfds, NULL, NULL, &tv);
        switch (ready) {
            case 0:  // TIMEOUT
                finishGame(gameState);
                break;
            case -1:
                if (errno == EINTR) continue;
                perror("select");
                exit(EXIT_FAILURE);
                break;
            default:
                for (int i = 0; i < masterData.playerCount; i++) {
                    if (FD_ISSET(pipes[playerChecking][0], &readfds)) break;
                    if (++playerChecking >= gameState->playerCount) playerChecking = 0;
                }
                break;
        }

        if (gameState->playerArray[playerChecking].isBlocked) {
            if (++playerChecking >= gameState->playerCount) playerChecking = 0;
            continue;
        }

        sem_wait(&semaphores->masterMutex);     // Bloqueo a los jugadores al inicio del loop
        sem_wait(&semaphores->gameStateMutex);  // Espero a que terminen los que ya estaban leyendo

        unsigned char nextMove = -1;
        read(pipes[playerChecking][0], &nextMove, 1);
        sem_post(&semaphores->playerSems[playerChecking]);

        if (nextMove == EOF) {
            gameState->playerArray[playerChecking].isBlocked = true;
        } else {
            updatePlayerPosition(gameState, &gameState->playerArray[playerChecking], playerChecking, nextMove);
            checkBlockedPlayers(gameState);

            if (!(masterData.viewBinary == NULL)) {
                sem_post(&semaphores->masterToView);
                sem_wait(&semaphores->viewToMaster);
            }

            bool allBlocked = true;
            for (int i = 0; i < gameState->playerCount; i++) {
                if (!gameState->playerArray[i].isBlocked) allBlocked = false;
            }
            if (allBlocked) gameState->finished = true;
        }

        sem_post(&semaphores->gameStateMutex);
        sem_post(&semaphores->masterMutex);

        if (++playerChecking >= gameState->playerCount) playerChecking = 0;

        usleep(masterData.delay * 1000);
        if (getTimeMs() - masterData.savedTime > masterData.timeout * 1000) finishGame(gameState);
    }

    if (!(masterData.viewBinary == NULL)) {
        sem_post(&semaphores->masterToView);
        sem_wait(&semaphores->viewToMaster);
    }

    for (int i = 0; i < masterData.playerCount; i++) {
        sem_post(&semaphores->playerSems[i]);
    }

    printEndGame(gameState);

    // finalizar procesos
    int status;
    pid_t finished_pid;
    while ((finished_pid = wait(&status)) > 0) {
        if (WIFEXITED(status)) {
            printf("Proceso con PID %d terminó con estado %d\n", finished_pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Proceso con PID %d terminó por señal %d\n", finished_pid, WTERMSIG(status));
        } else {
            printf("Proceso con PID %d terminó de manera inesperada\n", finished_pid);
        }
    }
    freeResources(gameState, semaphores);
    printf("Proceso con PID: %d (Master) terminó \n", getpid());

    return 0;
}

static void finishGame(gameState_t *gameState) {
    gameState->finished = true;
    for (int i = 0; i < masterData.playerCount; i++) {
        gameState->playerArray[i].isBlocked = true;
    }
}

static void printEndGame(gameState_t *gameState) {
    printf("JUEGO TERMINADO\n");

    playerRank_t rankings[MAXPLAYERS];

    getPlayersRanking(gameState, rankings);

    int winners = 1;
    for (int i = 0; i < gameState->playerCount - 1 && !comparePlayersRank(&rankings[0], &rankings[i + 1]); i++, winners++);
    if (winners > 1) {
        printf("EMPATE ENTRE LOS JUGADORES: ");
        for (int i = 0; i < winners; i++) printf("%s ", rankings[i].player->name);
        printf("\n");
    } else {
        printf("GANADOR: %s ", rankings[0].player->name);
        printf("\n\n");
    }
}

static void gameStateInit(gameState_t *gameState, unsigned short width, unsigned short height, const int seed, const int playerCount, char **playerBins) {
    gameState->width = width;
    gameState->height = height;
    gameState->playerCount = playerCount;
    gameState->finished = false;
    randomizeBoard(gameState->board, gameState->width, gameState->height, seed);

    for (int i = 0; i < playerCount; i++) {
        point_t spawnPoint = getSpawnPoint(i, gameState);
        gameState->playerArray[i] = *playerFromBin(playerBins[i], i, spawnPoint.x, spawnPoint.y, masterData.widthStr, masterData.heightStr);
    }
}

static void semaphoresInit(semaphores_t *semaphores) {
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
}

static void randomizeBoard(int *board, int width, int height, int seed) {
    srand(seed);
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            board[i * width + j] = rand() % 9 + 1;  // Inicializar todas las celdas a 0
        }
    }
}

static pid_t newProc(const char *binary, char *const argv[]) {
    pid_t pid = fork();

    if (pid == -1) {
        printf("Fork error -1");
        return -1;
    }
    // pid 0 si es el hijo
    if (pid == 0) {
        // como soy el hijo me reemplazo por el binario de un player
        execvp(binary, argv);
        // aca nunca se llega porque exec no crea un nuevo proceso, sino que
        // reemplaza el actual
        printf("Error en execvp");
        exit(EXIT_FAILURE);
    }
    return pid;
}

static pid_t newPipedProc(const char *binary, int pipe_fd, char *const argv[]) {
    pid_t pid = fork();

    if (pid == -1) {
        printf("Fork error -1");
        return -1;
    }
    if (pid == 0) {
        // proceso hijo
        dup2(pipe_fd, STDOUT_FILENO);
        close(pipe_fd);

        // TODO: CERRAR PIPES EXTRA AQUÍ

        execvp(binary, argv);
        // si se llega aquí, exec falló.
        printf("Error en execvp");
        exit(EXIT_FAILURE);
    }
    close(pipe_fd);
    return pid;
}

static point_t getSpawnPoint(int playerIndex, gameState_t *gamestate) {
    srand(masterData.seed + playerIndex);
    point_t toReturn = {playerIndex, playerIndex};  // valor por defecto en caso de error
    point_t usedPoints[playerIndex - 1];
    for (int i = 0; i < playerIndex - 1; i++) {
        usedPoints[i].x = gamestate->playerArray[i].x;
        usedPoints[i].y = gamestate->playerArray[i].y;
    }
    char used = 1;
    while (used) {
        toReturn.x = (rand() % (gamestate->width - 2)) + 1;  // +/-1 para evitar las esquinas
        toReturn.y = (rand() % (gamestate->height - 2)) + 1;
        used = 0;
        for (int i = 0; i < playerIndex; i++) {
            if (usedPoints[i].x == toReturn.x && usedPoints[i].y == toReturn.y) {
                used = 1;
            }
        }
    }
    gamestate->board[toReturn.y * gamestate->width + toReturn.x] = (-1) * playerIndex;  // Comer la celda inicial
    return toReturn;
}

static void checkBlockedPlayers(gameState_t *gameState) {
    for (int i = 0; i < gameState->playerCount; i++) {
        player_t *player = &gameState->playerArray[i];
        if (!player->isBlocked) {
            bool isBlocked = true;
            unsigned short x = player->x;
            unsigned short y = player->y;
            int directions[8][2] = {
                {0, -1},  // Norte
                {1, -1},  // Noreste
                {1, 0},   // Este
                {1, 1},   // Sureste
                {0, 1},   // Sur
                {-1, 1},  // Suroeste
                {-1, 0},  // Oeste
                {-1, -1}  // Noroeste
            };

            for (int i = 0; i < 8 && isBlocked; i++) {
                int xToTest = x + directions[i][0];
                int yToTest = y + directions[i][1];

                if (xToTest >= 0 && xToTest < gameState->width && yToTest >= 0 && yToTest < gameState->height) {
                    if (gameState->board[yToTest * gameState->width + xToTest] > 0) {
                        isBlocked = false;
                        break;
                    }
                }
            }
            player->isBlocked = isBlocked;
        }
    }
}

static player_t *playerFromBin(char *binPath, int intSuffix, unsigned short x, unsigned short y, char *widthStr, char *heightStr) {
    player_t *player = malloc(sizeof(player_t));
    checkMalloc(player, "malloc failed for player", EXIT_FAILURE);

    char *argv[] = {binPath, widthStr, heightStr, NULL};
    pipe(pipes[intSuffix]);
    pid_t pid = newPipedProc(binPath, pipes[intSuffix][1], argv);
    if (pid == -1) {
        printf("Error creando proceso jugador %s con binario: %s\n", player->name, binPath);
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

static void updatePlayerPosition(gameState_t *gamestate, player_t *player, int playerIndex, unsigned char direccion) {
    point_t newPosition = {player->x, player->y};
    char isValid;

    switch (direccion) {
        case NORTH:
            if (newPosition.y > 0) {
                newPosition.y--;
                isValid = 1;
            } else {
                isValid = 0;
            }
            break;
        case NORTHEAST:
            if (newPosition.y > 0 && newPosition.x < gamestate->width - 1) {
                newPosition.y--;
                newPosition.x++;
                isValid = 1;
            } else {
                isValid = 0;
            }
            break;
        case EAST:
            if (newPosition.x < gamestate->width - 1) {
                newPosition.x++;
                isValid = 1;
            } else {
                isValid = 0;
            }
            break;
        case SOUTHEAST:
            if (newPosition.y < gamestate->height - 1 && newPosition.x < gamestate->width - 1) {
                newPosition.y++;
                newPosition.x++;
                isValid = 1;
            } else {
                isValid = 0;
            }
            break;
        case SOUTH:
            if (newPosition.y < gamestate->height - 1) {
                newPosition.y++;
                isValid = 1;
            } else {
                isValid = 0;
            }
            break;
        case SOUTHWEST:
            if (newPosition.y < gamestate->height - 1 && newPosition.x > 0) {
                newPosition.y++;
                newPosition.x--;
                isValid = 1;
            } else {
                isValid = 0;
            }
            break;
        case WEST:
            if (newPosition.x > 0) {
                newPosition.x--;
                isValid = 1;
            } else {
                isValid = 0;
            }
            break;
        case NORTHWEST:
            if (newPosition.y > 0 && newPosition.x > 0) {
                newPosition.y--;
                newPosition.x--;
                isValid = 1;
            } else {
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

    if (isValid) {
        player->x = newPosition.x;
        player->y = newPosition.y;
        player->validReqs++;
        player->score += gamestate->board[player->y * gamestate->width + player->x];
        gamestate->board[player->y * gamestate->width + player->x] = (-1) * playerIndex;  // Comer la celda
        masterData.savedTime = getTimeMs();
    } else {
        player->invalidReqs++;
    }
}

static long long getTimeMs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static void saveParams(int argc, char *argv[]) {
    masterData.seed = time(NULL);
    masterData.savedTime = getTimeMs();

    char opt;
    while ((opt = getopt(argc, argv, "w:h:d:t:v:p:s:")) != -1) {
        switch (opt) {
            case 'w':
                if (atoi(optarg) > 10) {
                    masterData.width = (unsigned short)atoi(optarg);
                    masterData.widthStr = optarg;
                }
                break;
            case 'h':
                if (atoi(optarg) > 10) {
                    masterData.height = (unsigned short)atoi(optarg);
                    masterData.heightStr = optarg;
                }
                break;
            case 'd':
                if (atoi(optarg) > 0) masterData.delay = (unsigned int)atoi(optarg);
                break;
            case 't':
                if (atoi(optarg) > 0) masterData.timeout = (unsigned int)atoi(optarg);
                break;
            case 's':
                masterData.seed = atoi(optarg);
                break;
            case 'v':
                masterData.viewBinary = optarg;
                break;
            case 'p':
                optind--;
                while (optind < argc && argv[optind] && argv[optind][0] != '-' && masterData.playerCount < MAXPLAYERS) {
                    masterData.playerBinaries[masterData.playerCount++] = argv[optind++];
                }
                break;
        }
    }
}

static void freeResources(gameState_t *gameState, semaphores_t *semaphores) {
    for (int i = 0; i < 9; i++) {
        if (pipes[i][0] != -1) close(pipes[i][0]);
        if (pipes[i][1] != -1) close(pipes[i][1]);
    }

    destroySemaphores(semaphores);

    if (shm_unlink("/game_state") == -1) {
        perror("shm_unlink game_state");
    }
    if (shm_unlink("/game_sync") == -1) {
        perror("shm_unlink game_sync");
    }
    munmap(gameState, sizeof(gameState_t) + gameState->width * gameState->height * sizeof(int));
    munmap(semaphores, sizeof(semaphores_t));
}