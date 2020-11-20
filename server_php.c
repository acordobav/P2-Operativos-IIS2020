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
    serverAddr.sin_port = htons(8085); // Puerto
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
                ChildProcess process = childs[i];

                // Se verifica si el proceso hijo se encuentra disponible
                if(*process.status) {
                    // Reiniciar disponibilidad del proceso hijo
                    *process.status = 0;

                    // Aumentar contador de consultas atendidas
                    requestCounter++;

                    // Establecer write enable para la solicitud
                    *process.write_enable = requestCounter;

                    // Se envia el fd del cliente que debe atender el proceso hijo
                    sendFileDescriptor(process.pipe, clientSocket);

                    // Se despierta al proceso hijo
                    sem_post(process.semaphore);

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
        printf("%d\n", process.id);
        process.pipe = server_sd;
        close(worker_sd);

        childs[i] = process;
    }
    sleep(1);
}

void processFunction(char* semaphore_name, int* status, int* write_enable, int worker_sd){
    // Limpieza de memoria
    free(sem_names);
    free(childs);

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

        int clientSocket = receiveFileDescriptor(worker_sd);
        
        // Se atiende la solicitud
        attendRequest(clientSocket, *write_enable, folderpath);
        
        // Se restablece el estado de disponible
        *status = 1;
    }
    exit(0);
}

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
    for (int i = 0; i < processCount; i++) {
        // Destruir semaforos
        sem_unlink(sem_names[i]);
        
        // Eliminar procesos
        kill(childs[i].id, SIGKILL);
    }

    // Cerrar la conexion
    shutdown(serverSocket, SHUT_RDWR);

    free(childs);
    free(sem_names);
    free(folderpath);
    exit(0);
} 