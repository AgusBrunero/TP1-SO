CC = gcc
CFLAGS = -g -Wall -std=c99 -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE
LDFLAGS = -pthread -lrt
NCURSES_LIBS = -lncursesw
SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = obj
PLAYERS_SRC := $(wildcard src/playersIA/*.c)
PLAYERS_BIN := $(patsubst src/playersIA/%.c, $(BIN_DIR)/%, $(PLAYERS_SRC))

# Crear directorios necesarios
$(shell mkdir -p $(BIN_DIR) $(OBJ_DIR))

# Archivo objeto de la librería
UTILS_OBJ = $(OBJ_DIR)/chompChampsUtils.o

all: $(BIN_DIR)/master $(BIN_DIR)/player $(BIN_DIR)/view
	chmod +x $(BIN_DIR)/master $(BIN_DIR)/player $(BIN_DIR)/view
	@echo "\n\033[33mPara correr el programa hacer:\n./run <FILAS> <COLUMNAS> <DELAY> <TIMEOUT> <SEED>\033[0m"

$(UTILS_OBJ): $(SRC_DIR)/chompChampsUtils.c $(SRC_DIR)/chompChampsUtils.h $(SRC_DIR)/defs.h
	$(CC) $(CFLAGS) -c -o $@ $(SRC_DIR)/chompChampsUtils.c

$(BIN_DIR)/master: $(SRC_DIR)/master.c $(SRC_DIR)/defs.h $(UTILS_OBJ)
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/master.c $(UTILS_OBJ) $(LDFLAGS)

$(BIN_DIR)/player: $(SRC_DIR)/player.c $(SRC_DIR)/defs.h $(UTILS_OBJ)
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/player.c $(UTILS_OBJ) $(LDFLAGS)

$(BIN_DIR)/view: $(SRC_DIR)/view.c $(SRC_DIR)/defs.h $(UTILS_OBJ)
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/view.c $(UTILS_OBJ) $(LDFLAGS) $(NCURSES_LIBS)

memcheck: all
	valgrind --leak-check=full \
			 --show-leak-kinds=all \
			 --track-origins=yes \
			 --verbose \
			 --log-file=valgrind-out.txt \
			 ./run 20 20 20 40 123

clean:
	rm -f $(BIN_DIR)/master $(BIN_DIR)/player $(BIN_DIR)/view
	rm -f $(OBJ_DIR)/*.o


.PHONY: all clean
