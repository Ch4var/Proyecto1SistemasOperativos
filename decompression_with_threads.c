#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <locale.h>
#include <time.h>
#include <pthread.h>
#include "heap.c"

#define MAX_THREADS 10

struct timespec start, end;
long seconds, nanoseconds;
double elapsed;

// Estructura para almacenar información del archivo
typedef struct {
    int largo_nombre;
    char *nombre;
    int tamano_comprimido;
    int caracteres_comprimidos;
    long inicio_archivo;
    long inicio_contenido;  // Almacena la posición dentro del archivo donde comienza el contenido comprimido
} ArchivoComprimido;

// Estructura para pasar a los hilos como argumento
typedef struct {
    const char *archivo_bin_path;
    ArchivoComprimido *archivos;
    int start;
    int end;
    struct Node *root;
} ThreadArgs;

ArchivoComprimido* obtener_punteros_archivos(FILE *archivoComprimido, int *num_archivos) {
    ArchivoComprimido *archivos = NULL;
    int contador_archivos = 0;

    int largo_nombre;
    while (fread(&largo_nombre, sizeof(int), 1, archivoComprimido) == 1) {
        long inicio_archivo = ftell(archivoComprimido);
        char *nombre = (char*)malloc(largo_nombre + 1);
        if (!nombre) {
            perror("Error al asignar memoria para el nombre del archivo");
            break;
        }

        if ((int) fread(nombre, sizeof(char), largo_nombre, archivoComprimido) != largo_nombre) {
            free(nombre);
            break;
        }
        nombre[largo_nombre] = '\0';

        int tamano_comprimido;
        if (fread(&tamano_comprimido, sizeof(int), 1, archivoComprimido) != 1) {
            free(nombre);
            break;
        }

        int caracteres_comprimidos;
        if (fread(&caracteres_comprimidos, sizeof(int), 1, archivoComprimido) != 1) {
            free(nombre);
            break;
        }

        long inicio_contenido = ftell(archivoComprimido);

        if (fseek(archivoComprimido, tamano_comprimido, SEEK_CUR) != 0) {
            perror("Error en fseek");
            free(nombre);
            break;
        }

        archivos = (ArchivoComprimido*)realloc(archivos, (contador_archivos + 1) * sizeof(ArchivoComprimido));
        if (!archivos) {
            perror("Error al reallocar memoria para archivos");
            free(nombre);
            break;
        }

        archivos[contador_archivos].largo_nombre = largo_nombre;
        archivos[contador_archivos].nombre = nombre;
        archivos[contador_archivos].tamano_comprimido = tamano_comprimido;
        archivos[contador_archivos].caracteres_comprimidos = caracteres_comprimidos;
        archivos[contador_archivos].inicio_archivo = inicio_archivo;
        archivos[contador_archivos].inicio_contenido = inicio_contenido;

        contador_archivos++;
    }

    *num_archivos = contador_archivos;
    return archivos;
}

struct Node *deserializar_arbol(FILE *archivo) {
    struct Node *root = newNode('$', 0);
    uint32_t value;
    uint8_t longitud_codigo;
    char codigo[MAX_TREE_HT];

    while (fread(&value, sizeof(uint32_t), 1, archivo) == 1) {
        if (value == 0xFFFFFFFF) {
            break;
        }

        fread(&longitud_codigo, sizeof(uint8_t), 1, archivo);
        fread(codigo, sizeof(char), longitud_codigo, archivo);
        codigo[longitud_codigo] = '\0';

        struct Node *actual = root;
        for (int i = 0; i < longitud_codigo; i++) {
            if (codigo[i] == '0') {
                if (!actual->left) actual->left = newNode('$', 0);
                actual = actual->left;
            } else {
                if (!actual->right) actual->right = newNode('$', 0);
                actual = actual->right;
            }
        }
        actual->value = value;
    }
    return root;
}

int leer_bit(FILE *entrada, int *bit_pos, int *byte_actual) {
    if (*bit_pos == 0) {
        *byte_actual = fgetc(entrada);
        if (*byte_actual == EOF) {
            return -1;
        }
    }
    int bit = (*byte_actual >> (7 - *bit_pos)) & 1;
    *bit_pos = (*bit_pos + 1) % 8;
    return bit;
}

void *descomprimir_archivo_hilo(void *args) {
    ThreadArgs *datos = (ThreadArgs*)args;
    for (int i = datos->start; i < datos->end; i++) {
        FILE *entrada = fopen(datos->archivo_bin_path, "rb");
        if (!entrada) {
            perror("Error al abrir el archivo comprimido en el hilo");
            pthread_exit(NULL);
        }

        ArchivoComprimido archivo = datos->archivos[i];

        fseek(entrada, archivo.inicio_contenido, SEEK_SET);

        FILE *archivoSalida = fopen(archivo.nombre, "w");
        if (!archivoSalida) {
            perror("Error al crear el archivo descomprimido");
            pthread_exit(NULL);
        }

        struct Node *actual = datos->root;
        int bit_pos = 0, byte_actual = 0, bit;
        char output_buffer[1024];
        int buffer_index = 0;
        int bytes_leidos = 0;
        int byte_counter = 0, caracter_counter = 0;

        while (bytes_leidos < archivo.tamano_comprimido && caracter_counter < archivo.caracteres_comprimidos) {
            bit = leer_bit(entrada, &bit_pos, &byte_actual);
            if (bit == -1) {
                break;
            }

            if (bit == 0) {
                actual = actual->left;
            } else {
                actual = actual->right;
            }

            if (!actual->left && !actual->right) {
                char utf8_char[5];
                unicode_to_utf8(actual->value, utf8_char);
                int utf8_len = strlen(utf8_char);

                if (buffer_index + utf8_len < (int) sizeof(output_buffer)) {
                    memcpy(&output_buffer[buffer_index], utf8_char, utf8_len);
                    buffer_index += utf8_len;
                } else {
                    fwrite(output_buffer, sizeof(char), buffer_index, archivoSalida);
                    buffer_index = 0;
                    memcpy(&output_buffer[buffer_index], utf8_char, utf8_len);
                    buffer_index += utf8_len;
                }

                actual = datos->root;
                caracter_counter++;
            }
            byte_counter++;
            if (byte_counter == 8) {
                byte_counter = 0;
                bytes_leidos++;
            }
        }

        if (buffer_index > 0) {
            fwrite(output_buffer, sizeof(char), buffer_index, archivoSalida);
        }

        fclose(archivoSalida);
        fclose(entrada);
    }
    pthread_exit(NULL);
}

int main() {
    setlocale(LC_ALL, "");

    clock_gettime(CLOCK_MONOTONIC, &start);

    FILE *archivoComprimido = fopen("comprimido.bin", "rb");
    if (!archivoComprimido) {
        perror("Error al abrir el archivo comprimido");
        return 1;
    }

    struct Node *root = deserializar_arbol(archivoComprimido);
    if (!root) {
        perror("Error al deserializar el árbol de Huffman");
        fclose(archivoComprimido);
        return 1;
    }

    int num_archivos;
    ArchivoComprimido *archivos = obtener_punteros_archivos(archivoComprimido, &num_archivos);

    int archivos_por_hilo = num_archivos / MAX_THREADS;
    pthread_t threads[MAX_THREADS];
    ThreadArgs args[MAX_THREADS];

    for (int i = 0; i < MAX_THREADS; i++) {
        args[i].archivo_bin_path = "comprimido.bin";
        args[i].archivos = archivos;
        args[i].root = root;
        args[i].start = i * archivos_por_hilo;
        args[i].end = (i == MAX_THREADS - 1) ? num_archivos : (i + 1) * archivos_por_hilo;

        pthread_create(&threads[i], NULL, descomprimir_archivo_hilo, &args[i]);
    }

    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    fclose(archivoComprimido);

    clock_gettime(CLOCK_MONOTONIC, &end);

    seconds = end.tv_sec - start.tv_sec;
    nanoseconds = end.tv_nsec - start.tv_nsec;

    if (nanoseconds < 0) {
        seconds--;
        nanoseconds += 1000000000;
    }

    elapsed = seconds + nanoseconds*1e-9;
    printf("Tiempo transcurrido: %.5f segundos.\n", elapsed);

    return 0;
}
