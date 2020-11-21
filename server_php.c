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

#include <sys/time.h>

#include "server_functions.c"

#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
char* sem_name = "/sem_serverphp_c";
char* parent_sem = "/sem_parent_php";
char* report_filename = "pre_heavy_process.txt";

typedef struct ChildProcess {
    pid_t id;
    int* status;
    int* write_enable;
    int pipe;
    sem_t* semaphore;
} ChildProcess;

int processCount = 0;       // Cantidad de procesos
int serverSocket;           // Socket descriptor del servidor
char* folderpath;           // Directorio donde se guardan las imagenes
char** sem_names;           // Lista con los nombres de los semaforos
ChildProcess* childs;       // Lista con la informacion de los procesos hijo

void createProcesses(int count);
void processFunction(char* semaphore_name, int* status, int* write_enable, int worker_sd);
void handle_sigint(int sig);
int receiveFileDescriptor(int worker_sd);
void sendFileDescriptor(int server_sd, int clientSocket);

int main(int argc, char **argv) {
    signal(SIGINT, handle_sigint);  // Manejo de la senal SIGINT

    // Verificacion numero de argumentos
    if(argc != 2) {
        printf("Debe ingresar la cantidad de procesos como argumento\n");
        exit(EXIT_FAILURE);
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
    serverAddr.sin_port = htons(8087); // Puerto
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Direccion IP
    
    // Se asigna el puerto al socket
    bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

    // Escucha de conexiones entrantes
    listen(serverSocket, __INT_MAX__);
    
    // Apertura del semaforo para control del proceso padre
    sem_t* sem_parent = sem_open(parent_sem, O_CREAT, SEM_PERMS, processCount);
    
    if(sem_parent == SEM_FAILED) {
        perror("sem_open() failed - parent_sem");
        exit(EXIT_FAILURE);
    }
    
    // Creacion de los procesos
    createProcesses(processCount);

    FILE* pfile = fopen(report_filename, "w");
    fclose(pfile);

    while (1) {
        // Contador de consultas
        int requestCounter = 0;

        // Recibir cantidad de solicitudes que seran enviadas
        int requests = receiveRequestsNumber(serverSocket);

        // start timer
        struct timeval t1, t2;
        double elapsedTime;
        gettimeofday(&t1, NULL);

        // Manejo de las consultas de los clientes
        while (1) {
            // Estructura para obtener la informacion del cliente
            struct sockaddr_in clientAddr;
            unsigned int sin_size = sizeof(clientAddr);
            // Se espera por una conexion con un cliente
            int clientSocket = accept(serverSocket, (struct sockaddr *) &clientAddr, &sin_size);

            // Se espera a que exista un proceso hijo disponible
            sem_wait(sem_parent);
            for (int i = 0; i < processCount; i++) {
                ChildProcess process = childs[i];

                // Se verifica si el proceso hijo se encuentra disponible
                if(*process.status) {
                    *process.status = 0;  // Reiniciar disponibilidad del proceso hijo
                    requestCounter++;  // Aumentar contador de consultas atendidas
                    *process.write_enable = requestCounter;  // Establecer write enable para la solicitud
                    sendFileDescriptor(process.pipe, clientSocket); // Envio del fd del cliente
                    sem_post(process.semaphore);  // Se despierta al proceso hijo
                    break;
                }
            }
            
            printf("%d solicitudes recibidas!\n", requestCounter);

            if (requestCounter == requests){
                // Espera a que todos los procesos hijos terminen
                for (int i = 0; i < processCount; i++)
                    sem_wait(sem_parent);

                // Obtener tiempo transcurrido
                gettimeofday(&t2, NULL);
                elapsedTime = (t2.tv_sec - t1.tv_sec);  // segundos
                elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000000.0;  // us to s
                printf("Tiempo: %f s\n", elapsedTime);

                // Actualizar reporte
                pfile = fopen(report_filename, "a");
                fprintf(pfile, "%d %f ", processCount, elapsedTime);
                fclose(pfile);

                break;
            }
        }
        // Restablecer disponibilidad de los procesos hijos
        for (int i = 0; i < processCount; i++) sem_post(sem_parent);
    }
    return 0;
}

/**
 * Funcion para crear una cantidad count de worker process
 * count: cantidad de procesos a crear
*/
void createProcesses(int count) {
    // Solicitud de espacio para almacenar en memoria las listas de control
    childs = (ChildProcess*) malloc(sizeof(*childs)*processCount);    
    sem_names = (char**) malloc(sizeof(char*)*count); 

    for (int i = 0; i < count; i++) {
        ChildProcess process;
        
        // Creacion del nombre del semaforo
        char* sem_num = int2str(i);
        sem_names[i] = concat(sem_name, sem_num);
        free(sem_num);

        // Creacion del espacio de memoria compartida
        int protection = PROT_READ | PROT_WRITE;
        int visibility = MAP_SHARED | MAP_ANONYMOUS;

        // Estado de disponibilidad del proceso hijo
        process.status = (int*) mmap(NULL, sizeof(int*), protection, visibility, -1, 0);
        *process.status = 1;
        
        // Write enable del proceso hijo
        process.write_enable = (int*) mmap(NULL, sizeof(int*), protection, visibility, -1, 0);
        
        // Apertura del semaforo
        process.semaphore = sem_open(sem_names[i], O_CREAT | O_EXCL, SEM_PERMS, 0);
        if(process.semaphore == SEM_FAILED){
            perror("sem_open() failed - child semaphore");
            exit(EXIT_FAILURE);
        }

        // Creacion del pipe para IPC
        int server_sd, worker_sd, pair_sd[2];
        if(socketpair(AF_UNIX, SOCK_DGRAM, 0, pair_sd) < 0) {
            exit(EXIT_FAILURE);
        }
        server_sd = pair_sd[0];
        worker_sd = pair_sd[1];

        // Creacion del proceso hijo
        process.id = fork();

        if(process.id == 0) {  // Proceso hijo
            close(server_sd);
            processFunction(sem_names[i], 
                            process.status, 
                            process.write_enable,
                            worker_sd); 
        }
        printf("Process id: %d\n", process.id);
        process.pipe = server_sd;
        close(worker_sd);

        childs[i] = process;
        
    }
    sleep(1);
}

/**
 * Funcion para atender las solicitudes enviadas por el proceso padre
 * semaphore_name: identificador del semaforo
 * status: memoria compartida para el estado del proceso
 * write_enable: determina si la imagen recibida se debe guardar o no
 * worker_sd: descriptor del pipe de lectura del proceso hijo
*/
void processFunction(char* semaphore_name, int* status, int* write_enable, int worker_sd){
    // Limpieza de memoria
    free(sem_names);
    free(childs);

    // Apertura del semaforo para control del proceso padre
    sem_t* sem_parent = sem_open(parent_sem, O_RDWR);

    // Apertura del semaforo para sincronizacion con el proceso padre
    sem_t *semaphore = sem_open(semaphore_name, O_RDWR);
    sem_unlink(semaphore_name);
    if (semaphore == SEM_FAILED) {
        *status = 0;
        perror("sem_open(3) failed");
        exit(EXIT_FAILURE);
    }

    // Ciclo infinito para atender las consultas de los clientes
    while(1) {
        // Se espera por senal del proceso padre
        sem_wait(semaphore);

        // Se recibe el descriptor del cliente
        int clientSocket = receiveFileDescriptor(worker_sd);
        
        // Se atiende la solicitud
        attendRequest(clientSocket, *write_enable, folderpath);
        
        // Se restablece el estado de disponible
        *status = 1;
        sem_post(sem_parent);
    }
    exit(0);
}

/**
 * Funcion para recibir un file descriptor proveniente del proceso padre
 * worker_sd: descriptor del pipe de lectura del proceso hijo
 * return: descriptor enviado por el proceso padre
*/
int receiveFileDescriptor(int worker_sd) {
    struct msghdr  child_msg;
    int pass_sd, rc;

    memset(&child_msg,   0, sizeof(child_msg));
    char cmsgbuf[CMSG_SPACE(sizeof(int))];
    child_msg.msg_control = cmsgbuf; // make place for the ancillary message to be received
    child_msg.msg_controllen = sizeof(cmsgbuf);

    rc = recvmsg(worker_sd, &child_msg, 0);
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&child_msg);
    if (cmsg == NULL || cmsg -> cmsg_type != SCM_RIGHTS) {
        printf("The first control structure contains no file descriptor.\n");
        exit(0);
    }
    memcpy(&pass_sd, CMSG_DATA(cmsg), sizeof(pass_sd));

    return pass_sd;
}

/**
 * Funcion para enviar un file descriptor a uno de los procesos hijos
 * server_sd: descriptor del pipe de escritura del proceso padre
 * clientSocket: descriptor del socket que se debe enviar al proceso hijo
*/
void sendFileDescriptor(int server_sd, int clientSocket) {
    struct msghdr parent_msg;

    memset(&parent_msg, 0, sizeof(parent_msg));
    struct cmsghdr *cmsg;
    char cmsgbuf[CMSG_SPACE(sizeof(clientSocket))];
    parent_msg.msg_control = cmsgbuf;
    parent_msg.msg_controllen = sizeof(cmsgbuf); // necessary for CMSG_FIRSTHDR to return the correct value
    cmsg = CMSG_FIRSTHDR(&parent_msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(clientSocket));
    memcpy(CMSG_DATA(cmsg), &clientSocket, sizeof(clientSocket));
    parent_msg.msg_controllen = cmsg->cmsg_len; // total size of all control blocks

    if((sendmsg(server_sd, &parent_msg, 0)) < 0)
    {
        perror("sendmsg()");
        exit(EXIT_FAILURE);
    }
}

/**
 * Funcion para manejar la accion realizada cuando el usuario
 * utiliza CTRL+C para detener el proceso padre
*/
void handle_sigint(int sig) { 
    sem_unlink(parent_sem);
    for (int i = 0; i < processCount; i++) {
        sem_close(childs[i].semaphore);  // Destruir semaforos
        kill(childs[i].id, SIGKILL);  // Eliminar procesos
    }
    shutdown(serverSocket, SHUT_RDWR);  // Cerrar la conexion servidor

    free(childs);
    free(sem_names);
    free(folderpath);
    exit(0);
}