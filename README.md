# Autores, Curso y Sede
**Instituto Tecnológico de Costa Rica**
*Principios de Sistemas Operativos. Grupo 40*
Integrantes:
*    Sergio Chavarría Avilés- 2021077879 - Centro Académico de Alajuela
*    José Ignacio Sánchez Morales- 2022081784 - Centro Académico Local San José
*    Adrián Josué Vega Barrantes- 2022104007 - Centro académico Local San José

Profesor:
*    Rodolfo Mora Zamora 

## Tabla de contenidos
- [Descripción](#Descripción)
- [Compilación](#Compilación)

# Descripción:

Este proyecto consiste en la programación del algoritmo de Huffman en el lenguaje de programación C. Esto con el objetivo de comprimir 100 libros en formato de texto plano obtenidos del Gutenberg y luego descomprimirlos volviendo a generar los 100 libros originales. Para esto se programaron 3 versiones de compresión y 3 versiones de descompresión. Para la compresión y descompresión se programó una versión serial, una versión paralela utilizando fork y una versión concurrente utilizando pthreads. Esto con el objetivo de poder ver la mejora en el tiempo de ejecución entre las versiones.

# Compilación:
Para poder ejecutar el código en el ambiente Fedora Worktstation 40 es necesario darle permisos al archivo llamado gcc\_install.sh, el cual es un script que se encarga de instalar la herramienta gcc, para compilar y ejecutar programas en c, y make, para compilar automáticamente los archivos del proyecto. Esto se logra abriendo una terminal donde se encuentra el proyecto y ejecutando el siguiente comando: 
chmod +x gcc_install.sh

Este archivo primero actualizará los repositorios de Fedora, luego instalará gcc y make y por último compilará todos los programas. 

Una vez compilados los archivos, simplemente se ejecutan de la siguiente manera:

./compression 

./decompression

./compression_with_fork

./decompression_with_fork

./compression_with_threads

./decompression_with_threads

