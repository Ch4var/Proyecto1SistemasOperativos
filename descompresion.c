#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <locale.h>
#include "heap3.c"

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
            printf("Fin del archivo");
            return -1; // Fin de archivo
        }
    }
    int bit = (*byte_actual >> (7 - *bit_pos)) & 1;
    *bit_pos = (*bit_pos + 1) % 8;
    return bit;
}

void descomprimir_archivos(FILE *entrada, struct Node *root) {
    while (1) {
        int largo_nombre, largo_archivo;

        // Leer el largo del nombre del archivo
        if (fread(&largo_nombre, sizeof(int), 1, entrada) != 1) {
            break; // No se pudo leer el largo del nombre
        }
        printf("LargoNombre--------------------%d\n", largo_nombre);

        char nombreArchivo[1024];
        // Leer el nombre del archivo
        if (fread(nombreArchivo, sizeof(char), largo_nombre, entrada) != largo_nombre) {
            break; // No se pudo leer el nombre completo
        }
        nombreArchivo[largo_nombre] = '\0';  // Terminar la cadena del nombre
        printf("nombreArchivo--------------------%s\n", nombreArchivo);

        // Verificar el nombre leído
        printf("Descomprimiendo archivo: %s\n", nombreArchivo);

        // Crear el archivo de salida con el nombre leído
        FILE *archivoSalida = fopen(nombreArchivo, "w");
        if (!archivoSalida) {
            perror("Error al crear el archivo descomprimido");
            return;
        }

        // Leer el largo del archivo comprimido
        if (fread(&largo_archivo, sizeof(int), 1, entrada) != 1) {
            perror("Error al leer el largo del archivo comprimido");
            fclose(archivoSalida);
            return;
        }
        printf("Largo del archivo comprimido: %u bytes\n", largo_archivo);

        // Variables de decodificación y buffer
        struct Node *actual = root;
        int bit_pos = 0, byte_actual = 0, bit;
        char output_buffer[1024];
        int buffer_index = 0;
        uint32_t bytes_leidos = 0;  // Para el conteo de bytes del archivo comprimido

        int byte_counter = 0;

        // Descomprimir el contenido del archivo
        while (bytes_leidos < largo_archivo) {
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

        // Reiniciar las variables de lectura de bits para el siguiente archivo
        bit_pos = 0;
        byte_actual = 0;

        // Verificar si hemos llegado al final del archivo comprimido
        if (feof(entrada)) {
            break;
        }
    }
}

int main() {
    setlocale(LC_ALL, "");

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

    printf("Árbol de Huffman deserializado correctamente.\n");

    // Descomprimir los archivos usando el árbol cargado
    descomprimir_archivos(archivoComprimido, root);  

    fclose(archivoComprimido);

    return 0;
}