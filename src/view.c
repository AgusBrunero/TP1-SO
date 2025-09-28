/*
 * view.c (ncurses + UTF-8 characters, double-buffered with left/right layout)
 *
 * - Lado izquierdo: ASCII art (título) y debajo el tablero.
 * - Lado derecho: dimensiones del mapa, estado y columna con puntajes de jugadores.
 *
 * Usa:
 *  - ■ para celdas tomadas
 *  - ඞ para jugador
 *
 * Compilar:
 *  gcc src/view.c -o bin/view -lncursesw -pthread -lrt
 *
 * Asegurate de guardar el archivo en UTF-8 y de tener locale UTF-8
 * (setlocale(LC_ALL, "") está presente).
 */

#include <fcntl.h>
#include <locale.h>
#include <ncurses.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdbool.h>

#include "chompChampsUtils.h"
#include "defs.h"

#define MAX_PLAYERS 9
#define CELL_W 3
#define PLAYER_BOX_H 1
#define MIN_COLS_REQUIRED 40
#define MIN_LINES_REQUIRED 12

/* símbolos UTF-8 */
#define TAKE_CHAR "■"
#define PLAYER_CHAR "ඞ"

/* forward */
static void init_colors_ncurses(void);
static void draw_left_win(WINDOW *left, gameState_t* gameState, const char **ascii_lines, int ascii_count);
static void draw_right_win(WINDOW *right, gameState_t* gameState);
static int player_color_pair(int idx);

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <shm_id> <shm_size_bytes>", argv[0]);
        return EXIT_FAILURE;
    }

    gameState_t* gameState = NULL;
    semaphores_t* semaphores = NULL;
    openReadShm(strtoul(argv[1], NULL, 10), strtoul(argv[2], NULL, 10),
                &gameState, &semaphores);

    setlocale(LC_ALL, "");

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    nodelay(stdscr, TRUE);

    leaveok(stdscr, TRUE);
    syncok(stdscr, FALSE);

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_colors_ncurses();
    }

    WINDOW *left_win = NULL;
    WINDOW *right_win = NULL;
    int last_rows = -1, last_cols = -1;

    /* ASCII art (5 líneas) */
    const char *ascii_lines[] = {
        "   ___ _                      ___ _                       ",
        "  / __| |_  ___ _ __  _ __   / __| |_  __ _ _ __  _ __ ___",
        " | (__| ' \\/ _ \\ '  \\| '_ \\ | (__| ' \\/ _` | '  \\| '_ (_-<",
        "  \\___|_||_\\___/_|_|_| .__/  \\___|_||_\\__,_|_|_|_| .__/__/",
        "                     |_|                         |_|      "
    };
    const int ascii_count = sizeof(ascii_lines)/sizeof(ascii_lines[0]);

    while (!gameState->finished) {
        sem_wait(&semaphores->masterToView);

        int rows, cols;
        getmaxyx(stdscr, rows, cols);

        bool size_changed = (rows != last_rows) || (cols != last_cols);
        last_rows = rows; last_cols = cols;

        if (rows < MIN_LINES_REQUIRED || cols < MIN_COLS_REQUIRED) {
            werase(stdscr);
            mvwprintw(stdscr, 0, 0, "Ventana demasiado pequeña: mínimo %dx%d (filas x columnas).", MIN_LINES_REQUIRED, MIN_COLS_REQUIRED);
            mvwprintw(stdscr, 1, 0, "Redimensioná la terminal y volvé a intentarlo.");
            wnoutrefresh(stdscr);
            doupdate();
            sem_post(&semaphores->viewToMaster);
            napms(100);
            continue;
        }

        /* calcular columnas: left 60%, right 40% con 1 columna de separación */
        int left_w = (cols * 60) / 100;
        if (left_w < 20) left_w = cols - 20; /* fallback si terminal pequeña */
        int sep = 1;
        int right_w = cols - left_w - sep;
        if (right_w < 20) { right_w = 20; left_w = cols - right_w - sep; }
        int win_h = rows;

        /* crear/reubicar ventanas si es necesario */
        if (left_win == NULL || size_changed) {
            if (left_win) { delwin(left_win); left_win = NULL; }
            left_win = newwin(win_h, left_w, 0, 0);
            if (!left_win) { endwin(); fprintf(stderr, "No se pudo crear left_win"); exit(EXIT_FAILURE); }
            leaveok(left_win, TRUE); syncok(left_win, FALSE);
        } else {
            mvwin(left_win, 0, 0);
            wresize(left_win, win_h, left_w);
            werase(left_win);
        }

        if (right_win == NULL || size_changed) {
            if (right_win) { delwin(right_win); right_win = NULL; }
            right_win = newwin(win_h, right_w, 0, left_w + sep);
            if (!right_win) { endwin(); fprintf(stderr, "No se pudo crear right_win"); exit(EXIT_FAILURE); }
            leaveok(right_win, TRUE); syncok(right_win, FALSE);
        } else {
            mvwin(right_win, 0, left_w + sep);
            wresize(right_win, win_h, right_w);
            werase(right_win);
        }

        draw_left_win(left_win, gameState, ascii_lines, ascii_count);
        draw_right_win(right_win, gameState);

        wnoutrefresh(left_win);
        wnoutrefresh(right_win);

        doupdate();

        sem_post(&semaphores->viewToMaster);
        napms(10);
    }

    sem_wait(&semaphores->masterToView);
    if (left_win) werase(left_win);
    if (right_win) werase(right_win);
    draw_left_win(left_win ? left_win : stdscr, gameState, ascii_lines, ascii_count);
    draw_right_win(right_win ? right_win : stdscr, gameState);
    if (left_win) wnoutrefresh(left_win);
    if (right_win) wnoutrefresh(right_win);
    doupdate();
    sem_post(&semaphores->viewToMaster);

    if (left_win) { delwin(left_win); left_win = NULL; }
    if (right_win) { delwin(right_win); right_win = NULL; }

    curs_set(1);
    endwin();

    if (gameState != NULL) {
        size_t gameStateSize = sizeof(gameState_t) + (size_t)gameState->width * gameState->height * sizeof(int);
        if (munmap(gameState, gameStateSize) == -1) perror("view: munmap(gameState) failed");
    }
    if (semaphores != NULL) {
        if (munmap(semaphores, sizeof(semaphores_t)) == -1) perror("view: munmap(semaphores) failed");
    }

    return 0;
}

static void draw_left_win(WINDOW *left, gameState_t* gameState, const char **ascii_lines, int ascii_count) {
    if (!left || !gameState) return;
    int win_h, win_w; getmaxyx(left, win_h, win_w);

    werase(left);

    /* imprimir ASCII art centrado en la parte superior */
    for (int i = 0; i < ascii_count; ++i) {
        const char *line = ascii_lines[i];
        int lx = (win_w - (int)strlen(line)) / 2;
        if (lx < 0) lx = 0;
        mvwprintw(left, i, lx, "%s", line);
    }

    int board_start_y = ascii_count + 1;
    int available_h = win_h - board_start_y;
    if (available_h <= 0) return;

    /* ancho requerido para el tablero */
    int board_w_req = gameState->width * CELL_W;
    int board_w = board_w_req > win_w ? win_w : board_w_req;
    int board_x = (win_w - board_w) / 2;
    if (board_x < 0) board_x = 0;

    int playerCount = gameState->playerCount;
    for (int i = 0; i < gameState->height && i < available_h; i++) {
        for (int j = 0; j < gameState->width && j * CELL_W < board_w; j++) {
            int x = board_x + j * CELL_W;
            int y = board_start_y + i;
            bool isPlayer = false; int playerIdx = -1;
            for (int p = 0; p < playerCount; p++) {
                if (gameState->playerArray[p].x == j && gameState->playerArray[p].y == i) { isPlayer = true; playerIdx = p; break; }
            }
            if (isPlayer) {
                int pair = player_color_pair(playerIdx);
                if (pair) wattron(left, COLOR_PAIR(pair) | A_BOLD);
                mvwprintw(left, y, x, " %s ", PLAYER_CHAR);
                if (pair) wattroff(left, COLOR_PAIR(pair) | A_BOLD);
            } else {
                int cellValue = gameState->board[i * gameState->width + j];
                if (cellValue > 0) {
                    mvwprintw(left, y, x, "%2d ", cellValue);
                } else {
                    int ownerIdx = cellValue * (-1);
                    int pair = player_color_pair(ownerIdx);
                    if (pair) wattron(left, COLOR_PAIR(pair));
                    mvwprintw(left, y, x, " %s ", TAKE_CHAR);
                    if (pair) wattroff(left, COLOR_PAIR(pair));
                }
            }
        }
    }
}

static void draw_right_win(WINDOW *right, gameState_t* gameState) {
    if (!right || !gameState) return;
    int win_h, win_w; getmaxyx(right, win_h, win_w);
    werase(right);

    /* Map dimensions and state at top-right area (left-aligned inside right window) */
    char mapbuf[64]; snprintf(mapbuf, sizeof(mapbuf), "Mapa: %dx%d", gameState->width, gameState->height);
    char statebuf[64]; const char* estado = gameState->finished ? "Finalizado" : "En juego"; snprintf(statebuf, sizeof(statebuf), "Estado: %s", estado);
    mvwprintw(right, 0, 1, "%s", mapbuf);
    mvwprintw(right, 1, 1, "%s", statebuf);

    mvwhline(right, 2, 0, ACS_HLINE, win_w);

    playerRank_t rankings[MAX_PLAYERS];
    getPlayersRanking(gameState, rankings);
    int n = gameState->playerCount;

    int start_y = 3;
    for (int i = 0; i < n && (start_y + i) < win_h; ++i) {
        player_t *pl = rankings[i].player;
        int pair = player_color_pair(rankings[i].originalIndex);
        if (pair) wattron(right, COLOR_PAIR(pair) | A_BOLD);
        char line[128]; snprintf(line, sizeof(line), "%-12s  %4d", pl->name, pl->score);
        mvwprintw(right, start_y + i, 1, "%s", line);
        if (pair) wattroff(right, COLOR_PAIR(pair) | A_BOLD);
    }
}

/* Inicializa pares de colores (1..9) segun indice de jugador */
static void init_colors_ncurses(void) {
    init_pair(1, COLOR_RED,    -1);
    init_pair(2, COLOR_GREEN,  -1);
    init_pair(3, COLOR_YELLOW, -1);
    init_pair(4, COLOR_BLUE,   -1);
    init_pair(5, COLOR_MAGENTA,-1);
    init_pair(6, COLOR_CYAN,   -1);
    init_pair(7, COLOR_WHITE,  -1);
    init_pair(8, COLOR_YELLOW, -1);
    init_pair(9, COLOR_MAGENTA,-1);
}

/* Devuelve el numero de pair color para el jugador (1..9). Si no hay color, 0 */
static int player_color_pair(int idx) {
    if (idx < 0 || idx >= MAX_PLAYERS) return 0;
    return idx + 1; /* init_pair 1..9 */
}
