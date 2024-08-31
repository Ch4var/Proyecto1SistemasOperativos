#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdint.h>
#include <string.h>
#include <locale.h>

#define MAX_UNICODE 0x10FFFF // Máximo valor de un carácter Unicode

typedef struct {
    uint32_t unicode_char;
    int frecuencia;
} FrecuenciaCaracter;

void unicode_to_utf8(unsigned int codepoint, char *output) {
    if (codepoint <= 0x7F) {
        output[0] = codepoint;
        output[1] = '\0';
    } else if (codepoint <= 0x7FF) {
        output[0] = 0xC0 | (codepoint >> 6);
        output[1] = 0x80 | (codepoint & 0x3F);
        output[2] = '\0';
    } else if (codepoint <= 0xFFFF) {
        output[0] = 0xE0 | (codepoint >> 12);
        output[1] = 0x80 | ((codepoint >> 6) & 0x3F);
        output[2] = 0x80 | (codepoint & 0x3F);
        output[3] = '\0';
    } else if (codepoint <= 0x10FFFF) {
        output[0] = 0xF0 | (codepoint >> 18);
        output[1] = 0x80 | ((codepoint >> 12) & 0x3F);
        output[2] = 0x80 | ((codepoint >> 6) & 0x3F);
        output[3] = 0x80 | (codepoint & 0x3F);
        output[4] = '\0';
    }
}

// Función para procesar el archivo
void procesar_archivo(const char *input_filename, const char *output_filename) {
    FILE *input_file = fopen(input_filename, "r");
    FILE *output_file = fopen(output_filename, "w");
    if (!input_file || !output_file) {
        perror("Error al abrir el archivo");
        exit(EXIT_FAILURE);
    }

    char line[256];
    while (fgets(line, sizeof(line), input_file)) {
        unsigned int codepoint;
        int count;
        if (sscanf(line, "U+%X : %d", &codepoint, &count) == 2) {
            char utf8_char[5];
            unicode_to_utf8(codepoint, utf8_char);
            fprintf(output_file, "%s : %d\n", utf8_char, count);
        }
    }

    fclose(input_file);
    fclose(output_file);
}

void contar_frecuencias(char *nombre_archivo, int *frecuencias) {
    FILE *archivo = fopen(nombre_archivo, "r");
    if (!archivo) {
        perror("No se pudo abrir el archivo");
        return;
    }

    int c;
    while ((c = fgetc(archivo)) != EOF) {
        if ((c & 0x80) == 0) { // 1-byte character (ASCII)
            frecuencias[c]++;
        } else { // Multibyte character
            ungetc(c, archivo); // Devolver el carácter para leerlo correctamente
            uint32_t unicode_char;
            int bytes_leidos = fscanf(archivo, "%lc", &unicode_char);
            if (bytes_leidos == 1 && unicode_char <= MAX_UNICODE) {
                frecuencias[unicode_char]++;
            }
        }
    }

    fclose(archivo);
}

int comparar_frecuencias(const void *a, const void *b) {
    FrecuenciaCaracter *fa = (FrecuenciaCaracter *)a;
    FrecuenciaCaracter *fb = (FrecuenciaCaracter *)b;
    return fa->frecuencia - fb->frecuencia;
}

int main() {
    setlocale(LC_ALL, ""); // Para manejar correctamente UTF-8

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

    while ((entrada = readdir(directorio)) != NULL) {
        if (entrada->d_type == DT_REG) { // Si es un archivo regular
            char ruta_completa[512];
            snprintf(ruta_completa, sizeof(ruta_completa), "%s/%s", carpeta, entrada->d_name);
            contar_frecuencias(ruta_completa, frecuencias);
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
    qsort(frecuencia_array, num_caracteres, sizeof(FrecuenciaCaracter), comparar_frecuencias);

    // Almacenar las frecuencias en un archivo
    FILE *archivo_salida = fopen("frecuencias.txt", "w");
    if (!archivo_salida) {
        perror("No se pudo abrir el archivo de salida");
        free(frecuencia_array);
        free(frecuencias);
        return 1;
    }

    for (int i = 0; i < num_caracteres; i++) {
        fprintf(archivo_salida, "U+%04X : %d\n", frecuencia_array[i].unicode_char, frecuencia_array[i].frecuencia);
    }

    fclose(archivo_salida);
    free(frecuencia_array);
    free(frecuencias);
    
    procesar_archivo("frecuencias.txt", "frecuenciasConvertidas.txt");
    
    return 0;
}

