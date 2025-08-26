//
// Created by nacho on 8/25/2025.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <bits/fcntl-linux.h>
#include <sys/mman.h>


/*
 * Espero 2 parametro en forma shm_open name, and byteSize
 */
int main(int argc, char* argv[]) {
    int shrMemFd = shm_open(argv[1], O_RDWR, 0666);
    char * shmMem = mmap(NULL, strtol(argv[2], NULL, 10), PROT_READ | PROT_WRITE, MAP_SHARED, shrMemFd, 0);


    printf("¡Hola! Soy un JUGADOR con PID: %d\n", getpid());
    printf("Recibí ancho: %s, alto: %s\n", argv[1], argv[2]);
    sleep(1);  // Simular trabajo
    printf("Jugador terminando...\n");
    return 0;
}
