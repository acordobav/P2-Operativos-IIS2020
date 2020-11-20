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

#include "receive_image.c"
#include "general_functions.c"

#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
char* sem_name = "/sem_serverphp_c";

char* folderpath;
int processCount;
pid_t* process_id;  // Lista con los identificadores de los hijos
sem_t** semaphores;  // Lista con los semaforos
char** sem_names;  // Lista con los nombres de los semaforos
int** child_status;  // Lista con el estado de los hijos
int** child_write_enable;  // Lista con el write enable de los hijos

void createProcesses(int count);
void processFunction(char* semaphore_name, int* status, int* write_enable);
void attendRequest(int clientSocket, int id);
void createFolder();

int main(int argc, char **argv) {
    // Verificacion numero de argumentos
    if(argc != 2) {
        printf("Debe ingresar la cantidad de procesos como argumento\n");
        exit(1);
    }
    processCount = atoi(argv[1]);
    createProcesses(processCount);

    for (int i = 0; i < processCount; i++) {
        printf("%d\n", process_id[i]);
    }
    /*return 0;

    // Creacion de directorios
    createFolder();
    printf("%s\n", folderpath);

    // Creacion del descriptor del socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    
    // Configuracion de direccion y puerto del servidor
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080); // Puerto
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Direccion IP
    
    // Se asigna el puerto al socket
    bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

    // Escucha de conexiones entrantes
    listen(serverSocket, __INT_MAX__);

    // Manejo de las consultas de los clientes
    while (1) {
        // Estructura para obtener la informacion del cliente
        struct sockaddr_in clientAddr;
        unsigned int sin_size = sizeof(clientAddr);

        // Se espera por una conexion con un cliente
        int clientSocket = accept(serverSocket, (struct sockaddr *) &clientAddr, &sin_size);

        int pid = 1;//fork();
        if(pid == 0) attendRequest(clientSocket, processCount); //Child process
        //printf("%d solicitudes recibidas!\n", processCount);
    }

    // Cerrar la conexion
    shutdown(serverSocket, SHUT_RDWR);*/
    
    // Destruir semaforos
    for (int i = 0; i < processCount; i++) {
        sem_unlink(sem_names[i]);
        free(sem_names[i]);
        //waitpid(process_id[i], NULL, 0);
        kill(process_id[i], SIGKILL);
    }
    printf("Hijos terminados:)\n");

    free(sem_names);
    free(process_id);
    free(semaphores);
    free(folderpath);

    return 0;
}

void createProcesses(int count) {
    // Solicitud de espacio para almacenar en memoria las listas de control
    process_id = (pid_t*) malloc(sizeof(pid_t)*count); 
    sem_names = (char**) malloc(sizeof(char*)*count); 
    semaphores = (sem_t**) malloc(sizeof(sem_t*)*count);  
    child_status = (int**) malloc(sizeof(int*)*count); 
    child_write_enable = (int**) malloc(sizeof(int*)*count); 


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
        *child_status[i] = 0;
        // Write enable del proceso hijo
        child_write_enable[i] = (int*) mmap(NULL, sizeof(int*), protection, visibility, -1, 0);
        *child_write_enable[i] = 0;

        // Creacion del proceso hijo
        process_id[i] = fork();

        // Apertura del semaforo
        semaphores[i] = sem_open(sem_names[i], O_CREAT | O_EXCL, SEM_PERMS, 0);

        if(process_id[i] == 0) {  // Proceso hijo
            processFunction(sem_names[i], child_status[i], child_write_enable[i]);
            //exit(0); //Child process
        }
    }
    sleep(2);
    /*
    for (int j = 0; j < 2; j++)
    {
        for (int i = 0; i < count; i++)
        {
            sem_post(semaphores[i]);
        }
        sleep(1);
    }
    */


}

void processFunction(char* semaphore_name, int* status, int* write_enable){
    // Limpieza de memoria
    free(process_id);
    free(sem_names);
    free(semaphores);
    free(child_status);
    free(child_write_enable);

    // Apertura del semaforo para sincronizacion con el proceso padre
    sem_t *semaphore = sem_open(semaphore_name, O_RDWR);
    if (semaphore == SEM_FAILED) {
        perror("sem_open(3) failed");
        exit(EXIT_FAILURE);
    }

    /*
    sleep(15);
    sem_wait(semaphore);
    printf("He revivido!\n");
    sem_wait(semaphore);
    printf("He revivido 2 veces!\n");
    sem_unlink(semaphore_name);
    //exit(0);
    */
    printf("%d\n", *write_enable);
    exit(0);

}

/**
 * Funcion que se encarga de atender la solicitud del cliente
 * clientSocket: identificador del socket del cliente
 * id: identificador del proceso que atiende la solicutd
*/
void attendRequest(int clientSocket, int id) {
    unsigned char* buffer = (char*) malloc(sizeof(unsigned char)*BUFFER_SIZE);
    
    // Limpieza del buffer
    memset(buffer, 0, sizeof(unsigned char)*BUFFER_SIZE);
    
    // Se espera por el mensaje de inicio
    recv(clientSocket, buffer, BUFFER_SIZE, 0);

    // Recepcion del archivo
    Image* image = receiveImage(clientSocket);

    // Procesamiento de la imagen
    Image filtered = sobel_filter(*image);

    // Guardado de la imagen
    if (id <= 100) {
        char* s_id = int2str(id);
        char* filename = concat(s_id,".png");
        char* filepath = concat(folderpath, filename);
        writeImage(filepath, filtered);

        free(s_id);
        free(filename);
        free(filepath);
    }
    
    // Limpieza de memoria
    free(image->data);
    free(filtered.data);
    free(image);
    free(buffer);

    // Se cierra la conexion
    shutdown(clientSocket, SHUT_RDWR);
    exit(0);
}

/**
 * Funcion para crear los directorios del servidor
*/
void createFolder() {
    // Creacion carpeta raiz de los servidor Heavy Process
    createDirectory("pre_heavy_process");

    char *sCounter = NULL;
    char *name = NULL;
    int created = -1; 
    int counter = 0;

    // Se determina el numero de contenedor correspondiente
    while (created != 0){
        counter++;
        sCounter = int2str(counter);
        name = concat("pre_heavy_process/server", sCounter);
        created = createDirectory(name);
    }
    // Se actualiza la variable global
    folderpath = concat(name, "/");
    free(name);
}
