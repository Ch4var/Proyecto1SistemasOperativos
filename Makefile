# Variables
CC=gcc
CFLAGS= -Wall -Wextra   
LDFLAGS= -pthread  

# Archivos fuente
SRC_COMPRESSION=compression.c
SRC_DECOMPRESSION=decompression.c
SRC_COMPRESSION_FORK=compression_with_fork.c
SRC_DECOMPRESSION_FORK=decompression_with_fork.c
SRC_COMPRESSION_THREADS=compression_with_threads.c
SRC_DECOMPRESSION_THREADS=decompression_with_threads.c  

# Archivos ejecutables
BIN_COMPRESSION=compression
BIN_DECOMPRESSION=decompression
BIN_COMPRESSION_FORK=compression_with_fork
BIN_DECOMPRESSION_FORK=decompression_with_fork
BIN_COMPRESSION_THREADS=compression_with_threads
BIN_DECOMPRESSION_THREADS=decompression_with_threads    

# Compilar todo
all: $(BIN_COMPRESSION) $(BIN_DECOMPRESSION) $(BIN_COMPRESSION_FORK) $(BIN_DECOMPRESSION_FORK) $(BIN_COMPRESSION_THREADS) $(BIN_DECOMPRESSION_THREADS)

# Compilar cada archivo
$(BIN_COMPRESSION): $(SRC_COMPRESSION)
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_DECOMPRESSION): $(SRC_DECOMPRESSION)
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_COMPRESSION_FORK): $(SRC_COMPRESSION_FORK)
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_DECOMPRESSION_FORK): $(SRC_DECOMPRESSION_FORK)
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_COMPRESSION_THREADS): $(SRC_COMPRESSION_THREADS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(BIN_DECOMPRESSION_THREADS): $(SRC_DECOMPRESSION_THREADS)  
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

# Limpiar los archivos binarios
clean:
	rm -f $(BIN_COMPRESSION) $(BIN_DECOMPRESSION) $(BIN_COMPRESSION_FORK) $(BIN_DECOMPRESSION_FORK) $(BIN_COMPRESSION_THREADS) $(BIN_DECOMPRESSION_THREADS)

# Limpiar todo 
distclean: clean

# Regla para reconstruir todo
rebuild: clean all
