#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "receive_image.c"

void listenClient(int serverSocket);

int main() {
    // Creacion de directorios
    //char* dfolderpath = createDataFolders(); // Guardar ruta del folder de almacenamiento
    //printf("Ruta: %s\n\n", dfolderpath);

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

    while (1) {
        listenClient(serverSocket);
    }

    // Cerrar la conexion
    shutdown(serverSocket, SHUT_RDWR);
    //free(dfolderpath);
    return 0;
}

void listenClient(int serverSocket) {
    // Estructura para obtener la informacion del cliente
    struct sockaddr_in clientAddr;
    unsigned int sin_size = sizeof(clientAddr);

    // Se espera por una conexion con un cliente
    int clientSocket = accept(serverSocket, (struct sockaddr *) &clientAddr, &sin_size);
    char *ipClient = inet_ntoa(clientAddr.sin_addr);
    printf("Conexion entrante: %s\n", ipClient);

    unsigned char* buffer = (char*) malloc(sizeof(unsigned char)*BUFFER_SIZE);
    while (1) {
        // Limpieza del buffer
        memset(buffer, 0, sizeof(unsigned char)*BUFFER_SIZE);
        
        // Se espera por el mensaje de inicio
        if (recv(clientSocket, buffer, BUFFER_SIZE, 0) == 0) { 
            // Se perdio la conexion con el cliente
            break;
        }

        // Recepcion del archivo
        Image* image = receiveImage(clientSocket);

        Image filtered = sobel_filter(*image);
        writeImage("filtrada.png", filtered);
        
        free(image->data);
        free(filtered.data);
        free(image);
    }

    printf("Conexion perdida! Esperando nueva conexion...\n");
    free(buffer);

    // Se cierra la conexion
    shutdown(clientSocket, SHUT_RDWR);
}