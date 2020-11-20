#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

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