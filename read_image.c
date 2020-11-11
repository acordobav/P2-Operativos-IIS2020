#include <stdio.h>
#include <string.h>
#include <png.h>
#include <stdlib.h>

#include "matrix.c"
#include "image.c"

/**
 * Funcion leer una imagen png
 * filepath: ruta de la imagen
 * return: matriz de c con la representacion de la imagen
*/ 
Image readImage(char* filepath) {
    FILE *pFile = fopen(filepath, "rb");
    if(!pFile) {
        printf("Error al leer el archivo %s\n", filepath);
        exit(1);
    }
    // Se leen los primeros 8 bits del archivo para
    // verificar que sea una imagen en formato png
    int len = 8; // Largo del buffer
    char header[len]; // Buffer
    fread(header, 1, len, pFile); // Lectura de los primeros 8 bits
    int is_png = !png_sig_cmp(header, 0, len); 
    if (!is_png) {
        printf("Archivo %s no es una imagen en formato png\n", filepath);
        fclose(pFile);
        remove(filepath); // Se elimina el archivo de la carpeta de procesamineto
        exit(1);
    }

    // Creacion de la estructura de lectura
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if(setjmp(png_jmpbuf(png_ptr))) {
        printf("Error al obtener la informacion del archivo %s\n", filepath);
        fclose(pFile);
        exit(1);
    }

    // Lectura de la informacion de la imagen
    png_init_io(png_ptr, pFile);
    png_set_sig_bytes(png_ptr, len); // Se indica que se han leido 8 bits
    png_read_info(png_ptr, info_ptr);
    int width = png_get_image_width(png_ptr, info_ptr);   // Ancho
    int height = png_get_image_height(png_ptr, info_ptr); // Largo
    int channels = png_get_channels(png_ptr, info_ptr);   // Canales
    int number_of_passes = png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

    // Lectura de los datos
    if (setjmp(png_jmpbuf(png_ptr))) {
        printf("Error durante la lectura de los pixeles\n");
        fclose(pFile);
        exit(1);
    }

    // Memoria para almacenar los pixeles de la imagen
    png_bytep *row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
    for (int i = 0; i < height; i++) {
        row_pointers[i] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));
    }

    int** data = createMatrix(width, height);
    // Lectura de los pixeles
    png_read_image(png_ptr, row_pointers);
    for (int y = 0; y < height; y++) {
        png_byte* row = row_pointers[y];
        for (int x = 0; x < width; x++) {
            png_byte* pixel = &(row[x*channels]);
            // Lectura del pixel, se guarda en la posicion correspondiente en data
            data[x][y] = pixel[0];
        }
    }

    // Creacion del struct que almacena la imagen
    Image image;
    image.data = data;
    image.rows = width;
    image.cols = height;

    // Limpieza de memoria
    fclose(pFile);
    for (int i = 0; i < height; i++) free(row_pointers[i]);
    free(row_pointers);

    return image;
}