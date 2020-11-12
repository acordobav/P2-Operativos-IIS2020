#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>

#include "constants.c"
#include "process_image.c"

/**
 * Funcion para recibir una imagen proveniente de un socket cliente
 * socket: descriptor del socket cliente
 * return: puntero de la imagen enviada por el cliente
*/ 
Image* receiveImage(int socket) {
    // Mensaje del cliente con el nombre y dimension del archivo
    unsigned char clientMessage[BUFFER_SIZE];
    recv(socket, &clientMessage, BUFFER_SIZE, 0);

    // Extraccion de la informacion de la imagen
    char *rows = strtok(clientMessage, "*");
    char *cols = strtok(NULL, "*");
    char *size = strtok(NULL, "*");

    // Solicitar memoria para guardar la imagen recibida
    int** data = malloc(atoi(size));
    int offset = 0;

    // Calculo aproximado de iteraciones necesarias
    int iter = (atoi(size)/BUFFER_SIZE);
    int iter_max = iter + 5; // Para asegurar la recepcion completa

    // Creacion del buffer para leer el mensaje enviado por el socket cliente
    unsigned char* buffer = (char*) malloc(sizeof(unsigned char)*BUFFER_SIZE);
    
    // Recepcion del archivo
    for (int i = 0; i < iter_max; i++) {
        // Limpieza del buffer
        memset(buffer, 0, sizeof(unsigned char)*BUFFER_SIZE);

        // Lectura del mensaje entrante
        int receivedBytes = recv(socket, buffer, BUFFER_SIZE, 0); 

        if (receivedBytes == 0) { // Conexion perdida
            free(buffer);
            exit(1);
        }

        // Se verifica si ha finalizado el envio del archivo
        if (strcmp(END_MSG, buffer) == 0) break;
        
        // Se verifica si se recibio el mensaje correctamente (bytes recibidos = BUFFER_SIZE)
        if(receivedBytes != BUFFER_SIZE && i < iter) {
            int completed = 0;
            while (!completed) {
                // Envio del mensaje indicando una recepcion incompleta
                int s = send(socket, INCOMPLETE_MSG, BUFFER_SIZE, 0);

                if (s == 0) { // Conexion perdida
                    free(buffer);
                    printf("Error en la recepcion del archivo\n");
                    exit(1);
                }
                // Se limpia el buffer
                memset(buffer, 0, sizeof(unsigned char)*BUFFER_SIZE);

                // Se recibe de nuevo el mensaje
                receivedBytes = recv(socket, buffer, BUFFER_SIZE, 0);

                // Se verifica si el nuevo mensaje se recibio correctamente
                if (receivedBytes == BUFFER_SIZE) {
                    completed = 1;
                    memcpy((void*)data+offset, (void*)buffer, receivedBytes);
                    send(socket, COMPLETE_MSG, BUFFER_SIZE, 0);
                }
            }
        } else {
            // Se almacena la informacion recibida
            memcpy((void*)data+offset, (void*)buffer, receivedBytes);
            send(socket, COMPLETE_MSG, BUFFER_SIZE, 0);
        }
        offset += BUFFER_SIZE;
    }
    // Limpieza de memoria
    free(buffer);
    
    Image *image = malloc(sizeof(image));
    image->rows = atoi(rows);
    image->cols = atoi(cols);
    image->data = data;

    // Correccion punteros de las filas a la direccion correcta
    int *ptr = (int *)(data + image->rows);
    for(int i = 0; i < image->rows; i++) 
        data[i] = (ptr + image->cols * i); 

    return image;
}