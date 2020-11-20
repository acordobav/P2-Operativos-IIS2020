#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "server_functions.c"

char* folderpath;

void childFunction(int clientSocket, int id);

int main() {
    // Creacion de directorios
    folderpath = createFolder("heavy_process");
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

        processCount++;
        if(fork() == 0) childFunction(clientSocket, processCount); //Child process
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
void childFunction(int clientSocket, int id) {
    attendRequest(clientSocket, id, folderpath);
    exit(0);
}