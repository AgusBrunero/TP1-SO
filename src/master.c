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

//Prototipos de funciones

/*
 * destruye todos los semáforos en el struct semaphores_t
 */
void destroySemaphores(semaphores_t *semaphores);

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
static player_t * playerFromBin(char * binPath, int intSuffix, int x, int y, const char * gameStateShmName, size_t gameStateByteSize, const char * semaphoresShmName, size_t semaphoresByteSize) {
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

    char arg2[32];
    snprintf(arg2, sizeof(arg2), "%lu", gameStateByteSize);

    char arg3[32];
    snprintf(arg3, sizeof(arg3), "%lu", semaphoresByteSize);

    char * const argv[6] = {
        binPath,
        (char *)gameStateShmName,
        arg2,
        (char *)semaphoresShmName,
        arg3,
        NULL
    };

    pid_t pid = newProc(binPath, argv);
    if (pid == -1) {
        printf("Error creando proceso jugador %s with binary: %s\n", player->name, binPath);
        exit(EXIT_FAILURE);
    }
    player->pid = pid;
    player->isBlocked = false;
    return player;
}

/*
 * No retorna nada, pero popula un gameState * con los parametros dados
 * Asume todos los parametros correctos y que hay al menos 1 player
 * Parametros de entrada: procName, width, height, delay, timeout, seed, viewBin, playerBin1, playerBin2, ...
*/
static void populateGameStateFromArgs(gameState_t * gameState, semaphores_t *semaphores, const char* gameStateDir, size_t gameStateByteSize, const char* semaphoresDir, size_t semaphoresByteSize, int argc, char* argv[]) {
    int width = strtol(argv[1], NULL, 10);
    int height = strtol(argv[2], NULL, 10);
    gameState->width = width;
    gameState->height = height;

    delay = strtol(argv[3], NULL, 10);
    timeout = strtol(argv[4], NULL, 10);
    seed = strtol(argv[5], NULL, 10);

    //resto los primeros 6 argumentos fijos
    //procName, w,h,d,t,seed,viewBin
    gameState->playerCount = argc - MIN_MASTER_ARGC;
    gameState->finished = false;
    randomizeBoard(gameState->board, width, height, seed);

    // Crea los procesos de players
    for (int i = 0; i < gameState->playerCount; i++) {
        point_t spawnPoint = getSpawnPoint(i, width, height);
        gameState->playerArray[i] = *playerFromBin(argv[MIN_MASTER_ARGC + i], i + 1, spawnPoint.x, spawnPoint.y, gameStateDir, gameStateByteSize, semaphoresDir, semaphoresByteSize);
    }
}

const char * gameState_shm_name = "/gamestate_shm";
const char * semaphores_shm_name = "/semaphores_shm";

int main(int argc, char *argv[]) {
    
    printf("Master PID: %d\n", getpid());
    const int width = strtol(argv[1], NULL, 10);
    const int height = strtol(argv[2], NULL, 10);
    const char* viewBinary = argv[6];

    // Inicialización de la memoria compartida para gameState
    const int shm_fd = shm_open(gameState_shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }
    const size_t boardByteSize = height * width * sizeof(int);
    const size_t gameStateStructByteSize = sizeof(gameState_t) + boardByteSize;
    char structByteSizeStr[16];
    snprintf(structByteSizeStr, sizeof(structByteSizeStr), "%d", (int)gameStateStructByteSize);

    //ftruncate resizea el bloque de memoria compartida de shm_open
    if (ftruncate(shm_fd, gameStateStructByteSize) == -1) {
        perror("ftruncate failed");
        exit(EXIT_FAILURE);
    }

    gameState_t * gameState = mmap(NULL, gameStateStructByteSize, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (gameState == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }
    close(shm_fd); // El descriptor ya no es necesario después de mmap



    // Inicialización de la memoria compartida para semáforos
    const int sem_shm_fd = shm_open(semaphores_shm_name, O_CREAT | O_RDWR, 0666);
    if (sem_shm_fd == -1) {
        perror("shm_open failed for semaphores");
        exit(EXIT_FAILURE);
    }
    const size_t semaphoresStructByteSize = sizeof(semaphores_t);
    // ftruncate resizea el bloque de memoria compartida de shm_open
    if (ftruncate(sem_shm_fd, semaphoresStructByteSize) == -1) {
        perror("ftruncate failed for semaphores");
        exit(EXIT_FAILURE);
    }
    semaphores_t *semaphores = mmap(NULL, semaphoresStructByteSize, PROT_READ | PROT_WRITE, MAP_SHARED, sem_shm_fd, 0);
    if (semaphores == MAP_FAILED) {
        perror("mmap failed for semaphores");
        exit(EXIT_FAILURE);
    }
    close(sem_shm_fd); // El descriptor ya no es necesario después de mmap

    // Inicializar los semáforos
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



    // Crear proceso de vista
    char * const viewArgs[] = {(char *)viewBinary, (char *)gameState_shm_name, structByteSizeStr, NULL};
    pid_t view_pid = newProc(viewBinary, viewArgs);

    if (view_pid == -1) {
        printf("Error creando proceso vista\n");
        return EXIT_FAILURE;
    }
    printf("Creado proceso vista con PID: %d\n", view_pid);

    populateGameStateFromArgs(gameState,semaphores, gameState_shm_name, gameStateStructByteSize, semaphores_shm_name, semaphoresStructByteSize, argc, argv);

    while (!gameState->finished) {
        // En en while:
        // 1. Esperar a que los jugadores envíen movimientos (si los hay) [TODO]
        // 2. Procesar y actualizar el estado del juego [TODO]
        // 3. Notificar a la vista que hay cambios para mostrar [OK]
        // 4. Esperar a que la vista termine de mostrar los cambios [OK]

        // TODO: recibir_movimiento() (también validarlo en la función);
        
        sem_wait(&semaphores->masterMutex);
        sem_wait(&semaphores->gameStateMutex);  // para qué es este?

        // TODO: Actualizar el estado del juego

        sem_post(&semaphores->masterMutex); // Liberar la lectura para los players

        sem_post(&semaphores->masterToView); // Notificar a la vista que hay cambios
        sem_wait(&semaphores->viewToMaster); // Esperar a que la vista termine de mostrar los cambios
        
        
        printf("Paso un ciclo de master\n");
        sleep(delay); // Usar el delay configurado
        gameState->finished = true; // Agregado temporalmente para salir del loop
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
    munmap(gameState, gameStateStructByteSize);
    shm_unlink(gameState_shm_name);
    printf("Terminó master con PID: %d\n", getpid());

    return 0;
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