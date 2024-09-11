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

void utf8_to_unicode(char *utf8, uint32_t *unicode) {
    unsigned char *str = (unsigned char *)utf8;
    
    if (str[0] <= 0x7F) {
        *unicode = str[0];  // 1 byte UTF-8 (ASCII)
    } else if ((str[0] & 0xE0) == 0xC0) {
        *unicode = ((str[0] & 0x1F) << 6) | (str[1] & 0x3F);  // 2 bytes UTF-8
    } else if ((str[0] & 0xF0) == 0xE0) {
        *unicode = ((str[0] & 0x0F) << 12) | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F);  // 3 bytes UTF-8
    } else if ((str[0] & 0xF8) == 0xF0) {
        *unicode = ((str[0] & 0x07) << 18) | ((str[1] & 0x3F) << 12) | ((str[2] & 0x3F) << 6) | (str[3] & 0x3F);  // 4 bytes UTF-8
    }
}


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
            printf("utf8 char: %s", utf8_char);
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