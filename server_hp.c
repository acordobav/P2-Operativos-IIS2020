#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/time.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>

#include "server_functions.c"

#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
char* parent_sem = "/parent_sem_hp";
char* report_filename = "heavy_process.txt";

char* folderpath;  // Directorio donde se guardan las imagenes
int serverSocket;  // Descriptor del socker servidor

void childFunction(int clientSocket, int id);
void handle_sigint(int sig);

int main() {
    signal(SIGINT, handle_sigint);  // Manejo de la senal SIGINT

    // Creacion de directorios
    folderpath = createFolder("heavy_process");
    printf("%s\n", folderpath);

    // Creacion del descriptor del socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    
    // Configuracion de direccion y puerto del servidor
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080); // Puerto
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Direccion IP
    
    // Se asigna el puerto al socket
    bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

    // Escucha de conexiones entrantes
    listen(serverSocket, __INT_MAX__);

    // Apertura del semaforo para control del proceso padre
    sem_t* sem_parent = sem_open(parent_sem, O_CREAT, SEM_PERMS, 0);

    FILE* pfile = fopen(report_filename, "w");
    fclose(pfile);
    while (1) {
        // Recibir cantidad de solicitudes que seran enviadas
        int totalRequests = receiveRequestsNumber(serverSocket);

        // start timer
        struct timeval t1, t2;
        double elapsedTime;
        gettimeofday(&t1, NULL);

        int processCount = 0;
        // Manejo de las consultas de los clientes
        while (1) {
            // Estructura para obtener la informacion del cliente
            struct sockaddr_in clientAddr;
            unsigned int sin_size = sizeof(clientAddr);

            // Se espera por una conexion con un cliente
            int clientSocket = accept(serverSocket, (struct sockaddr *) &clientAddr, &sin_size);

            processCount++;
            if(fork() == 0) { //Child process
                childFunction(clientSocket, processCount); 
                exit(EXIT_SUCCESS);

            } else { // Parent process
                printf("%d solicitudes recibidas!\n", processCount);
                if (processCount == totalRequests){
                    // Espera a que todos los procesos hijos terminen
                    for (int i = 0; i < processCount; i++) sem_wait(sem_parent);
                    break;
                }
            }
        }
        
        // Obtener tiempo transcurrido
        gettimeofday(&t2, NULL);
        elapsedTime = (t2.tv_sec - t1.tv_sec); // segundos
        elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000000.0;   // us to s
        printf("Tiempo: %f s\n", elapsedTime);

        // Actualizar reporte
        pfile = fopen(report_filename, "a");
        fprintf(pfile, "%d %f ", processCount, elapsedTime);
        fclose(pfile);
    }

    // Cerrar la conexion
    shutdown(serverSocket, SHUT_RDWR);
    
    free(folderpath);
    return 0;
}

/**
 * Funcion que se encarga de atender la solicitud del cliente
 * clientSocket: identificador del socket del cliente
 * id: identificador del proceso que atiende la solicutd
*/
void childFunction(int clientSocket, int id) {
    // Apertura del semaforo para control del proceso padre
    sem_t* sem_parent = sem_open(parent_sem, O_RDWR);

    // Procesamiento de la solicitud
    attendRequest(clientSocket, id, folderpath);

    // Indicar al proceso padre de la finalizacion
    sem_post(sem_parent);
    sem_close(sem_parent);

    exit(EXIT_SUCCESS);
}

/**
 * Funcion para manejar la accion realizada cuando el usuario
 * utiliza CTRL+C para detener el proceso padre
*/
void handle_sigint(int sig) { 
    sem_unlink(parent_sem);
    free(folderpath);
    shutdown(serverSocket, SHUT_RDWR);
    exit(0);
}