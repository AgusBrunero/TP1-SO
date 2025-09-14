CC = gcc
CFLAGS = -g -Wall -std=c99 -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE
LDFLAGS = -pthread -lrt
SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = obj
PLAYERS_SRC := $(wildcard src/playersIA/*.c)
PLAYERS_BIN := $(patsubst src/playersIA/%.c, $(BIN_DIR)/%, $(PLAYERS_SRC))
TESTS_SRC = $(wildcard tests/*.c)
TESTS_BIN = $(BIN_DIR)/tests


# Crear directorios necesarios
$(shell mkdir -p $(BIN_DIR) $(OBJ_DIR))

# Archivo objeto de la librería
UTILS_OBJ = $(OBJ_DIR)/chompChampsUtils.o

all: $(BIN_DIR)/master $(BIN_DIR)/player $(BIN_DIR)/view
	chmod +x $(BIN_DIR)/master $(BIN_DIR)/player $(BIN_DIR)/view
	@echo "\n\033[33mPara correr el programa hacer:\n./run <FILAS> <COLUMNAS> <DELAY> <TURNOS> <SEED>\033[0m"

# Regla para compilar la librería como archivo objeto
$(UTILS_OBJ): $(SRC_DIR)/chompChampsUtils.c $(SRC_DIR)/chompChampsUtils.h $(SRC_DIR)/defs.h
	$(CC) $(CFLAGS) -c -o $@ $(SRC_DIR)/chompChampsUtils.c

# Reglas para los ejecutables - ahora incluyen la librería
$(BIN_DIR)/master: $(SRC_DIR)/master.c $(SRC_DIR)/defs.h $(UTILS_OBJ)
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/master.c $(UTILS_OBJ) $(LDFLAGS)

$(BIN_DIR)/player: $(SRC_DIR)/player.c $(SRC_DIR)/defs.h $(UTILS_OBJ)
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/player.c $(UTILS_OBJ) $(LDFLAGS)

$(BIN_DIR)/view: $(SRC_DIR)/view.c $(SRC_DIR)/defs.h $(UTILS_OBJ)
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/view.c $(UTILS_OBJ) $(LDFLAGS)


players: $(PLAYERS_BIN)
$(BIN_DIR)/%: src/playersIA/%.c $(SRC_DIR)/player.c $(SRC_DIR)/defs.h $(UTILS_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	chmod +x $@

tests: $(TESTS_BIN)

$(TESTS_BIN): $(TESTS_SRC) $(UTILS_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	chmod +x $@

clean:
	rm -f $(BIN_DIR)/master $(BIN_DIR)/player $(BIN_DIR)/view $(BIN_DIR)/tests
	rm -f $(OBJ_DIR)/*.o

.PHONY: all clean tests