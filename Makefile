CC = gcc
CFLAGS = -Wall -std=c99 -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE
LDFLAGS = -pthread -lrt
NCURSES_LIBS = -lncursesw
SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = obj
PLAYERS_SRC := $(wildcard src/playersIA/*.c)

# Crear directorios necesarios
$(shell mkdir -p $(BIN_DIR) $(OBJ_DIR))

# Archivo objeto de la librería
UTILS_OBJ = $(OBJ_DIR)/chompChampsUtils.o

all: $(BIN_DIR)/master $(BIN_DIR)/player $(BIN_DIR)/view
	chmod +x $(BIN_DIR)/master $(BIN_DIR)/player $(BIN_DIR)/view

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
			 ./bin/master -p ./bin/player -v ./bin/view -d 10

clean:
	rm -f $(BIN_DIR)/master $(BIN_DIR)/player $(BIN_DIR)/view
	rm -f $(OBJ_DIR)/*.o

runtest: all
	./$(BIN_DIR)/master -p ./$(BIN_DIR)/player ./$(BIN_DIR)/player -v ./$(BIN_DIR)/view -d 50

docker-pull:
	docker pull agodio/itba-so-multi-platform:3.0

docker-run:
	docker run -it --rm -v $(PWD):/root/workspace agodio/itba-so-multi-platform:3.0 /bin/bash -c "cd /root/workspace && exec bash"

docker-setup:
	docker run -it --rm -v $(PWD):/root/workspace agodio/itba-so-multi-platform:3.0 /bin/bash -c "cd /root/workspace && make setup && exec bash"

setup: 
	apt install -y locales libncursesw5-dev && \
	echo "en_US.UTF-8 UTF-8" >> /etc/locale.gen && \
	locale-gen en_US.UTF-8 && \
	update-locale LANG=en_US.UTF-8 && \
	echo 'export LANG=en_US.UTF-8' >> ~/.bashrc && \
	echo 'export LC_ALL=en_US.UTF-8' >> ~/.bashrc

.PHONY: all clean setup memcheck runtest docker-pull docker-run docker-setup
