#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdint.h>
#include <string.h>
#include <locale.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>
#include "heap.c"

#define MAX_UNICODE 0x10FFFF
#define BUFFER_SIZE 512
#define INITIAL_BUFFER_SIZE 1024*1024
#define MAX_FILES 100
#define MAX_THREADS 10

struct timespec start, end;
long seconds, nanoseconds;
double elapsed;
int bit_buffer, bit_count = 0;
int size;

//Estructura para los datos del hilo
typedef struct {
    char **files;
    int start;
    int end;
    char *carpeta;
    char *archivoTemporal;
    Code *codes;
} ThreadArgs;

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
                *buffer_capacity *= 2;
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

int comprimir_a_buffer(FILE *entrada, Code *codes, char **buffer, int *caracteres_comprimidos) {
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
    char character[5];
    while ((ch = fgetc(entrada)) != EOF) {
        // Convertir el carácter leído a UTF-8 (usamos un solo carácter en este caso)
        
        //unicode_to_utf8(ch, character);

        snprintf(character, sizeof(character), "%c", ch);

        char *resultado = NULL;
        for (int i = 0; i < size; i++) {
            if (strcmp(codes[i].utf8_char, character) == 0) {
                resultado = codes[i].code;
                break;  // Salir del bucle si encontramos el carácter
            }
        }

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

void comprimir_archivo(FILE *entrada, FILE *salida, Code *codes, const char *nombreArchivo) {    
    // Buffer dinámico para almacenar el archivo comprimido
    char *buffer_comprimido = NULL;

    // Comprimir el archivo en el buffer
    int caracteres_comprimidos;
    int tamano_comprimido = comprimir_a_buffer(entrada, codes, &buffer_comprimido, &caracteres_comprimidos);
    
    // Guardar el nombre, tamaño comprimido, y contenido comprimido en el archivo de salida
    guardar_archivo_comprimido(salida, nombreArchivo, buffer_comprimido, tamano_comprimido, caracteres_comprimidos);

    // Liberar el buffer comprimido
    free(buffer_comprimido);
}

int compareCodes(const void *a, const void *b) {
    const Code *codeA = (const Code *)a;
    const Code *codeB = (const Code *)b;
    
    // Comparar las longitudes de los códigos
    return strlen(codeA->code) - strlen(codeB->code);
}

void *comprimir_archivos_en_rango(void *args) {
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    char **files = threadArgs->files;
    int start = threadArgs->start;
    int end = threadArgs->end;
    char *carpeta = threadArgs->carpeta;
    char *archivoTemporal = threadArgs->archivoTemporal;
    Code *codes = threadArgs->codes;

    FILE *archivoComprimido = fopen(archivoTemporal, "wb");
    if (!archivoComprimido) {
        perror("Error al abrir archivo temporal");
        pthread_exit(NULL);
    }

    for (int i = start; i < end; i++) {
        char rutaCompleta[1024];
        snprintf(rutaCompleta, sizeof(rutaCompleta), "%s/%s", carpeta, files[i]);

        FILE *archivoEntrada = fopen(rutaCompleta, "rb"); // Abre en modo binario
        if (archivoEntrada) {
            comprimir_archivo(archivoEntrada, archivoComprimido, codes, files[i]);
            fclose(archivoEntrada);
        } else {
            perror("Error al abrir archivo de entrada");
        }
    }

    fclose(archivoComprimido);
    pthread_exit(NULL);
}


int main() {
    clock_gettime(CLOCK_MONOTONIC, &start);
    setlocale(LC_ALL, "");

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

    char *file_list[MAX_FILES];
    int file_count = 0;

    while ((entrada = readdir(directorio)) != NULL) {
        if (entrada->d_type == DT_REG) {
            char ruta_completa[1024];
            snprintf(ruta_completa, sizeof(ruta_completa), "%s/%s", carpeta, entrada->d_name);
            contar_frecuencias(ruta_completa, frecuencias);
            file_list[file_count] = strdup(entrada->d_name);
            file_count++;
        }
    }

    closedir(directorio);

    int num_caracteres = 0;
    for (int i = 0; i <= MAX_UNICODE; i++) {
        if (frecuencias[i] > 0) {
            num_caracteres++;
        }
    }

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

    qsort(frecuencia_array, num_caracteres, sizeof(FrecuenciaCaracter), comparar_frecuencias);

    uint32_t arr[num_caracteres];
    int freq[num_caracteres];
    for (int i = 0; i < num_caracteres; i++) {
        arr[i] = frecuencia_array[i].unicode_char;
        freq[i] = frecuencia_array[i].frecuencia;
    }

    size = num_caracteres;

    struct Node *root = HuffmanCodes(arr, freq, size);

    free(frecuencia_array);
    free(frecuencias);

    Code *codes = malloc(num_caracteres * sizeof(Code));
    char temparr[256];
    int index = 0;
    getAllCodes(root, temparr, 0, codes, &index);

    qsort(codes, num_caracteres, sizeof(Code), compareCodes);

    FILE *archivoComprimido = fopen("comprimido.bin", "wb");
    if (!archivoComprimido) {
        perror("Error al abrir el archivo de salida");
        return 1;
    }

    serializar_arbol_con_separador(root, archivoComprimido);
    fclose(archivoComprimido);

    int archivos_por_hilo = file_count / MAX_THREADS;
    pthread_t threads[MAX_THREADS];
    ThreadArgs threadArgs[MAX_THREADS];

    for (int i = 0; i < MAX_THREADS; i++) {
        int start = i * archivos_por_hilo;
        int end = (i == MAX_THREADS - 1) ? file_count : (i + 1) * archivos_por_hilo;

        char archivoTemporal[256];
        snprintf(archivoTemporal, sizeof(archivoTemporal), "temp_comprimido_%d.bin", i);

        threadArgs[i].files = file_list;
        threadArgs[i].start = start;
        threadArgs[i].end = end;
        threadArgs[i].carpeta = carpeta;
        threadArgs[i].archivoTemporal = strdup(archivoTemporal);
        threadArgs[i].codes = codes;

        pthread_create(&threads[i], NULL, comprimir_archivos_en_rango, &threadArgs[i]);
    }

    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
        free(threadArgs[i].archivoTemporal);
    }

    archivoComprimido = fopen("comprimido.bin", "ab");
    if (!archivoComprimido) {
        perror("Error al abrir el archivo comprimido final");
        return 1;
    }

    for (int i = 0; i < MAX_THREADS; i++) {
        char archivoTemporal[256];
        snprintf(archivoTemporal, sizeof(archivoTemporal), "temp_comprimido_%d.bin", i);

        FILE *archivoTemporalPtr = fopen(archivoTemporal, "rb");
        if (archivoTemporalPtr) {
            char buffer[BUFFER_SIZE];
            size_t bytesLeidos;
            while ((bytesLeidos = fread(buffer, 1, BUFFER_SIZE, archivoTemporalPtr)) > 0) {
                fwrite(buffer, 1, bytesLeidos, archivoComprimido);
            }
            fclose(archivoTemporalPtr);
            remove(archivoTemporal);
        }
    }

    fclose(archivoComprimido);

    for (int i = 0; i < file_count; i++) {
        free(file_list[i]);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    seconds = end.tv_sec - start.tv_sec;
    nanoseconds = end.tv_nsec - start.tv_nsec;

    if (nanoseconds < 0) {
        seconds--;
        nanoseconds += 1000000000;
    }

    elapsed = seconds + nanoseconds * 1e-9;

    printf("Tiempo transcurrido: %.9f segundos\n", elapsed);

    free(codes);
    free(root);

    return 0;
}
