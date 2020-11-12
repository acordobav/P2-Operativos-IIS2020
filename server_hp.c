#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "receive_image.c"
#include "general_functions.c"

char* folderpath;

void attendRequest(int clientSocket, int id);
void createFolder();

int main() {
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

    int processCount = 0;
    // Manejo de las consultas de los clientes
    while (1) {
        // Estructura para obtener la informacion del cliente
        struct sockaddr_in clientAddr;
        unsigned int sin_size = sizeof(clientAddr);

        // Se espera por una conexion con un cliente
        int clientSocket = accept(serverSocket, (struct sockaddr *) &clientAddr, &sin_size);
        //char *ipClient = inet_ntoa(clientAddr.sin_addr);

        processCount++;
        if(fork() == 0) attendRequest(clientSocket, processCount); //Child process
        printf("%d solicitudes recibidas!\n", processCount);
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
 * Funcion para crear un directorio
 * filepath: ruta con la direccion y nombre de la carpeta
 * return: 0 en caso exitoso, -1 si hubo un error
*/ 
int createDirectory(char* filepath) {
    return mkdir(filepath, 0777);
}

/**
 * Funcion para crear el directorios de carpetas del servidor
*/
void createFolder() {
    // Creacion carpeta raiz de los servidor Heavy Process
    createDirectory("heavy_process");

    char *sCounter = NULL;
    char *name = NULL;
    int created = -1; 
    int counter = 0;

    // Se determina el numero de contenedor correspondiente
    while (created != 0){
        counter++;
        sCounter = int2str(counter);
        name = concat("heavy_process/server", sCounter);
        created = createDirectory(name);
    }
    // Se actualiza la variable global
    folderpath = concat(name, "/");
    free(name);
}