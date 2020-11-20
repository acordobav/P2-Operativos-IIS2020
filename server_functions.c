#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>

#include "receive_image.c"
#include "general_functions.c"

int receiveRequestsNumber(int serverSocket) {
    // Estructura para obtener la informacion del cliente
    struct sockaddr_in clientAddr;
    unsigned int sin_size = sizeof(clientAddr);

    // Se espera por una conexion con un cliente
    int clientSocket = accept(serverSocket, (struct sockaddr *) &clientAddr, &sin_size);

    // Creacion y limpieza del buffer
    unsigned char* buffer = (char*) malloc(sizeof(unsigned char)*BUFFER_SIZE);
    memset(buffer, 0, sizeof(unsigned char)*BUFFER_SIZE);

    // Se espera por el mensaje de inicio
    recv(clientSocket, buffer, BUFFER_SIZE, 0);

    return atoi(buffer);
}

/**
 * Funcion que se encarga de atender la solicitud del cliente
 * clientSocket: identificador del socket del cliente
 * id: identificador del proceso que atiende la solicutd
*/
void attendRequest(int clientSocket, int id, char* folderpath) {
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
}

/**
 * Funcion para crear un directorio
 * filepath: ruta con la direccion y nombre de la carpeta
 * return: 0 en caso exitoso, -1 si hubo un error
*/ 
int createDirectory(char* filepath) {
    return mkdir(filepath, 0777);
}

/**
 * Funcion para crear las carpetas del servidor
*/
char* createFolder(char* server_type) {
    // Creacion carpeta raiz de los servidor Heavy Process
    createDirectory(server_type);

    char *sCounter = NULL;
    char *name = NULL;
    int created = -1; 
    int counter = 0;
    char* fname = concat(server_type, "/server");

    // Se determina el numero de contenedor correspondiente
    while (created != 0){
        counter++;
        sCounter = int2str(counter);
        name = concat(fname, sCounter);
        created = createDirectory(name);
    }
    // Se actualiza la variable global
    char* folderpath = concat(name, "/");
    free(name);
    free(fname);
    return folderpath;
}