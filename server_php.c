#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <semaphore.h>
#include <fcntl.h>

#include <sys/mman.h>
#include <sys/wait.h>

#include "server_functions.c"

#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
char* sem_name = "/sem_serverphp_c";

int processCount = 0;       // Cantidad de procesos
int serverSocket;           // Socket descriptor del servidor
char* folderpath;           // Directorio donde se guardan las imagenes
pid_t* process_id;          // Lista con los identificadores de los hijos
sem_t** semaphores;         // Lista con los semaforos
char** sem_names;           // Lista con los nombres de los semaforos
int** child_status;         // Lista con el estado de los hijos
int** child_write_enable;   // Lista con el write enable de los hijos
int** child_socket_client;  // Lista con para pasar el cliente a los hijos

void createProcesses(int count);
void processFunction(char* semaphore_name, int* status, int* write_enable, int* client_socket);
void handle_sigint(int sig);

int main(int argc, char **argv) {
    // Manejo de la senal SIGINT
    signal(SIGINT, handle_sigint);

    // Verificacion numero de argumentos
    if(argc != 2) {
        printf("Debe ingresar la cantidad de procesos como argumento\n");
        exit(1);
    }
    processCount = atoi(argv[1]);

    // Creacion de directorios
    folderpath = createFolder("pre_heavy_process");
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

    // Contador de consultas
    int requestCounter = 0;

    // Creacion de los procesos
    createProcesses(processCount);

    // Manejo de las consultas de los clientes
    while (1) {
        // Estructura para obtener la informacion del cliente
        struct sockaddr_in clientAddr;
        unsigned int sin_size = sizeof(clientAddr);

        // Se espera por una conexion con un cliente
        int clientSocket = accept(serverSocket, (struct sockaddr *) &clientAddr, &sin_size);

        int assigned = 0;
        while(!assigned) {
            for (int i = 0; i < processCount; i++) {
                // Se verifica si el proceso hijo se encuentra disponible
                if(*child_status[i]) {
                    // Reiniciar disponibilidad del proceso hijo
                    *child_status[i] = 0;

                    printf("Hijo disponible! %d %d\n", process_id[i], clientSocket);

                    // Aumentar contador de consultas atendidas
                    requestCounter++;

                    // Establecer write enable para la solicitud
                    *child_write_enable[i] = requestCounter;

                    // Establecer cliente que debe atender el proceso hijo
                    *child_socket_client[i] = clientSocket;

                    // Se despierta al proceso hijo
                    sem_post(semaphores[i]);

                    assigned = 1;
                    break;
                }
            }
        }
        printf("%d solicitudes recibidas!\n", requestCounter);
    }

    return 0;
}

void createProcesses(int count) {
    // Solicitud de espacio para almacenar en memoria las listas de control
    process_id = (pid_t*) malloc(sizeof(pid_t)*count); 
    sem_names = (char**) malloc(sizeof(char*)*count); 
    semaphores = (sem_t**) malloc(sizeof(sem_t*)*count);  
    child_status = (int**) malloc(sizeof(int*)*count); 
    child_write_enable = (int**) malloc(sizeof(int*)*count); 
    child_socket_client = (int**) malloc(sizeof(int*)*count); 


    for (int i = 0; i < count; i++) {
        // Creacion del nombre del semaforo
        char* sem_num = int2str(i);
        sem_names[i] = concat(sem_name, sem_num);
        free(sem_num);

        // Creacion del espacio de memoria compartida
        int protection = PROT_READ | PROT_WRITE;
        int visibility = MAP_SHARED | MAP_ANONYMOUS;

        // Estado de disponibilidad del proceso hijo
        child_status[i] = (int*) mmap(NULL, sizeof(int*), protection, visibility, -1, 0);
        *child_status[i] = 1;
        
        // Write enable del proceso hijo
        child_write_enable[i] = (int*) mmap(NULL, sizeof(int*), protection, visibility, -1, 0);
        *child_write_enable[i] = 0;
        
        // Memoria para pasar el cliente al proceso hijo
        child_socket_client[i] = (int*) mmap(NULL, sizeof(int*), protection, visibility, -1, 0);
        *child_socket_client[i] = -1;

        // Apertura del semaforo
        semaphores[i] = sem_open(sem_names[i], O_CREAT | O_EXCL, SEM_PERMS, 0);

        // Creacion del proceso hijo
        process_id[i] = fork();

        if(process_id[i] == 0) {  // Proceso hijo
            processFunction(sem_names[i], 
                            child_status[i], 
                            child_write_enable[i], 
                            child_socket_client[i]);
        }
        printf("%d\n", process_id[i]);
    }
    sleep(1);
}

void processFunction(char* semaphore_name, int* status, int* write_enable, int* client_socket){
    // Limpieza de memoria
    free(process_id);
    free(sem_names);
    free(semaphores);
    free(child_status);
    free(child_write_enable);
    free(child_socket_client);

    // Apertura del semaforo para sincronizacion con el proceso padre
    sem_t *semaphore = sem_open(semaphore_name, O_RDWR);
    if (semaphore == SEM_FAILED) {
        *status = 0;
        perror("sem_open(3) failed");
        exit(EXIT_FAILURE);
    }

    // Ciclo infinito para atender las consultas de los clientes
    while(1) {
        // Se espera por senal del proceso padre
        sem_wait(semaphore);
        // Se atiende la solicitud
        attendRequest(*client_socket, *write_enable, folderpath);
        
        // Se restablece el estado de disponible
        *status = 1;
    }
    exit(0);
}

void handle_sigint(int sig) { 
    for (int i = 0; i < processCount; i++) {
        // Destruir semaforos
        sem_unlink(sem_names[i]);
        free(sem_names[i]);
        
        // Eliminar procesos
        kill(process_id[i], SIGKILL);
    }

    // Cerrar la conexion
    shutdown(serverSocket, SHUT_RDWR);

    free(process_id);
    free(sem_names);
    free(semaphores);
    free(folderpath);
    free(child_status);
    free(child_write_enable);
    free(child_socket_client);

    exit(0);
} 