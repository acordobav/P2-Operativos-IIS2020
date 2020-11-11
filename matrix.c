#include <stdio.h>
#include <stdlib.h>

/**
 * Funcion para crear una matriz de C, asignandole espacio en memoria
 * rows: numero de la cantidad de filas
 * cols: numero de la cantidad de columnas
 * return: puntero a la matriz
*/
int** createMatrix(int rows, int cols) {
    int len = sizeof(int *) * rows + sizeof(int) * cols * rows; 
    int **matrix = (int **)malloc(len); 
    int *ptr = (int *)(matrix + rows);

    // Correccion puntero de las filas a la direccion correcta
    for(int i = 0; i < rows; i++) 
        matrix[i] = (ptr + cols * i); 

    // Inicializacion en ceros
    for(int i = 0; i < rows; i++)
        for(int j = 0; j < cols; j++)
            matrix[i][j] = 0;
    
    return matrix;
}