#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <locale.h>
#include <time.h>
#include "heap.c"
#include <unistd.h>  // Para fork()
#include <sys/wait.h>  // Para wait()

#define MAX_PROCESSES 10

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

ArchivoComprimido* obtener_punteros_archivos(FILE *archivoComprimido, int *num_archivos) {
    // Array dinámico de archivos
    ArchivoComprimido *archivos = NULL;
    int contador_archivos = 0;

    int largo_nombre;
    // Mientras se puedan leer los tamaños de nombre, archivo comprimido, y caracteres comprimidos
    while (fread(&largo_nombre, sizeof(int), 1, archivoComprimido) == 1) {
        long inicio_archivo = ftell(archivoComprimido);
        // Asignar espacio para el nombre del archivo
        char *nombre = (char*)malloc(largo_nombre + 1);
        if (!nombre) {
            perror("Error al asignar memoria para el nombre del archivo");
            break;
        }

        // Leer el nombre del archivo
        if (fread(nombre, sizeof(char), largo_nombre, archivoComprimido) != largo_nombre) {
            free(nombre);
            break;
        }
        nombre[largo_nombre] = '\0'; // Agregar terminador nulo

        // Leer el tamaño del archivo comprimido
        int tamano_comprimido;
        if (fread(&tamano_comprimido, sizeof(int), 1, archivoComprimido) != 1) {
            free(nombre);
            break;
        }

        // Leer el número de caracteres del archivo original
        int caracteres_comprimidos;
        if (fread(&caracteres_comprimidos, sizeof(int), 1, archivoComprimido) != 1) {
            free(nombre);
            break;
        }

        // Obtener la posición actual (inicio del contenido comprimido)
        long inicio_contenido = ftell(archivoComprimido);

        // Saltar el contenido comprimido
        if (fseek(archivoComprimido, tamano_comprimido, SEEK_CUR) != 0) {
            perror("Error en fseek");
            free(nombre);
            break;
        }

        // Incrementar el tamaño del array
        archivos = (ArchivoComprimido*)realloc(archivos, (contador_archivos + 1) * sizeof(ArchivoComprimido));
        if (!archivos) {
            perror("Error al reallocar memoria para archivos");
            free(nombre);
            break;
        }

        // Llenar la información del archivo
        archivos[contador_archivos].largo_nombre = largo_nombre;
        archivos[contador_archivos].nombre = nombre;
        archivos[contador_archivos].tamano_comprimido = tamano_comprimido;
        archivos[contador_archivos].caracteres_comprimidos = caracteres_comprimidos;
        archivos[contador_archivos].inicio_archivo = inicio_archivo;
        archivos[contador_archivos].inicio_contenido = inicio_contenido;

        contador_archivos++;
    }

    // Devolver la cantidad de archivos leídos
    *num_archivos = contador_archivos;
    return archivos;
}

struct Node *deserializar_arbol(FILE *archivo) {
    struct Node *root = newNode('$', 0);  // Nodo raíz vacío
    uint32_t value;
    uint8_t longitud_codigo;
    char codigo[MAX_TREE_HT];

    while (fread(&value, sizeof(uint32_t), 1, archivo) == 1) {
        // Comprobar si llegamos al separador
        if (value == 0xFFFFFFFF) {
            break;  // Termina la deserialización del árbol
        }

        // Leer el código del carácter
        fread(&longitud_codigo, sizeof(uint8_t), 1, archivo);
        fread(codigo, sizeof(char), longitud_codigo, archivo);
        codigo[longitud_codigo] = '\0';  // Añadir el terminador

        // Insertar el carácter en el árbol según su código
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
        // Al final del código, asignamos el valor del carácter
        actual->value = value;
    }
    return root;
}


int leer_bit(FILE *entrada, int *bit_pos, int *byte_actual) {
    if (*bit_pos == 0) {
        *byte_actual = fgetc(entrada);
        if (*byte_actual == EOF) {
            return -1; // Fin de archivo
        }
    }
    int bit = (*byte_actual >> (7 - *bit_pos)) & 1;
    *bit_pos = (*bit_pos + 1) % 8;
    return bit;
}

void descomprimir_archivo_hijo(const char *archivo_bin_path, ArchivoComprimido *archivos, int start, int end, struct Node *root) {
    for (int i = start; i < end; i++) {
        FILE *entrada = fopen(archivo_bin_path, "rb");
        if (!entrada) {
            perror("Error al abrir el archivo comprimido en el proceso hijo");
            exit(1);
        }
        
        ArchivoComprimido archivo = archivos[i];

        // Mover el puntero al inicio del contenido comprimido
        fseek(entrada, archivo.inicio_contenido, SEEK_SET);

        // Crear el archivo de salida con el nombre del archivo descomprimido
        FILE *archivoSalida = fopen(archivo.nombre, "w");
        if (!archivoSalida) {
            perror("Error al crear el archivo descomprimido");
            exit(1);
        }

        // Variables de decodificación y buffer
        struct Node *actual = root;
        int bit_pos = 0, byte_actual = 0, bit;
        char output_buffer[1024];
        int buffer_index = 0;
        uint32_t bytes_leidos = 0;  // Para el conteo de bytes del archivo comprimido

        int byte_counter = 0, caracter_counter = 0;

        // Descomprimir el contenido del archivo
        while (bytes_leidos < archivo.tamano_comprimido && caracter_counter < archivo.caracteres_comprimidos) {
            // Leer bit del archivo comprimido
            bit = leer_bit(entrada, &bit_pos, &byte_actual);
            if (bit == -1) {
                break;  // Fin del archivo o error
            }

            // Recorrer el árbol de Huffman para decodificar
            if (bit == 0) {
                actual = actual->left;
            } else {
                actual = actual->right;
            }

            // Si llegamos a una hoja, decodificamos el carácter
            if (!actual->left && !actual->right) {
                char utf8_char[5];
                unicode_to_utf8(actual->value, utf8_char);
                int utf8_len = strlen(utf8_char);  // Tamaño en bytes del carácter decodificado

                // Agregar al buffer de salida
                if (buffer_index + utf8_len < sizeof(output_buffer)) {
                    memcpy(&output_buffer[buffer_index], utf8_char, utf8_len);
                    buffer_index += utf8_len;
                } else {
                    // Escribir el buffer si está lleno
                    fwrite(output_buffer, sizeof(char), buffer_index, archivoSalida);
                    buffer_index = 0;
                    memcpy(&output_buffer[buffer_index], utf8_char, utf8_len);
                    buffer_index += utf8_len;
                }

                // Volver a la raíz del árbol de Huffman para continuar decodificando
                actual = root;
                caracter_counter++;
            }
            byte_counter++;
            if (byte_counter == 8) {
                byte_counter = 0;
                bytes_leidos++;
            }
        }

        // Escribir los datos restantes en el archivo de salida
        if (buffer_index > 0) {
            fwrite(output_buffer, sizeof(char), buffer_index, archivoSalida);
        }

        // Cerrar el archivo de salida
        fclose(archivoSalida);
    }
}

int main() {
    setlocale(LC_ALL, "");

    clock_gettime(CLOCK_MONOTONIC, &start);

    // Abrir el archivo comprimido
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

    // Obtener los punteros a los archivos
    int num_archivos;
    ArchivoComprimido *archivos = obtener_punteros_archivos(archivoComprimido, &num_archivos);

    int archivos_por_proceso = num_archivos / MAX_PROCESSES;
    pid_t pids[MAX_PROCESSES];

    for (int i = 0; i < MAX_PROCESSES; i++) {
        pids[i] = fork();

        if (pids[i] == -1) {
            perror("Error al hacer fork");
            exit(1);
        }

        if (pids[i] == 0) {  // Código del proceso hijo
            // Determinar el rango de archivos para cada proceso
            int start = i * archivos_por_proceso;
            int end = (i == MAX_PROCESSES - 1) ? num_archivos : (i + 1) * archivos_por_proceso;

            // Descomprimir los archivos en el rango [start, end)
            descomprimir_archivo_hijo("comprimido.bin", archivos, start, end, root);

            exit(0);  // El proceso hijo termina aquí
        }
    }

    for (int i = 0; i < MAX_PROCESSES; i++) {
        waitpid(pids[i], NULL, 0);  // Esperar a que los procesos hijos terminen
    }

    fclose(archivoComprimido);

    clock_gettime(CLOCK_MONOTONIC, &end);

    seconds = end.tv_sec - start.tv_sec;
    nanoseconds = end.tv_nsec - start.tv_nsec;

    if (nanoseconds < 0) {
        seconds--;
        nanoseconds += 1000000000;
    }

    elapsed = seconds + nanoseconds * 1e-9;

    printf("Tiempo transcurrido: %.9f segundos\n", elapsed);

    free(archivos);
    free(root);

    return 0;
}