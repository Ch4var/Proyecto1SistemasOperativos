#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdint.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#include "heap3.c"

#define MAX_UNICODE 0x10FFFF // Definir el máximo valor Unicode
#define BUFFER_SIZE 512  // Tamaño del buffer para escritura
#define INITIAL_BUFFER_SIZE 1024*1024
#define BUFFER_INCREMENT 1024*1024

struct timespec start, end;
long seconds, nanoseconds;
double elapsed;

int bit_buffer, bit_count = 0;

void guardar_codigos(struct Node *root, char *codigo_actual, int profundidad, FILE *archivo) {
    // Caso base: si el nodo es una hoja, guardamos el valor y el código
    if (isLeaf(root)) {
        // Escribir el carácter (uint32_t) y su longitud de código
        fwrite(&root->value, sizeof(uint32_t), 1, archivo);
        
        // Escribir la longitud del código
        uint8_t longitud_codigo = profundidad;
        fwrite(&longitud_codigo, sizeof(uint8_t), 1, archivo);
        
        // Escribir el código
        fwrite(codigo_actual, sizeof(char), profundidad, archivo);
        return;
    }

    // Si no es una hoja, seguir el recorrido del árbol
    // Añadir '0' al código actual para la izquierda
    if (root->left) {
        codigo_actual[profundidad] = '0';
        guardar_codigos(root->left, codigo_actual, profundidad + 1, archivo);
    }

    // Añadir '1' al código actual para la derecha
    if (root->right) {
        codigo_actual[profundidad] = '1';
        guardar_codigos(root->right, codigo_actual, profundidad + 1, archivo);
    }
}

// Función para guardar el árbol y añadir un separador al final
void serializar_arbol_con_separador(struct Node *root, FILE *archivo) {
    char codigo_actual[MAX_TREE_HT];
    guardar_codigos(root, codigo_actual, 0, archivo);

    // Añadir un separador al final del árbol
    uint32_t separador = 0xFFFFFFFF; // Delimitador que indica el final del árbol
    fwrite(&separador, sizeof(uint32_t), 1, archivo);
}

void escribir_bits_a_buffer(char **buffer, int *buffer_size, int *buffer_capacity, const char *codigo, int *bit_count, int *bit_buffer) {
    while (*codigo) {
        // Añade el bit al buffer
        *bit_buffer = (*bit_buffer << 1) | (*codigo - '0');
        (*bit_count)++;
        
        // Si ya tenemos 8 bits, escribimos un byte en el buffer
        if (*bit_count == 8) {
            if (*buffer_size >= *buffer_capacity) {
                *buffer_capacity += BUFFER_INCREMENT;
                *buffer = realloc(*buffer, *buffer_capacity);
                if (!*buffer) {
                    perror("Error al reasignar memoria");
                    exit(EXIT_FAILURE);
                }
            }
            (*buffer)[*buffer_size] = *bit_buffer;
            (*buffer_size)++;
            *bit_count = 0;
            *bit_buffer = 0;
        }
        codigo++;
    }   
}

int comprimir_a_buffer(FILE *entrada, struct Node *root, char **buffer, int *caracteres_comprimidos) {
    int bit_buffer = 0, bit_count = 0;
    int buffer_size = 0;
    int buffer_capacity = INITIAL_BUFFER_SIZE;

    // Asignar memoria inicial
    *buffer = malloc(buffer_capacity);
    if (!*buffer) {
        perror("Error al asignar memoria");
        exit(EXIT_FAILURE);
    }

    *caracteres_comprimidos = 0;
    int ch;
    while ((ch = fgetc(entrada)) != EOF) {
        char codigo[256];
        char *resultado = getCode(root, codigo, 0, ch); 
        if (resultado != NULL) {
            escribir_bits_a_buffer(buffer, &buffer_size, &buffer_capacity, resultado, &bit_count, &bit_buffer);
            (*caracteres_comprimidos)++;
        }
    }

    // Escribir cualquier bit restante en el buffer
    if (bit_count > 0) {
        bit_buffer <<= (8 - bit_count);
        if (buffer_size >= buffer_capacity) {
            buffer_capacity *= 2;
            *buffer = realloc(*buffer, buffer_capacity);
            if (!*buffer) {
                perror("Error al reasignar memoria");
                exit(EXIT_FAILURE);
            }
        }
        (*buffer)[buffer_size] = bit_buffer;
        buffer_size++;
    }
    
    return buffer_size;  // Retorna el tamaño del buffer comprimido
}

void guardar_archivo_comprimido(FILE *salida, const char *nombreArchivo, char *contenido_comprimido, int tamano_comprimido, int caracteres_comprimidos) {
    // Escribir el largo del nombre del archivo
    int largo_nombre = strlen(nombreArchivo);
    fwrite(&largo_nombre, sizeof(int), 1, salida);

    // Escribir el nombre del archivo
    fwrite(nombreArchivo, sizeof(char), largo_nombre, salida);

    // Escribir el largo del archivo comprimido
    fwrite(&tamano_comprimido, sizeof(int), 1, salida);

    // Escribir la cantidad de caracteres
    fwrite(&caracteres_comprimidos, sizeof(int), 1, salida);

    // Escribir el contenido del archivo comprimido
    fwrite(contenido_comprimido, sizeof(char), tamano_comprimido, salida);
}

void comprimir_archivo(FILE *entrada, FILE *salida, struct Node *root, const char *nombreArchivo) {
    // Buffer dinámico para almacenar el archivo comprimido
    char *buffer_comprimido = NULL;

    // Comprimir el archivo en el buffer
    int caracteres_comprimidos;
    int tamano_comprimido = comprimir_a_buffer(entrada, root, &buffer_comprimido, &caracteres_comprimidos);

    // Guardar el nombre, tamaño comprimido, y contenido comprimido en el archivo de salida
    guardar_archivo_comprimido(salida, nombreArchivo, buffer_comprimido, tamano_comprimido, caracteres_comprimidos);

    // Liberar el buffer comprimido
    free(buffer_comprimido);
}


int main() {
    setlocale(LC_ALL, ""); 

    clock_gettime(CLOCK_MONOTONIC, &start);

    char carpeta[] = "libros";
    struct dirent *entrada;
    DIR *directorio = opendir(carpeta);
    if (!directorio) {
        perror("No se pudo abrir el directorio");
        return 1;
    }

    int *frecuencias = calloc(MAX_UNICODE + 1, sizeof(int));
    if (!frecuencias) {
        perror("Error al asignar memoria");
        closedir(directorio);
        return 1;
    }

    // Contar las frecuencias
    while ((entrada = readdir(directorio)) != NULL) {
        if (entrada->d_type == DT_REG) { // Si es un archivo regular
            char ruta_completa[1024];
            snprintf(ruta_completa, sizeof(ruta_completa), "%s/%s", carpeta, entrada->d_name);
            contar_frecuencias(ruta_completa, frecuencias); // Función definida por ti
        }
    }

    closedir(directorio);

    // Contar cuántos caracteres tienen frecuencias mayores que 0
    int num_caracteres = 0;
    for (int i = 0; i <= MAX_UNICODE; i++) {
        if (frecuencias[i] > 0) {
            num_caracteres++;
        }
    }

    // Crear un array de estructuras para ordenar por frecuencia
    FrecuenciaCaracter *frecuencia_array = malloc(num_caracteres * sizeof(FrecuenciaCaracter));
    if (!frecuencia_array) {
        perror("Error al asignar memoria");
        free(frecuencias);
        return 1;
    }

    int j = 0;
    for (int i = 0; i <= MAX_UNICODE; i++) {
        if (frecuencias[i] > 0) {
            frecuencia_array[j].unicode_char = i;
            frecuencia_array[j].frecuencia = frecuencias[i];
            j++;
        }
    }

    // Ordenar por frecuencia
    qsort(frecuencia_array, num_caracteres, sizeof(FrecuenciaCaracter), comparar_frecuencias); // Función definida por ti

    uint32_t arr[num_caracteres];
    int freq[num_caracteres];
    for (int i = 0; i < num_caracteres; i++) {
        arr[i] = frecuencia_array[i].unicode_char;
        freq[i] = frecuencia_array[i].frecuencia;
    }

    int size = num_caracteres;

    // Generar los códigos de Huffman
    struct Node *root = HuffmanCodes(arr, freq, size);  // Definir HuffmanCodes

    free(frecuencia_array);
    free(frecuencias);

    // Comprimir archivos
    FILE *archivoComprimido = fopen("comprimido.bin", "wb");
    if (!archivoComprimido) {
        perror("Error al abrir el archivo de salida");
        return 1;
    }

    serializar_arbol_con_separador(root, archivoComprimido);

    directorio = opendir(carpeta);
    if (!directorio) {
        perror("No se pudo abrir el directorio");
        fclose(archivoComprimido);
        return 1;
    }

    while ((entrada = readdir(directorio)) != NULL) {
        if (entrada->d_type == DT_REG) { // Solo archivos regulares
            char rutaCompleta[1024];
            snprintf(rutaCompleta, sizeof(rutaCompleta), "%s/%s", carpeta, entrada->d_name);

            FILE *archivoEntrada = fopen(rutaCompleta, "r");
            if (archivoEntrada) {
                // Comprimir el contenido del archivo
                comprimir_archivo(archivoEntrada, archivoComprimido, root, entrada->d_name);
                fclose(archivoEntrada);
            }
        }
    }


    fclose(archivoComprimido);
    closedir(directorio); 

    clock_gettime(CLOCK_MONOTONIC, &end);

    seconds = end.tv_sec - start.tv_sec;
    nanoseconds = end.tv_nsec - start.tv_nsec;

    if (nanoseconds < 0) {
        seconds--;
        nanoseconds += 1000000000;
    }

    elapsed = seconds + nanoseconds * 1e-9;

    printf("Tiempo transcurrido: %.9f segundos\n", elapsed);

    free(root);

    return 0;
}