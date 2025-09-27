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
#define PLAYER_BOX_H 5
#define PLAYER_BOX_GAP 3
#define MIN_COLS_REQUIRED 20
#define MIN_LINES_REQUIRED 10

/* símbolos UTF-8 */
#define TAKE_CHAR "■"
#define PLAYER_CHAR "ඞ"

static void init_colors_ncurses(void);
static void draw_board_into_win(WINDOW *win, gameState_t* gameState);
static void draw_ranking_into_win(WINDOW *win, gameState_t* gameState);
static int player_color_pair(int idx);

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr,
                "Uso: %s <shm_id> <shm_size_bytes>\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    gameState_t* gameState = NULL;
    semaphores_t* semaphores = NULL;
    openReadShm(strtoul(argv[1], NULL, 10), strtoul(argv[2], NULL, 10),
                &gameState, &semaphores);

    setlocale(LC_ALL, "");

    /* ncurses init */
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    nodelay(stdscr, TRUE);

    /* minimizar movimientos visibles del cursor y sincronizaciones automáticas */
    leaveok(stdscr, TRUE);
    syncok(stdscr, FALSE);

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_colors_ncurses();
    }

    WINDOW *board_win = NULL;
    WINDOW *rank_win  = NULL;
    int last_rows = -1, last_cols = -1;

    while (!gameState->finished) {
        sem_wait(&semaphores->masterToView);

        int rows, cols;
        getmaxyx(stdscr, rows, cols);

        /* Si cambió el tamaño de la terminal, recrear ventanas */
        bool size_changed = (rows != last_rows) || (cols != last_cols);
        last_rows = rows; last_cols = cols;

        if (rows < MIN_LINES_REQUIRED || cols < MIN_COLS_REQUIRED) {
            /* mostrar mensaje de tamaño insuficiente en stdscr (virtual) */
            werase(stdscr);
            const char *m1 = "Ventana demasiado pequeña:";
            const char *m2 = "Redimensioná la terminal y volvé a intentarlo.";
            mvwprintw(stdscr, 0, 0, "%s mínimo %dx%d (filas x columnas).", m1, MIN_LINES_REQUIRED, MIN_COLS_REQUIRED);
            mvwprintw(stdscr, 1, 0, "%s", m2);
            wnoutrefresh(stdscr);
            doupdate();

            sem_post(&semaphores->viewToMaster);
            /* pequeña espera para no spamear */
            napms(50);
            continue;
        }

        /* calcular dimensiones del tablero y ranking */
        int board_h = gameState->height;
        int board_w = gameState->width * CELL_W;
        int board_y = 1;
        int board_x = (cols - board_w) / 2;
        if (board_x < 0) board_x = 0;

        /* ranking: calcular filas necesarias */
        playerRank_t rankings[MAX_PLAYERS];
        getPlayersRanking(gameState, rankings);
        int nplayers = gameState->playerCount;
        int ranking_rows_needed = ((nplayers + 2) / 3) * PLAYER_BOX_H + 2;
        int ranking_h = ranking_rows_needed;
        int ranking_y = board_y + board_h + 1;
        if (ranking_y + ranking_h > rows) {
            /* intentar poner ranking más arriba si no cabe */
            ranking_y = rows - ranking_h - 1;
            if (ranking_y < board_y + board_h + 1) {
                ranking_y = board_y + board_h + 1; /* fallback */
                /* si aún no entra, ajusto ranking_h para que quepa */
                if (ranking_y + ranking_h > rows) ranking_h = rows - ranking_y - 1;
            }
        }
        int ranking_w = cols;
        int ranking_x = 0;

        /* crear/reubicar ventanas si no existen o cambió tamaño */
        if (board_win == NULL || size_changed) {
            if (board_win) {
                delwin(board_win);
                board_win = NULL;
            }
            /* asegurarse que cabe */
            if (board_h > rows - 2) board_h = rows - 2;
            if (board_w > cols) board_w = cols;
            board_win = newwin(board_h, board_w, board_y, board_x);
            if (!board_win) {
                endwin();
                fprintf(stderr, "No se pudo crear board_win\n");
                exit(EXIT_FAILURE);
            }
            leaveok(board_win, TRUE);
            syncok(board_win, FALSE);
        } else {
            /* si existe pero quizás la posición cambió, movemos y ajustamos */
            mvwin(board_win, board_y, board_x);
            wresize(board_win, board_h, board_w);
            werase(board_win);
        }

        if (rank_win == NULL || size_changed) {
            if (rank_win) {
                delwin(rank_win);
                rank_win = NULL;
            }
            if (ranking_h < 1) ranking_h = 1;
            rank_win = newwin(ranking_h, ranking_w, ranking_y, ranking_x);
            if (!rank_win) {
                endwin();
                fprintf(stderr, "No se pudo crear rank_win\n");
                exit(EXIT_FAILURE);
            }
            leaveok(rank_win, TRUE);
            syncok(rank_win, FALSE);
        } else {
            mvwin(rank_win, ranking_y, ranking_x);
            wresize(rank_win, ranking_h, ranking_w);
            werase(rank_win);
        }

        /* dibujar en ventanas (todo en la "pantalla virtual") */
        werase(stdscr); /* usar stdscr sólo para título si se desea */
        /* Título centrado en stdscr */
        const char* estado = gameState->finished ? "Finalizado" : "En juego";
        char title[128];
        snprintf(title, sizeof(title), "CHOMPCHAMPS: Mapa %dx%d | Estado: %s",
                 gameState->width, gameState->height, estado);
        int title_x = (cols - (int)strlen(title)) / 2;
        if (title_x < 0) title_x = 0;
        mvwprintw(stdscr, 0, title_x, "%s", title);

        /* dibujar tablero y ranking en sus ventanas */
        draw_board_into_win(board_win, gameState);
        draw_ranking_into_win(rank_win, gameState);

        /* volcar ventanas al virtual screen (wnoutrefresh) */
        wnoutrefresh(stdscr);       /* título */
        wnoutrefresh(board_win);
        wnoutrefresh(rank_win);

        /* actualizar pantalla física UNA SOLA VEZ */
        doupdate();

        /* liberar al master */
        sem_post(&semaphores->viewToMaster);

        /* pequeña espera para evitar bucle excesivo */
        napms(10);
    }

    /* Último repintado final (igual que dentro del loop) */
    sem_wait(&semaphores->masterToView);

    /* asegurarse que ventanas existan */
    if (board_win) werase(board_win);
    if (rank_win) werase(rank_win);
    werase(stdscr);

    /* volver a dibujar final */
    draw_board_into_win(board_win ? board_win : stdscr, gameState);
    if (rank_win) draw_ranking_into_win(rank_win, gameState);

    if (board_win) wnoutrefresh(board_win);
    if (rank_win) wnoutrefresh(rank_win);
    wnoutrefresh(stdscr);
    doupdate();

    sem_post(&semaphores->viewToMaster);

    /* cleanup */
    if (board_win) { delwin(board_win); board_win = NULL; }
    if (rank_win)  { delwin(rank_win);  rank_win  = NULL; }

    curs_set(1);
    endwin();

    if (gameState != NULL) {
        size_t gameStateSize = sizeof(gameState_t) + (size_t)gameState->width *
                                                         gameState->height *
                                                         sizeof(int);
        if (munmap(gameState, gameStateSize) == -1) {
            perror("view: munmap(gameState) failed");
        }
    }
    if (semaphores != NULL) {
        if (munmap(semaphores, sizeof(semaphores_t)) == -1) {
            perror("view: munmap(semaphores) failed");
        }
    }

    return 0;
}

/* Inicializa pares de colores (1..9) según índice de jugador */
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

/* Devuelve el número de pair color para el jugador (1..9). Si no hay color, 0 */
static int player_color_pair(int idx) {
    if (idx < 0 || idx >= MAX_PLAYERS) return 0;
    return idx + 1; /* init_pair 1..9 */
}

/* Dibuja el tablero dentro de la ventana pasada (coordenadas locales a la ventana) */
static void draw_board_into_win(WINDOW *win, gameState_t* gameState) {
    if (!win || !gameState) return;

    int win_h, win_w;
    getmaxyx(win, win_h, win_w);

    int playerCount = gameState->playerCount;
    /* top-left de contenido: (0,0) dentro de la ventana */
    for (int i = 0; i < gameState->height && i < win_h; i++) {
        for (int j = 0; j < gameState->width && j * CELL_W < win_w; j++) {
            int x = j * CELL_W;
            int y = i;
            bool isPlayer = false;
            int playerIdx = -1;
            for (int p = 0; p < playerCount; p++) {
                if (gameState->playerArray[p].x == j &&
                    gameState->playerArray[p].y == i) {
                    isPlayer = true;
                    playerIdx = p;
                    break;
                }
            }

            if (isPlayer) {
                int pair = player_color_pair(playerIdx);
                if (pair) wattron(win, COLOR_PAIR(pair) | A_BOLD);
                mvwprintw(win, y, x, " %s ", PLAYER_CHAR);
                if (pair) wattroff(win, COLOR_PAIR(pair) | A_BOLD);
            } else {
                int cellValue = gameState->board[i * gameState->width + j];
                if (cellValue > 0) {
                    mvwprintw(win, y, x, "%2d ", cellValue);
                } else {
                    int ownerIdx = cellValue * (-1);
                    int pair = player_color_pair(ownerIdx);
                    if (pair) wattron(win, COLOR_PAIR(pair));
                    mvwprintw(win, y, x, " %s ", TAKE_CHAR);
                    if (pair) wattroff(win, COLOR_PAIR(pair));
                }
            }
        }
    }
}

/* Dibuja el ranking dentro de la ventana pasada (coordenadas locales a la ventana) */
static void draw_ranking_into_win(WINDOW *win, gameState_t* gameState) {
    if (!win || !gameState) return;

    playerRank_t rankings[MAX_PLAYERS];
    getPlayersRanking(gameState, rankings);

    int n = gameState->playerCount;
    int win_h, win_w;
    getmaxyx(win, win_h, win_w);

    /* Calcular ancho de nombre */
    int nameWidth = 0;
    for (int i = 0; i < n; i++) {
        int len = (int)strlen(rankings[i].player->name);
        if (len > nameWidth) nameWidth = len;
    }
    if (nameWidth < 4) nameWidth = 4;
    if (nameWidth > 20) nameWidth = 20;

    int box_w = nameWidth + 14;
    if (box_w < 20) box_w = 20;

    /* Empezar a dibujar desde la línea 0 de la ventana */
    int idx = 0;
    int current_row = 0;
    while (idx < n) {
        int remain = n - idx;
        int per_row = remain >= 3 ? 3 : remain;

        int total_w = per_row * box_w + (per_row - 1) * PLAYER_BOX_GAP;
        int start_x = (win_w - total_w) / 2;
        if (start_x < 0) start_x = 0;

        int y0 = current_row * PLAYER_BOX_H;
        /* si no hay espacio vertical suficiente, cortamos */
        if (y0 + PLAYER_BOX_H > win_h) break;

        for (int k = 0; k < per_row; k++) {
            int pidx = idx + k;
            int x0 = start_x + k * (box_w + PLAYER_BOX_GAP);

            int pair = player_color_pair(rankings[pidx].originalIndex);
            if (pair) wattron(win, COLOR_PAIR(pair) | A_BOLD);
            mvwprintw(win, y0 + 0, x0, "Jugador: %-*s", nameWidth, rankings[pidx].player->name);
            if (pair) wattroff(win, COLOR_PAIR(pair) | A_BOLD);

            mvwprintw(win, y0 + 1, x0, "Puntaje: %*d", nameWidth - 2, rankings[pidx].player->score);
            mvwprintw(win, y0 + 2, x0, "Válidos: %*d", nameWidth - 2, rankings[pidx].player->validReqs);
            mvwprintw(win, y0 + 3, x0, "Inválidos:%*d", nameWidth - 2, rankings[pidx].player->invalidReqs);
            mvwprintw(win, y0 + 4, x0, "Bloqueado: %s", rankings[pidx].player->isBlocked ? "Si" : "No");
        }

        idx += per_row;
        current_row++;
    }
}
