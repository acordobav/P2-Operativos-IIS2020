#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "read_image.c"
#include "constants.c"

int isPNG(char* filename);
int sendImage(Image image, int socket);
char* int2str(int number);
char* concat(const char *s1, const char *s2);

int main(int argc, char **argv) {
    if(argc != 4) {
        printf("Debe ingresar ip, puerto, imagen como argumentos\n");
        return 0;
    }
    // Direccion IP del servidor
    char* serverIP = argv[1];

    // Puerto del servidor
    char* serverPort = argv[2];
    int port = atoi(serverPort);

    // Ruta de la imagen
    char* filepath = argv[3];
    if(!isPNG(filepath)) {
        printf("Imagen debe tener formato png\n");
        return -1;
    }

    // Lectura de la imagen
    Image image = readImage(filepath); 

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
        return -1;
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
    return 0;

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

/**
 * Funcion para obtener un string a partir de un numero
 * number: numero que se desea convertir a string
 * return: string que representa al numero
*/
char* int2str(int number) {
    int n = snprintf(NULL, 0, "%d", number);
    char* sNumber = malloc(n+1);
    snprintf(sNumber, n+1, "%d", number);
    return sNumber;
}

/**
 * Funcion para concatenar un string s2 al final de un string s1
 * s1: string que se desea concatenar
 * s2: string que se desea concatenar
 * return: string con la concatenacion de s1 y s2
*/ 
char* concat(const char *s1, const char *s2) {
    const size_t len1 = strlen(s1); // Largo de s1
    const size_t len2 = strlen(s2); // Largo de s2
    char *result = malloc(len1 + len2 + 1); // +1 para el null-terminator
    memcpy(result, s1, len1);
    memcpy(result + len1, s2, len2 + 1);
    return result;
}