CC = gcc
CFLAGS = -Wall -std=c99 -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE
LDFLAGS = -pthread -lrt

SRC_DIR = src
BIN_DIR = bin

TESTS_SRC = $(wildcard tests/*.c)
TESTS_BIN = $(BIN_DIR)/tests

all: $(BIN_DIR)/master $(BIN_DIR)/player $(BIN_DIR)/view
	chmod +x $(BIN_DIR)/master $(BIN_DIR)/player $(BIN_DIR)/view
	chmod +x run
	@echo "\n\033[33mPara correr el programa hacer:\n./run <FILAS> <COLUMNAS> <DELAY> <TURNOS> <SEED>\033[0m"

$(BIN_DIR)/master: $(SRC_DIR)/master.c $(SRC_DIR)/defs.h
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/master.c $(LDFLAGS)

$(BIN_DIR)/player: $(SRC_DIR)/player.c $(SRC_DIR)/defs.h
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/player.c $(LDFLAGS)

$(BIN_DIR)/view: $(SRC_DIR)/view.c $(SRC_DIR)/defs.h
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/view.c $(LDFLAGS)

tests: $(TESTS_BIN)
$(TESTS_BIN): $(TESTS_SRC)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	chmod +x $@

clean:
	rm -f $(BIN_DIR)/master $(BIN_DIR)/player $(BIN_DIR)/view $(BIN_DIR)/tests

.PHONY: all clean