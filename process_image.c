#include <stdio.h>
#include <string.h>
#include <png.h>
#include <stdlib.h>

#include "matrix.c"
#include "image.c"

int** conv2(int m1, int n1, int** matrix, int m2, int n2, int kernel[][n2]);
void writeImage(char* filename, Image image);
Image sobel_filter(Image image);

/**
 * Funcion para aplicar la convolucion entre una matriz y un kernel
 * mRows: numero de filas de la matriz
 * mCols: numero de columnas de la matriz
 * matrix: matriz que se quiere convolucionar
 * kRows: numero de filas del kernel
 * kCols: numero de columnas del kernel
 * kernel: matriz del kernel
*/
int** conv2(int mRows, int mCols, int** matrix, int kRows, int kCols, int kernel[][kCols]) {
    int rows = mRows+kRows-1; // Numero de filas del resultado
    int cols = mCols+kCols-1; // Numero de columnas del resultado
    int** result = createMatrix(rows, cols); // Matriz del resultado

    // Inicio primera sumatoria
    for(int j = 1; j <= rows; j++) {
        // p inicial
        int p_in = j-kRows+1;
        if(1 > p_in) p_in = 1; 
    
        // p final
        int p_fin = j;
        if(mRows < p_fin) p_fin = mRows;

        for(int p = p_in; p <= p_fin; p++) {
            // Inicio segunda sumatoria
            for(int k = 1; k <= cols; k++) {
                // q inicial
                int q_in = k-kCols+1;
                if(1 > q_in) q_in = 1;

                // q final
                int q_fin = k;
                if(mCols < q_fin) q_fin = mCols;

                // Calculo de la convolucion
                for(int q = q_in; q <= q_fin; q++) {
                    result[j-1][k-1] = result[j-1][k-1] + (matrix[p-1][q-1] * kernel[j-p][k-q]);
                }
            }
        }
    }
    return result;
}

/**
 * Funcion que aplica el filtro Sobel para detectar bordes verticales
 * image: struct de tipo Image con la imagen a filtrar
 * return: struct de tipo Image con la imagen filtrada
*/
Image sobel_filter(Image image) {
    // Kernel bordes verticales
    int kernel[3][3] = {
        {-1, -2, -1},
        { 0,  0,  0},
        { 1,  2,  1}
    };

    // Convolucion con el kernel de Sobel
    int** conv2Result = conv2(image.rows, image.cols, image.data, 3, 3, kernel);
    
    // Creacion del struct de la nueva imagen 
    Image filtered;
    filtered.rows = image.rows-2;
    filtered.cols = image.cols-2;
    filtered.data = createMatrix(image.rows-2, image.cols-2);

    // Recorre la imagen para copiar los valores de los pixeles centrales
    for (int i = 1; i < image.rows-1; i++) {
        for (int j = 1; j < image.cols-1; j++) {
            int value = conv2Result[i][j];

            // Truncamiento de los valores
            if(value < 0) value = 0;
            if(value > 255) value = 255;
            filtered.data[i-1][j-1] = value;
        }
    }

    free(conv2Result);
    return filtered;
}

/**
 * Funcion que escribe una imagen en un archivo .png
 * image: struct de tipo Image con la informacion de la imagen a escribir
*/
void writeImage(char* filename, Image image) {
	FILE *fp = NULL;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	png_bytep row = NULL;
	
	// Apertura del archivo para escritura
	fp = fopen(filename, "wb");
	if (fp == NULL) {
		fprintf(stderr, "Error al crear el archivo %s para escritura\n", filename);
		exit(1);
	}

	// Inicializacion de estructura de escritura
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		fprintf(stderr, "Could not allocate write struct\n");
		exit(1);
	}

	// Initialize info structure
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		fprintf(stderr, "Could not allocate info struct\n");
		exit(1);
	}

	// Setup Exception handling
	if (setjmp(png_jmpbuf(png_ptr))) {
		fprintf(stderr, "Error during png creation\n");
		exit(1);
	}

	png_init_io(png_ptr, fp);

	// Write header (8 bit colour depth)
	png_set_IHDR(png_ptr, info_ptr, image.rows, image.cols,
			8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_write_info(png_ptr, info_ptr);

	// Allocate memory for one row (1 byte per pixel - RGB)
	row = (png_bytep) malloc(image.rows * sizeof(png_byte));

	// Write image data
	for (int y = 0 ; y < image.cols; y++) {
		for (int x = 0; x < image.rows; x++) {
			png_byte* pixel = &(row[x]);
            *pixel = image.data[x][y];
		}
		png_write_row(png_ptr, row);
	}

	// End write
	png_write_end(png_ptr, NULL);

    // Limpieza de memoria
	fclose(fp);
	png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
	png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	free(row);
}