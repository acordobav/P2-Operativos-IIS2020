#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "general_functions.c"
#include "read_image.c"
#include "constants.c"

typedef struct Request {
    int ncycles; 
    int port; 
    char* serverIP;
    Image image;
} Request;

int isPNG(char* filename);
int sendImage(Image image, int socket);
char* int2str(int number);
char* concat(const char *s1, const char *s2);
void* sendRequest(void* request);
void* sendRequestsNumber(char* serverIP, int port, int requests);

int main(int argc, char **argv) {
    if(argc != 6) {
        printf("Debe ingresar ip, puerto, imagen, threads y ciclos como argumentos\n");
        return 0;
    }
    // Direccion IP del servidor
    char* serverIP = argv[1];

    // Puerto del servidor
    int port = atoi(argv[2]);

    // Ruta de la imagen
    char* filepath = argv[3];
    if(!isPNG(filepath)) {
        printf("Imagen debe tener formato png\n");
        return -1;
    }

    // Numero de threads
    int nthreads = atoi(argv[4]);

    // Numero de ciclos
    int ncycles = atoi(argv[5]);

    // Enviar cantidad de solicitudes que seran enviadas
    sendRequestsNumber(serverIP, port, nthreads*ncycles);

    // Lectura de la imagen
    Image image = readImage(filepath); 

    // Creacion del struct para pasar argumentos a los hilos
    Request* request = malloc(sizeof(request));
    request->ncycles = ncycles;
    request->port = port;
    request->serverIP = serverIP;
    request->image = image;

    // Creacion de los hilos
    pthread_t threads_id[nthreads];
    for (int i = 0; i < nthreads; i++) {
        pthread_t thread;
        pthread_create(&threads_id[i], NULL, sendRequest, request);
    }

    printf("Todos los hilos creados\n");

    // Espera a que todos los hilos finalicen
    for (int i = 0; i < nthreads; i++)
       pthread_join(threads_id[i], NULL);
        
    printf("Hilos finalizados\n");

    return 0;
}

/**
 * Funcion para enviar una cantidad n de solicitud al servidor
 * requestStruct: struct con los argumentos de la solicitud
*/
void* sendRequest(void* requestStruct){
    // Parametros de la solicitud
    Request* request = (Request*) requestStruct;
    int ncycles = request->ncycles;
    int port = request->port;
    char* serverIP = request->serverIP;
    Image image = request->image;

    for (int i = 0; i < ncycles; i++) {
        // Creacion del socket
        int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        
        // Configuracion de direccion y puerto del cliente
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port); // Puerto
        serverAddr.sin_addr.s_addr = inet_addr(serverIP); // Direccion IP del servidor 

        // Se intenta conectar con el puerto de la direccion ip establecida
        int connectionStatus = connect(clientSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
        
        // Verificar estado de conexion
        if (connectionStatus == -1) {
            printf("Error al intentar conectar con el servidor\n");
            exit(1);
        }

        unsigned char* buffer = (char*) malloc(sizeof(unsigned char)*BUFFER_SIZE);
        while (1) {
            // Limpieza del buffer
            memset(buffer, 0, sizeof(unsigned char)*BUFFER_SIZE);

            // Envio del mensaje de inicio
            send(clientSocket, START_MSG, BUFFER_SIZE, 0);

            // Envio del archivo
            sendImage(image, clientSocket);
            
            // Envio mensaje de finalizacion
            if (!send(clientSocket, END_MSG, sizeof(END_MSG), 0)) {
                printf("Error al enviar mensaje de finalizacion");
                break;
            }
            break;  // BORRAR
        }
        // Se cierra la conexion con el servidor
        shutdown(clientSocket, SHUT_RDWR);
    }
}

/**
 * Funcion para enviar la cantidad de solicitudes 
 * que seran enviadas al servidor
 * requestStruct: struct con los argumentos de la solicitud
*/
void* sendRequestsNumber(char* serverIP, int port, int requests){
    // Creacion del socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    
    // Configuracion de direccion y puerto del cliente
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port); // Puerto
    serverAddr.sin_addr.s_addr = inet_addr(serverIP); // Direccion IP del servidor 

    // Se intenta conectar con el puerto de la direccion ip establecida
    if (connect(clientSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        printf("Error al intentar conectar con el servidor\n");
        exit(1);
    }

    // Envio del mensaje de inicio
    char* s_requests = int2str(requests);
    if (!send(clientSocket, s_requests, BUFFER_SIZE, 0)) {
        printf("Error al enviar mensaje de cantidad de solicitudes");
        exit(1);
    }
    // Se cierra la conexion con el servidor
    shutdown(clientSocket, SHUT_RDWR);
    free(s_requests);
}

/**
 * Funcion para enviar un archivo a un servidor
 * filepath: direccion del archivo que se desea enviar
 * socket: descriptor del socket servidor
 * return: 0 si todo salio bien, -1 en caso contrario
*/ 
int sendImage(Image image, int socket) {
    // Calculo del tamano de la imagen en bytes
    int size = sizeof(int *) * image.rows + sizeof(int) * image.cols * image.rows; 
    char* ssize = int2str(size);

    // Creacion del mensaje que indica datos de la imagen
    char* rows = int2str(image.rows);
    char* cols = int2str(image.cols); 
    char* aux1 = concat(rows, "*");
    char* aux2 = concat(cols, "*");
    char* aux3 = concat(aux1,aux2);
    char* imageData = concat(aux3,ssize);

    // Limpieza de memoria
    free(ssize);
    free(rows);
    free(cols);
    free(aux1);
    free(aux2);
    free(aux3);
    
    // Envio de los datos de la imagen al servidor
    if (!send(socket, imageData, BUFFER_SIZE, 0)) {
        printf("Error al enviar datos del archivo");
        exit(1);
    }
    free(imageData);

    int offset = 0;
    int bsize = BUFFER_SIZE;

    unsigned char* buffer = (char*) malloc(sizeof(unsigned char)*BUFFER_SIZE);
    char* status = (char*) malloc(sizeof(char)*BUFFER_SIZE);
    
    // Envio del archivo al servidor
    while (offset < size) {
        // Limpieza del buffer
        memset(buffer, 0, sizeof(unsigned char)*BUFFER_SIZE);
        memset(status, 0, sizeof(unsigned char)*BUFFER_SIZE);

        // Lectura del archivo
        if((size - offset) < BUFFER_SIZE ) bsize = size - offset;
        memcpy((void*)buffer, (void*)image.data + offset, bsize);

        int completed = 0;

        // while para enviar archivo hasta conseguir respuesta exitosa
        while (!completed) {
            //Envio del archivo
            if (!send(socket, buffer, bsize, 0)){
                printf("Error al enviar chunk del archivo\n");
                free(buffer);
                free(status);
                return -1;
            }
            // Se obtiene la respuesta enviada por el servidor
            recv(socket, status, BUFFER_SIZE, 0);
            if (strcmp(status, COMPLETE_MSG) == 0) completed = 1;
        }
        offset += BUFFER_SIZE;
    }
    // Limpieza de memoria
    free(buffer);
    free(status);

    return 0;
}

/**
 * Funcion para verificar que un archivo tiene terminacion .png
 * filename: string con el nombre del archivo
 * return: 1 si es .png, 0 en caso contrario
*/ 
int isPNG(char* filename) {
    // Se realiza una copia del string original
    char * copy = malloc(strlen(filename) + 1);
    strcpy(copy, filename);

    // Se divide el filename utilizando un punto como delimitador
    char ch[] = ".";
    char * token = strtok(copy, ch);
    while(token != NULL){
        if(strcmp(token, "png") == 0) {
            free(copy);
            return 1;
        }
        token = strtok(NULL, ch);
    }
    free(copy);
    return 0;
}